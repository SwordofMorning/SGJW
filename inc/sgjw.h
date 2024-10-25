#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// 16 bytes char, Big-Endian, end of file label.
extern const uint8_t SGJW_EOF[];
// 4 bytes int, Little-Endian.
extern const size_t SGJW_OFFSET_BYTES;
// 2 bytes int, Little-Endian.
extern const size_t SGJW_VERSION_BYTES;
// 2 bytes int, Little-Endian.
extern const size_t SGJW_WIDTH_BYTES;
// 2 bytes int, Little-Endian.
extern const size_t SGJW_HEIGHT_BYTES;
// 14 bytes char, Big-Endian, ASCII: YYYYMMDDHHMMSS.
extern const size_t SGJW_TIME_BYTES;
// 4 bytes float, Little-Endian.
extern const size_t SGJW_EMISSIVITY_BYTES;
// 4 bytes float, Little-Endian.
extern const size_t SGJW_AMBIENT_TEMP_BYTES;
// 1 byte int.
extern const size_t SGJW_FOV_BYTES;
// 4 bytes int, Little-Endian.
extern const size_t SGJW_DISTANCE_BYTES;
// 1 byte int.
extern const size_t SGJW_HUMIDITY_BYTES;
// 4 bytes float, Little-Endian.
extern const size_t SGJW_REFLECTIVE_TEMP_BYTES;
// 32 bytes char, Big-Endian.
extern const size_t SGJW_MANUFACTURER_BYTES;
// 32 bytes char, Big-Endian.
extern const size_t SGJW_PRODUCT_BYTES;
// 32 bytes char, Big-Endian.
extern const size_t SGJW_SERIAL_BYTES;
// 8 bytes, float64 (not double), Little-Endian.
extern const size_t SGJW_LONGITUDE_BYTES;
// 8 bytes, float64 (not double), Little-Endian.
extern const size_t SGJW_LATITUDE_BYTES;
// 4 bytes int, Little-Endian.
extern const size_t SGJW_ALTITUDE_BYTES;
// 4 bytes int, Little-Endian, i.e. Description.
extern const size_t SGJW_APPENDIX_BYTES;

struct StateGridJPEG
{
    
};

int8_t State_Grid_JPEG_Reader(const char* path);

int8_t State_Grid_JPEG_Writer(const char* path);

#ifdef __cplusplus
}
#endif