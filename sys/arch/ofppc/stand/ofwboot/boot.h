#ifndef BOOT_H_
#define BOOT_H_

#include <sys/types.h>

typedef void (*boot_entry_t)(int, int, int (*)(void *), void *, u_int);

void main(void);

#endif /* BOOT_H_ */
