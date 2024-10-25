#include "sgjw.h"

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
        printf("No such file: [%s].\n", filepath);
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
        printf("Malloc buffer fail.\n");
        goto close_return;
    }

    /* ----- Step 4 : Write Buffer ----- */

    size_t read_size = fread(*buffer, 1, file_size, file);
    if (read_size != file_size)
    {
        printf("Buffer size not equal file size.\n");
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

static int8_t File_Verification_EOF(uint8_t* buffer, size_t buffer_size)
{
    int8_t retval = 0;
    const uint8_t expected_end[] = {0x37, 0x66, 0x07, 0x1A, 0x12, 0x3A, 0x4C, 0x9F, 0xA9, 0x5D, 0x21, 0xD2, 0xDA, 0x7D, 0x26, 0xBC};
    const size_t expected_end_size = sizeof(expected_end);

    if (buffer_size < expected_end_size)
    {
        retval = -1;
    }
    else
    {
        size_t offset = buffer_size - expected_end_size;
        if (memcmp(buffer + offset, expected_end, expected_end_size) != 0)
        {
            retval = -1;
        }
    }

    return retval;
}

    // 2. get offsite:
    // 4 bytes before [0x37 0x66 0x07 0x1a 0x12 0x3a 0x4c 0x9f 0xa9 0x5d 0x21 0xd2 0xda 0x7d 0x26 0xbc]

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
        printf("Read file: [%s] failed.\n", filepath);
        goto free_return;
    }
    printf("Read Success\n");

    /* ----- Step 2 : Verification ----- */

    ret = File_Verification_EOF(_bin_original, buffer_size);
    if (ret != 0)
    {
        retval = -2;
        printf("File EOF label verification fail.\n");
        goto free_return;
    }
    printf("Verification Success\n");

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