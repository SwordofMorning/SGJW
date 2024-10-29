#include "../inc/sgjw.h"

int main(int argc, char** argv)
{
    StateGridJPEG jpeg;
    State_Grid_JPEG_Read(argv[1], &jpeg);

    if (argv[2])
        State_Grid_JPEG_Append(argv[2], &jpeg);

    State_Grid_JPEG_Delete_OBJ(&jpeg);
    return 0;
}