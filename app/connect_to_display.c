#include "connect_to_display.h"
#include <stdint.h>
#include <pthread.h>

void* push_to_display_task(void *arg)
{
    // 连接到显示器的任务
    printf("push to display task...\n");
    while (1)
    {
        // 将数据发送给显示器
        sleep(1);
    }
}


void* receive_from_display_task(void *arg)
{
    // 接收显示器数据的任务
    printf("receive from display task...\n");
    while (1)
    {
        // 接收显示器数据
        sleep(1);
    }
}