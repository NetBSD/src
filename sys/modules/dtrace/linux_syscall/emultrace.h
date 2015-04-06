/*
 * The syscall arguments are processed into a DTrace argument array
 * using a generated function. See sys/kern/makesyscalls.sh.
 */
#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_mmap.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/linux_syscall.h>
#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_systrace_args.c>

#define emulname	linux
#define EMULNAME	LINUX
