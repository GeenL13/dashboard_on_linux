#include "main.h"
#include <pthread.h>

#include "can_receive.h"
#include "connect_to_display.h"
#include "can.h"





int main(void)
{
    // 初始化共享数据
    shared_data_t shared_data;
    pthread_mutex_init(&shared_data.display_ids.mutex, NULL);
    pthread_mutex_init(&shared_data.can_frames.mutex, NULL);

    // 初始化can实例
    can_thread_param_t can_thread_param_can0 = {
        .shared_data = &shared_data
    };

    // 创建线程
    pthread_t can_receive_thread_can0;
    pthread_t push_to_display_thread;
    pthread_t receive_from_display_thread;
    pthread_create(&can_receive_thread_can0, NULL, can_receive_task, (void *)&can_thread_param_can0);
    pthread_create(&push_to_display_thread, NULL, push_to_display_task, (void *)&shared_data);
    pthread_create(&receive_from_display_thread, NULL, receive_from_display_task, (void *)&shared_data);

    // 回收线程
    pthread_join(can_receive_thread_can0, NULL);
    pthread_join(push_to_display_thread, NULL);
    pthread_join(receive_from_display_thread, NULL);

    // 销毁互斥锁
    pthread_mutex_destroy(&shared_data.display_ids.mutex);
    pthread_mutex_destroy(&shared_data.can_frames.mutex);
    printf("Hello, World!\n");
    
    return 0;
}