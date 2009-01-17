/*	$NetBSD: pmap_kernel.c,v 1.2.4.2 2009/01/17 13:28:26 mjf Exp $	*/

#include <sys/param.h>
#include <uvm/uvm_extern.h>
extern struct pmap kernel_pmap_;
struct pmap *const kernel_pmap_ptr = &kernel_pmap_;
