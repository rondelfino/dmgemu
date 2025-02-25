#include "idu.h"

static void increment(u16 *data, s8 value)
{
    if (*data >= OAM_START && *data <= 0xFFFF)
    {
        // TODO: Handle OAM bug
    }

    (*data) += value;
}

void idu_increment(u16 *data)
{
    increment(data, 1);
}

void idu_decrement(u16 *data)
{
    increment(data, -1);
}
