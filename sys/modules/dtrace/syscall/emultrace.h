/*
 * The syscall arguments are processed into a DTrace argument array
 * using a generated function. See sys/kern/makesyscalls.sh.
 */
#include <sys/syscall.h>
#include <kern/systrace_args.c>

#define emulname	netbsd
#define EMULNAME	NETBSD
#define NATIVE
