/*	$NetBSD: hesiod.h,v 1.1.4.2 1997/05/23 20:31:39 lukem Exp $	*/

/* This file contains definitions for use by the Hesiod name service and
 * applications.
 *
 *  Copyright (C) 1989 by the Massachusetts Institute of Technology
 * 
 *    Export of software employing encryption from the United States of
 *    America is assumed to require a specific license from the United
 *    States Government.  It is the responsibility of any person or
 *    organization contemplating export to obtain such a license before
 *    exporting.
 * 
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 * Original version by Steve Dyer, IBM/Project Athena.
 *
 */
#ifndef _HESIOD_H_
#define _HESIOD_H_

/* Configuration information. */

#ifndef _PATH_HESIOD_CONF			/* Configuration file. */
#define _PATH_HESIOD_CONF	"/etc/hesiod.conf"
#endif

#define DEF_RHS		""			/* Defaults if HESIOD_CONF */
#define DEF_LHS		""			/*    file is not present. */

/* Error codes. */

#define	HES_ER_UNINIT	-1	/* uninitialized */
#define	HES_ER_OK	0	/* no error */
#define	HES_ER_NOTFOUND	1	/* Hesiod name not found by server */
#define HES_ER_CONFIG	2	/* local problem (no config file?) */
#define HES_ER_NET	3	/* network problem */

/* Declaration of routines */
#include <sys/cdefs.h>

__BEGIN_DECLS
char	 *hes_to_bind	__P((char *, char *));
char	**hes_resolve	__P((char *, char *));
int	  hes_error	__P((void));
void	  hes_free	__P((char **));
__END_DECLS

#endif /* ! _HESIOD_H_ */
