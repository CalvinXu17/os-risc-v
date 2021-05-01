#ifndef STRING_H
#define STRING_H

int strlen(const char *s);
int strnlen(const char *s, int len);
char* strcpy(char *to, const char *from);
char* strncpy(char *to, const char *from, int len);
char* strcat(char *to, const char *from);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
char* strchr(const char *s, char c);
char* strfind(const char *s, char c);
int strtol(const char *s, char **endptr, int base);
void* memset(void *s, char c, int n);
void* memmove(void *to, const void *from, int n);
void* memcpy(void *to, const void *from, int n);
int memcmp(const void *v1, const void *v2, int n);

#endif