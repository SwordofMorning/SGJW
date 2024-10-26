#include "../inc/sgjw.h"

int main(int argc, char** argv)
{
    StateGridJPEG jpeg;
    State_Grid_JPEG_Reader(argv[1], &jpeg);
    State_Grid_JPEG_Delete(&jpeg);
    return 0;
}