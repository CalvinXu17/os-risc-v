#ifndef SDCARD_H
#define SDCARD_H

#include "type.h"

void sdcard_init(void);

void sdcard_read_sector(uchar *buf, int sectorno);

void sdcard_write_sector(uchar *buf, int sectorno);

void test_sdcard(void);

#endif 
