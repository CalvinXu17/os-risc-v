#include "string.h"

int strlen(const char *s)
{
    int cnt = 0;
    while (*s ++ != '\0')
        cnt ++;
    return cnt;
}

int strnlen(const char *s, int len)
{
    int cnt = 0;
    while (cnt < len && *s ++ != '\0')
        cnt ++;
    return cnt;
}

char* strcpy(char *to, const char *from)
{
    char *p = to;
    while ((*p ++ = *from ++) != '\0') {}
    return to;
}

char* strncpy(char *to, const char *from, int len)
{
    char *p = to;
    while (len > 0)
    {
        if ((*p = *from) != '\0')
            from++;
        p++, len--;
    }
    return to;
}

char* strcat(char *to, const char *from)
{
    return strcpy(to + strlen(to), from);
}


int strcmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && *s1 == *s2)
    {
        s1++;
        s2++;
    }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}

int strncmp(const char *s1, const char *s2, int n)
{
    while (n > 0 && *s1 != '\0' && *s1 == *s2)
    {
        n--, s1++, s2++;
    }
    return (n == 0) ? 0 : (int)((unsigned char)*s1 - (unsigned char)*s2);
}

char* strchr(const char *s, char c)
{
    while (*s != '\0')
    {
        if (*s == c)
            return (char*)s;
        s++;
    }
    return 0;
}

char* strfind(const char *s, char c)
{
    while (*s != '\0')
    {
        if (*s == c)
            break;
        s++;
    }
    return (char*)s;
}

int strtol(const char *s, char **endptr, int base)
{
    int neg = 0;
    long val = 0;

    while (*s == ' ' || *s == '\t')
        s++;

    if (*s == '+')
    {
        s++;
    }
    else if (*s == '-')
    {
        s++, neg = 1;
    }

    if ((base == 0 || base == 16) && (s[0] == '0' && s[1] == 'x'))
    {
        s += 2, base = 16;
    }
    else if (base == 0 && s[0] == '0')
    {
        s++, base = 8;
    }
    else if (base == 0)
    {
        base = 10;
    }

    while (1)
    {
        int dig;

        if (*s >= '0' && *s <= '9')
        {
            dig = *s - '0';
        }
        else if (*s >= 'a' && *s <= 'z')
        {
            dig = *s - 'a' + 10;
        }
        else if (*s >= 'A' && *s <= 'Z')
        {
            dig = *s - 'A' + 10;
        }
        else
            break;
        if (dig >= base)
            break;
        s++, val = (val * base) + dig;
    }

    if (endptr)
    {
        *endptr = (char *) s;
    }
    return (neg ? -val : val);
}

void* memset(void *s, char c, int n)
{
    char *p = s;
    while (n-- > 0)
        *p++ = c;
    return s;
}

void* memmove(void *to, const void *from, int n)
{
    const char *s = from;
    char *d = to;
    if (s < d && s + n > d)
    {
        s += n, d += n;
        while (n-- > 0)
        {
            *-- d = *-- s;
        }
    } else
    {
        while (n-- > 0)
        {
            *d++ = *s++;
        }
    }
    return to;
}

void* memcpy(void *to, const void *from, int n)
{
    const char *s = from;
    char *d = to;
    while (n-- > 0)
        *d++ = *s++;
    return to;
}

int memcmp(const void *v1, const void *v2, int n)
{
    const char *s1 = (const char*)v1;
    const char *s2 = (const char*)v2;
    while (n-- > 0)
    {
        if (*s1 != *s2)
            return (int)((unsigned char)*s1 - (unsigned char)*s2);
        s1++, s2++;
    }
    return 0;
}

