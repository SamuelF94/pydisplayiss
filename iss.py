#!/usr/bin/python

import sys
import requests
import datetime, time
import json
import threading 
import time
import os.path
import os
import pydisplay
import RPi.GPIO as GPIO

# some globals
http_proxy  = "http://192.168.50.10:8118"
https_proxy = "http://192.168.50.10:8118"
ftp_proxy   = "http://192.168.50.10:8118"

proxyDict = { 
              "http"  : http_proxy, 
              "https" : https_proxy, 
              "ftp"   : ftp_proxy
            }

headersDict = {"Accept": "application/json" }
headersAllDict = {"Accept": "*/*" , "User-Agent" : "PythonISS/1.0 (http://www.feutry.fr; sfeutry@club-internet.fr) python script" }

sUrlPosition = 'http://api.open-notify.org/iss-now.json'
sUrlInfo1 = "https://corquaid.github.io/international-space-station-APIs/JSON/people-in-space.json"
CurrentData = {'expedition_patch':'' , 'expedition_image':''}
patchImg = 'patch.bmp3'
crewImg = 'crew.bmp3'



bAction = False
s = requests.Session()


# convert patch.png -resize 480x320 -gravity center -extent 480x320 patch.bmp3

def getImage( imageType,localImageName ):
	r = s.get(sUrlInfo1, headers= headersDict, proxies=proxyDict)
	jsontext = r.text
	response = json.loads(jsontext)
	patchUrl = response[imageType]
#	print(patchUrl)
#	print(CurrentData['expedition_patch'])
	if ( patchUrl != CurrentData[imageType] or not (os.path.exists(localImageName))):
		print("redownloading file")
		test = pydisplay.clearScreen()
		test = pydisplay.drawString("Processing...",20,20,4)
		CurrentData[imageType]= patchUrl
		img = s.get(patchUrl,headers=headersAllDict, proxies=proxyDict)
		fileName = patchUrl.split("/")[-1:][0]
		open(fileName, 'wb').write(img.content)
		sCommand = "convert " + fileName + " -resize 480x320 -gravity center -extent 480x320 " + localImageName
		print(sCommand)
		os.system(sCommand)
		Save()
	pydisplay.showBMP(localImageName)





def callback_mission():
	global bAction
	bAction = True
	getImage("expedition_patch",patchImg)
	bAction = False
	exit()

def callback_crewphoto():
	global bAction
	bAction = True
	getImage("expedition_image",crewImg)
	bAction = False
	exit()

def callback_crewmembers():
	global bAction
	bAction = True
	print("crew members")
	bAction = False

def callback_spacecrafts():
	global bAction
	bAction = True
	print("spacecrafts")
	bAction = False

def callback_surprise():
	global bAction
	bAction = True
	print("surprise")
	bAction = False

def callback_shutdown():
	global bAction
	bAction = True
	print("shutdown")
	bAction = False




def GPIOSetup():
	# GPIO initialization 
	GPIO.setmode(GPIO.BCM) # use GPIO number and not pin numbers
	# le HIGH 
	GPIO.setup(12, GPIO.OUT, initial = GPIO.HIGH)

	GPIO.setup(5, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)	# display mission sign -  pin 29
	GPIO.setup(6, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)	# display crew photo - pin 31 
	GPIO.setup(13, GPIO.IN,pull_up_down=GPIO.PUD_DOWN)	# display crew members - pin 33
	GPIO.setup(19, GPIO.IN,pull_up_down=GPIO.PUD_DOWN)	# display currently docked spacecrafts - pin 35
	GPIO.setup(26, GPIO.IN,pull_up_down=GPIO.PUD_DOWN)	# display i don't know what... yet - pin 37
	GPIO.setup(21, GPIO.IN,pull_up_down=GPIO.PUD_DOWN)	#  shutdown - pin 40

	GPIO.add_event_detect(5, GPIO.RISING, callback=callback_mission, bouncetime=300)
	GPIO.add_event_detect(6, GPIO.RISING, callback=callback_crewphoto, bouncetime=300)
	GPIO.add_event_detect(13, GPIO.RISING, callback=callback_crewmembers, bouncetime=300)
	GPIO.add_event_detect(19, GPIO.RISING, callback=callback_spacecrafts, bouncetime=300)
	GPIO.add_event_detect(26, GPIO.RISING, callback=callback_surprise, bouncetime=300)
	GPIO.add_event_detect(21, GPIO.RISING, callback=callback_shutdown, bouncetime=300)



def Init():
	global CurrentData
	with open('iss.json') as json_file:
		CurrentData = json.load(json_file)

def Save():
	if ( CurrentData != None ):
		with open('iss.json', 'w') as outfile:
			outfile.write(json.dumps(CurrentData))


Init()
test = pydisplay.showBMP("world.bmp")

while True:
	r = s.get(sUrlPosition, headers= headersDict, proxies=proxyDict)
	jsontext = r.text
	response = json.loads(jsontext)
	position = response["iss_position"]
	longitude =int(round(float(position["longitude"])))
	latitude =int(round(float(position["latitude"])))
#	print(longitude, latitude)
	callback_crewphoto()
	if ( bAction == False ):
		test = pydisplay.showBMP("world.bmp")
		test = pydisplay.showISS(longitude, latitude)
	time.sleep(10)

