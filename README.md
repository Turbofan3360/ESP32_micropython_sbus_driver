# ESP32 Micropython SBUS Driver #

### The Code: ###
SBUS is a common R/C serial protocol capable of transmitting up to 18 channels with only a single wire. I am using an ESP-32 to read data off my R8EF receiver.

Here, I am using UART 1 - here’s a full table of UARTS and their TX/RX pins for the ESP32-S3 WROOM I am using:

| *UART No.* | *TX Pin:* | *RX Pin:* |
|----------|---------|---------|
|  UART 0  |    1    |    3    |
|  UART 1  |   10    |    9    |
|  UART 2  |   17    |   16    |

The SbusReceive class handles all of the UART receiving, and then handles the required bit-shifting to get to the final channel values for the first 16 channels - returned as a list of ints. When initialised, you have to make sure to feed this class the UART TX pin that you are using. Don't forgot to connect up your receiver ground to the ESP32's ground as well!

The ChannelValues class is fed the data from SbusReceive, and outputs the sservo angles/percentages/PWM duty cycles as approriate to the R8EF receiver & SG90 (180 degree) servos I am using. This class can be easily modified to suit other servos and/or receivers.

*Note: I haven't implemented the 17th/18th channels in my SBUS code, as I my receiver doesn't utilise those channels. They are digital channels, and so need to be decoded differently to the others.*

I also integrated the Micropython SBUS code with a few PWM channels from the ESP32 as well - so you can use your ESP32 to drive servos (I used SG90 servos) based on the channel data received over SBUS.

### Implementation: ###

Please see the bottom of each code file for a more detailed example of how to implement the driver.

```python3
if __name__ == "__main__":
	sbus = SbusReceive(3)
    	channelvalues = ChannelValues()
	
	while True:
        	data = sbus.read_data()
        
        	if data:
            		print(channelvalues.get_duty_cycles(data))
```

### Embedded C Module: ###

For higher performance, in the embedded_c_module folder you will find the .c, .h and .cmake files to compile the SBUS driver into micropython firmware - there is a guide to compiling this below. 

This returns 0->100% values for each channel - if it returns values outside of this, the SBUS_MIN_CH_VALUE and SBUS_MAX_CH_VALUE definitions in the sbus.h header may need tweaking.

Example usage:
```python3
import sbus

pin_number = 2
port_number = 2

# NOTE: This embedded module requires the UART pin number and port number to be passed in
radio = sbus.SBUS(pin_number, port_number)

print(radio.read_data())
```

### Compiling the module into firmware: ###

To do this, you will need:
 - ESP-IDF cloned from github
 - Micropython cloned from github

1. Enter your esp-idf directory, and run ./install.sh (only needs to be run the first time)
2. Enter your esp-idf directory and run . ./export.sh (needs to be run every new terminal session)
3. Download the files from embedded_c_module and place them in a directory of your choosing
4. Enter your directory ~/micropython/ports/esp32 (can be replaced with whichever micropython board you are using if the code is suitably adapted)
5. Run the make command, specifying USER_C_MODULES=/path/to/sbus/embedded_c_module/micropython.cmake (replace with your file path)

For me, with an ESP32-S3 that has octal SPIRAM, the full make command is:
```
make BOARD=ESP32_GENERIC_S3 BOARD_VARIANT=SPIRAM_OCT USER_C_MODULES=/path/to/sbus/embedded_c_module/micropython.cmake
```


### References: ###
 - <https://github.com/Sokrates80/sbus_driver_micropython/tree/master>
 - <https://github.com/btrinite/SBUS-for-esp32>