/* $NetBSD: kvm86.h,v 1.1 2002/07/07 12:56:34 drochner Exp $ */

void kvm86_init(void);
void kvm86_gpfault(struct trapframe *);
extern int kvm86_incall;

void *kvm86_bios_addpage(u_int32_t);
void kvm86_bios_delpage(u_int32_t, void *);
int kvm86_bioscall(int, struct trapframe *);

/* for migration from bioscall() */
#include <machine/bioscall.h>
int kvm86_bioscall_simple(int, struct bioscallregs *);
