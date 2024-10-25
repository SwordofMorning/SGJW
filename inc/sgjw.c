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
static int8_t Open_File_In_Binary(const char* filepath, uint8_t** buffer)
{
    int8_t retval = 0;

    /* ----- Step 1 : Open File ----- */

    FILE* file = fopen(filepath, "rb");
    if (file == NULL)
    {
        retval = -1;
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
        retval = -2;
        goto close_return;
    }

    /* ----- Step 4 : Write Buffer ----- */

    size_t read_size = fread(*buffer, 1, file_size, file);
    if (read_size != file_size)
    {
        free(*buffer);
        retval = -3;
    }

close_return:
    fclose(file);
direct_return:
    return retval;
}

int8_t State_Grid_JPEG_Reader(const char* path)
{
    int8_t retval = 0;

    /* ----- Step 1 : Read Data ----- */

    uint8_t* _bin_original = NULL;
    int8_t ret_original = Open_File_In_Binary(path, &_bin_original);
    if (ret_original != 0)
    {
        retval = -1;
        printf("Read file [%s] failed with error code: %d\n", path, ret_original);
        goto free_return;
    }

    printf("Read Success\n");

free_return:
    if (ret_original == 0)
        free(_bin_original);

    return retval;
}

int8_t State_Grid_JPEG_Writer(const char* path)
{
    // place holder
    return 0 > 1 ? 0 : 1;
}