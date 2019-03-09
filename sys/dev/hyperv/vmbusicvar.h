/*	$NetBSD: vmbusicvar.h,v 1.1.2.2 2019/03/09 17:10:19 martin Exp $	*/
/*	$OpenBSD: hypervic.c,v 1.13 2017/08/10 15:25:57 mikeb Exp $	*/

/*-
 * Copyright (c) 2009-2016 Microsoft Corp.
 * Copyright (c) 2012 NetApp Inc.
 * Copyright (c) 2012 Citrix Inc.
 * Copyright (c) 2016 Mike Belopuhov <mike@esdenera.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
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

/*
 * The OpenBSD port was done under funding by Esdenera Networks GmbH.
 */

#ifndef	_VMBUSICVAR_H_
#define	_VMBUSICVAR_H_

#include <sys/sysctl.h>

#include <dev/hyperv/vmbusvar.h>

struct vmbusic_softc {
	device_t		sc_dev;

	struct vmbus_channel	*sc_chan;

	void			*sc_buf;
	size_t			sc_buflen;

	uint32_t		sc_fwver;	/* framework version */
	uint32_t		sc_msgver;	/* message version */

	struct sysctllog	*sc_log;
};

int	vmbusic_probe(struct vmbus_attach_args *, const struct hyperv_guid *);
int	vmbusic_attach(device_t, struct vmbus_attach_args *,
	    vmbus_channel_callback_t);
int	vmbusic_detach(device_t, int);

int	vmbusic_negotiate(struct vmbusic_softc *, void *, uint32_t *, uint32_t,
	    uint32_t);
int	vmbusic_sendresp(struct vmbusic_softc *, struct vmbus_channel *,
	    void *, uint32_t, uint64_t);

#endif	/* _VMBUSICVAR_H_ */
