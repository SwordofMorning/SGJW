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
const size_t SGJW_SN_BYTES = 32;
// 8 bytes, float64 (not double), Little-Endian.
const size_t SGJW_LONGITUDE_BYTES = 8;
// 8 bytes, float64 (not double), Little-Endian.
const size_t SGJW_LATITUDE_BYTES = 8;
// 4 bytes int, Little-Endian.
const size_t SGJW_ALTITUDE_BYTES = 4;
// 4 bytes int, Little-Endian, i.e. Description.
const size_t SGJW_APPENDIX_LENGTH_BYTES = 4;

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
        Debug("No such file: [%s]\n", filepath);
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
 * @brief Get N bytes int, max 8 bytes.
 * 
 * @param buffer* JPEG binary buffer.
 * @param offset offset in buffer.
 * @param nbytes how many bytes to get.
 * @return buffer: [offset, offset + nbytes).
 * 
 * @note Contains little-endian to big-endian conversion.
 */
static uint64_t Binary_Get_Uint_L2B(uint8_t* buffer, size_t offset, size_t nbytes)
{
    uint64_t value = 0;

    for (int i = 0; i < nbytes; ++i)
    {
        value |= ((uint64_t)buffer[offset + i] << 8 * i);
    }

    return value;
}

/**
 * @brief Get 4 bytes float.
 * 
 * @param buffer* JPEG binary buffer.
 * @param offset offset in buffer.
 * @param length return buffer size, which always 4 bytes.
 * @return buffer: [offset, offset + 4).
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

    value.u32 = Binary_Get_Uint_L2B(buffer, offset, 4);

    return value.f32;
}

/**
 * @brief Get 8 bytes float.
 * 
 * @param buffer* JPEG binary buffer.
 * @param offset offset in buffer.
 * @param length return buffer size, which always 8 bytes.
 * @return buffer: [offset, offset + 8).
 * 
 * @note Contains little-endian to big-endian conversion.
 */
static double Binary_Get_Float64_L2B(uint8_t* buffer, size_t offset)
{
    union
    {
        uint64_t u64;
        double f64;
    } value;

    value.u64 = Binary_Get_Uint_L2B(buffer, offset, 8);

    return value.f64;
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
        offset = Binary_Get_Uint_L2B(buffer, offset_start, offset_size);
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

    obj->version = (uint16_t*)malloc(sizeof(uint16_t));
    if (obj->version == NULL)
    {
        retval = -4;
        Debug("Allocate memory for version failed.\n");
        goto free_return;
    }

    *(obj->version) = Binary_Get_Uint_L2B(_bin_original, offset, SGJW_VERSION_BYTES);
    offset += SGJW_VERSION_BYTES;
    Debug("Version: [%x][%d]\n", *(obj->version), *(obj->version));

    /* ----- Step 5 : Get Width ----- */

    obj->width = (uint16_t*)malloc(sizeof(uint16_t));
    if (obj->width == NULL)
    {
        retval = -5;
        Debug("Allocate memory for width failed.\n");
        goto free_return;
    }

    *(obj->width) = Binary_Get_Uint_L2B(_bin_original, offset, SGJW_WIDTH_BYTES);
    offset += SGJW_WIDTH_BYTES;
    Debug("Width: [%x][%d]\n", *(obj->width), *(obj->width));

    /* ----- Step 6 : Get Height ----- */

    obj->height = (uint16_t*)malloc(sizeof(uint16_t));
    if (obj->height == NULL)
    {
        retval = -6;
        Debug("Allocate memory for height failed.\n");
        goto free_return;
    }

    *(obj->height) = Binary_Get_Uint_L2B(_bin_original, offset, SGJW_HEIGHT_BYTES);
    offset += SGJW_HEIGHT_BYTES;
    Debug("Height: [%x][%d]\n", *(obj->height), *(obj->height));

    /* ----- Step 7 : Get Date ----- */

    obj->date = (char*)malloc(sizeof(char) * SGJW_DATE_BYTES);
    if (obj->date == NULL)
    {
        retval = -7;
        Debug("Allocate memory for date failed.\n");
        goto free_return;
    }

    Binary_Get_Char(_bin_original, offset, SGJW_DATE_BYTES, obj->date);
    offset += SGJW_DATE_BYTES;
    Debug("Date: [%s]\n", obj->date);

    /* ----- Step 8 : Get Float Mat ----- */

    obj->matrix = Binary_Get_Matrix(_bin_original, offset, *(obj->width), *(obj->height));

    if (obj->matrix == NULL)
    {
        retval = -8;
        Debug("Get float matrix failed.\n");
        goto free_return;
    }

    Debug("Matrix[0][0]: [%.2f]\n", obj->matrix[0]);
    Debug("Matrix[0][1]: [%.2f]\n", obj->matrix[1]);
    Debug("Matrix[%d][%d]: [%.2f]\n", *(obj->width), *(obj->width), obj->matrix[*(obj->width) * *(obj->height) - 1]);
    offset += *(obj->width) * *(obj->height) * SGJW_FLOAT32_BYTES;

    /* ----- Step 9 : Get Emissivity ----- */

    obj->emissivity = (float*)malloc(sizeof(float));
    if (obj->emissivity == NULL)
    {
        retval = -9;
        Debug("Allocate memory for emissivity failed.\n");
        goto free_return;
    }

    *(obj->emissivity) = Binary_Get_Float32_L2B(_bin_original, offset);;
    offset += SGJW_EMISSIVITY_BYTES;
    Debug("Emissivity: [%x][%.2f]\n", *(obj->emissivity), *(obj->emissivity));

    /* ----- Step 10 : Get Ambient Temperature ----- */

    obj->ambient_temp = (float*)malloc(sizeof(float));
    if (obj->ambient_temp == NULL)
    {
        retval = -10;
        Debug("Allocate memory for ambient temperature failed.\n");
        goto free_return;
    }

    *(obj->ambient_temp) = Binary_Get_Float32_L2B(_bin_original, offset);
    offset += SGJW_AMBIENT_TEMP_BYTES;
    Debug("Ambient Temperature: [%x][%.2f]\n", *(obj->ambient_temp), *(obj->ambient_temp));

    /* ----- Step 11 : Get FOV ----- */

    obj->fov = (uint8_t*)malloc(sizeof(uint8_t));
    if (obj->fov == NULL)
    {
        retval = -10;
        Debug("Allocate memory for FOV failed.\n");
        goto free_return;
    }

    *(obj->fov) = Binary_Get_Uint_L2B(_bin_original, offset, SGJW_FOV_BYTES);
    offset += SGJW_FOV_BYTES;
    Debug("FOV: [%x][%d]\n", *(obj->fov), *(obj->fov));

    /* ----- Step 11 : Get Distance ----- */

    obj->distance = (uint32_t*)malloc(sizeof(uint32_t));
    if (obj->distance == NULL)
    {
        retval = -11;
        Debug("Allocate memory for distance failed.\n");
        goto free_return;
    }

    *(obj->distance) = Binary_Get_Uint_L2B(_bin_original, offset, SGJW_DISTANCE_BYTES);
    offset += SGJW_DISTANCE_BYTES;
    Debug("Distance: [%x][%d]\n", *(obj->distance), *(obj->distance));

    /* ----- Step 12 : Get Humidity ----- */

    obj->humidity = (uint8_t*)malloc(sizeof(uint8_t));
    if (obj->humidity == NULL)
    {
        retval = -12;
        Debug("Allocate memory for Humidity failed.\n");
        goto free_return;
    }

    *(obj->humidity) = Binary_Get_Uint_L2B(_bin_original, offset, SGJW_HUMIDITY_BYTES);;
    offset += SGJW_HUMIDITY_BYTES;
    Debug("Humidity: [%x][%d]\n", *(obj->humidity), *(obj->humidity));

    /* ----- Step 13 : Get Reflective Temperature ----- */

    obj->reflective_temp = (float*)malloc(sizeof(float));
    if (obj->reflective_temp == NULL)
    {
        retval = -13;
        Debug("Allocate memory for reflective temperature failed.\n");
        goto free_return;
    }

    *(obj->reflective_temp) = Binary_Get_Float32_L2B(_bin_original, offset);
    offset += SGJW_REFLECTIVE_TEMP_BYTES;
    Debug("Reflective Temperature: [%x][%.2f]\n", *(obj->reflective_temp), *(obj->reflective_temp));

    /* ----- Step 14 : Get Manufacturer ----- */

    obj->manufacturer = (char*)malloc(sizeof(char) * SGJW_MANUFACTURER_BYTES);
    if (obj->manufacturer == NULL)
    {
        retval = -14;
        Debug("Allocate memory for manufacturer failed.\n");
        goto free_return;
    }

    Binary_Get_Char(_bin_original, offset, SGJW_MANUFACTURER_BYTES, obj->manufacturer);
    offset += SGJW_MANUFACTURER_BYTES;
    Debug("Manufacturer: [%s]\n", obj->manufacturer);

    /* ----- Step 15 : Get Product ----- */

    obj->product = (char*)malloc(sizeof(char) * SGJW_PRODUCT_BYTES);
    if (obj->product == NULL)
    {
        retval = -15;
        Debug("Allocate memory for product failed.\n");
        goto free_return;
    }

    Binary_Get_Char(_bin_original, offset, SGJW_PRODUCT_BYTES, obj->product);
    offset += SGJW_PRODUCT_BYTES;
    Debug("Product: [%s]\n", obj->product);

    /* ----- Step 16 : Get SN ----- */

    obj->sn = (char*)malloc(sizeof(char) * SGJW_SN_BYTES);
    if (obj->sn == NULL)
    {
        retval = -16;
        Debug("Allocate memory for serial number failed.\n");
        goto free_return;
    }

    Binary_Get_Char(_bin_original, offset, SGJW_SN_BYTES, obj->sn);
    offset += SGJW_SN_BYTES;
    Debug("Serial Number: [%s]\n", obj->sn);

    /* ----- Step 17 : Get Longitude ----- */

    obj->longitude = (double*)malloc(sizeof(double));
    if (obj->longitude == NULL)
    {
        retval = -17;
        Debug("Allocate memory for longitude failed.\n");
        goto free_return;
    }

    *(obj->longitude) = Binary_Get_Float64_L2B(_bin_original, offset);
    offset += SGJW_LONGITUDE_BYTES;
    Debug("Longitude: [%x][%.2f]\n", *(obj->longitude), *(obj->longitude));

    /* ----- Step 18 : Get Latitude ----- */

    obj->latitude = (double*)malloc(sizeof(double));
    if (obj->latitude == NULL)
    {
        retval = -18;
        Debug("Allocate memory for latitude failed.\n");
        goto free_return;
    }

    *(obj->latitude) = Binary_Get_Float64_L2B(_bin_original, offset);
    offset += SGJW_LATITUDE_BYTES;
    Debug("Latitude: [%x][%.2f]\n", *(obj->latitude), *(obj->latitude));

    /* ----- Step 19 : Get Altitude ----- */

    obj->altitude = (uint32_t*)malloc(sizeof(uint32_t));
    if (obj->altitude == NULL)
    {
        retval = -19;
        Debug("Allocate memory for altitudes failed.\n");
        goto free_return;
    }

    *(obj->altitude) = Binary_Get_Uint_L2B(_bin_original, offset, SGJW_ALTITUDE_BYTES);
    offset += SGJW_ALTITUDE_BYTES;
    Debug("Altitude: [%x][%d]\n", *(obj->altitude), *(obj->altitude));

    /* ----- Step 20 : Get Appendix Length ----- */

    obj->appendix_length = (uint32_t*)malloc(sizeof(uint32_t));
    if (obj->appendix_length == NULL)
    {
        retval = -20;
        Debug("Allocate memory for appendix length failed.\n");
        goto free_return;
    }

    *(obj->appendix_length) = Binary_Get_Uint_L2B(_bin_original, offset, SGJW_APPENDIX_LENGTH_BYTES);
    offset += SGJW_APPENDIX_LENGTH_BYTES;
    Debug("Appendix Length: [%x][%d]\n", *(obj->appendix_length), *(obj->appendix_length));

    /* ----- Step 21 : Get Appendix ----- */

    if (*(obj->appendix_length) == 0)
        goto free_return;

    obj->appendix = (char*)malloc(sizeof(char) * *(obj->appendix_length));
    if (obj->appendix == NULL)
    {
        retval = -21;
        Debug("Allocate memory for serial number failed.\n");
        goto free_return;
    }

    Binary_Get_Char(_bin_original, offset, *(obj->appendix_length), obj->appendix);
    offset += *(obj->appendix_length);
    Debug("Appendix: [%s]\n", obj->appendix);

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
        obj->matrix,
        obj->emissivity,
        obj->ambient_temp,
        obj->fov,
        obj->distance,
        obj->humidity,
        obj->reflective_temp,
        obj->manufacturer,
        obj->product,
        obj->sn,
        obj->longitude,
        obj->latitude,
        obj->altitude,
        obj->appendix_length,
        obj->appendix
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