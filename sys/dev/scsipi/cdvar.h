/*	$NetBSD: cdvar.h,v 1.31.6.1 2017/12/03 11:37:32 jdolecek Exp $	*/

/*
 * Copyright (c) 1997 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <dev/dkvar.h>

#define	CDRETRIES	4

struct cd_softc {
	struct dk_softc sc_dksc;

	int flags;
#define	CDF_ANCIENT	0x10		/* disk is ancient; for minphys */
#define	CDF_EJECTED	0x20		/* be silent when flushing cache */

	struct scsipi_periph *sc_periph;

	struct cd_parms {
		u_int blksize;
		u_long disksize;	/* total number sectors */
		u_long disksize512;	/* total number sectors */
	} params;

	struct callout sc_callout;
};

#define CDGP_RESULT_OK          0       /* parameters obtained */
#define CDGP_RESULT_OFFLINE     1       /* no media, or otherwise losing */
#define CDGP_RESULT_UNFORMATTED 2       /* unformatted media (max params) */
