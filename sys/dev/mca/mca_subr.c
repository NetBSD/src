/*	$NetBSD: mca_subr.c,v 1.1 2000/05/11 15:42:05 jdolecek Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * Copyright (c) 1996-1999 Scott D. Telford.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Scott Telford <s.telford@ed.ac.uk>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * MCA Bus subroutines
 */

#include "opt_mcaverbose.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/mca/mcavar.h>
#include <dev/mca/mcadevs.h>

#ifdef MCAVERBOSE
/*
 * Descriptions of of known MCA devices
 */
struct mca_knowndev {
	int		 id;		/* MCA ID */
	const char	*name;		/* text description */
};

#include <dev/mca/mcadevs_data.h>

#endif /* MCAVERBOSE */

/* supported peripheals */
const int mca_suppdevs[] = {
	MCA_PRODUCT_AHA1640,
	MCA_PRODUCT_3C523,
	MCA_PRODUCT_ITR,
	0
};

/*
 * Returns 1 if a device is supported (i.e. device driver exists for it)
 * 0 otherwise.
 */
int
mca_issupp(id)
	int id;
{
	int i;
	for(i=0; mca_suppdevs[i]; i++) {
		if (id == mca_suppdevs[i])
			return 1;
	}

	return 0;
}

void
mca_devinfo(id, cp)
	int id;
	char *cp;
{
#ifdef MCAVERBOSE
	const struct mca_knowndev *kdp;
	
	kdp = mca_knowndevs;
        for (; kdp->name != NULL && kdp->id != id; kdp++);

	if (kdp->name != NULL)
		sprintf(cp, "%s (0x%04x)", kdp->name, id);
	else
#endif /* MCAVERBOSE */
		sprintf(cp, "product 0x%04x", id);
}
