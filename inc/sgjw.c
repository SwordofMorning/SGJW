#include "sgjw.h"

#ifndef SGJW_DEBUG
#define SGJW_DEBUG 1
#endif

/* ====================================================================================================== */
/* ======================================== Constants Definition ========================================= */
/* ====================================================================================================== */

// Field sizes
#define SGJW_EOF_BYTES 16
#define SGJW_OFFSET_BYTES 4
#define SGJW_VERSION_BYTES 2
#define SGJW_WIDTH_BYTES 2
#define SGJW_HEIGHT_BYTES 2
#define SGJW_DATE_BYTES 14
#define SGJW_FLOAT32_BYTES 4
#define SGJW_EMISSIVITY_BYTES 4
#define SGJW_AMBIENT_TEMP_BYTES 4
#define SGJW_FOV_BYTES 1
#define SGJW_DISTANCE_BYTES 4
#define SGJW_HUMIDITY_BYTES 1
#define SGJW_REFLECTIVE_TEMP_BYTES 4
#define SGJW_MANUFACTURER_BYTES 32
#define SGJW_PRODUCT_BYTES 32
#define SGJW_SN_BYTES 32
#define SGJW_LONGITUDE_BYTES 8
#define SGJW_LATITUDE_BYTES 8
#define SGJW_ALTITUDE_BYTES 4
#define SGJW_APPENDIX_LENGTH_BYTES 4

// clang-format off
static const uint8_t SGJW_EOF_SIGNATURE[] = {
    0x37, 0x66, 0x07, 0x1A, 0x12, 0x3A, 0x4C, 0x9F,
    0xA9, 0x5D, 0x21, 0xD2, 0xDA, 0x7D, 0x26, 0xBC
};
// clang-format on

/* ====================================================================================================== */
/* ======================================== Debug Function ============================================== */
/* ====================================================================================================== */

static void Debug(const char* format, ...)
{
#if SGJW_DEBUG
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif
}

/* ====================================================================================================== */
/* ======================================== Helper Structures =========================================== */
/* ====================================================================================================== */

// Field types
typedef enum
{
    FIELD_UINT8,
    FIELD_UINT16,
    FIELD_UINT32,
    FIELD_FLOAT32,
    FIELD_FLOAT64,
    FIELD_CHAR_ARRAY,
    FIELD_FLOAT_MATRIX
} FieldType;

// Field information structure
typedef struct
{
    // Pointer to the field in StateGridJPEG
    void** field_ptr;
    // Size of the field
    size_t size;
    // Number of elements (1 for scalar, >1 for arrays)
    size_t count;
    // Type of the field
    FieldType type;
    // Name of the field for debug
    const char* name;
} FieldInfo;

/* ====================================================================================================== */
/* ======================================== Helper Functions ============================================ */
/* ====================================================================================================== */

static void* Malloc_Field(void** field, size_t size, const char* field_name, int8_t* retval)
{
    *field = malloc(size);
    if (*field == NULL)
    {
        *retval = SGJW_ERROR_MALLOC_FAILED;
        Debug("Allocate memory for %s failed.\n", field_name);
        return NULL;
    }
    return *field;
}

static uint64_t Binary_Get_Uint_L2B(uint8_t* buffer, size_t offset, size_t nbytes)
{
    uint64_t value = 0;
    for (int i = 0; i < nbytes && i < 8; ++i)
    {
        value |= ((uint64_t)buffer[offset + i] << (8 * i));
    }
    return value;
}

static float Binary_Get_Float32_L2B(uint8_t* buffer, size_t offset)
{
    union
    {
        uint32_t u32;
        float f32;
    } value;
    value.u32 = (uint32_t)Binary_Get_Uint_L2B(buffer, offset, 4);
    return value.f32;
}

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

static void Binary_Get_Char(uint8_t* buffer, size_t offset, size_t length, char* ret)
{
    memcpy(ret, buffer + offset, length);

    // Ensure null termination
    ret[length] = '\0';
}

static void Binary_Set_Uint8_B2L(uint8_t* buffer, size_t offset, uint8_t data)
{
    buffer[offset] = data;
}

static void Binary_Set_Uint16_B2L(uint8_t* buffer, size_t offset, uint16_t data)
{
    for (int i = 0; i < sizeof(uint16_t); i++)
    {
        buffer[offset + i] = (data >> (i * 8)) & 0xFF;
    }
}

static void Binary_Set_Uint32_B2L(uint8_t* buffer, size_t offset, uint32_t data)
{
    for (int i = 0; i < sizeof(uint32_t); i++)
    {
        buffer[offset + i] = (data >> (i * 8)) & 0xFF;
    }
}

static void Binary_Set_Uint64_B2L(uint8_t* buffer, size_t offset, uint64_t data)
{
    for (int i = 0; i < sizeof(uint64_t); i++)
    {
        buffer[offset + i] = (data >> (i * 8)) & 0xFF;
    }
}

static void Binary_Set_Float32_B2L(uint8_t* buffer, size_t offset, float data)
{
    union
    {
        float f32;
        uint32_t u32;
    } value;
    value.f32 = data;
    Binary_Set_Uint32_B2L(buffer, offset, value.u32);
}

static void Binary_Set_Float64_B2L(uint8_t* buffer, size_t offset, double data)
{
    union
    {
        double f64;
        uint64_t u64;
    } value;
    value.f64 = data;
    Binary_Set_Uint64_B2L(buffer, offset, value.u64);
}

/* ====================================================================================================== */
/* ======================================== File Operations ============================================= */
/* ====================================================================================================== */

static size_t Open_File_In_Binary(const char* filepath, uint8_t** buffer)
{
    FILE* file = fopen(filepath, "rb");
    if (file == NULL)
    {
        Debug("No such file: [%s]\n", filepath);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    *buffer = (uint8_t*)malloc(file_size);
    if (*buffer == NULL)
    {
        Debug("Malloc buffer fail.\n");
        fclose(file);
        return 0;
    }

    size_t read_size = fread(*buffer, 1, file_size, file);
    if (read_size != file_size)
    {
        Debug("Buffer size not equal file size.\n");
        free(*buffer);
        fclose(file);
        return 0;
    }

    fclose(file);
    return read_size;
}

static int8_t Binary_Verification_EOF(uint8_t* buffer, size_t buffer_size)
{
    if (buffer_size < SGJW_EOF_BYTES)
        return SGJW_ERROR_INVALID_EOF;

    size_t offset = buffer_size - SGJW_EOF_BYTES;
    return (memcmp(buffer + offset, SGJW_EOF_SIGNATURE, SGJW_EOF_BYTES) == 0) ? SGJW_SUCCESS : SGJW_ERROR_INVALID_EOF;
}

static size_t Binary_Get_Offsite(uint8_t* buffer, size_t buffer_size)
{
    if (buffer_size < SGJW_EOF_BYTES + SGJW_OFFSET_BYTES)
        return 0;

    size_t offset_start = buffer_size - SGJW_EOF_BYTES - SGJW_OFFSET_BYTES;
    return Binary_Get_Uint_L2B(buffer, offset_start, SGJW_OFFSET_BYTES);
}

/* ====================================================================================================== */
/* ======================================== Field Operations ============================================ */
/* ====================================================================================================== */

static int8_t Read_Field_Value(uint8_t* buffer, size_t offset, FieldInfo* info, void* field)
{
    switch (info->type)
    {
        case FIELD_UINT8:
            *(uint8_t*)field = (uint8_t)Binary_Get_Uint_L2B(buffer, offset, 1);
            break;
        case FIELD_UINT16:
            *(uint16_t*)field = (uint16_t)Binary_Get_Uint_L2B(buffer, offset, 2);
            break;
        case FIELD_UINT32:
            *(uint32_t*)field = (uint32_t)Binary_Get_Uint_L2B(buffer, offset, 4);
            break;
        case FIELD_FLOAT32:
            *(float*)field = Binary_Get_Float32_L2B(buffer, offset);
            break;
        case FIELD_FLOAT64:
            *(double*)field = Binary_Get_Float64_L2B(buffer, offset);
            break;
        case FIELD_CHAR_ARRAY:
            Binary_Get_Char(buffer, offset, info->size, (char*)field);
            break;
        case FIELD_FLOAT_MATRIX:
            for (size_t i = 0; i < info->count; i++)
            {
                ((float*)field)[i] = Binary_Get_Float32_L2B(buffer, offset + i * 4);
            }
            break;
        default:
            return SGJW_ERROR_FIELD_READ_FAILED;
    }

    return SGJW_SUCCESS;
}

static int8_t Read_Field(uint8_t* buffer, size_t* offset, FieldInfo* info)
{
    int8_t retval = SGJW_SUCCESS;
    size_t alloc_size = info->size * (info->count ? info->count : 1);

    // Allocate memory for the field
    if (!Malloc_Field(info->field_ptr, alloc_size, info->name, &retval))
        return retval;

    // Read the field value
    retval = Read_Field_Value(buffer, *offset, info, *info->field_ptr);
    if (retval != SGJW_SUCCESS)
    {
        free(*info->field_ptr);
        *info->field_ptr = NULL;
        return retval;
    }

    // Debug output
    switch (info->type)
    {
        case FIELD_UINT8:
            Debug("%s: [%x][%d]\n", info->name, *(uint8_t*)*info->field_ptr, *(uint8_t*)*info->field_ptr);
            break;
        case FIELD_UINT16:
            Debug("%s: [%x][%d]\n", info->name, *(uint16_t*)*info->field_ptr, *(uint16_t*)*info->field_ptr);
            break;
        case FIELD_UINT32:
            Debug("%s: [%x][%d]\n", info->name, *(uint32_t*)*info->field_ptr, *(uint32_t*)*info->field_ptr);
            break;
        case FIELD_FLOAT32:
            Debug("%s: [%x][%.2f]\n", info->name, *(uint32_t*)*info->field_ptr, *(float*)*info->field_ptr);
            break;
        case FIELD_FLOAT64:
            Debug("%s: [%x][%.2f]\n", info->name, *(uint64_t*)*info->field_ptr, *(double*)*info->field_ptr);
            break;
        case FIELD_CHAR_ARRAY:
            Debug("%s: [%s]\n", info->name, (char*)*info->field_ptr);
            break;
        case FIELD_FLOAT_MATRIX:
            Debug("%s: First element [%.2f]\n", info->name, ((float*)*info->field_ptr)[0]);
            Debug("%s: Second element [%.2f]\n", info->name, ((float*)*info->field_ptr)[1]);
            break;
    }

    *offset += info->size * (info->count ? info->count : 1);
    return SGJW_SUCCESS;
}

static int8_t Write_Field_Value(uint8_t* buffer, size_t offset, FieldInfo* info, void* field)
{
    if (!buffer || !info || !field)
        return SGJW_ERROR_INVALID_PARAMS;

    switch (info->type)
    {
        case FIELD_UINT8:
            Binary_Set_Uint8_B2L(buffer, offset, *(uint8_t*)field);
            break;
        case FIELD_UINT16:
            Binary_Set_Uint16_B2L(buffer, offset, *(uint16_t*)field);
            break;
        case FIELD_UINT32:
            Binary_Set_Uint32_B2L(buffer, offset, *(uint32_t*)field);
            break;
        case FIELD_FLOAT32:
            Binary_Set_Float32_B2L(buffer, offset, *(float*)field);
            break;
        case FIELD_FLOAT64:
            Binary_Set_Float64_B2L(buffer, offset, *(double*)field);
            break;
        case FIELD_CHAR_ARRAY:
            memcpy(buffer + offset, field, info->size);
            break;
        case FIELD_FLOAT_MATRIX:
            for (size_t i = 0; i < info->count; i++)
            {
                Binary_Set_Float32_B2L(buffer, offset + i * sizeof(float), ((float*)field)[i]);
            }
            break;
        default:
            return SGJW_ERROR_FIELD_SET_FAILED;
    }

    return SGJW_SUCCESS;
}

static int8_t Write_Field(uint8_t* buffer, size_t* offset, FieldInfo* info)
{
    if (!buffer || !offset || !info || !*info->field_ptr)
        return SGJW_ERROR_INVALID_PARAMS;

    int8_t retval = Write_Field_Value(buffer, *offset, info, *info->field_ptr);
    if (retval != SGJW_SUCCESS)
        return retval;

    // Debug output
    switch (info->type)
    {
        case FIELD_UINT8:
            Debug("Write %s: [%x][%d]\n", info->name, *(uint8_t*)*info->field_ptr, *(uint8_t*)*info->field_ptr);
            break;
        case FIELD_UINT16:
            Debug("Write %s: [%x][%d]\n", info->name, *(uint16_t*)*info->field_ptr, *(uint16_t*)*info->field_ptr);
            break;
        case FIELD_UINT32:
            Debug("Write %s: [%x][%d]\n", info->name, *(uint32_t*)*info->field_ptr, *(uint32_t*)*info->field_ptr);
            break;
        case FIELD_FLOAT32:
            Debug("Write %s: [%.2f]\n", info->name, *(float*)*info->field_ptr);
            break;
        case FIELD_FLOAT64:
            Debug("Write %s: [%.2f]\n", info->name, *(double*)*info->field_ptr);
            break;
        case FIELD_CHAR_ARRAY:
            Debug("Write %s: [%s]\n", info->name, (char*)*info->field_ptr);
            break;
        case FIELD_FLOAT_MATRIX:
            Debug("Write %s: First element [%.2f]\n", info->name, ((float*)*info->field_ptr)[0]);
            break;
    }

    *offset += info->size * (info->count ? info->count : 1);
    return SGJW_SUCCESS;
}

/* ====================================================================================================== */
/* ========================================== Main APIs ================================================= */
/* ====================================================================================================== */

int8_t State_Grid_JPEG_Read(const char* filepath, StateGridJPEG* obj)
{
    /* ---------- Step 1 : File Verification ---------- */

    if (!filepath || !obj)
        return SGJW_ERROR_INVALID_PARAMS;

    int8_t retval = SGJW_SUCCESS;
    uint8_t* buffer = NULL;
    size_t buffer_size = Open_File_In_Binary(filepath, &buffer);

    if (buffer_size == 0)
    {
        Debug("Read file: [%s] failed.\n", filepath);
        return SGJW_ERROR_READ_FAILED;
    }
    Debug("Open Success\n");

    // Verify EOF signature
    retval = Binary_Verification_EOF(buffer, buffer_size);
    if (retval != SGJW_SUCCESS)
    {
        Debug("File EOF label verification fail.\n");
        goto cleanup;
    }
    Debug("Verification Success\n");

    // Get data offset
    size_t offset = Binary_Get_Offsite(buffer, buffer_size);
    if (offset == 0)
    {
        retval = SGJW_ERROR_INVALID_OFFSET;
        Debug("Get offset fail.\n");
        goto cleanup;
    }
    Debug("Offset is: [%x][%d]\n", offset, offset);

    /* ---------- Step 2 : Get Data in Binary ---------- */

    // clang-format off
    // Define all fields to be read
    FieldInfo fields[] = {
        { (void**)&obj->version, SGJW_VERSION_BYTES, 0, FIELD_UINT16, "Version" },
        { (void**)&obj->width, SGJW_WIDTH_BYTES, 0, FIELD_UINT16, "Width" },
        { (void**)&obj->height, SGJW_HEIGHT_BYTES, 0, FIELD_UINT16, "Height" },
        { (void**)&obj->date, SGJW_DATE_BYTES, 0, FIELD_CHAR_ARRAY, "Date" },
        { (void**)&obj->matrix, SGJW_FLOAT32_BYTES, 0, FIELD_FLOAT_MATRIX, "Matrix" }, // count will be set after reading width/height
        { (void**)&obj->emissivity, SGJW_EMISSIVITY_BYTES, 0, FIELD_FLOAT32, "Emissivity" },
        { (void**)&obj->ambient_temp, SGJW_AMBIENT_TEMP_BYTES, 0, FIELD_FLOAT32, "Ambient Temperature" },
        { (void**)&obj->fov, SGJW_FOV_BYTES, 0, FIELD_UINT8, "FOV" },
        { (void**)&obj->distance, SGJW_DISTANCE_BYTES, 0, FIELD_UINT32, "Distance" },
        { (void**)&obj->humidity, SGJW_HUMIDITY_BYTES, 0, FIELD_UINT8, "Humidity" },
        { (void**)&obj->reflective_temp, SGJW_REFLECTIVE_TEMP_BYTES, 0, FIELD_FLOAT32, "Reflective Temperature" },
        { (void**)&obj->manufacturer, SGJW_MANUFACTURER_BYTES, 0, FIELD_CHAR_ARRAY, "Manufacturer" },
        { (void**)&obj->product, SGJW_PRODUCT_BYTES, 0, FIELD_CHAR_ARRAY, "Product" },
        { (void**)&obj->sn, SGJW_SN_BYTES, 0, FIELD_CHAR_ARRAY, "Serial Number" },
        { (void**)&obj->longitude, SGJW_LONGITUDE_BYTES, 0, FIELD_FLOAT64, "Longitude" },
        { (void**)&obj->latitude, SGJW_LATITUDE_BYTES, 0, FIELD_FLOAT64, "Latitude" },
        { (void**)&obj->altitude, SGJW_ALTITUDE_BYTES, 0, FIELD_UINT32, "Altitude" },
        { (void**)&obj->appendix_length, SGJW_APPENDIX_LENGTH_BYTES, 0, FIELD_UINT32, "Appendix Length" }
    };
    // clang-format on

    // Read each field
    for (size_t i = 0; i < sizeof(fields) / sizeof(FieldInfo); ++i)
    {
        // Special handling for matrix field
        if (fields[i].type == FIELD_FLOAT_MATRIX)
        {
            fields[i].count = (*obj->width) * (*obj->height);
        }

        retval = Read_Field(buffer, &offset, &fields[i]);
        if (retval != SGJW_SUCCESS)
        {
            Debug("Failed to read field: %s\n", fields[i].name);
            goto cleanup;
        }
    }

    // Read appendix if present
    if (obj->appendix_length && *(obj->appendix_length) > 0)
    {
        // clang-format off
        FieldInfo appendix_field = 
        {
            (void**)&obj->appendix,
            *(obj->appendix_length),
            0,
            FIELD_CHAR_ARRAY,
            "Appendix"
        };
        // clang-format on

        retval = Read_Field(buffer, &offset, &appendix_field);
        if (retval != SGJW_SUCCESS)
        {
            Debug("Failed to read appendix\n");
            goto cleanup;
        }
    }

cleanup:
    if (buffer)
        free(buffer);

    if (retval != SGJW_SUCCESS)
        State_Grid_JPEG_Delete_OBJ(obj);

    return retval;
}

int8_t State_Grid_JPEG_Append(const char* filepath, StateGridJPEG* obj)
{
    if (!filepath || !obj)
        return SGJW_ERROR_INVALID_PARAMS;

    int8_t retval = SGJW_SUCCESS;

    /* ---------- Step 1 : Calculate total size ---------- */
    size_t matrix_size = (*(obj->width)) * (*(obj->height)) * sizeof(float);
    size_t appendix_size = obj->appendix_length ? *(obj->appendix_length) : 0;

    // clang-format off
    size_t fixed_size = SGJW_VERSION_BYTES + SGJW_WIDTH_BYTES + SGJW_HEIGHT_BYTES +
                        SGJW_DATE_BYTES + SGJW_EMISSIVITY_BYTES + SGJW_AMBIENT_TEMP_BYTES +
                        SGJW_FOV_BYTES + SGJW_DISTANCE_BYTES + SGJW_HUMIDITY_BYTES +
                        SGJW_REFLECTIVE_TEMP_BYTES + SGJW_MANUFACTURER_BYTES +
                        SGJW_PRODUCT_BYTES + SGJW_SN_BYTES + SGJW_LONGITUDE_BYTES +
                        SGJW_LATITUDE_BYTES + SGJW_ALTITUDE_BYTES + SGJW_APPENDIX_LENGTH_BYTES;
    // clang-format on

    size_t total_size = fixed_size + matrix_size + appendix_size + SGJW_OFFSET_BYTES + SGJW_EOF_BYTES;

    /* ---------- Step 2 : Create temporary buffer ---------- */
    uint8_t* temp_buffer = (uint8_t*)malloc(total_size);
    if (!temp_buffer)
        return SGJW_ERROR_MALLOC_FAILED;

    size_t offset = 0;

    /* ---------- Step 3 : Write fields to buffer ---------- */
    // clang-format off
    FieldInfo fields[] = {
        { (void**)&obj->version, SGJW_VERSION_BYTES, 0, FIELD_UINT16, "Version" },
        { (void**)&obj->width, SGJW_WIDTH_BYTES, 0, FIELD_UINT16, "Width" },
        { (void**)&obj->height, SGJW_HEIGHT_BYTES, 0, FIELD_UINT16, "Height" },
        { (void**)&obj->date, SGJW_DATE_BYTES, 0, FIELD_CHAR_ARRAY, "Date" },
        { (void**)&obj->matrix, SGJW_FLOAT32_BYTES, (*(obj->width)) * (*(obj->height)), FIELD_FLOAT_MATRIX, "Matrix" },
        { (void**)&obj->emissivity, SGJW_EMISSIVITY_BYTES, 0, FIELD_FLOAT32, "Emissivity" },
        { (void**)&obj->ambient_temp, SGJW_AMBIENT_TEMP_BYTES, 0, FIELD_FLOAT32, "Ambient Temperature" },
        { (void**)&obj->fov, SGJW_FOV_BYTES, 0, FIELD_UINT8, "FOV" },
        { (void**)&obj->distance, SGJW_DISTANCE_BYTES, 0, FIELD_UINT32, "Distance" },
        { (void**)&obj->humidity, SGJW_HUMIDITY_BYTES, 0, FIELD_UINT8, "Humidity" },
        { (void**)&obj->reflective_temp, SGJW_REFLECTIVE_TEMP_BYTES, 0, FIELD_FLOAT32, "Reflective Temperature" },
        { (void**)&obj->manufacturer, SGJW_MANUFACTURER_BYTES, 0, FIELD_CHAR_ARRAY, "Manufacturer" },
        { (void**)&obj->product, SGJW_PRODUCT_BYTES, 0, FIELD_CHAR_ARRAY, "Product" },
        { (void**)&obj->sn, SGJW_SN_BYTES, 0, FIELD_CHAR_ARRAY, "Serial Number" },
        { (void**)&obj->longitude, SGJW_LONGITUDE_BYTES, 0, FIELD_FLOAT64, "Longitude" },
        { (void**)&obj->latitude, SGJW_LATITUDE_BYTES, 0, FIELD_FLOAT64, "Latitude" },
        { (void**)&obj->altitude, SGJW_ALTITUDE_BYTES, 0, FIELD_UINT32, "Altitude" },
        { (void**)&obj->appendix_length, SGJW_APPENDIX_LENGTH_BYTES, 0, FIELD_UINT32, "Appendix Length" }
    };
    // clang-format on

    for (size_t i = 0; i < sizeof(fields) / sizeof(FieldInfo); ++i)
    {
        retval = Write_Field(temp_buffer, &offset, &fields[i]);
        if (retval != SGJW_SUCCESS)
        {
            Debug("Failed to write field: %s\n", fields[i].name);
            goto cleanup;
        }
    }

    // Write appendix if present
    if (obj->appendix_length && *(obj->appendix_length) > 0)
    {
        // clang-format off
        FieldInfo appendix_field = {
            (void**)&obj->appendix,
            *(obj->appendix_length),
            0,
            FIELD_CHAR_ARRAY,
            "Appendix"
        };
        // clang-format on

        retval = Write_Field(temp_buffer, &offset, &appendix_field);
        if (retval != SGJW_SUCCESS)
        {
            Debug("Failed to write appendix\n");
            goto cleanup;
        }
    }

    /* ---------- Step 4 : Write offset ---------- */
    FILE* file_size_check = fopen(filepath, "rb");
    if (!file_size_check)
    {
        retval = SGJW_ERROR_FILE_WRITE;
        goto cleanup;
    }

    // Get original file size
    fseek(file_size_check, 0, SEEK_END);
    long original_file_size = ftell(file_size_check);
    fclose(file_size_check);

    // Calculate and write the actual offset
    uint32_t offset_in_file = (uint32_t)(original_file_size);
    Binary_Set_Uint32_B2L(temp_buffer, offset, offset_in_file);
    offset += SGJW_OFFSET_BYTES;
    Debug("Offset: [%x]\n", offset_in_file);

    /* ---------- Step 5 : Write EOF signature ---------- */
    memcpy(temp_buffer + offset, SGJW_EOF_SIGNATURE, SGJW_EOF_BYTES);
    offset += SGJW_EOF_BYTES;

    /* ---------- Step 6 : Append to file ---------- */
    FILE* file = fopen(filepath, "ab");
    if (!file)
    {
        retval = SGJW_ERROR_FILE_WRITE;
        goto cleanup;
    }

    if (fwrite(temp_buffer, 1, total_size, file) != total_size)
    {
        retval = SGJW_ERROR_FILE_WRITE;
        fclose(file);
        goto cleanup;
    }

    fclose(file);
    Debug("Write Success!\n");
cleanup:
    if (temp_buffer)
        free(temp_buffer);

    return retval;
}

void State_Grid_JPEG_Delete_OBJ(StateGridJPEG* obj)
{
    if (!obj)
        return;

    // clang-format off
    void* pointers[] = {
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

    for (size_t i = 0; i < sizeof(pointers) / sizeof(void*); i++)
    {
        Debug("Try to free pointer %zu\n", i);
        if (pointers[i])
        {
            free(pointers[i]);
            pointers[i] = NULL;
            Debug("Freed pointer %zu\n", i);
        }
    }

    // Clear all pointers in the structure
    memset(obj, 0, sizeof(StateGridJPEG));
}