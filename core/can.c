#include "can.h"

void can_init(can_t *hcan, const char *ifname)
{
    // 创建socket
    hcan->sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (0 > hcan->sockfd)
    {
        perror("%s: socket error", ifname);
        exit(EXIT_FAILURE);
    }

    // 获取can设备
    strcpy(hcan->ifr.ifr_name, ifname);
    ioctl(hcan->sockfd, SIOCGIFINDEX, &hcan->ifr);
    hcan->can_addr.can_family = AF_CAN;
    hcan->can_addr.can_ifindex = hcan->ifr.ifr_ifindex;

    // 绑定can0和socket
    int res = bind(hcan->sockfd, (struct sockaddr *)&hcan->can_addr, sizeof(hcan->can_addr));
    if (res < 0)
    {
        perror("%s: bind error", ifname);
        close(hcan->sockfd);
        exit(EXIT_FAILURE);
    }
}

void can_filter_config(can_t *hcan, uint32_t can_id, uint32_t can_mask)
{
    hcan->filter.can_id = can_id;
    hcan->filter.can_mask = can_mask;

    // 设置can过滤规则
    int res = setsockopt(hcan->sockfd, SOL_CAN_RAW, CAN_RAW_FILTER, &hcan->filter, sizeof(hcan->filter));
    if (res < 0)
    {
        perror("%s: setsockopt error", hcan->ifr.ifr_name);
        close(hcan->sockfd);
        exit(EXIT_FAILURE);
    }
}