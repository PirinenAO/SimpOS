#ifndef SIMPOS_MEMORY_H
#define SIMPOS_MEMORY_H
#include <stddef.h>

/* function prototypes*/
void *memset(void *ptr, int c, size_t size);
int memcmp(void *s1, void *s2, int count);
void *memcpy(void *dest, void *src, int len);

#endif