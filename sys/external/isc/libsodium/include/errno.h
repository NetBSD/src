#include <sys/cdefs.h>
#include <sys/errno.h>

#define errno	libsodium_errno

extern int libsodium_errno;
