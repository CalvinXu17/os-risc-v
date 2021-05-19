#ifndef FILE_H
#define FILE_H

struct file
{
    void *data; // 是file则为xfat_file，是输入输出设备则为console
};

struct file_operations
{

};

#endif