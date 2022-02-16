/*	$NetBSD: pmap.h,v 1.41 2022/02/16 23:31:13 riastradh Exp $	*/

#ifndef _POWERPC_PMAP_H_
#define _POWERPC_PMAP_H_

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#include "opt_modular.h"
#endif

#if !defined(_MODULE)

#if defined(PPC_BOOKE)
#include <powerpc/booke/pmap.h>
#elif defined(PPC_IBM4XX)
#include <powerpc/ibm4xx/pmap.h>
#elif defined(PPC_OEA) || defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/pmap.h>
#elif defined(_KERNEL)
#error unknown PPC variant
#endif

#endif /* !_MODULE */

#include <uvm/pmap/pmap_pvt.h>

#if !defined(_LOCORE) && (defined(MODULAR) || defined(_MODULE))
/*
 * Both BOOKE and OEA use __HAVE_VM_PAGE_MD but IBM4XX doesn't so define
 * a compatible vm_page_md so that struct vm_page is the same size for all
 * PPC variants.
 */
#ifndef __HAVE_VM_PAGE_MD
#define __HAVE_VM_PAGE_MD
#define VM_MDPAGE_INIT(pg) __nothing

struct vm_page_md {
	uintptr_t mdpg_dummy[5];
};
#endif /* !__HAVE_VM_PAGE_MD */

__CTASSERT(sizeof(struct vm_page_md) == sizeof(uintptr_t)*5);

#endif /* !LOCORE && (MODULAR || _MODULE) */

#endif /* !_POWERPC_PMAP_H_ */
