#!/usr/bin/python

import sys
import requests
import datetime, time
import json
import threading 
import time
import os.path
import os
import RPi.GPIO as GPIO
from geopy.distance  import geodesic
import pydisplay

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


sUrlCurrentPosition = 'http://ip-api.com/json/'
sUrlPosition = 'http://api.open-notify.org/iss-now.json'
sUrlInfo1 = "https://corquaid.github.io/international-space-station-APIs/JSON/people-in-space.json"
sUrlInfo2 = "https://corquaid.github.io/international-space-station-APIs/JSON/iss-docked-spacecraft.json"
cacheDir = 'cache'
CurrentData = {'expedition_patch':'' , 'expedition_image':''}
patchImg = 'cache/patch.bmp3'
crewImg = 'cache/crew.bmp3'
diapo_tempo = 15
bBlink = False

bAction = False
s = requests.Session()





class Blinker (threading.Thread):
	def __init__(self):
		threading.Thread.__init__(self)

	def run(self):
		global bBlink
		while True:
			try:
				GPIO.output(20, bBlink)
				time.sleep(0.5)
				GPIO.output(20, 0)
				time.sleep(0.5)


def RGB( red, green, blue ):
	return red + (green * 256) + (blue * 65536)

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
		test = pydisplay.drawString("Processing...",20,20,4,RGB(205,105,201))
		CurrentData[imageType]= patchUrl
		img = s.get(patchUrl,headers=headersAllDict, proxies=proxyDict)
		fileName = cacheDir + '/' + patchUrl.split("/")[-1:][0]
		open(fileName, 'wb').write(img.content)
		sCommand = "convert " + fileName + " -resize 480x320 -gravity center -extent 480x320 " + localImageName
		print(sCommand)
		os.system(sCommand)
		Save()
	pydisplay.showBMP(localImageName)




def callback_mission():
	try:
		global bAction
		bAction = True
		getImage("expedition_patch",patchImg)
		time.sleep(10)
	except:
		print("callback_mission exception")
	finally:
		bAction = False
	

def callback_crewphoto():
	try:
		global bAction
		bAction = True
		getImage("expedition_image",crewImg)
		time.sleep(10)
		bAction = False
	except:
		print("callback_crewphoto exception")
	finally:
		bAction = False


def callback_crewmembers():
	try:
		global bAction
		bAction = True
		r = s.get(sUrlInfo1, headers= headersDict, proxies=proxyDict)
		jsontext = r.text
		response = json.loads(jsontext)
		pydisplay.clearScreen()
		i = 0
		pydisplay.drawString("Current Crew",80,20,2,RGB(205,105,201))
		for astro in response["people"]:
			#print(astro)
			if ( astro["iss"] == True ):
				pydisplay.drawString(astro["position"] + ":",20,50+i*25,-1,RGB(0,125,255))
				pydisplay.drawString(astro["name"]  + " (" + astro["country"] + ")",200,80+i*25,-1,RGB(0,125,255))
				i = i +1
		time.sleep(10)
	except:
		print("callback_crewmembers exception")
	finally:
		bAction = False


def callback_spacecrafts():
	try:
		global bAction
		bAction = True
		r = s.get(sUrlInfo2, headers= headersDict, proxies=proxyDict)
		jsontext = r.text
		response = json.loads(jsontext)
		pydisplay.clearScreen()
		i = 0
		pydisplay.drawString("Visiting spaceships",80,20,2,RGB(205,105,201))
		for spacecraft in response["spacecraft"]:
			if ( spacecraft["iss"] == True ):
				pydisplay.drawString(spacecraft["name"] + ":",20,50+i*25,-1,RGB(0,125,255))
				pydisplay.drawString(spacecraft["country"] + " (" + spacecraft["mission_type"] + ")",200,80+i*25,-1,RGB(0,125,255))
				i = i +1
		time.sleep(10)
	except:
		print("callback_spacecrafts exception")
	finally:
		bAction = False

def callback_diapo():
	try:
		global bAction
		bAction = True
		r = s.get(sUrlInfo1, headers= headersDict, proxies=proxyDict)
		jsontext = r.text
		response = json.loads(jsontext)
		pydisplay.clearScreen()
		i = 0
		for astro in response["people"]:
			localImageName = cacheDir + '/' + astro["name"].replace(" ","_") + ".BMP3"
			start = time.time()
			if ( astro["iss"] == True ):
				if ( not os.path.exists(localImageName) ):
					img = s.get(astro["image"],headers=headersAllDict, proxies=proxyDict)
					fileName = cacheDir + '/' + astro["image"].split("/")[-1:][0]
					open(fileName, 'wb').write(img.content)
					sCommand = "convert " + fileName + " -resize 480x320 -gravity center -extent 480x320 " + localImageName
					print(sCommand)
					os.system(sCommand)
				end = time.time()
				if ( (i > 0) and (end-start)<diapo_tempo):
					time.sleep(diapo_tempo-(end-start))
				pydisplay.showBMP(localImageName)
				pydisplay.clearScreenBottom();
				displayName = astro["name"].center(30)
				pydisplay.drawString(displayName,0,280,2,RGB(255,125,125),1)
				displayName = (astro["position"] + " " + astro["country"]).center(30)
				pydisplay.drawString(displayName,0,300,2,RGB(255,125,125),1)
				i = i +1
	except:
		print("callback_diapo exception")
	finally:
		bAction = False

def callback_shutdown():
	try:
		global bAction
		bAction = True
		print("shutdown")
		os.system("sudo /usr/sbin/shutdown now");
	except:
		print("callback_shutdown exception")
	finally:
		bAction = False



def GPIOSetup():
	# GPIO initialization 
	GPIO.setmode(GPIO.BCM) # use GPIO number and not pin numbers
	# le HIGH 
	GPIO.setup(20, GPIO.OUT, initial = GPIO.HIGH)	# HIGH reference
	GPIO.setup(16, GPIO.OUT, initial = GPIO.LOW)	# LED to blink

	GPIO.setup(5, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)	# display mission sign -  pin 29
	GPIO.setup(6, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)	# display crew photo - pin 31 
	GPIO.setup(13, GPIO.IN,pull_up_down=GPIO.PUD_DOWN)	# display crew members - pin 33
	GPIO.setup(19, GPIO.IN,pull_up_down=GPIO.PUD_DOWN)	# display currently docked spacecrafts - pin 35
	GPIO.setup(26, GPIO.IN,pull_up_down=GPIO.PUD_DOWN)	# display crew diaporama pin 37
	GPIO.setup(21, GPIO.IN,pull_up_down=GPIO.PUD_DOWN)	#  shutdown - pin 40

	GPIO.add_event_detect(5, GPIO.RISING, callback=callback_mission, bouncetime=300)
	GPIO.add_event_detect(6, GPIO.RISING, callback=callback_crewphoto, bouncetime=300)
	GPIO.add_event_detect(13, GPIO.RISING, callback=callback_crewmembers, bouncetime=300)
	GPIO.add_event_detect(19, GPIO.RISING, callback=callback_spacecrafts, bouncetime=300)
	GPIO.add_event_detect(26, GPIO.RISING, callback=callback_surprise, bouncetime=300)
	GPIO.add_event_detect(21, GPIO.RISING, callback=callback_shutdown, bouncetime=300)



def Init():
	global CurrentData
	if ( not os.path.exists(cacheDir)):
		os.mkdir(cacheDir)
	with open('iss.json') as json_file:
		CurrentData = json.load(json_file)

def Save():
	if ( CurrentData != None ):
		with open('iss.json', 'w') as outfile:
			outfile.write(json.dumps(CurrentData))


Init()
test = pydisplay.showBMP("world.bmp")
b = Blinker()
b.start()
r = s.get(sUrlCurrentPosition, headers= headersDict, proxies=proxyDict)
jsontext = r.text
response = json.loads(jsontext)
#print(response)
MyLat = response["lat"]
MyLon = response["lon"]
MyPos = ( MyLat, MyLon )
while True:
	try:
		r = s.get(sUrlPosition, headers= headersDict, proxies=proxyDict)
		jsontext = r.text
		response = json.loads(jsontext)
		position = response["iss_position"]
		longitude =int(round(float(position["longitude"])))
		latitude =int(round(float(position["latitude"])))
	#	print(longitude, latitude)
		if ( bAction == False ):
			test = pydisplay.showBMP("world.bmp")
			test = pydisplay.showISS(longitude, latitude)
			ISSPos = ( latitude, longitude )
			if ( geodesic( MyPos,ISSPos).km < 800 ):
				bBlink = True
			else
				bblick = False;
	except:
		print("Main loop exception");
	finally:
		time.sleep(10)

