#include "sbus.h"

mp_obj_t sbus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    /**
     * Checks all the given arguments, tests the micropython UART object, and handles initialization of the driver
     * Also initializes the micropython object which is passed back
    */
	uart_port_t uart_num;
    uint8_t uart_pin, uart_id;
	nlr_buf_t cpu_state;

    // Checking arguments
    mp_arg_check_num(n_args, n_kw, 2, 2, false);

    // Getting arguments data
    uart_pin = mp_obj_get_uint(args[0]);
    uart_id = mp_obj_get_uint(args[1]);

    // Ensuring UART ID and Pin number are valid - configured for ESP32-S3
    if ((uart_id != 1) && (uart_id != 2)){
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("UART ID can only be 1 or 2"));
    }
    if (uart_pin > 45){
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Only 45 pins on ESP32-S3: Please enter valid pin number"));
    }

    if (uart_id == 1){
        uart_num = UART_NUM_1;
    }
    else {
        uart_num = UART_NUM_2;
    }

	// Configuring UART parameters
	uart_config_t uart_config = {
		.baud_rate = 100000,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_EVEN,
		.stop_bits = UART_STOP_BITS_2,
	};

	if (nlr_push(&cpu_state) == 0){
		// Configuring UART
		uart_param_config(uart_num, &uart_config);
		uart_set_line_inverse(uart_num, UART_SIGNAL_RXD_INV);
		uart_set_pin(uart_num, UART_PIN_NO_CHANGE, uart_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		// Creating the ESP-IDF UART for RX only - 256 byte RXbuf, no TX
    	uart_driver_install(uart_num, 256, 0, 0, NULL, 0);

		nlr_pop();
	}
	else {
		mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("UART object failed to initialize"));
	}

    // Creating and allocating memory to the "self" instance of this module
    sbus_obj_t *self = m_new_obj(sbus_obj_t);

    // Initialising required data in the "self" object
    self->base.type = &sbus_type;
    self->uart_number = uart_num;

    return MP_OBJ_FROM_PTR(self);
}

static int16_t find_sbus_frame_start(uint8_t *array, uint16_t length){
    /**
     * Utility to find the index of the start of the SBUS data frame
     * Basically, a C implementation of .find() in python
     * Returns the index of the start, or -1 if it's not found
    */
    uint16_t i;

    for (i = 0; i < length - 1; i++){
        if ((array[i] == 0x00) && (array[i + 1] == 0x0F)){
            return i + 1;
        }
    }

    return -1;
}

static void extract_channel_data(uint8_t* data_frame, uint16_t* output){
	/**
	 * Function to do all the required bit-shifting to get the actual SBUS channel values out
	*/
	uint8_t i, byte_in_sbus = 1, bit_in_sbus = 0, bit_in_channel = 0, channel = 0;

	for (i = 0; i < 176; i++){
		if (data_frame[byte_in_sbus] & (1 << bit_in_sbus)){
			output[channel] |= 1 << bit_in_channel;
		}

		bit_in_sbus++;
		bit_in_channel++;

		if (bit_in_sbus == 8){
			bit_in_sbus = 0;
			byte_in_sbus ++;
		}

		if (bit_in_channel == 11){
			bit_in_channel = 0;
			channel++;
		}

		// Only bothering with first 16 channels - 17 & 18 are encoded digitally, so need to be extracted differently
		if (channel == 16){
			return;
		}
	}
}

mp_obj_t read_data(mp_obj_t self_in){
    /**
     * Function to read and return a SBUS data frame, broken down by channel
    */
    sbus_obj_t *self = MP_OBJ_TO_PTR(self_in);

	uint8_t *int_data = NULL, data_temp[32], data_frame[25], length_read, data_len = 0, i;
	uint16_t channels[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int8_t index = -1;
	uint32_t start_time = mp_hal_ticks_ms();
	nlr_buf_t cpu_state;

	// Reads until it finds start of a data frame AND 25 bytes after that
	while ((index == -1) || (data_len - index < 25)){
		if (mp_hal_ticks_ms() - start_time > 1000){
			mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("UART read timed out: No data"));
		}

		// Reading new data
		length_read = uart_read_bytes(self->uart_number, data_temp, 32, 1);

		// Allocating more memory
		int_data = (uint8_t*) realloc(int_data, data_len + length_read);

		if ((int_data == NULL) && (data_len != 0)){
			mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Could not allocate data array memory"));
		}

		// Adding new data to the array
		memcpy(int_data + data_len, data_temp, length_read);
		data_len += length_read;

		// Searching for start of data frame in array (only needs to be done if the start hasn't already been found)
		if (index == -1){
			index = find_sbus_frame_start(int_data, data_len);
		}
	}

	// Clipping out data frame
	memcpy(data_frame, int_data + index, 25);
	free(int_data);

	//Extracting raw channel data
	extract_channel_data(data_frame, channels);

	// Creating a micropython array object to return
	mp_obj_t retvals[16];
	for (i = 0; i < 16; i++){
		retvals[i] = mp_obj_new_int(channels[i]);
	}

	return mp_obj_new_list(16, retvals);
}
static MP_DEFINE_CONST_FUN_OBJ_1(sbus_read_data_obj, read_data);



/**
 * Code here exposes the module functions above to micropython as an object
*/

// Defining the functions that are exposed to micropython
static const mp_rom_map_elem_t sbus_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_read_data), MP_ROM_PTR(&sbus_read_data_obj)},
};
static MP_DEFINE_CONST_DICT(sbus_locals_dict, sbus_locals_dict_table);

// Overall module definition
MP_DEFINE_CONST_OBJ_TYPE(
    sbus_type,
    MP_QSTR_sbus,
    MP_TYPE_FLAG_NONE,
    make_new, sbus_make_new,
    locals_dict, &sbus_locals_dict
);

// Defining global constants
static const mp_rom_map_elem_t sbus_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__) , MP_ROM_QSTR(MP_QSTR_sbus) },
    { MP_ROM_QSTR(MP_QSTR_SBUS), MP_ROM_PTR(&sbus_type) },
};
static MP_DEFINE_CONST_DICT(sbus_globals_table, sbus_module_globals_table);

// Creating module object
const mp_obj_module_t sbus_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&sbus_globals_table,
};

MP_REGISTER_MODULE(MP_QSTR_sbus, sbus_module);