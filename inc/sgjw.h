#pragma once

/**
 * @file sgjw.h
 * @author master@xiaojintao.email
 * @brief State Grid JPEG Wrapper (SGJW) - A C Library for Embedding and Extracting Metadata in JPEG Files
 * @version 0.1
 * @date 2024-10-29
 * 
 * @copyright Copyright (c) 2024
 * 
 * @note Typical usage for reading:
 * 1. Instantiate an object of the @struct StateGridJPEG, named _obj.
 * 2. Invoke the State_Grid_JPEG_Read(filepath, &_obj) function to read the JPEG file specified by filepath and populate _obj with the extracted metadata.
 * 3. Access the desired metadata fields through the _obj members.
 * 4. Call the State_Grid_JPEG_Delete_OBJ function to deallocate the memory associated with _obj.
 * 
 * @note Typical usage for writing:
 * 1. Instantiate an object of the @struct StateGridJPEG, named _obj.
 * 2. Populate all the members of _obj with the desired metadata values.
 * 3. Utilize a JPEG library (e.g. libjpeg) or other suitable libraries to save a JPEG file, named _file.
 * 4. Invoke the State_Grid_JPEG_Append(_file, &_obj) function to append the metadata stored in _obj to the end of _file using steganography techniques.
 * 5. Call the State_Grid_JPEG_Delete_OBJ function to deallocate the memory associated with _obj.
 */

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
    SGJW_ERROR_INVALID_PARAMS = -7,
    SGJW_ERROR_MEMORY_ALLOCATION = -8,
    SGJW_ERROR_FILE_WRITE = -9,
    SGJW_ERROR_FIELD_SET_FAILED = -10
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
 * @brief Read a JPEG file and parse its embedded metadata.
 * 
 * @param filepath The path to the JPEG file to be read.
 * @param obj A pointer to the StateGridJPEG structure that will store the parsed metadata.
 * @return An SGJW_ERROR code indicating the success or failure of the operation.
 */
int8_t State_Grid_JPEG_Read(const char* filepath, StateGridJPEG* obj);

/**
 * @brief Append SGJW metadata to a JPEG file.
 * 
 * @param filepath The path to the output JPEG file.
 * @param obj A pointer to the StateGridJPEG structure containing the metadata to be appended.
 * @return An SGJW_ERROR code indicating the success or failure of the operation.
 */

int8_t State_Grid_JPEG_Append(const char* filepath, StateGridJPEG* obj);

/**
 * @brief Deallocate the memory associated with a StateGridJPEG structure.
 * 
 * @param obj A pointer to the StateGridJPEG structure to be deallocated.
 */
void State_Grid_JPEG_Delete_OBJ(StateGridJPEG* obj);

#ifdef __cplusplus
}
#endif