#include "printk.h"
#include "sbi.h"
#include "spinlock.h"

spinlock printk_lock;

void printk_init(void)
{
    init_spinlock(&printk_lock, "printk_lock");
}

int put_unum(uint64 num, int base)
{
    int cnt = 1;
    char buffer[24] = {0};
    uint64 c= num;
    
    int i;
    if(c) {
        cnt--;
        while(c)
        {
            cnt++;
            c /= base;
        }
    }
    if(cnt > 24)
    {
        puts("<error num>");
    }
    i = cnt-1;
    do {
        c = num % base;
        if(c>9) c = c - 10 + 'A';
        else c = c + '0';
        buffer[i--] = c;
        num /= base;
    } while(num);
    for(i=0;i<cnt;i++)
    {
        putc(buffer[i]);
    }
    return cnt;
}

int put_num(uint64 num, int base)
{
    int cnt = 1;
    if(((int64)num) < 0)
    {
        putc('-');
        cnt++;
        num = num - 1;
        num = ~num; // 负数补码转正数
    }
    cnt += put_unum(num, base);
    return cnt;
}

static int vprintf(const char *fmt, va_list ap) 
{
    int cnt = 0;
	for(; *fmt != '\0'; fmt++)
	{
		if (*fmt != '%') {
			if(*fmt != '\n' && *fmt != '\t')
            	cnt++;
			putc(*fmt);            
			continue;   		
		}
		fmt++;
		
		switch (*fmt) {
            case 'l':
                if(*(fmt+1)=='d')
                {
                    fmt++;
                    cnt+=put_num(va_arg(ap, int64), 10);
                } else if(*(fmt+1)=='o')
                {
                    fmt++;
                    cnt+=put_num(va_arg(ap, uint64), 8);
                } else if(*(fmt+1)=='u')
                {
                    fmt++;
                    cnt+=put_num(va_arg(ap, uint64), 10);
                } else if(*(fmt+1)=='x')
                {
                    fmt++;
                    cnt+=put_unum(va_arg(ap, uint64), 16);
                } else
                {
                    putc('%');
                    putc(*fmt);
                    if(*fmt != '\n' && *fmt != '\t')
                        cnt++;
                }
                break;
            case 'd': cnt+=put_num(va_arg(ap, int), 10); break;
            case 'o': cnt+=put_num(va_arg(ap, unsigned int), 8); break;				
            case 'u': cnt+=put_num(va_arg(ap, unsigned int), 10); break;
            case 'x': cnt+=put_unum(va_arg(ap, unsigned int), 16); break;
            case 'c': putc((char)va_arg(ap, int)); cnt++; break;
            case 's': cnt+=puts(va_arg(ap, char *)); break;     
            default:  
                putc(*fmt);
				if(*fmt != '\n' && *fmt != '\t')
                	cnt++;
                break;
		}
	}
	return cnt;
}

int printk(const char *fmt, ...) 
{
    //lock(&printk_lock);
	va_list ap;
	va_start(ap, fmt);
	int ret = vprintf(fmt, ap);
	va_end(ap);
    //unlock(&printk_lock);
	return ret;
}
