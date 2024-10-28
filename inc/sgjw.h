#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct _StateGridJPEG
{
    // File version, @attention which is hex, 0x0100(big-endian) means version 1.0.
    uint16_t* version;
    // Width of matrix.
    uint16_t* width;
    // Height of matrix.
    uint16_t* height;
    // Date YYYYMMDDHHMMSS, 14 bytes.
    char* date;
    // Temperature matrix data, IEEE-754 Floating Point, 32 bits float. @attention Which is Celsius.
    float* matrix;
    // Emissivity, @attention range in [0, 1].
    float* emissivity;
    // Ambient temperature. @attention Which is Celsius.
    float* ambient_temp;
    // FOV.
    uint8_t* fov;
    // Distance.
    uint32_t* distance;
    // Humidity.
    uint8_t* humidity;
    // Reflective temperature. @attention Which is Celsius.
    float* reflective_temp;
    // Manufacturer, 32 bytes.
    char* manufacturer;
} StateGridJPEG;

int8_t State_Grid_JPEG_Reader(const char* filepath, StateGridJPEG* obj);

int8_t State_Grid_JPEG_Writer(const char* path);

void State_Grid_JPEG_Delete(StateGridJPEG* obj);

#ifdef __cplusplus
}
#endif