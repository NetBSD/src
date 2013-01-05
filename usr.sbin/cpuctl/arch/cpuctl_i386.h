/*      $NetBSD: cpuctl_i386.h,v 1.1 2013/01/05 15:27:45 dsl Exp $      */

/* Interfaces to code in i386-asm.S */

#define	x86_cpuid(a,b)	x86_cpuid2((a),0,(b))

void x86_cpuid2(uint32_t, uint32_t, uint32_t *);
uint32_t x86_identify(void);
