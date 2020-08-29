#!/usr/bin/env python3

from AudioAnalyzer import AudioAnalyzer
import led
from Server import Server


leds = led.Controller(18, 23, 24, audioAnalyzer=AudioAnalyzer())
leds.start()

server = Server(leds, 3001)
server.start()

# def onAudioChange(metadata):
#     leds.setMode(led.Mode.MUSIC, volume=metadata["volume"], pitch=metadata["pitch"])

