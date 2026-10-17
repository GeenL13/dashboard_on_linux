#include "can_receive.h"
#include "can.h"
#include "main.h"
#include "shared.h"


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
    uint32_t sd_buf_drop_count; // 丢帧计数
    uint32_t display_drop_count; // 丢帧计数
} can_receive_info_t;


static uint64_t get_timestamp_us(void);

void* can_receive_task(void *arg)
{
    printf("channel:%d can receive task...\n", ((can_thread_param_t *)arg)->hcan->channel);
    // 取出参数
    can_thread_param_t *param = (can_thread_param_t *)arg;  // can线程参数结构体
    shared_data_t* shared_data = param->shared_data;  // 共享数据结构体
    can_t* hcan = param->hcan;  // can实例
    display_id_t* display_ids = &shared_data->display_ids;   // 共享数据中的显示id结构体
    can_frame_array_t* can_frames = &shared_data->can_frames; // 共享数据中的can帧数组结构体
    can_frame_array_t* can_frames_display = &shared_data->can_frames_display; // 共享数据中的can帧数组结构体，用于显示

    int res = 0;    // read返回值
    can_receive_info_t info = {0}; // can接收信息结构体
    uint64_t last_print_time = get_timestamp_us();    // 上次打印时间戳
    uint32_t last_rx_count = 0;      // 上次接收计数

    printf("channel:%d thread creat success: %d", hcan->channel, (int)pthread_self());

    // can初始化
    can_init(hcan); // 后续应该改成根据参数传入的can实例进行初始化
    // can接收任务
    printf("channel:%d can receive task...\n", hcan->channel);
    while (1)
    {
        // printf("is reading can data...\n");
        res = read(hcan->sockfd, &hcan->frame_rx, sizeof(hcan->frame_rx));
        if (res < 0)
        {
            perror("read error");
            info.error_count++;
            continue;
        }
        info.rx_count++;

        // 每秒打印接收到的can数据数量，帧率以及其他信息
        uint64_t current_time = get_timestamp_us();
        if (current_time - last_print_time >= 1000000)
        {
            uint32_t rx_count_diff = info.rx_count - last_rx_count;
            last_rx_count = info.rx_count;
            printf("channel:%d can receive count: %u, error count: %u, sd buf drop count: %u, display drop count: %u, frame rate: %.2f fps\n", 
                hcan->channel,
                info.rx_count, 
                info.error_count, 
                info.sd_buf_drop_count,
                info.display_drop_count,
                (double)rx_count_diff / ((current_time - last_print_time) / 1000000.0));
            last_print_time = current_time; // 更新时间戳
        }
        // 打印接收到的can数据
        // printf("can_id: 0x%X, can_dlc: %d, data: ", hcan->frame_rx.can_id, hcan->frame_rx.can_dlc);
        // for (uint32_t i = 0; i < hcan->frame_rx.can_dlc; i++)
        // {
        //     printf("%02X ", hcan->frame_rx.data[i]);
        // }
        // printf("rx_count: %u, error_count: %u", info.rx_count, info.error_count);
        // printf("\n");

        // 处理接收到的can数据
        // 填入can数据帧
        can_frame_t can_frame;
        can_frame.channel = hcan->channel;
        can_frame.timestamp_us = get_timestamp_us();
        can_frame.can_id = hcan->frame_rx.can_id;   // 这个id是socketcan的can_id，包含了扩展帧标志位和远程帧标志位
        can_frame.dlc = hcan->frame_rx.can_dlc;
        memcpy(can_frame.data, hcan->frame_rx.data, hcan->frame_rx.can_dlc);

        // 将can数据帧添加到共享数据中，用于写入SD卡
        pthread_mutex_lock(&can_frames->mutex);
        // 检查是否可以写入
        if (can_frames->count >= MAX_CAN_FRAME)
        {
            // 缓冲区已满，丢弃数据
            //printf("channel:%d can frame buffer full, discard data\n", hcan->channel);
            info.sd_buf_drop_count++;
            //pthread_mutex_unlock(&can_frames->mutex);
        }
        else
        {
            can_frames->frame[can_frames->head] = can_frame;
            can_frames->head = (can_frames->head + 1) % MAX_CAN_FRAME;
            can_frames->count++;    // 增加计数
        }
        pthread_mutex_unlock(&can_frames->mutex);

        // 将can数据帧添加到共享数据中，用于显示   
        int match = 0;    
        pthread_mutex_lock(&display_ids->mutex);
        for (uint32_t i = 0; i < display_ids->length; i++)
        {
            if (can_frame.channel == display_ids->display_id_frame[i].channel
                    && can_frame.can_id == display_ids->display_id_frame[i].display_can_id)
            {
                // 当前can帧需要发送给显示器
                match = 1;
                break;
            }
        }
        pthread_mutex_unlock(&display_ids->mutex);
        // 若为ui需要的数据，则将数据发送给显示线程
        // test 
        match = 1;
        if (match)
        {
            // 将can数据帧添加到共享数据中，用于显示
            pthread_mutex_lock(&can_frames_display->mutex);
            // 检查是否可以写入
            if (can_frames_display->count >= MAX_CAN_FRAME)
            {
                // 缓冲区已满，丢弃数据
                info.display_drop_count++;
                //printf("channel:%d can frame buffer full, discard data\n", hcan->channel);
                //pthread_mutex_unlock(&can_frames_display->mutex);
            }
            else
            {
                can_frames_display->frame[can_frames_display->head] = can_frame;
                can_frames_display->head = (can_frames_display->head + 1) % MAX_CAN_FRAME;
                can_frames_display->count++;    // 增加计数
            }
            pthread_mutex_unlock(&can_frames_display->mutex);
        }
    }
}

static uint64_t get_timestamp_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}