#!/usr/bin/env python3

import asyncio
import json
import websockets
import time
from threading import Thread
from functools import partial
import led


class Server(Thread):
  def __init__(self, leds, port):
    Thread.__init__(self)

    self.loop = asyncio.new_event_loop()
    asyncio.set_event_loop(self.loop)

    self.leds = leds
    self.leds.stateChangeListener = self.onLedStateChange

    self.PORT = port
    self.clients = set()

    self.setupMessage = json.dumps({
      "type": "setup",
      "payload": {
        "modes": [mode.name for mode in led.Mode]
      }
    })


  def run(self):
    print("Starting server on port {}...".format(self.PORT))

    startServer = websockets.serve(self.handleClient, "", self.PORT, loop=self.loop)
    self.loop.run_until_complete(startServer)
    self.loop.run_forever()

  async def handleClient(self, client, path):
    print("New client connected")

    self.clients.add(client)

    try:
      await client.send(self.setupMessage)
      self.sendLedStateUpdate(client, 1000 * time.time(), self.leds.state)

      async for message in client:
        self.onClientMessage(json.loads(message))

    finally:
      print("Client disconnected")
      self.clients.remove(client)

  def onClientMessage(self, message):
    type = message["type"]
    timestamp = message["timestamp"]
    payload = message["payload"]

    if "mode" in payload:
      payload["mode"] = led.Mode[payload["mode"]]
    if "color" in payload:
      payload["color"] = led.Color(payload["color"])

    self.leds.setState(payload, timestamp)

  def sendLedStateUpdate(self, client, timestamp, stateUpdate):
    # copy state
    stateUpdate = {key: value for key, value in stateUpdate.items()}
    
    if "mode" in stateUpdate:
      stateUpdate["mode"] = stateUpdate["mode"].name
    if "color" in stateUpdate:
      stateUpdate["color"] = stateUpdate["color"].toHex()

    task = asyncio.tasks.ensure_future(client.send(json.dumps({
      "type": "stateChanged",
      "timestamp": timestamp,
      "payload": stateUpdate
    })))

  def onLedStateChange(self, timestamp, stateUpdate):
    for client in self.clients:
      self.loop.call_soon_threadsafe(self.sendLedStateUpdate, client, timestamp, stateUpdate)