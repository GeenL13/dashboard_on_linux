#include "connect_to_display.h"
#include <stdint.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "shared.h"

#define DISPLAY_SOCKET_PATH "/tmp/display_socket"  // 显示器进程的UNIX域套接字路径

static int sendall(int sockfd, const void *buf, size_t len);

void* push_to_display_task(void *arg)
{
    // 连接到显示器的任务
    printf("push to display task...\n");
    // 取出参数
    push_to_display_param_t *param = (push_to_display_param_t *)arg;  // 参数结构体
    shared_data_t* shared_data = param->shared_data;  // 共享数据结构体
    // display_id_t* display_ids = &shared_data->display_ids;   // 共享数据中的显示id结构体
    can_frame_array_t* can_frames_display = &shared_data->can_frames_display; // 共享数据中的can帧数组结构体，用于显示

    // 发送数据给显示器进程
    int sockfd = -1; // 套接字文件描述符
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket error");
        return NULL;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, DISPLAY_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    unlink(DISPLAY_SOCKET_PATH); // 删除旧的套接字文件，防止绑定失败
    if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        printf("bind error: %s\n", strerror(errno));
        // 关闭套接字
        close(sockfd);
        // 进入恢复状态
        sleep(1);
        return NULL;
    }
    if (listen(sockfd, 5) < 0)
    {
        printf("listen error: %s\n", strerror(errno));
        // 关闭套接字
        close(sockfd);
        // 进入恢复状态
        sleep(1);
        return NULL;
    }
    printf("Waiting for display process to connect...\n");
    int connfd = accept(sockfd, NULL, NULL);
    if (connfd < 0)
    {
        printf("accept error: %s\n", strerror(errno));
        // 关闭套接字
        close(sockfd);
        // 进入恢复状态
        sleep(1);
        return NULL;
    }
    printf("Display process connected.\n");

    while (1)
    {
        // 判断是否有新的can帧需要发送给显示器
        can_frame_t can_frame;
        pthread_mutex_lock(&can_frames_display->mutex);   // 锁定can帧缓冲区
        if (can_frames_display->count > 0)
        {       
            can_frame = can_frames_display->frame[can_frames_display->tail];
            // 更新尾指针和计数
            can_frames_display->tail = (can_frames_display->tail + 1) % MAX_CAN_FRAME;
            can_frames_display->count--;
            pthread_mutex_unlock(&can_frames_display->mutex);   // 解锁can帧缓冲区
            // 将数据发送给显示器
            // test
            //printf("Sending can frame to display process: channel=%d, can_id=0x%X, dlc=%d\n", 
            //   can_frame.channel, can_frame.can_id, can_frame.dlc);
            int res = sendall(connfd, &can_frame, sizeof(can_frame_t));
            if (res < 0)
            {
                printf("send error: %s\n", strerror(errno));
                // 关闭套接字
                close(connfd);
                close(sockfd);
                // 进入恢复状态
                sleep(1);
            }
        }
        else
        {
            pthread_mutex_unlock(&can_frames_display->mutex);   // 解锁can帧缓冲区
            usleep(1000);   // 等待 1 毫秒
        }
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

static int sendall(int sockfd, const void *buf, size_t len)
{
    size_t total_sent = 0;
    const uint8_t *ptr = (const uint8_t *)buf;

    while (total_sent < len) 
    {
        // test
        //printf("Sending data to display process...\n");
        ssize_t sent = send(sockfd, ptr + total_sent, len - total_sent, 0);
        if (sent < 0) 
        {
            // test
            //printf("send error: %s\n", strerror(errno));
            if (errno == EINTR) 
            {
                continue; // 被信号中断，重试
            }
            perror("send error");
            return -1; // 发送错误
        }
        if (sent == 0) 
        {
            fprintf(stderr, "send error: sent 0 bytes\n");
            return -1; // 发送错误
        }
        total_sent += (size_t)sent;
    }
    return 0; // 成功发送所有数据
}