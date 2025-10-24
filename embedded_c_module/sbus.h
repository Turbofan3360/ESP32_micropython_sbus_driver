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

// Constant definitions
#define SBUS_READ_TIMEOUT_MS 1000

// Object definition
typedef struct {
    mp_obj_base_t base;

    uart_port_t uart_number;
} sbus_obj_t;

// Function definitions
static int8_t find_sbus_frame_start(const uint8_t* array, uint8_t length);
static void extract_channel_data(const uint8_t* data_frame, uint16_t* output);

extern const mp_obj_type_t sbus_type;

#endif