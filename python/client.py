import zmq
import random
import time


def noteon(socket, note, velocity):
    a = bytes([0x90, note, velocity])
    socket.send(a)


def noteoff(socket, note):
    a = bytes([0x80, note, 0])
    socket.send(a)


context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind("tcp://*:5556")

while True:
    note = random.randrange(20, 100)

    noteon(socket, note, 0x67)
    noteon(socket, note + 3, 0x67)
    noteon(socket, note + 5, 0x67)
    time.sleep(0.1)

    noteoff(socket, note)
    noteoff(socket, note + 3)
    noteoff(socket, note + 5)
    time.sleep(1)
