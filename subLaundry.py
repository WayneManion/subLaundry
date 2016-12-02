# -*- coding: utf-8 -*-
import datetime
import paho.mqtt.client as mqtt
import urllib2

# Initialize variables.
wStartTime = 0            # Record washer start time
dStartTime = 0            # Record dryer start time
wLastHundred = [0] * 100  # Last 100 washer tap counts
dLastHundred = [0] * 100  # Last 100 dryer tap counts
wRunning = False          # Washer running/not running state
dRunning = False          # Dryer running/not running state 
# Use your own IFTTT URLs for these
wFinishedURL = "https://URL1"
dFinishedURL = "https://URL2"
wStartedURL = "https://URL3"
dStartedURL = "https://URL4"

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, rc):
    print("Connected with result code " + str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("laundry/test")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    # Bring the global variables into the function.
    global wStartTime
    global dStartTime
    global wLastHundred
    global dLastHundred
    global wRunning
    global dRunning
    global wFinishedURL
    global dFinishedURL
    global wStartedURL
    global dStartedURL

    # Log MQTT message and write message to stdout with time stamp.
    words = str(datetime.datetime.now()) + '\t' + str(msg.payload)
    # Your filename here.
	outFile = open("laundryLog.txt", 'a');
    outFile.write(words + '\n')
    outFile.close()
    print(words)

    # Pull the washer and dryer tap counts out of the MQTT message if possible.
    results = [int(s) for s in words.split() if s.isdigit()]
    if (len(results) >= 2):
        wLastHundred = [results[0]] + wLastHundred
        wLastHundred.pop()
        dLastHundred = [results[1]] + dLastHundred
        dLastHundred.pop()

    # Detect washer start.
    if (wRunning == False and min(wLastHundred[0:4]) > 10):
        wRunning = True
        wStartTime = datetime.datetime.now()
        urllib2.urlopen(wStartedURL).read()
        print("Detected washer start.")
    
    # Detect dryer start.
    if (dRunning == False and min(dLastHundred[0:4]) > 10):
        dRunning = True
        dStartTime = datetime.datetime.now()
        urllib2.urlopen(dStartedURL).read()
        print("Detected dryer start.")
    
    # Detect washer finish. 
    # It takes a pretty long period of inactivity to be sure the washer is done.
    if (wRunning == True and max(wLastHundred[0:34]) < 10):
        wRunning = False
        wStartTime = 0
        print("Detected washer finish.")
        if (dRunning == False):
            urllib2.urlopen(wFinishedURL).read()
            print("Sent washer finished email.")
    
    # Detect dryer finish.
    if (dRunning == True and max(dLastHundred[0:9]) < 10):
        dRunning = False
        dStartTime = 0
        print("Detected dryer finish.")
        if (wRunning == False):
            urllib2.urlopen(dFinishedURL).read()
            print("Sent dryer finished email.")

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    # Comment this out if your MQTT server does not require authentication. 
    client.username_pw_set("USER", password="PASSWORD")
    # Use your own particulars here. 
    client.connect("MQTT_BROKER_ADDR", 1883, 60)

    # Blocking call that processes network traffic, dispatches callbacks and
    # handles reconnecting.
    # Other loop*() functions are available that give a threaded interface and a
    # manual interface.
    client.loop_forever()

if __name__ == "__main__":
    main()