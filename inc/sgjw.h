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
}StateGridJPEG;

int8_t State_Grid_JPEG_Reader(const char* filepath, StateGridJPEG* obj);

int8_t State_Grid_JPEG_Writer(const char* path);

void State_Grid_JPEG_Delete(StateGridJPEG* obj);

#ifdef __cplusplus
}
#endif