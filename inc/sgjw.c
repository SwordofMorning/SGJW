#include "sgjw.h"

#ifndef SGJW_DEBUG
#define SGJW_DEBUG 1
#endif

static void Debug(const char* format, ...)
{
#if SGJW_DEBUG
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif
}

// 16 bytes char, Big-Endian, end of file label.
const uint8_t SGJW_EOF[] = {0x37, 0x66, 0x07, 0x1A, 0x12, 0x3A, 0x4C, 0x9F, 0xA9, 0x5D, 0x21, 0xD2, 0xDA, 0x7D, 0x26, 0xBC};
// 4 bytes int, Little-Endian.
const size_t SGJW_OFFSET_BYTES = 4;
// 2 bytes int, Little-Endian, @attention which is HEX, i.e. 0x0001(little-endian) means version 1.0, not version 16.
const size_t SGJW_VERSION_BYTES = 2;
// 2 bytes int, Little-Endian.
const size_t SGJW_WIDTH_BYTES = 2;
// 2 bytes int, Little-Endian.
const size_t SGJW_HEIGHT_BYTES = 2;
// 14 bytes char, Big-Endian, ASCII: YYYYMMDDHHMMSS.
const size_t SGJW_TIME_BYTES = 14;
// 4 bytes float, Little-Endian.
const size_t SGJW_EMISSIVITY_BYTES = 4;
// 4 bytes float, Little-Endian.
const size_t SGJW_AMBIENT_TEMP_BYTES = 4;
// 1 byte int.
const size_t SGJW_FOV_BYTES = 1;
// 4 bytes int, Little-Endian.
const size_t SGJW_DISTANCE_BYTES = 4;
// 1 byte int.
const size_t SGJW_HUMIDITY_BYTES = 1;
// 4 bytes float, Little-Endian.
const size_t SGJW_REFLECTIVE_TEMP_BYTES = 4;
// 32 bytes char, Big-Endian.
const size_t SGJW_MANUFACTURER_BYTES = 32;
// 32 bytes char, Big-Endian.
const size_t SGJW_PRODUCT_BYTES = 32;
// 32 bytes char, Big-Endian.
const size_t SGJW_SERIAL_BYTES = 32;
// 8 bytes, float64 (not double), Little-Endian.
const size_t SGJW_LONGITUDE_BYTES = 8;
// 8 bytes, float64 (not double), Little-Endian.
const size_t SGJW_LATITUDE_BYTES = 8;
// 4 bytes int, Little-Endian.
const size_t SGJW_ALTITUDE_BYTES = 4;
// 4 bytes int, Little-Endian, i.e. Description.
const size_t SGJW_APPENDIX_BYTES = 4;

/**
 * @brief Read file in binary, then copy data to buffer.
 * 
 * @param filepath which file need to open.
 * @param buffer uint8_t buffer for storing file.
 * @return success or not.
 * @retval 0, success.
 * @retval -1, no such file.
 * @retval -2, malloc buffer fail.
 * @retval -3, read from file fail.
 */
static size_t Open_File_In_Binary(const char* filepath, uint8_t** buffer)
{
    /* ----- Step 1 : Open File ----- */

    FILE* file = fopen(filepath, "rb");
    if (file == NULL)
    {
        Debug("No such file: [%s].\n", filepath);
        goto direct_return;
    }

    /* ----- Step 2 : Get File Size ----- */

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    /* ----- Step 3 : Malloc ----- */

    *buffer = (uint8_t*)malloc(file_size * sizeof(uint8_t));
    if (*buffer == NULL)
    {
        Debug("Malloc buffer fail.\n");
        goto close_return;
    }

    /* ----- Step 4 : Write Buffer ----- */

    size_t read_size = fread(*buffer, 1, file_size, file);
    if (read_size != file_size)
    {
        Debug("Buffer size not equal file size.\n");
        goto free_return;
    }

    return read_size;

free_return:
    free(*buffer);
close_return:
    fclose(file);
direct_return:
    return 0;
}

/**
 * @brief Verification whether the buffer end with 
 * 0x37, 0x66, 0x07, 0x1A, 0x12, 0x3A, 0x4C, 0x9F, 0xA9, 0x5D, 0x21, 0xD2, 0xDA, 0x7D, 0x26, 0xBC
 * 
 * @param buffer JPEG binary buffer.
 * @param buffer_size size of buffer
 * @return success or not.
 * @retval 0, success.
 * @retval -1, invalid file.
 */
static int8_t Binary_Verification_EOF(uint8_t* buffer, size_t buffer_size)
{
    int8_t retval = 0;
    const size_t expected_end_size = sizeof(SGJW_EOF);

    if (buffer_size < expected_end_size)
    {
        retval = -1;
    }
    else
    {
        size_t offset = buffer_size - expected_end_size;
        if (memcmp(buffer + offset, SGJW_EOF, expected_end_size) != 0)
        {
            retval = -1;
        }
    }

    return retval;
}

/**
 * @brief Get steganography data offset.
 * 
 * @param buffer JPEG binary buffer.
 * @param buffer_size size of buffer
 * @return offset.
 */
static size_t Binary_Get_Offsite(uint8_t* buffer, size_t buffer_size)
{
    size_t offset = 0;
    const size_t expected_end_size = 16;
    const size_t offset_size = 4;

    if (buffer_size >= expected_end_size + offset_size)
    {
        size_t offset_start = buffer_size - expected_end_size - offset_size;
        // clang-format off
        offset = (buffer[offset_start] << 8 * 0) |
                 (buffer[offset_start + 1] << 8 * 1) |
                 (buffer[offset_start + 2] << 8 * 2) |
                 (buffer[offset_start + 3] << 8 * 3);
        // clang-format on
    }

    return offset;
}


/**
 * @brief Get 2 bytes data.
 * 
 * @param buffer JPEG binary buffer.
 * @param offset return begin.
 * @param length return buffer size.
 * @return buffer: [offset, offset + length).
 * 
 * @note Contains little-endian to big-endian conversion.
 */
static uint16_t Binary_Get_Uint16_L2B(uint8_t* buffer, size_t offset, size_t length)
{
    uint16_t retval = 0;

    for (int i = offset; i < offset + length; ++i)
    {
        retval |= buffer[i] << 8 * (i - offset);
    }

    return retval;
}

int8_t State_Grid_JPEG_Reader(const char* filepath, StateGridJPEG* obj)
{
    int8_t retval = 0;
    int8_t ret;

/* ----- Step 1 : Read Data ----- */

    uint8_t* _bin_original = NULL;
    size_t buffer_size = Open_File_In_Binary(filepath, &_bin_original);
    if (buffer_size == 0)
    {
        retval = -1;
        Debug("Read file: [%s] failed.\n", filepath);
        goto free_return;
    }
    Debug("Read Success\n");

/* ----- Step 2 : Verification ----- */

    ret = Binary_Verification_EOF(_bin_original, buffer_size);
    if (ret != 0)
    {
        retval = -2;
        Debug("File EOF label verification fail.\n");
        goto free_return;
    }
    Debug("Verification Success\n");

/* ----- Step 3 : Get Offset ----- */

    size_t offset = Binary_Get_Offsite(_bin_original, buffer_size);
    if (offset == 0)
    {
        retval = -3;
        Debug("Get offset fail, which is 0.\n");
        goto free_return;
    }
    Debug("Offset is: [%x][%d]\n", offset, offset);

/* ----- Step 4 : Get Version ----- */

    uint16_t version = Binary_Get_Uint16_L2B(_bin_original, offset, SGJW_VERSION_BYTES);
    Debug("Version is: [%x][%d]\n", version, version);
    
    obj->version = (uint16_t*)malloc(sizeof(uint16_t));
    if (obj->version == NULL)
    {
        retval = -4;
        Debug("Allocate memory for version failed.\n");
        goto free_return;
    }

    *(obj->version) = version;
    offset += SGJW_VERSION_BYTES;

/* ----- Step 5 : Get Width ----- */

    uint16_t width = Binary_Get_Uint16_L2B(_bin_original, offset, SGJW_WIDTH_BYTES);
    Debug("Width is: [%x][%d]\n", width, width);

    obj->width = (uint16_t*)malloc(sizeof(uint16_t));
    if (obj->width == NULL)
    {
        retval = -5;
        Debug("Allocate memory for width failed.\n");
        goto free_return;
    }

    *(obj->width) = width;
    offset += SGJW_WIDTH_BYTES;

/* ----- Step 6 : Get Height ----- */

    uint16_t height = Binary_Get_Uint16_L2B(_bin_original, offset, SGJW_HEIGHT_BYTES);
    Debug("Height is: [%x][%d]\n", height, height);

    obj->height = (uint16_t*)malloc(sizeof(uint16_t));
    if (obj->height == NULL)
    {
        retval = -6;
        Debug("Allocate memory for height failed.\n");
        goto free_return;
    }

    *(obj->height) = height;
    offset += SGJW_HEIGHT_BYTES;

free_return:
    if (buffer_size > 0)
        free(_bin_original);

    return retval;
}

int8_t State_Grid_JPEG_Writer(const char* path)
{
    // todo
    return 0;
}

void State_Grid_JPEG_Delete(StateGridJPEG* obj)
{
    if (!obj)
        return;

    if (obj->version)
    {
       free(obj->version);
       obj->version = NULL;
    }
    if (obj->width)
    {
       free(obj->width);
       obj->width = NULL;
    }
    if (obj->height)
    {
       free(obj->height);
       obj->height = NULL;
    }
}