from machine import UART, Pin
import time

class SbusReceive:
    def __init__(self, pin):
        """
        Micropython SBUS RC Radio protocol receiver for an ESP32.
        
        This driver expects one input parameter - the number of the ESP32 pin that is connected to the signal pin of the radio receiver's SBUS port.
        Don't forget to connect the radio receiver's ground to the ESP32's ground!
        
        Potential methods:
            read_data() - returns a list of the raw channel values for the first 16 SBUS channels
        
        SBUS has the potential to support up to 18 channels, but channels 17 and 18 are digital. They are not supported by this driver
        """
        
        self.sbus = UART(1, 100000, rx=pin)
        self.sbus.init(100000, bits=8, parity=0, stop=2, invert=self.sbus.INV_RX)
            
    def read_data(self):
        """
        Method that returns a list of the raw values of the 16 channels encoded in each SBUS packet. Channels 17 and 18 are not supported.
        
        Data is returned as a list of integers.
        
        No parameters are required.
        """
        
        data = bytearray()
            
        while self.sbus.any() < 25:
            time.sleep_us(3) # Length of time it takes to send one SBUS frame
            
        data = self.sbus.read(25)
            
        index = data.find (b'\x00\x0f')
        while  index == -1:
            nextbytes = self.sbus.read(25)
                
            if nextbytes:
                data += nextbytes
                index = data.find (b'\x00\x0f')
            
        data = data[index+1:]
            
        while len(data) < 25:
            nextbytes = self.sbus.read(25 - len(data))
            if nextbytes:
                data += nextbytes
            
        data_decoded = self._extract_channel_data(data)
        return data_decoded
    
    @micropython.native
    def _extract_channel_data(self, frame):
        channels = 16*[0]
        byte_in_sbus = bit_in_sbus = ch = bit_in_channel = 0
        data = frame[1:23]
        
        for i in range(0, 176):
            if data[byte_in_sbus] & (1 << bit_in_sbus):
                channels[ch] |= (1 << bit_in_channel)            
            
            bit_in_sbus += 1
            bit_in_channel += 1
            
            if bit_in_sbus == 8:
                bit_in_sbus = 0
                byte_in_sbus += 1
                
            if bit_in_channel == 11:
                bit_in_channel = 0
                ch += 1
            
        return channels[:16]

class ChannelValues:
    def __init__(self):
        """
        Class that decodes the raw SBUS channel values into servo angles (for some channels) and percentages for others.
        
        Designed to suit the R8EF receiver and SG90 servos I'm using.
        
        Potential methods:
            get_control_values(sbus_channel_ints) - returns a list of aileron, elevator, rudder angles and throttle percentage
            get_switch_values(sbus_channel_ints) - returns the values of the switch channels (channels 4 and 6)
            get_aux_values(sbus_channel_ints) - returns the values of the aux channels (channels 5 and 7)
            get_duty_cycles(sbus_channel_ints) - returns the PWM duty cycle lengths (to control the SG90 servos) for each channel
            
        Required parameter for all these methods is the raw values of the SBUS channels, returned from the SbusReceive class
        """
        
        self.channels_angles = [0, 1, 3]
        self.channels_percentages = [2, 4, 5, 6, 7]
    
    @micropython.native
    def _find_duty_cycle(self, angle):
        duty_cycle = (angle/90)+1.5
        duty_cycle *= 51.2
        return int(duty_cycle)
    
    @micropython.native
    def get_control_values(self, channel_values):
        aileron_degrees = (channel_values[0]-1000)*0.09
        elavator_degrees = (channel_values[1]-1000)*0.09
        rudder_degrees = (channel_values[3]-1000)*0.09
        
        throttle_percent = int((channel_values[2]-200)/16)
        return aileron_degrees, elavator_degrees, rudder_degrees, throttle_percent
    
    @micropython.native
    def get_switch_values(self, channel_values):
        ch5 = (channel_values[4]-200)/16
        ch7 = (channel_values[6] -200)/16
        return ch5, ch7
    
    @micropython.native
    def get_aux_values(self, channel_values):
        aux1 = (channel_values[5]-200)/16
        aux2 = (channel_values[7]-200)/16
        return aux1, aux2
    
    @micropython.native
    def get_duty_cycles(self, channel_values):
        channels = 8*[0]
        
        for i in self.channels_angles:
            servo_angle = (channel_values[i]-1000)*0.09
            channels[i] = self._find_duty_cycle(servo_angle)
        
        for i in self.channels_percentages:
            channels[i] = (channel_values[i]-200)/16
        
        return channels


if __name__ == "__main__":
    from machine import PWM
    
    aileron_pin = 45
    elavator_pin = 13
    rudder_pin = 14
    sbus_pin = 3
    
    sbus = SbusReceive(sbus_pin)
    channelvalues = ChannelValues()

    aileron = PWM(Pin(aileron_pin), freq=50, duty_ns=1500)
    elavator = PWM(Pin(elavator_pin), freq=50, duty_ns=1500)
    rudder = PWM(Pin(rudder_pin), freq=50, duty_ns=1500)

    while True:
        data = sbus.read_data()
        
        if data:
            servo_throttle_angles = channelvalues.get_duty_cycles(data)
            
            aileron.duty(servo_throttle_angles[0])
            elavator.duty(servo_throttle_angles[1])
            rudder.duty(servo_throttle_angles[3])