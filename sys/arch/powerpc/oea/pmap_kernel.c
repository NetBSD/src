/*	$NetBSD: pmap_kernel.c,v 1.2.10.2 2009/05/04 08:11:44 yamt Exp $	*/

#include <sys/param.h>
#include <uvm/uvm_extern.h>
extern struct pmap kernel_pmap_;
struct pmap *const kernel_pmap_ptr = &kernel_pmap_;
