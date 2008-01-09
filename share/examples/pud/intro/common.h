#include <sys/ioctl.h>

#define INTROTOGGLE	_IOWR('i', 1, int)
#define INTROTOGGLE_R	_IOW('i', 2, int)
