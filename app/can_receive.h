#ifndef __CAN_RECEIVE_H_
#define __CAN_RECEIVE_H_

#include "shared.h"
#include "can.h"

// can线程参数结构体
typedef struct
{
    shared_data_t *shared_data; // 共享数据
    can_t *hcan; // can实例指针
} can_thread_param_t;

void* can_receive_task(void *arg);

#endif