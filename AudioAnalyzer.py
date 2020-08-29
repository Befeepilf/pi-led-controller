#!/usr/bin/env python3

import time
import pyaudio
import numpy as np
from threading import Thread
import aubio


class AudioAnalyzer(Thread):
	def __init__(self, resultCallback = None):
		Thread.__init__(self)

		self.SAMPLERATE = 44100
		self.BUFFER_SIZE = 2048

		self.p = pyaudio.PyAudio()
		self.audio_stream = self.p.open(
			start=False,
			format=pyaudio.paFloat32,
			channels=1,
			rate=self.SAMPLERATE,
			input=True,
			output=False,
			frames_per_buffer=self.BUFFER_SIZE,
			stream_callback=self.callback
		)

		self.detectPitch = aubio.pitch("schmitt", 2 * self.BUFFER_SIZE, self.BUFFER_SIZE, self.SAMPLERATE)
		self.detectPitch.set_unit("Hz")

		self.applyLowPass = aubio.digital_filter(order=3)
		self.applyLowPass.set_biquad(
			1.26234556e-5,
			2.52469111e-5,
			1.26234556e-5,
			-1.98992401,
			0.98997451
		)

		self.applyHighPass = aubio.digital_filter(order=3)
		self.applyHighPass.set_biquad(
			0.95086723,
			-1.90173447,
			0.95086723,
			-1.89931967,
			0.90414926
		)

		self.resultCallback = resultCallback

	def run(self):
		self.stopped = False
		self.audio_stream.start_stream()

		while not self.stopped:
			# keep the thread active
			time.sleep(0.1)

	def stop(self):
		self.audio_stream.close()
		self.p.terminate()
		self.stopped = True

	def pause(self):
		if not self.audio_stream.is_stopped():
			self.audio_stream.stop_stream()

	def resume(self):
		if self.audio_stream.is_stopped():
			self.audio_stream.start_stream()


	def callback(self, audio_buffer, frame_count, time_info, status):
		audio_data = np.frombuffer(audio_buffer, dtype=np.float32) # min=-1, max=1	
		lows_data = self.applyLowPass(audio_data)

		lowsPitch = self.detectPitch(lows_data)[0]
		volume = np.max(np.absolute(audio_data))

		if self.resultCallback:
			self.resultCallback({
				"volume": volume.item(),
				"pitch": {
					"lows": lowsPitch.item()
				}
			})


		return (None, pyaudio.paContinue)