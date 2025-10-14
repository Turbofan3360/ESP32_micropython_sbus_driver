#ifndef SBUS_H
#define SBUS_H

#include <stdlib.h>
#include <stdint.h>

#include "py/runtime.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/mphal.h"
#include "py/nlr.h"

#include "driver/uart.h"
#include "esp_err.h"

// Constant definitions

// Object definition
typedef struct {
    mp_obj_base_t base;

    uart_port_t uart_number;
} sbus_obj_t;

// Function definitions
static const uint8_t* mparray_to_int(mp_obj_t bytearray)
static int16_t find_in_array(uint8_t *array, uint16_t length, uint8_t value_to_look_for, int16_t starting_point)


extern const mp_obj_type_t sbus_type;

#endif