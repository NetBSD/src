/*
 * The syscall arguments are processed into a DTrace argument array
 * using a generated function. See sys/kern/makesyscalls.sh.
 */
#define COMPAT_NETBSD32

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_systrace_args.c>

#define emulname	netbsd32
#define EMULNAME	NETBSD32
