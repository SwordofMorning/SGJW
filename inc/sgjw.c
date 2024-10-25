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

uint8_t const SGJW_EOF[] = {0x37, 0x66, 0x07, 0x1A, 0x12, 0x3A, 0x4C, 0x9F, 0xA9, 0x5D, 0x21, 0xD2, 0xDA, 0x7D, 0x26, 0xBC};
const size_t SGJW_IR_DATA_OFFSET_LENGTH = 4;

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
        offset = (buffer[offset_start] << 8 * 0) |
                 (buffer[offset_start + 1] << 8 * 1) |
                 (buffer[offset_start + 2] << 8 * 2) |
                 (buffer[offset_start + 3] << 8 * 3);
    }

    return offset;
}


/**
 * @brief Get file version.
 * 
 * @param buffer JPEG binary buffer.
 * @param offset offset of version.
 * @param length length of version.
 * @return file version .
 */
static uint16_t Binary_Get_Version(uint8_t* buffer, size_t offset, size_t length)
{
    uint16_t version = 0;

    for (int i = offset; i < offset + length; ++i)
    {
        version |= buffer[i] << 8 * i;
    }

    return version;
}


int8_t State_Grid_JPEG_Reader(const char* filepath)
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
    // uint16_t version = Binary_Get_Version(_bin_original, );


free_return:
    if (buffer_size > 0)
        free(_bin_original);

    return retval;
}

int8_t State_Grid_JPEG_Writer(const char* path)
{
    // place holder
    return 0 > 1 ? 0 : 1;
}