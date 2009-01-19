/*	$NetBSD: pmap_kernel.c,v 1.2.6.2 2009/01/19 13:16:37 skrll Exp $	*/

#include <sys/param.h>
#include <uvm/uvm_extern.h>
extern struct pmap kernel_pmap_;
struct pmap *const kernel_pmap_ptr = &kernel_pmap_;
