/* $NetBSD: kvm86.h,v 1.5 2005/12/26 19:23:59 perry Exp $ */

void kvm86_init(void);
void kvm86_gpfault(struct trapframe *);
extern int kvm86_incall;

void *kvm86_bios_addpage(uint32_t);
void kvm86_bios_delpage(uint32_t, void *);
size_t kvm86_bios_read(uint32_t, char *, size_t);
int kvm86_bioscall(int, struct trapframe *);

/* for migration from bioscall() */
#include <machine/bioscall.h>
int kvm86_bioscall_simple(int, struct bioscallregs *);
