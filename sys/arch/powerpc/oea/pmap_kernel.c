/*	$NetBSD: pmap_kernel.c,v 1.2 2008/12/11 19:33:12 pooka Exp $	*/

#include <sys/param.h>
#include <uvm/uvm_extern.h>
extern struct pmap kernel_pmap_;
struct pmap *const kernel_pmap_ptr = &kernel_pmap_;
