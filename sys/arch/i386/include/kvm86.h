/* $NetBSD: kvm86.h,v 1.3 2002/10/01 12:57:07 fvdl Exp $ */

void kvm86_init(void);
void kvm86_gpfault(struct trapframe *);

void *kvm86_bios_addpage(u_int32_t);
void kvm86_bios_delpage(u_int32_t, void *);
size_t kvm86_bios_read(u_int32_t, char *, size_t);
int kvm86_bioscall(int, struct trapframe *);

/* for migration from bioscall() */
#include <machine/bioscall.h>
int kvm86_bioscall_simple(int, struct bioscallregs *);
