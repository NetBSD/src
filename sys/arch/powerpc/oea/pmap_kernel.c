/*	$NetBSD: pmap_kernel.c,v 1.2.2.2 2008/12/13 01:13:24 haad Exp $	*/

#include <sys/param.h>
#include <uvm/uvm_extern.h>
extern struct pmap kernel_pmap_;
struct pmap *const kernel_pmap_ptr = &kernel_pmap_;
