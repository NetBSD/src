#if defined (_ARM_ARCH_4T) || defined(__ARM_ARCH_ISA_THUMB)
# define RET		bx		lr
#else
# define RET		mov		pc, lr
#endif
