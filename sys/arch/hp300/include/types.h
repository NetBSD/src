/*	$NetBSD: types.h,v 1.25 2025/12/03 02:15:08 thorpej Exp $	*/

#ifndef _HP300_TYPES_H_
#define	_HP300_TYPES_H_

#include <m68k/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_MM_MD_KERNACC
#define	__HAVE_BUS_SPACE_8

/*
 * 68020-based hp300 machines don't do indivisible R-M-W cycles
 * (used by CAS, CAS2, and TAS) correctly; /BERR is signaled,
 * so we need to avoid them.
 *
 * XXX Future optimization: if kernel is built without 68020 support,
 * XXX avoidance not required.
 */
#define	__HAVE_M68K_BROKEN_RMC		1	/* XXX 1 */

#endif /* !_HP300_TYPES_H_ */
