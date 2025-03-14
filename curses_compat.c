#include "curses_compat.h"

static int nc_attributes = 0;

void nc_setattrib(int flag) {
    nc_attributes |= flag;
}

void nc_clearattrib(int flag) {
    nc_attributes &= ~flag;
}

bool nc_has_chinese(void) {
    return (nc_attributes & A_NC_BIG5) != 0;
}
