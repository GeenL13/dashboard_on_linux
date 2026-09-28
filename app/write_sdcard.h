#ifndef __WRITE_SDCARD_H_
#define __WRITE_SDCARD_H_

#include "shared.h"

typedef struct
{
    shared_data_t *shared_data; // 共享数据
} write_sdcard_param_t;

void* write_sdcard_task(void *arg);

#endif