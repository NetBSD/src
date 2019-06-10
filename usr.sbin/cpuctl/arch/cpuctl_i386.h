/*      $NetBSD: cpuctl_i386.h,v 1.2.36.1 2019/06/10 22:10:29 christos Exp $      */

/* Interfaces to code in i386-asm.S */

#define	x86_cpuid(a,b)	x86_cpuid2((a),0,(b))

void x86_cpuid2(uint32_t, uint32_t, uint32_t *);
uint32_t x86_identify(void);
uint32_t x86_xgetbv(void);
