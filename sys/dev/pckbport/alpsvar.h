/* $NetBSD: alpsvar.h,v 1.1.4.2 2017/12/03 11:37:30 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Ryo ONODERA <ryo@tetera.org>
 * Copyright (c) 2008 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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

#ifndef _DEV_PCKBCPORT_ALPSVAR_H
#define _DEV_PCKBCPORT_ALPSVAR_H

#include <dev/pckbport/pmsreg.h>

struct alps_softc {
	uint32_t	version;

	bool		initializing;
	uint32_t	res_x, res_y;
	uint16_t	last_x1, last_y1, last_z1;
	uint16_t	last_x2, last_y2;
	int		nfingers;
	int		last_nfingers;
};

int	pms_alps_probe_init(void *);
void	pms_alps_enable(void *);
void	pms_alps_resume(void *);

#endif /* !_DEV_PCKBCPORT_ALPSVAR_H */
