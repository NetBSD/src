/*      $NetBSD: cpuctl_i386.h,v 1.4 2019/05/21 05:29:21 mlelstv Exp $      */

/* Interfaces to code in i386-asm.S */

#define	x86_cpuid(a,b)	x86_cpuid2((a),0,(b))

void x86_cpuid2(uint32_t, uint32_t, uint32_t *);
uint32_t x86_identify(void);
uint32_t x86_xgetbv(void);
