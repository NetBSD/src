/*      $NetBSD: cpuctl_i386.h,v 1.2.6.2 2013/02/25 00:30:42 tls Exp $      */

/* Interfaces to code in i386-asm.S */

#define	x86_cpuid(a,b)	x86_cpuid2((a),0,(b))

void x86_cpuid2(uint32_t, uint32_t, uint32_t *);
uint32_t x86_identify(void);
uint32_t x86_xgetbv(void);
