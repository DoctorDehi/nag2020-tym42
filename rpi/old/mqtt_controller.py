# Import used libraries
import sys
sys.path.append("/home/pi/.local/lib/python2.7/site-packages")
sys.path.append("/home/pi/.local/lib/python2.7/dist-packages")
import paho.mqtt.client as mqtt
import RPi.GPIO as GPIO
from RPLCD.i2c import CharLCD
import thread
import time

import requests

# Disable warnings
GPIO.setwarnings(True)
#Select GPIO mode
GPIO.setmode(GPIO.BOARD)

# List of authorized uuid's - faster and simplier than database
# in case this is a family house a database is unnecessary
authorized_uuids = {
    "Denca" : "1215655141",
    "Luky" : "137255199142",
}

# dict that stores all needed status values
status = {
    "stairway light" : "off",
    "light level" : 0,
    "garden light"  : "off",
    "latest access" : "",
    "barrier" : "closed",
    "security system" : "off",
}


# instance of our LCD display
lcd = CharLCD(i2c_expander='PCF8574', address=0x27, port=1,
              cols=16, rows=2, dotsize=8,
              charmap='A02',
              auto_linebreaks=True,
              backlight_enabled=True)


# Definition of  alarm thread
def alarm(client):
    topic = "security/alarm"
    #Set buzzer - pin 23 as output
    buzzer=16
    alarm_switch=18
    GPIO.setup(buzzer,GPIO.OUT)
    GPIO.setup(alarm_switch, GPIO.IN)
    global status
    #Run forever loop
    while True:
        #  Check if alarm should be on
        if GPIO.input(alarm_switch) == GPIO.LOW:
            if status["security system"] != "off":
                status["security system"] = "off"
                client.publish(topic, "state:off")
        else:
            if status["security system"] == "off":
                status["security system"] = "on"
                client.publish(topic, "state:on")
        if status["security system"] == "break-in":
            GPIO.output(buzzer,GPIO.HIGH)
            time.sleep(0.15) # Delay in seconds
            GPIO.output(buzzer,GPIO.LOW)
        time.sleep(0.15)


# Definition of diplay thread
def display_status(lcd):
    global status
    while True:
        # cycle that iterates status variables
        for state in status:
            # clean display
            lcd.clear()
            # print actual status variable
            lcd.cursor_pos = (0,0)
            lcd.write_string(state + ":")
            lcd.cursor_pos = (1,0)
            lcd.write_string(str(status[state]))
            # wait 5 seconds
            time.sleep(5)




# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("garden/light_sensor")
    client.subscribe("garden/rfid_reader")
    client.subscribe("garden/barrier")
    client.subscribe("stairway/light")
    client.subscribe("security/alarm")


# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global status
    # receieved message
    message = str(msg.payload)
    # message topic
    topic = msg.topic


    # if our message is from topic garden/light_sensor
    if topic == "garden/light_sensor":
        # if message contains value of intensity of light in the garden
        if message.startswith("light-level:"):
            # save this value to status variable
            status["light level"] = float(message[len("light-level:"):])
            # try to send data to API
            try:
                requests.post("https://api.nag-iot.zcu.cz/v2/value/lightlevel", headers={"X-Api-Key":"cr8mlx0MluU1LP3r", "Content-Type" : "text/plain"}, data=str(status["light level"]))
                print("Succesfully posted light level to API")
            # if there is connection error print error message
            except requests.exceptions.ConnectionError as e:
                print("Post to API failed")

            # when light level is lower than 10 
            if status["light level"]  < 10:
                # turn on garden light
                status["garden light"] = "on"
                client.publish("garden/light", "state:on")
            # in case there is enough light
            else:
                # turn off garden light
                status["garden light"] = "off"
                client.publish("garden/light", "state:off")

    # if our message is from topic garden/rfid_reader
    elif topic == "garden/rfid_reader":
        # if message cantains uuid
        if message.startswith("uuid:"):
            # separate uuid
            uuid = message[len("uuid:"):]
            authorized = False
            # check if this uuid is in authorized uuid's
            for user in authorized_uuids.keys():
                if authorized_uuids[user] == uuid:
                    authorized = True
                    response = "authorized"
                    status["latest access"] = user
                    # if this uuid is authorized 
                    if status["barrier"] == "closed":
                        # open barrier
                        status["barrier"] = "opened"
                        client.publish("garden/barrier", "set:open")
                    break
            # in case this uuid isn't on the list, it's unauthorized
            if not authorized:
                response = "unauthorized"
            # send info to rfid reader
            client.publish("garden/rfid_reader", response)

    # if our message is from topic garden/barrier
    elif topic == "garden/barrier":
        # if message contains info about barrier state
        if message.startswith("state:"):
            # save this info to status variable
            status["barrier"] =  message[len("state:"):]


    # if our message is from topic stairway/light
    elif topic == "stairway/light":
        # if message contains info about stairway light state
        if message.startswith("state:"):
            # save this info to status variable
            status["stairway light"] = message[len("state:"):]


    # if our message is from topic security/alarm
    elif topic == "security/alarm":
       if "motion" in message and status["security system"] == "on":
            status["security system"] = "break-in"

    # print status variables - for testing and development 
    print(status)


# create instance of client and set loop functions
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# connect client to MQTT server
client.connect("localhost", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.

try:
    # start alarm thread
    thread.start_new_thread(alarm, (client,))
    # start display thread
    thread.start_new_thread(display_status, (lcd,))
    # start client loop
    client.loop_forever()

# code fot GPIO pin cleanup - clean exit
except KeyboardInterrupt:
    print("Program was aborted")
except:
    print("Some error occurred")
finally:
    GPIO.cleanup()
