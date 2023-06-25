#define	PTRACE_HOOKS	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PTRACE_HOOKS
 .global _KERNEL_OPT_PTRACE_HOOKS
 .equiv _KERNEL_OPT_PTRACE_HOOKS,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PTRACE_HOOKS\n .global _KERNEL_OPT_PTRACE_HOOKS\n .equiv _KERNEL_OPT_PTRACE_HOOKS,0x1\n .endif");
#endif
#define	PTRACE	1
#ifdef _LOCORE
 .ifndef _KERNEL_OPT_PTRACE
 .global _KERNEL_OPT_PTRACE
 .equiv _KERNEL_OPT_PTRACE,0x1
 .endif
#else
__asm(" .ifndef _KERNEL_OPT_PTRACE\n .global _KERNEL_OPT_PTRACE\n .equiv _KERNEL_OPT_PTRACE,0x1\n .endif");
#endif
