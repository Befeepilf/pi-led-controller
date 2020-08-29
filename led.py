#!/usr/bin/env python3

import time
import math
from enum import Enum
from threading import Thread
import pigpio


class Color:
	def __init__(self, red, green=None, blue=None):
		self.__radd__ = self.__add__
		self.__rsub__ = self.__sub__
		self.__rmul__ = self.__mul__
		self.__rtruediv__ = self.__truediv__

		if green == None and blue == None:
			if type(red) is str:
				# red = hex code

				hex = red.lstrip('#')
				hexLen = len(hex)
				
				rgb = tuple(int(hex[i:i + hexLen // 3], 16) for i in range(0, hexLen, hexLen // 3))
				
				self.red = rgb[0]
				self.green = rgb[1]
				self.blue = rgb[2]
			else:
				self.red = red
				self.green = red
				self.blue = red
		else:
			self.red = red
			self.green = green
			self.blue = blue

	def toHex(self):
		return "#{:02x}{:02x}{:02x}".format(
			int(self.red),
			int(self.green),
			int(self.blue)
		)

	def copy(self):
		return Color(self.red, self.green, self.blue)

	def __add__(self, color):
		return Color(
			self.red + color.red,
			self.green + color.green,
			self.blue + color.blue
		)

	def __iadd__(self, color):
		self.red = self.red + color.red
		self.green = self.green + color.green
		self.blue = self.blue + color.blue
		return self

	def __sub__(self, color):
		return Color(
			self.red - color.red,
			self.green - color.green,
			self.blue - color.blue
		)

	def __isub__(self, color):
		self.red = self.red - color.red
		self.green = self.green - color.green
		self.blue = self.blue - color.blue
		return self

	def __mul__(self, factor):
		return Color(
			self.red * factor,
			self.green * factor,
			self.blue * factor
		)

	def __imul__(self, factor):
		self.red *= factor
		self.green *= factor
		self.blue *= factor
		return self

	def __truediv__(self, divider):
		return Color(
			self.red / divider,
			self.green / divider,
			self.blue / divider
		)

	def __itruediv__(self, divider):
		self.red /= divider
		self.green /= divider
		self.blue /= divider
		return self


colors = [Color(15, 0, 0)]

# to violet
for i in range(255):
	colors.append(Color(255, 0, i + 1))
# to blue
for i in range(256):
	colors.append(Color(255 - i, 0, 255))
# to green
for i in range(256):
	colors.append(Color(0, i, 255 - i))
# to red
for i in range(256):
	colors.append(Color(i, 255 - i, 0))

MAX_COLOR_INDEX = len(colors) - 1


class Mode(Enum):
	OFF = 0
	MANUAL = 1
	AMBIENT = 2
	MUSIC = 3


class Controller(Thread):
	def __init__(self, redBoardPin, greenBoardPin, blueBoardPin, audioAnalyzer = None, stateChangeListener = None):
		Thread.__init__(self)
		
		self.gpio = pigpio.pi()

		self.pins = {
			"red": redBoardPin,
			"green": greenBoardPin,
			"blue": blueBoardPin
		}

		self.audioAnalyzer = audioAnalyzer
		self.audioAnalyzer.resultCallback = self.__onAudioChange
		self.audioMetadata = None

		self.state = {
			"mode": Mode.OFF,
			"modeOptions": {},
			"brightness": 1,
			"color": Color(0)
		}

		self.stateUpdaters = {
			"mode": self.__setMode,
			"modeOptions": self.__setModeOptions,
			"color": self.__setColor
		}

		self.isLooping = False
		self.stateChangeListener = stateChangeListener

		self.__setMode(Mode.MUSIC)



	def setState(self, stateUpdate, timestamp=1000*time.time()):
		for propertyName, value in stateUpdate.items():
			if propertyName in self.stateUpdaters:
				self.stateUpdaters[propertyName](value)
			else:
				self.state[propertyName] = value

		self.__stateChanged(timestamp, stateUpdate)

	def __stateChanged(self, timestamp, stateUpdate):
		if self.stateChangeListener:
			self.stateChangeListener(timestamp, stateUpdate)


	def __setMode(self, mode):
		if mode == Mode.MUSIC:
			if not self.audioAnalyzer.is_alive():
				self.audioAnalyzer.start()
			else:
				self.audioAnalyzer.resume()

			self.__setModeOptions({
				"smoothing": 5,
				"minBrightness": 0.1,
				"maxBrightness": 1
			})

		else:
			self.audioAnalyzer.pause()
		
		self.state["mode"] = mode

	def __setModeOptions(self, modeOptions):
		for key, value in modeOptions.items():
			self.state["modeOptions"][key] = value

	def __setColor(self, color, smoothing=0):
		if smoothing == 0:
			self.state["color"] = color
			self.__applyColor()
		else:
			colorDiff = color - self.state["color"]
			numSteps = min(smoothing, int(max(
				abs(colorDiff.red),
				abs(colorDiff.green),
				abs(colorDiff.blue)
			)))

			if numSteps == 0:
				self.state["color"] = color
				self.__applyColor()
				return

			colorDiff /= numSteps
			
			for i in range(numSteps):
				self.state["color"] += colorDiff
				self.__applyColor()


	def __applyColor(self):
		# this method takes quite some time to execute
		for channel in ("red", "green", "blue"):
			self.gpio.set_PWM_dutycycle(
				self.pins[channel],
				self.state["brightness"] * getattr(self.state["color"], channel)
			)
		

	def __onAudioChange(self, audioMetadata):
		self.audioMetadata = audioMetadata

	def run(self):
		print("Starting LED event loop...")

		self.isLooping = True

		while self.isLooping:
			if self.state["mode"] == Mode.OFF:
				self.__setColor(Color(0), smoothing=100)

			elif self.state["mode"] == Mode.AMBIENT:
				self.__setColor(Color(200, 0, 0), smoothing=100)

			elif self.state["mode"] == Mode.MUSIC:
				if self.audioMetadata:
					maxLowsPitch = 200
					lowsPitch = min(maxLowsPitch, self.audioMetadata["pitch"]["lows"])

					minBrightness = self.state["modeOptions"]["minBrightness"]
					maxBrightness = self.state["modeOptions"]["maxBrightness"]
					brightness = (maxBrightness - minBrightness) * self.audioMetadata["volume"] + minBrightness

					colorIndex = int(lowsPitch / maxLowsPitch * MAX_COLOR_INDEX)
					color = colors[colorIndex].copy()

					if colorIndex != 0:
						color *= brightness

					self.__setColor(color, smoothing=self.state["modeOptions"]["smoothing"])

			elif self.state["mode"] == Mode.MANUAL:
				pass


	def stop(self):
		self.isLooping = False