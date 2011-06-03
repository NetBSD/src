/* $NetBSD: system.h,v 1.1.2.1 2011/06/03 13:27:43 cherry Exp $ */

#define PAGE_SIZE getpagesz()

unsigned long getpagesz(void);
cpuid_t getcpus(void);
