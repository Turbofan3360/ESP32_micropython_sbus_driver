# ESP32 Micropython SBUS Driver #

### The Code: ###
SBUS is a common R/C serial protocol capable of transmitting up to 18 channels with only a single wire. I am using an ESP-32 to read data off my R8EF receiver - this code, specifically the ChannelValues class, can be modified easily to suit other receivers, however.

Here, I am using UART 1 - here’s a full table of UARTS and their TX/RX pins:

| *UART No.* | *TX Pin:* | *RX Pin:* |
|----------|---------|---------|
|  UART 0  |    1    |    3    |
|  UART 1  |   10    |    9    |
|  UART 2  |   17    |   16    |
|----------|---------|---------|

The SbusReceive class handles all of the UART receiving, and then handles the required bit-shifting to get to the final channel values for the first 16 channels - returned as a list of ints. When initialised, you have to make sure to feed this class the UART TX pin that you are using. Don't forgot to connect up your receiver ground to the ESP32's group as well!

*Note: I haven't implemented the 17th/18th channels in my SBUS code, as I my receiver doesn't utilise those channels. They are digital channels, and so need to be decoded differently to the others.*

I also integrated the SBUS code with a few PWM channels from the ESP32 as well - so you can use your ESP32 to drive servos (I used SG90 servos) based on the channel data received over SBUS. This has quite a high lag currently - hopefully I will be able to improve that in future! 

### Implementation: ###

Please see the bottom of each code file for an example of how to implement the driver.

```python3
if __name__ == "__main__":
	...
```

### References: ###
 - <https://github.com/Sokrates80/sbus_driver_micropython/tree/master>
 - <https://github.com/btrinite/SBUS-for-esp32>
