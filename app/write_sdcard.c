#include "write_sdcard.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define SDCARD_DIR "/mnt/sdcard"  // SD卡挂载路径
#define LOG_BATCH_FRAMES 2048  // 批量写入的can帧数量
can_frame_t can_frame_buffer[LOG_BATCH_FRAMES];  // 批量写入的can帧缓冲区， 48KiB

void* write_sdcard_task(void* arg)
{
    // 取出参数
    write_sdcard_param_t *param = (write_sdcard_param_t *)arg;  // 写入sd卡参数结构体
    shared_data_t *shared_data = (shared_data_t *)param->shared_data;  // 共享数据结构体
    can_frame_array_t *can_frames = &shared_data->can_frames;

    printf("Starting SD card write task...\n");
    // 检查sd卡是否挂载
    char device[256];      // SD卡设备路径
    char file_path[256];   // 日志文件路径
    char vfat[256];        // 文件系统类型
    char options[256];     // 挂载选项
    FILE* fp = fopen("/proc/mounts", "r");
    if (fp == NULL) 
    {
        perror("fopen /proc/mounts");
        return 0;
    }
    while (1)
    {
        if (fscanf(fp, "%255s %255s %255s %255s %*d %*d", 
            device, 
            file_path, 
            vfat, 
            options) == 4) 
        {

            if (strcmp(file_path, SDCARD_DIR) == 0) 
            {
                break;
            }
        }
        else 
        {
            printf("SD card is not mounted, waiting...\n");
            rewind(fp); // 回到文件开头重新读取
            sleep(1);
        }
    }
    fclose(fp);
    printf("SD card is mounted at %s\n", SDCARD_DIR);

    // 创建日志文件
    char log_file_path[256];
    time_t now; // 定义时间变量
    struct tm tm_info; // 定义时间结构体
    time(&now); // 获取当前时间
    localtime_r(&now, &tm_info); // 将时间转换为本地时间
    sprintf(log_file_path, "%s/canlog-%04d-%02d-%02d-%02d-%02d-%02d.bin",
             SDCARD_DIR,
             tm_info.tm_year + 1900,
             tm_info.tm_mon + 1,
             tm_info.tm_mday,
             tm_info.tm_hour,
             tm_info.tm_min,
             tm_info.tm_sec);
    int fd;
    while (1)
    {
        fd = open(log_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);   // 打开、清空、创建日志文件
        if (fd < 0)
        {
            perror("open log file");
            sleep(1);
            continue;
        }
        else
        {
            break;
        }
    }
    printf("Log file created: %s\n", log_file_path);
    while (1)
    {

        // 写入日志数据
        pthread_mutex_lock(&can_frames->mutex); // 锁定can帧缓冲区
        if (can_frames->count >= LOG_BATCH_FRAMES)
        {
            // 缓冲区已达到批量写入的数量，进行写入
            uint32_t len1 = (MAX_CAN_FRAME - can_frames->tail) < LOG_BATCH_FRAMES 
            ? (MAX_CAN_FRAME - can_frames->tail) : LOG_BATCH_FRAMES;
            uint32_t len2 = LOG_BATCH_FRAMES - len1;
            memcpy(can_frame_buffer, &can_frames->frame[can_frames->tail], len1 * sizeof(can_frame_t));
            memcpy(&can_frame_buffer[len1], &can_frames->frame[0], len2 * sizeof(can_frame_t));
            // 更新尾指针和计数
            can_frames->tail = (can_frames->tail + LOG_BATCH_FRAMES) % MAX_CAN_FRAME;
            can_frames->count -= LOG_BATCH_FRAMES;
            pthread_mutex_unlock(&can_frames->mutex);   // 解锁can帧缓冲区
            printf("Writing log data to SD card...\n");
            // 批量写入到SD卡
            uint32_t bytes = LOG_BATCH_FRAMES * sizeof(can_frame_t);
            uint32_t remaining = bytes;
            uint8_t* ptr = (uint8_t*)can_frame_buffer;
            while (remaining > 0) 
            {
                ssize_t len = write(fd, ptr, remaining);

                if (len > 0) {
                    ptr += (size_t)len;        // 前进实际写入的字节数
                    remaining -= (size_t)len;
                    continue;
                }

                if (len < 0 && errno == EINTR) {
                    continue;                  // 被信号中断，重试
                }

                if (len == 0) {
                    fprintf(stderr, "write returned 0 unexpectedly\n");
                } else {
                    perror("write log file");
                }
                break;
            }
        }
        else
        {
            pthread_mutex_unlock(&can_frames->mutex);   // 解锁can帧缓冲区
            usleep(1000);   // 等待 1 毫秒
        }
    }
    
}