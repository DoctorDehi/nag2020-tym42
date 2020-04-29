#!/bin/sh -i
# launcher.sh
# navigate to home directory, then to this directory, then execute python script, then back home

cd /home/pi/nag
sudo python mqtt_controller.py
cd
