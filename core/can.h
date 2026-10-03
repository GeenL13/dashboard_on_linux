#ifndef __CAN_H_
#define __CAN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

typedef struct
{
    char* can_name;     // can名称
    uint8_t channel;    // can通道
    struct ifreq ifr;   // can网卡
    struct sockaddr_can can_addr;   // can接口
    struct can_frame frame_tx;      // can帧
    struct can_frame frame_rx;      // can帧
    struct can_filter filter;       // can过滤器
    int sockfd;    // can实例
} can_t;


void can_init(can_t *hcan);
void can_filter_config(can_t *hcan, uint32_t can_id, uint32_t can_mask);

#endif