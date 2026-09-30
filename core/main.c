#include "main.h"
#include <pthread.h>
#include "can.h"

#include "can_receive.h"
#include "connect_to_display.h"
#include "write_sdcard.h"







int main(void)
{
    // 初始化共享数据
    shared_data_t shared_data = {0};
    pthread_mutex_init(&shared_data.display_ids.mutex, NULL);
    pthread_mutex_init(&shared_data.can_frames.mutex, NULL);

    // 初始化各线程参数实例
    can_t hcan0 = {0};
    can_thread_param_t can_thread_param_can0 = {
        .shared_data = &shared_data,
        .hcan = &hcan0
    };
    write_sdcard_param_t write_sdcard_param = {
        .shared_data = &shared_data
    };

    printf("Starting main task...\n");

    // 创建线程
    pthread_t can_receive_thread_can0;
    pthread_t push_to_display_thread;
    pthread_t receive_from_display_thread;
    pthread_t write_sdcard_thread;
    pthread_create(&can_receive_thread_can0, NULL, can_receive_task, (void *)&can_thread_param_can0);
    pthread_create(&push_to_display_thread, NULL, push_to_display_task, (void *)&shared_data);
    pthread_create(&receive_from_display_thread, NULL, receive_from_display_task, (void *)&shared_data);
    pthread_create(&write_sdcard_thread, NULL, write_sdcard_task, (void *)&write_sdcard_param);

    // 回收线程
    pthread_join(can_receive_thread_can0, NULL);
    pthread_join(push_to_display_thread, NULL);
    pthread_join(receive_from_display_thread, NULL);
    pthread_join(write_sdcard_thread, NULL);

    // 销毁互斥锁
    pthread_mutex_destroy(&shared_data.display_ids.mutex);
    pthread_mutex_destroy(&shared_data.can_frames.mutex);
    printf("Hello, World!\n");
    
    exit(0);
    return 0;
}