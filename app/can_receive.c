#include "can_receive.h"
#include "can.h"
#include "main.h"



// void can_reveive_init(void)
// {
//     // 初始化can接收
//     can_init(&hcan0, "can0");
//     can_filter_config(&hcan0, 0x123, 0x00); // 配置can过滤规则
//     can_init(&hcan1, "can1");
//     can_filter_config(&hcan1, 0x456, 0x00); // 配置can过滤规则
// }

typedef struct
{
    uint32_t error_count; // 错误计数
    uint32_t rx_count;    // 接收计数
} can_receive_info_t;


static uint64_t get_timestamp_us(void);

void* can_receive_task(void *arg)
{
    printf("can receive task...\n");
    // 取出参数
    can_thread_param_t *param = (can_thread_param_t *)arg;  // can线程参数结构体
    shared_data_t* shared_data = param->shared_data;  // 共享数据结构体
    can_t* hcan = param->hcan;  // can实例
    display_id_t* display_ids = &shared_data->display_ids;   // 共享数据中的显示id结构体
    can_frame_array_t* can_frames = &shared_data->can_frames; // 共享数据中的can帧数组结构体

    can_receive_info_t info = {0}; // can接收信息结构体
    int res = 0;
    printf("thread creat success: %d", (int)pthread_self());

    // can初始化
    can_init(hcan); // 后续应该改成根据参数传入的can实例进行初始化
    // can接收任务
    printf("can receive task...\n");
    while (1)
    {
        printf("is reading can data...\n");
        res = read(hcan->sockfd, &hcan->frame_rx, sizeof(hcan->frame_rx));
        if (res < 0)
        {
            perror("read error");
            info.error_count++;
            continue;
        }
        info.rx_count++;
        // 打印接收到的can数据
        // printf("can_id: 0x%X, can_dlc: %d, data: ", hcan->frame_rx.can_id, hcan->frame_rx.can_dlc);
        // for (uint32_t i = 0; i < hcan->frame_rx.can_dlc; i++)
        // {
        //     printf("%02X ", hcan->frame_rx.data[i]);
        // }
        // printf("rx_count: %u, error_count: %u", info.rx_count, info.error_count);
        // printf("\n");
        // 若为ui需要的数据，则将数据发送给qt
        for (uint32_t i = 0; i < display_ids->length; i++)
        {
            if (hcan->frame_rx.can_id == display_ids->display_can_id[i])
            {
                // 打印发送给qt的数据
                printf("send data to qt: can_id: 0x%X, can_dlc: %d, data: ", hcan->frame_rx.can_id, hcan->frame_rx.can_dlc);
                for (int j = 0; j < hcan->frame_rx.can_dlc; j++)
                {
                    printf("%02X ", hcan->frame_rx.data[j]);
                }
                printf("\n");
                break;
            }
        }

        // 处理接收到的can数据
        // 填入can数据帧
        can_frame_t can_frame;
        can_frame.channel = hcan->channel;
        can_frame.timestamp_us = get_timestamp_us();
        can_frame.can_id = hcan->frame_rx.can_id;
        can_frame.dlc = hcan->frame_rx.can_dlc;
        memcpy(can_frame.data, hcan->frame_rx.data, hcan->frame_rx.can_dlc);

        // 将can数据帧添加到共享数据中
        pthread_mutex_lock(&param->shared_data->can_frames.mutex);
        // 检查是否可以写入
        if (can_frames->count >= MAX_CAN_FRAME)
        {
            // 缓冲区已满，丢弃数据
            printf("can frame buffer full, discard data\n");
            pthread_mutex_unlock(&param->shared_data->can_frames.mutex);
            continue;
        }
        can_frames->frame[can_frames->head] = can_frame;
        can_frames->head = (can_frames->head + 1) % MAX_CAN_FRAME;
        can_frames->count++;    // 增加计数
        pthread_mutex_unlock(&param->shared_data->can_frames.mutex);
    }
}

static uint64_t get_timestamp_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}