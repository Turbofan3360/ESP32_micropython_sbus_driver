#include "sbus.h"

mp_obj_t sbus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    /**
     * Checks all the given arguments, tests the micropython UART object, and handles initialization of the driver
     * Also initializes the micropython object which is passed back
    */
    uint8_t uart_pin, uart_id;

    // Checking arguments
    mp_arg_check_num(n_args, n_kw, 2, 2, false);

    // Getting arguments data
    uart_pin = mp_obj_get_uint(args[0]);
    uart_id = mp_obj_get_uint(args[1]);

    // Ensuring UART ID and Pin number are valid - configured for ESP32-S3
    if ((uart_id != 1) && (uart_id != 2)){
        mp_raise_msg(&mp_type_ValueError("UART ID can only be 1 or 2"));
    }
    if ((uart_pin < 0) || (uart_pin > 45)){
        mp_raise_msg(&mp_type_ValueError("Only 45 pins on ESP32-S3: Please enter valid pin number"));
    }

    if (uart_id == 1){
        uart_port_t uart_num = UART_NUM_1;
    }
    else {
        uart_port_t uart_num = UART_NUM_2;
    }

	// Configuring UART parameters
	uart_config_t uart_config = {
		.baud_rate = 100000,
		.data_bits = UART_DATA_8_BITS,
		.parity_type = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_2,
	};
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	ESP_ERROR_CHECK(uart_set_line_inverse(uart_num, UART_SIGNAL_RXD_INV));
	ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_PIN_NO_CHANGE, uart_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

	// Creating the ESP-IDF UART for RX only - 256 byte RXbuf, no TX
    ESP_ERROR_CHECK(uart_driver_install(uart_num, 256, 0, 0, NULL, 0));

    // Creating and allocating memory to the "self" instance of this module
    sbus_obj_t *self = m_new_obj(sbus_obj_t);

    // Initialising required data in the "self" object
    self->base.type = &sbus_type;
    self->uart_number = uart_num;

    return MP_OBJ_FROM_PTR(self);
}

static const uint8_t* mparray_to_int(mp_obj_t bytearray){
    /**
     * Converts from a micropython bytearray to array of C ints
     * I.e. converts from python bytearray to a C array type
    */
    mp_buffer_info_t buf_info;

    mp_get_buffer_raise(bytearray, &buf_info, MP_BUFFER_READ);

    const uint8_t *data = (const uint8_t *)buf_info.buf;

    return data;
}

static int16_t find_in_array(uint8_t *array, uint16_t length, uint8_t value_to_look_for, int16_t starting_point){
    /**
     * Utility to find the index of a specific value in an array
     * Basically, a C implementation of .find() in python
     * Returns the index of the character, or -1 if it's not found
    */
    uint16_t i;

    // Making sure the starting point is valid - some cases mean that starting_point might be passed in as -1
    if (starting_point < 0){
        starting_point = 0;
    }

    // Utility to search through an array for a specific value and return the index
    for (i = starting_point; i < length; i++){
        if (array[i] == value_to_look_for){
            return i;
        }
    }

    return -1;
}

mp_obj_t read_data(mp_obj_t self_in){
    /**
     * Function to read and return a SBUS data frame, broken down by channel
    */
    sbus_obj_t *self = MP_OBJ_TO_PTR(self_in);

    uint8_t int_data = NULL;
    nlr_buf_t cpu_state;


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