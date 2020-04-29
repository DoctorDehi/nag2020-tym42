#Libraries
import sys
sys.path.append("/home/pi/.local/lib/python3.7/site-packages")
sys.path.append("/home/pi/.local/lib/python3.7/dist-packages")
import paho.mqtt.client as mqtt
import RPi.GPIO as GPIO
from RPLCD.i2c import CharLCD
import _thread
import time
import redis

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
    "Denca ISIC" : "15015093156",
}


status = {
    "stairway-light" : "off",
    "garden-light"  : "off",
    "barrier" : "closed",
    "latest access": "",
    "security-system" : "off",
    "light-level": 0,
    "humidity" : 0,
    "temperature" : 0,
}


lcd = CharLCD(i2c_expander='PCF8574', address=0x27, port=1,
	cols=16, rows=2, dotsize=8,
	charmap='A02',
      	auto_linebreaks=True,
        backlight_enabled=True)

r = redis.Redis(host='localhost', port=6379, db=0)


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
        #  Check if button switch is pressed
        if GPIO.input(alarm_switch) == GPIO.HIGH:
            if status["security-system"] == "off":
                turn_security_alarm_on(client, status)
            else:
                turn_security_alarm_off(client, status)

            while GPIO.input(alarm_switch) == GPIO.HIGH:
                time.sleep(0.25)


        # if there is break-in, beep and blink red LED
        if status["security-system"] == "break-in":
                GPIO.output(buzzer,GPIO.HIGH)
                time.sleep(0.15) # Delay in seconds
                GPIO.output(buzzer,GPIO.LOW)
        time.sleep(0.15)

# change any value of status
def change_status_value(variable, value, status):
    status[variable] = value
    r.set(variable, value)

# Set functions
def set_garden_light_on(client, status):
    change_status_value("garden-light", "on", status)
    client.publish("garden/light", "state:on")

def set_garden_light_off(client, status):
    change_status_value("garden-light", "off", status)
    client.publish("garden/light", "state:off")

def set_garden_barrier_open(client, status):
    change_status_value("barrier", "opened", status)
    client.publish("garden/barrier", "set:open")

def set_stairway_light_on(client, status):
    change_status_value("stairway-light", "on", status)
    client.publish("stairway/light", "set:on")

def set_stairway_light_off(client, status):
    change_status_value("stairway-light", "off", status)
    client.publish("stairway/light", "set:off")

def turn_security_alarm_off(client, status):
    change_status_value("security-system", "off", status)
    client.publish("security/alarm", "state:off")

def turn_security_alarm_on(client, status):
    change_status_value("security-system", "on", status)
    client.publish("security/alarm", "state:on")

def send_value_to_API(variable, value):
    try:
        requests.post("https://api.nag-iot.zcu.cz/v2/value/%s" % variable,
                      headers={"X-Api-Key": "cr8mlx0MluU1LP3r", "Content-Type": "text/plain"},
                      data=str(value))
        print("Succesfully posted value of %s to API" % variable)
    except requests.exceptions.ConnectionError as e:
        print("Post to API failed")

# Definition of diplay thread
def display_status(lcd):
    global status   
    while True:
        for state in status:
            lcd.clear()
            lcd.cursor_pos = (0,0)
            lcd.write_string(state + ":")
            lcd.cursor_pos = (1,0)
            lcd.write_string(str(status[state]))
            time.sleep(5)

# checks for changes of redis values
def check_redis(r, client):
    global status
    while True:
        for key in status.keys():
            redis_value = r.get(key).decode("UTF-8")
            if redis_value != status[key]:
                if key == "stairway-light":
                    if redis_value == "on":
                        set_stairway_light_on(client, status)
                    elif redis_value == "off":
                        set_stairway_light_off(client, status)
                if key == "garden-light":
                    if redis_value == "on":
                        set_garden_light_on(client, status)
                    elif redis_value == "off":
                        set_garden_light_off(client, status)
                if key == "barrier":
                    if redis_value == "opened":
                        set_garden_barrier_open(client, status)
                if key == "security-system":
                    if redis_value == "on":
                        turn_security_alarm_on(client, status)
                    elif redis_value == "off":
                        turn_security_alarm_off(client, status)
        time.sleep(1)


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("stairway/light")
    client.subscribe("garden/barrier")
    client.subscribe("garden/rfid_reader")
    client.subscribe("security/alarm")
    client.subscribe("garden/meteostation")


# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global status
    message = str(msg.payload)[2:-1]
    print(message)
    topic = msg.topic

    if topic == "garden/meteostation":
        if message.startswith("light-level:"):
            # update light-level value
            change_status_value("light-level", float(message[len("light-level:"):]), status)
            # send data to API
            send_value_to_API("lightlevel", status["light-level"])

            if status["light-level"]  < 10:
                set_gardenlight_on(client, status)
            else:
                set_gardenlight_off(client, status)

        elif message.startswith("humidity:"):
            # update humidity value
            change_status_value("humidity", float(message[len("humidity:"):]), status)
            # send data to API
            send_value_to_API("humidity", status["humidity"])

        elif message.startswith("temperature:"):
            # update temperature value
            change_status_value("temperature", float(message[len("temperature:"):]), status)
            # send data to API
            send_value_to_API("temperature", status["temperature"])


    elif topic == "garden/rfid_reader":
        if message.startswith("uuid:"):
            uuid = message[len("uuid:"):]
            authorized = False
            for user in authorized_uuids.keys():
                if authorized_uuids[user] == uuid:
                    authorized = True
                    response = "authorized"
                    change_status_value("latest access", user, status)
                    if status["barrier"] == "closed":
                        set_garden_barrier_open(client, status)
                    break
            if not authorized:
                response = "unauthorized"
            client.publish("garden/rfid_reader", response)

    elif topic == "garden/barrier":
        if message.startswith("state:"):
            change_status_value("barrier", message[len("state:"):], status)

    elif topic == "stairway/light":
        if message.startswith("state:"):
            change_status_value("stairway-light", message[len("state:"):], status)

    elif topic == "security/alarm":
       if "motion" in message and status["security-system"] == "on":
           change_status_value("security-system", "break-in", status)

    print(status)


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
try:
    # fill redis with default values
    for i in status.keys():
        r.set(i, status[i])

    # start threads for alarm and display
    _thread.start_new_thread(alarm, (client,))
    _thread.start_new_thread(display_status, (lcd,))
    _thread.start_new_thread(check_redis, (r, client))
    # run mqtt client in forever loop
    client.loop_forever()
except KeyboardInterrupt:
    print("Program was aborted")
except Exception:
    print("Some error occurred")
finally:
    GPIO.cleanup()

