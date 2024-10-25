#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int8_t State_Grid_JPEG_Reader(const char* path);

int8_t State_Grid_JPEG_Writer(const char* path);

#ifdef __cplusplus
}
#endif