# ESP32 Micropython SBUS Driver

SBUS is a common R/C serial protocol capable of transmitting 16 channels with only a single wire. I am trying to use an ESP-32 to read data off my R8EF receiver with the ultimate hope of creating an ESP-32 flight controller. On this page, I found that you can read data straight off the UART pins using: 

```
from machine import UART
uart = UART(1, 100000)
uart.init(100000, bits=8, parity=0, stop=2, invert=uart.INV_RX)
uart.read()
```

Here, I am using UART 1 - here’s a full table of UARTS and their TX/RX pins:



UART 0: TX 1, RX 3

UART 1: TX 10, RX 9

UART 2: TX 17, RX 16

After doing this and reading off the UART using uart.read() , I found I was reading (almost) exclusively nulls (0x00). After talking with other people, it appears I had misunderstood how buffers work when communicating via UARTs.

I then tried to use the uart.irq interrupt to read from the UART only when data comes through using the following code: 

However, this failed due to “AttributeError: 'UART' object has no attribute 'irq'” After googling, I discovered that uart.irq is annoyingly no longer supported in the new version of micropython.

I kept looking around for a replacement and found people have been using a module called uasyncio (which allows for multiple programs/modules to be running simultaneously by using a real-time clock to share CPU time between programs) to constantly monitor the UART and interrupt when it detects something.

At this point, I had taken a break to try and get servos working with my ESP-32s (see below). There, I was struggling until I realised that my servo and ESP-32 needed to have a common ground - i.e. connect servo ground and ESP-32 ground together. I then tried reading SBUS from my receiver again, connecting my receiver’s ground pin to my ESP-32’s ground pin. This completely solved my problem! I was using the following code:

```
from machine import UART

BAUD=100000

def initialise():
	uart = UART(1, BAUD)
	uart.init(BAUD, bits=8, parity=0, stop=2, invert=uart.INV_RX)
	return uart

uart = initialise()

while True:
	n = uart.any()
	if n:
    	chars = uart.read(n)
    	print("[{}] - {}".format(n,
                             	chars))
```

My next job now is to write code that can make sense of the data I’m getting.

Firstly, I had to get my code to sync up with the data stream. I did this by looking for the start and stop bytes - b’\x00\x0f’ and then reading 23 bytes (not reading start/stop bytes). After many attempts with varying degrees of success, I finally landed on a relatively simple solution that works reliably. See the code to do this in the file sync_to_sbus_stream.py above.

I then used code from this github repository (https://github.com/Sokrates80/sbus_driver_micropython/tree/master), slightly adjusted to fit into my code, to decode the data, converting the bytes into 11 bits per channel. I also implemented code to detect duplicate frames and avoided doing all the processing on them - a small improvement that may help me in future if I’m doing other heavy processing in the future. Due to a couple of issues and parts that weren't very efficient, I reworked my code in the read_data() method - ironically, there was an “if” statement that was supposed to be a “while”, but accidentally got changed - and for some reason, worked better than the “while” loop, which kept breaking! After much testing and debugging, though, I worked through all the issues and have a much better piece of code now. There is still, however, 1 5ms wait in the code, which I can't seem to get rid of as the code breaks without it. I've minimized the wait time as far as possible, but any help here would be appreciated!

I then implemented the class ChannelValues, which provides various methods to return servo angles (aileron/elavator/rudder) and percentages (for throttle/switches/aux channels) in different combinations when passed the decoded channel data. This has been customized to fit my T8FB/R8EF transmitter/receiver, and may need to be altered to suit other transmitters/receivers.

**Please note I haven't implemented the 17th/18th channels in my SBUS code, as I do not have a transmitter with that number of channels. They are digital channels, and so need to be decoded differently to the others.**

I then integrated code from my previous work with servos on the ESP32, which means that I can now move the control sticks on my R/C transmitter and get the servos on my R/C Plane to move! While this is cool, there is a bit of control lag, and the servo movements are a bit jittery. As a result, I re-worked my code such that it only goes to sync to the SBUS stream when it isn’t synced up (I did this by breaking out the syncing code into a separate function), which seems to have sped up the code quite a bit - although I’m not sure quite how well it’s worked! I will be doing more testing on this.
