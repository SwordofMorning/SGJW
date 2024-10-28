#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
typedef enum
{
    SGJW_SUCCESS = 0,
    SGJW_ERROR_FILE_NOT_FOUND = -1,
    SGJW_ERROR_MALLOC_FAILED = -2,
    SGJW_ERROR_READ_FAILED = -3,
    SGJW_ERROR_INVALID_EOF = -4,
    SGJW_ERROR_INVALID_OFFSET = -5,
    SGJW_ERROR_FIELD_READ_FAILED = -6,
    SGJW_ERROR_INVALID_PARAMS = -7
} SGJW_ERROR;

// Main structure
typedef struct
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
    // Product(type), 32 bytes.
    char* product;
    // Serial number, 32 bytes.
    char* sn;
    // Longitude, 8 bytes, IEEE-754 float64.
    double* longitude;
    // Latitude, 8 bytes, IEEE-754 float64.
    double* latitude;
    // Altitude 4 bytes.
    uint32_t* altitude;
    // Appendix information length, i.e. length of description.
    uint32_t* appendix_length;
    // Appendix information i.e. description.
    char* appendix;
} StateGridJPEG;

/**
 * @brief Read JPEG file and parse its content.
 * 
 * @param filepath Path to the JPEG file.
 * @param obj Pointer to StateGridJPEG structure.
 * @return SGJW_ERROR code.
 */
int8_t State_Grid_JPEG_Reader(const char* filepath, StateGridJPEG* obj);

/**
 * @brief Write StateGridJPEG structure to file.
 * 
 * @param path Path to output file.
 * @return SGJW_ERROR code.
 */
int8_t State_Grid_JPEG_Writer(const char* path);

/**
 * @brief Free all allocated memory in StateGridJPEG structure.
 * 
 * @param obj Pointer to StateGridJPEG structure.
 */
void State_Grid_JPEG_Delete(StateGridJPEG* obj);

#ifdef __cplusplus
}
#endif