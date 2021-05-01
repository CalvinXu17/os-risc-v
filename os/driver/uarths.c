/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "uarths.h"
#include "mm.h"
#define EOF -1

volatile uarths_t *const uarths = (volatile uarths_t *)UART;

uarths_interrupt_mode_t uarths_get_interrupt_mode(void)
{
    uint32 v_rx_interrupt = uarths->ip.rxwm;
    uint32 v_tx_interrupt = uarths->ip.txwm;
    return (v_rx_interrupt << 1) | v_tx_interrupt;
}

void uarths_set_interrupt_cnt(uarths_interrupt_mode_t interrupt_mode, uchar cnt)
{
    switch(interrupt_mode)
    {
        case UARTHS_SEND:
            uarths->txctrl.txcnt = cnt;
            break;
        case UARTHS_RECEIVE:
            uarths->rxctrl.rxcnt = cnt;
            break;
        case UARTHS_SEND_RECEIVE:
        default:
            uarths->txctrl.txcnt = cnt;
            uarths->rxctrl.rxcnt = cnt;
            break;
    }
}

int uarths_putchar(char c)
{
    while(uarths->txdata.full)
        continue;
    uarths->txdata.data = (uchar)c;

    return (c & 0xff);
}

int uarths_getchar(void)
{
    /* while not empty */
    uarths_rxdata_t recv = uarths->rxdata;

    if(recv.empty)
        return EOF;
    else
        return (recv.data & 0xff);
}

uint64 uarths_receive_data(uchar *buf, uint64 buf_len)
{
    uint64 i;
    for(i = 0; i < buf_len; i++)
    {
        uarths_rxdata_t recv = uarths->rxdata;
        if(recv.empty)
            break;
        else
            buf[i] = (recv.data & 0xFF);
    }
    return i;
}

uint64 uarths_send_data(const uchar *buf, uint64 buf_len)
{
    uint64 write = 0;
    while(write < buf_len)
    {
        uarths_putchar(*buf++);
        write++;
    }
    return write;
}

int uarths_puts(const char *s)
{
    while(*s)
        if(uarths_putchar(*s++) != 0)
            return -1;
    return 0;
}
