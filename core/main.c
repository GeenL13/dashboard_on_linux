#include "main.h"
#include <pthread.h>
#include <stdlib.h>
#include "can.h"
#include <string.h>

#include "can_receive.h"
#include "connect_to_display.h"
#include "write_sdcard.h"







int main(int argc, char **argv)
{
    // 参数判定
    int can_count = argc - 1;
    if (can_count < 1)
    {
        printf("%s: no param can\n", argv[0]);
        exit(1);
    }
    for (int i = 1; i < argc; i++)
    {
        size_t len = strlen(argv[i]);
        if (len > IFNAMSIZ - 1)
        {
            printf("%s: param %d is too long\n", argv[0], i);
            exit(1);
        }
    }
    // 初始化共享数据
    shared_data_t shared_data = {0};
    pthread_mutex_init(&shared_data.display_ids.mutex, NULL);
    pthread_mutex_init(&shared_data.can_frames.mutex, NULL);

    // 初始化各线程参数实例
    // 初始化can实例
    can_t *hcan = calloc(can_count, sizeof(can_t));
    can_thread_param_t *can_thread_param = calloc(can_count, sizeof(can_thread_param_t));
    if (!hcan || !can_thread_param)
    {
        perror("calloc");
        // for (int i = 0; i < can_count; i++)      // 此处free多余，因为未初始化
        // {
        //     free(hcan[i].can_name);
        // }
        free(hcan); 
        free(can_thread_param);
        exit(1);
    }
    for (int i = 0; i < can_count; i++)
    {
        hcan[i].can_name = strdup(argv[i + 1]);
        hcan[i].channel = i;
        if (!hcan[i].can_name)
        {
            perror("strdup");
            for (int k = 0; k < i; k++) free(hcan[k].can_name);
            free(hcan); 
            free(can_thread_param);
            exit(1);
        }
        can_thread_param[i].shared_data = &shared_data;
        can_thread_param[i].hcan = &hcan[i];
    }
    // 初始化其他实例
    write_sdcard_param_t write_sdcard_param = {
        .shared_data = &shared_data
    };

    printf("Starting main task...\n");

    // 创建线程
    pthread_t *can_receive_thread_can = calloc(can_count, sizeof(pthread_t));
    if (!can_receive_thread_can)
    {
        perror("calloc");
        for (int i = 0; i < can_count; i++)
        {
            free(hcan[i].can_name);
        }
        free(hcan);
        free(can_thread_param);
        exit(1);
    }
    for (int i = 0; i < can_count; i++)
    {
        pthread_create(&can_receive_thread_can[i], NULL, can_receive_task, (void *)&can_thread_param[i]);
    }
    pthread_t push_to_display_thread;
    pthread_t receive_from_display_thread;
    pthread_t write_sdcard_thread;
    pthread_create(&push_to_display_thread, NULL, push_to_display_task, (void *)&shared_data);
    pthread_create(&receive_from_display_thread, NULL, receive_from_display_task, (void *)&shared_data);
    pthread_create(&write_sdcard_thread, NULL, write_sdcard_task, (void *)&write_sdcard_param);

    // 回收线程
    for (int i = 0; i < can_count; i++)
    {
        pthread_join(can_receive_thread_can[i], NULL);
    }
    
    pthread_join(push_to_display_thread, NULL);
    pthread_join(receive_from_display_thread, NULL);
    pthread_join(write_sdcard_thread, NULL);

    // 销毁互斥锁
    pthread_mutex_destroy(&shared_data.display_ids.mutex);
    pthread_mutex_destroy(&shared_data.can_frames.mutex);
    printf("Hello, World!\n");
    
    for (int i = 0; i < can_count; i++)
    {
        free(hcan[i].can_name);
    }
    free(hcan);
    free(can_thread_param);
    free(can_receive_thread_can);

    return 0;
}