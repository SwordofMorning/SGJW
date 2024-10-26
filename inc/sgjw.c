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
const size_t SGJW_DATE_BYTES = 14;
// 4 bytes, used for temperature matrix.
const size_t SGJW_FLOAT32_BYTES = 4;
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

/* ====================================================================================================== */
/* ======================================== @par Static Function ======================================== */
/* ====================================================================================================== */

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
 * @brief Get 2 bytes int.
 * 
 * @param buffer JPEG binary buffer.
 * @param offset offset in buffer.
 * @return buffer: [offset, offset + 2).
 * 
 * @note Contains little-endian to big-endian conversion.
 */
static uint16_t Binary_Get_Uint16_L2B(uint8_t* buffer, size_t offset)
{
    // clang-format off
    uint16_t value = (buffer[offset] << 8 * 0) | 
                     (buffer[offset + 1] << 8 * 1);
    // clang-format on
    return value;
}

/**
 * @brief Get 4 bytes int.
 * 
 * @param buffer JPEG binary buffer.
 * @param offset offset in buffer.
 * @return buffer: [offset, offset + 4).
 * 
 * @note Contains little-endian to big-endian conversion.
 */
static uint32_t Binary_Get_Uint32_L2B(uint8_t* buffer, size_t offset)
{
    // clang-format off
    uint32_t value = (buffer[offset] << 8 * 0) |
                     (buffer[offset + 1] << 8 * 1) |
                     (buffer[offset + 2] << 8 * 2) |
                     (buffer[offset + 3] << 8 * 3);
    // clang-format on
    return value;
}

/**
 * @brief Get 4 bytes float.
 * 
 * @param buffer JPEG binary buffer.
 * @param offset offset in buffer.
 * @param length return buffer size, which always 4 bytes.
 * @return buffer: [offset, offset + length).
 * 
 * @note Contains little-endian to big-endian conversion.
 */
static float Binary_Get_Float32_L2B(uint8_t* buffer, size_t offset)
{
    union
    {
        uint32_t u32;
        float f32;
    } value;

    value.u32 = Binary_Get_Uint32_L2B(buffer, offset);

    return value.f32;
}

/**
 * @brief Get data as ASCII string.
 * 
 * @param buffer JPEG binary buffer.
 * @param offset offset in buffer.
 * @param length return buffer size.
 * @param ret write-back buffer.
 */
static void Binary_Get_Char(uint8_t* buffer, size_t offset, size_t length, char* ret)
{
    for (int i = offset; i < offset + length; ++i)
    {
        ret[i - offset] = buffer[i];
    }
}

/**
 * @brief Get steganography data offset.
 * 
 * @param buffer JPEG binary buffer.
 * @param buffer_size size of buffer.
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
        offset = Binary_Get_Uint32_L2B(buffer, offset_start);
    }

    return offset;
}

/**
 * @brief Get the JPEG's float temperature matrix.
 * 
 * @param buffer JPEG binary buffer.
 * @param offset offset in buffer.
 * @param width width of image.
 * @param height height of image.
 * @return float array, which size = width * height.
 * 
 * @note Contains little-endian to big-endian conversion.
 */
static float* Binary_Get_Matrix(uint8_t* buffer, size_t offset, size_t width, size_t height)
{
    size_t size = width * height;
    Debug("Picture size is: %d x %d = %d\n", width, height, size);

    float* float_mat = (float*)malloc(SGJW_FLOAT32_BYTES * size);
    if (float_mat == NULL)
    {
        Debug("Allocate memory for float matrix failed.\n");
        return NULL;
    }

    for (size_t i = 0; i < size; ++i)
    {
        float_mat[i] = Binary_Get_Float32_L2B(buffer, offset + i * 4);
    }

    return float_mat;
}

/* ========================================================================================== */
/* ======================================== @par API ======================================== */
/* ========================================================================================== */

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

    uint16_t version = Binary_Get_Uint16_L2B(_bin_original, offset);
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

    uint16_t width = Binary_Get_Uint16_L2B(_bin_original, offset);
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

    uint16_t height = Binary_Get_Uint16_L2B(_bin_original, offset);
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

    /* ----- Step 7 : Get DATA ----- */

    obj->date = (char*)malloc(sizeof(char) * SGJW_DATE_BYTES);
    if (obj->date == NULL)
    {
        retval = -7;
        Debug("Allocate memory for date failed.\n");
        goto free_return;
    }

    Binary_Get_Char(_bin_original, offset, SGJW_DATE_BYTES, obj->date);
    Debug("Capture date: [%s].\n", obj->date);

    offset += SGJW_DATE_BYTES;

    /* ----- Step 8 : Get Float Mat ----- */

    obj->matrix = Binary_Get_Matrix(_bin_original, offset, width, height);
    if (obj->matrix == NULL)
    {
        retval = -8;
        Debug("Get float matrix failed.\n");
        goto free_return;
    }
    Debug("Matrix[0][0] is: [%.2f]\n", obj->matrix[0]);
    Debug("Matrix[0][1] is: [%.2f]\n", obj->matrix[1]);
    Debug("Matrix[%d][%d] is: [%.2f]\n", width, height, obj->matrix[width * height - 1]);

    offset += width * height * SGJW_FLOAT32_BYTES;

    Debug("After Matrix, offset: [%x]\n", offset);

    /* ----- Step 9 : Get Emissivity ----- */

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

    // clang-format off
    void* pointers[] =
    {
        obj->version,
        obj->width,
        obj->height,
        obj->date,
        obj->matrix
    };
    // clang-format on

    for (int i = 0; i < sizeof(pointers) / sizeof(void*); i++)
    {
        if (pointers[i])
        {
            free(pointers[i]);
            pointers[i] = NULL;
            Debug("free ptr[%d]\n", i);
        }
    }
}