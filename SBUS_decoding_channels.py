from machine import UART

class SbusReceive:
    def __init__(self, port):
        self.sbus = UART(port, 100000)
        self.sbus.init(100000, bits=8, parity=0, stop=2, invert=self.sbus.INV_RX)
        
        self.TIMEOUT_PERIODS = 26
                    
    def get_data(self):
        timeout_flag = 0
        
        while timeout_flag < self.TIMEOUT_PERIODS:
            timeout_flag += 1
            testbytes = self.sbus.read(2)
            
            if testbytes == b'\x00\x0f':
                data = bytearray()
                
                while len(self.data) < 23:
                    nextbytes = self.sbus.read(23 - len(data))
                    if nextbytes:
                        data += nextbytes
                
                try:
                    data_decoded = self._extract_channel_data(data)
                except:
                    return None
                
                return data_decoded
        return None

    def _extract_channel_data(self, data):
        channels = 16*[0]
        byte_in_sbus = 0
        bit_in_sbus = 0
        ch = 0
        bit_in_channel = 0
        
        for i in range(0, 175):
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
            
        return channels[:8]

class ChannelValues:
    def __init__(self):
        self.channels_angles = [0, 1, 3]
        self.channels_percentages = [2, 4, 5, 6, 7]
    
    def get_control_values(self, channel_values):
        aileron_degrees = round((channel_values[0]-1000)*(9/160), 2)
        elavator_degrees = round((channel_values[1]-1000)*(9/160), 2)
        rudder_degrees = round((channel_values[3]-1000)*(9/160), 2)
        
        throttle_percent = round((channel_values[2]-200)/16, 2)
        return aileron_degrees, elavator_degrees, rudder_degrees, throttle_percent
    
    def get_switch_values(self, channel_values):
        ch5 = (channel_values[4]-200)/16
        ch7 = (channel_values[6] -200)/16
        return ch5, ch7
    
    def get_aux_values(self, channel_values):
        aux1 = round((channel_values[5]-200)/16, 2)
        aux2 = round((channel_values[7]-200)/16, 2)
        return aux1, aux2
    
    def get_values(self, channel_values):
        channels = 8*[0]
        
        for i in self.channels_angles:
            channels[i] = round((channel_values[i]-1000)*(9/160), 2)
        
        for i in self.channels_percentages:
            channels[i] = round((channel_values[i]-200)/16, 2)
        
        return channels


if __name__ == "__main__":
    import time
    
    sbus = SbusReceive(1)
    channelvalues = ChannelValues()

    while True:
        channels = sbus.get_data()

        if channels:
            print(channelvalues.get_values(channels)
                  
        time.sleep(1)
