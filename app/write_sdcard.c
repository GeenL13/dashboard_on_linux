#include "write_sdcard.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>

#define SDCARD_DIR "/mnt/sdcard"  // SD卡挂载路径
#define SDCARD_DEVICE "/dev/mmcblk0p1"  // SD卡设备路径
#define LOG_BATCH_FRAMES 2048  // 批量写入的can帧数量
can_frame_t can_frame_buffer[LOG_BATCH_FRAMES];  // 批量写入的can帧缓冲区， 48KiB

typedef enum
{
    INIT_STATE,
    WRITE_STATE,
    RECOVER_STATE
} write_sdcard_state_t;
write_sdcard_state_t write_sdcard_state = INIT_STATE;  // 写入sd卡状态机

static int check_sdcard_mount(void);
static int try_mount_sdcard(void);
static int write_all(int fd, const void *buf, size_t count);

void* write_sdcard_task(void* arg)
{
    // 取出参数
    write_sdcard_param_t *param = (write_sdcard_param_t *)arg;  // 写入sd卡参数结构体
    shared_data_t *shared_data = (shared_data_t *)param->shared_data;  // 共享数据结构体
    can_frame_array_t *can_frames = &shared_data->can_frames;

    printf("Starting SD card write task...\n");

    int fd;
    while (1)
    {
        switch (write_sdcard_state)
        {
            case INIT_STATE:
                printf("SD card write state: INIT_STATE\n");
                // 检查SD卡挂载情况
                while (1)
                {
                    if (0 == check_sdcard_mount())
                    {
                        break;
                    }
                    // 尝试挂载
                    try_mount_sdcard();
                    sleep(1);
                }

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

                fd = open(log_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);   // 打开、清空、创建日志文件
                if (fd < 0)
                {
                    perror("open log file");
                    // 卸载SD卡挂载点
                    umount(SDCARD_DIR);
                    // 进入恢复状态
                    write_sdcard_state = RECOVER_STATE;
                    break;
                }
                printf("Log file created: %s\n", log_file_path);
                // 进入写入状态
                write_sdcard_state = WRITE_STATE;
                break;
            case WRITE_STATE:
                //printf("SD card write state: WRITE_STATE\n");
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
                    int res = write_all(fd, can_frame_buffer, bytes);
                    if (res < 0) {
                        // 错误处理
                        // 关闭文件描述符
                        close(fd);
                        // 卸载SD卡挂载点
                        umount(SDCARD_DIR);
                        // 进入恢复状态
                        write_sdcard_state = RECOVER_STATE;
                        break;
                    }
                }
                else
                {
                    pthread_mutex_unlock(&can_frames->mutex);   // 解锁can帧缓冲区
                    usleep(1000);   // 等待 1 毫秒
                }
                break;
            case RECOVER_STATE:
                //printf("SD card write state: RECOVER_STATE\n");
                // 检查SD卡挂载情况
                if (1 == check_sdcard_mount())
                {
                    // SD 卡未挂载
                    try_mount_sdcard();
                    break;
                }
                // 进入初始化状态
                fd = -1;
                write_sdcard_state = INIT_STATE;
                break;
            default:
                printf("SD card write state: UNKNOWN_STATE\n");
                break;
        }
    }

}

// 检查sd卡是否挂载
static int check_sdcard_mount(void)
{ 
    char device[256];      // SD卡设备路径
    char file_path[256];   // 日志文件路径
    char vfat[256];        // 文件系统类型
    char options[256];     // 挂载选项
    FILE* fp = fopen("/proc/mounts", "r");
    if (fp == NULL) 
    {
        perror("fopen /proc/mounts");
        return 1;
    }
    while (fscanf(fp, "%255s %255s %255s %255s %*d %*d", 
            device, 
            file_path, 
            vfat, 
            options) == 4) 
    {

        if (strcmp(file_path, SDCARD_DIR) == 0) 
        {
            printf("SD card is mounted at %s\n", SDCARD_DIR);
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return 1;
}

// 尝试挂载sdcard
static int try_mount_sdcard(void)
{
    if(0 == mount(SDCARD_DEVICE, SDCARD_DIR, "vfat", 0, NULL))
    {
        printf("SD card mounted successfully at %s\n", SDCARD_DIR);
        return 0;
    }
    else
    {
        perror("mount SD card");
        return 1;
    }
}

static int write_all(int fd, const void *buf, size_t count)
{
    size_t total_written = 0;
    const uint8_t *ptr = (const uint8_t *)buf;

    while (total_written < count) 
    {
        ssize_t written = write(fd, ptr + total_written, count - total_written);
        if (written < 0) 
        {
            if (errno == EINTR) 
            {
                continue; // 被信号中断，重试
            }
            perror("write log error");
            return -1; // 写入错误
        }
        if (written == 0) 
        {
            fprintf(stderr, "write log error: wrote 0 bytes\n");
            return -1; // 写入错误
        }
        total_written += (size_t)written;
    }
    return 0; // 成功写入所有数据
}