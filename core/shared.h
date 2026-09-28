#ifndef __SHARED_H_
#define __SHARED_H_

#include "main.h"
#include "can.h"

// 显示id结构体
#define MAX_DISPLAY_CAN_ID 128
typedef struct
{
    uint32_t display_can_id[MAX_DISPLAY_CAN_ID]; // 需要显示的can_id
    uint32_t length; // canid数量
    pthread_mutex_t mutex; // 互斥锁
} display_id_t;

// can帧存储结构体
typedef struct
{
    uint8_t data[8]; // can数据
    uint8_t dlc; // can数据长度
    uint32_t can_id; // can_id
    uint64_t timestamp_us; // 相对时间戳
} can_frame_t;

// can帧数组结构体
#define MAX_CAN_FRAME 8192  // 大约 192 KiB
typedef struct
{
    can_frame_t frame[MAX_CAN_FRAME]; // can帧数组
    uint32_t head; // 头指针
    uint32_t tail; // 尾指针
    pthread_mutex_t mutex; // 互斥锁
} can_frame_array_t;

typedef struct
{
    struct can_frame frame[MAX_DISPLAY_CAN_ID]; // can帧数组
    uint32_t length; // can帧数量
    pthread_mutex_t mutex; // 互斥锁
} can_frame_array_t;

// 线程共享结构体
typedef struct
{
    display_id_t display_ids; // 显示id结构体
    can_frame_array_t can_frames; // can帧数组结构体
} shared_data_t;


#endif