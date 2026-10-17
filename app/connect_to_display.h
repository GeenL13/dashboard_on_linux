#ifndef __CONNECT_TO_DISPLAY_H_
#define __CONNECT_TO_DISPLAY_H_

#include "shared.h"

typedef struct
{
    shared_data_t *shared_data; // 共享数据
} push_to_display_param_t;

void* push_to_display_task(void *arg);
void* receive_from_display_task(void *arg);

#endif