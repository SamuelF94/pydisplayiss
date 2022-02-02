#!/usr/bin/python

import sys
import requests
import datetime, time
import json
import threading 
import pydisplay

test = pydisplay.showBMP("world.bmp")

http_proxy  = "http://192.168.50.10:8118"
https_proxy = "http://192.168.50.10:8118"
ftp_proxy   = "http://192.168.50.10:8118"

proxyDict = { 
              "http"  : http_proxy, 
              "https" : https_proxy, 
              "ftp"   : ftp_proxy
            }


headersDict = {"Accept": "application/json" }

s = requests.Session()
sUrl = 'http://api.open-notify.org/iss-now.json'
while True:
	r = s.get(sUrl, headers= headersDict, proxies=proxyDict)
	jsontext = r.text
	response = json.loads(jsontext)
	position = response["iss_position"]
	longitude =int(round(float(position["longitude"])))
	latitude =int(round(float(position["latitude"])))
#	print(longitude, latitude)
	test = pydisplay.showISS(longitude, latitude)
	sleep(10)

