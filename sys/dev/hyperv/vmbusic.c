/*	$NetBSD: vmbusic.c,v 1.1.2.2 2019/03/09 17:10:19 martin Exp $	*/
/*-
 * Copyright (c) 2014,2016 Microsoft Corp.
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

#include <sys/cdefs.h>
#ifdef __KERNEL_RCSID
__KERNEL_RCSID(0, "$NetBSD: vmbusic.c,v 1.1.2.2 2019/03/09 17:10:19 martin Exp $");
#endif
#ifdef __FBSDID
__FBSDID("$FreeBSD: head/sys/dev/hyperv/utilities/vmbus_ic.c 310317 2016-12-20 07:14:24Z sephe $");
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/reboot.h>

#include <dev/hyperv/vmbusvar.h>
#include <dev/hyperv/vmbusicreg.h>
#include <dev/hyperv/vmbusicvar.h>

#define VMBUS_IC_BRSIZE		(4 * PAGE_SIZE)

#define VMBUS_IC_VERCNT         2
#define VMBUS_IC_NEGOSZ		\
    offsetof(struct vmbus_icmsg_negotiate, ic_ver[VMBUS_IC_VERCNT])
__CTASSERT(VMBUS_IC_NEGOSZ < VMBUS_IC_BRSIZE);


int
vmbusic_probe(struct vmbus_attach_args *aa, const struct hyperv_guid *guid)
{

	if (memcmp(aa->aa_type, guid, sizeof(*aa->aa_type)) != 0)
		return 0;
	return 1;
}

int
vmbusic_attach(device_t dv, struct vmbus_attach_args *aa,
    vmbus_channel_callback_t cb)
{
	struct vmbusic_softc *sc = device_private(dv);

	sc->sc_dev = dv;
	sc->sc_chan = aa->aa_chan;

	sc->sc_buflen = VMBUS_IC_BRSIZE;
	sc->sc_buf = kmem_alloc(sc->sc_buflen, cold ? KM_NOSLEEP : KM_SLEEP);

	/*
	 * These services are not performance critical and do not need
	 * batched reading. Furthermore, some services such as KVP can
	 * only handle one message from the host at a time.
	 * Turn off batched reading for all util drivers before we open the
	 * channel.
	 */
	sc->sc_chan->ch_flags &= ~CHF_BATCHED;

	if (vmbus_channel_open(sc->sc_chan, sc->sc_buflen, NULL, 0, cb, sc)) {
		aprint_error_dev(dv, "failed to open channel\n");
		kmem_free(sc->sc_buf, sc->sc_buflen);
		sc->sc_buf = NULL;
		return ENXIO;
	}

	return 0;
}

int
vmbusic_detach(device_t dv, int flags)
{
	struct vmbusic_softc *sc = device_private(dv);
	int error;

	error = vmbus_channel_close(sc->sc_chan);
	if (error != 0)
		return error;

	if (sc->sc_buf != NULL) {
		kmem_free(sc->sc_buf, sc->sc_buflen);
		sc->sc_buf = NULL;
	}

	if (sc->sc_log != NULL) {
		sysctl_teardown(&sc->sc_log);
		sc->sc_log = NULL;
	}

	return 0;
}

int
vmbusic_negotiate(struct vmbusic_softc *sc, void *data, uint32_t *dlen0,
    uint32_t fw_ver, uint32_t msg_ver)
{
	struct vmbus_icmsg_negotiate *nego;
	uint32_t sel_fw_ver = 0, sel_msg_ver = 0;
	int i, cnt, dlen = *dlen0, error;
	bool has_fw_ver, has_msg_ver = false;

	/*
	 * Preliminary message verification.
	 */
	if (dlen < sizeof(*nego)) {
		device_printf(sc->sc_dev, "truncated ic negotiate, len %d\n",
		    dlen);
		return EINVAL;
	}
	nego = data;

	if (nego->ic_fwver_cnt == 0) {
		device_printf(sc->sc_dev, "ic negotiate does not contain "
		    "framework version %u\n", nego->ic_fwver_cnt);
		return EINVAL;
	}
	if (nego->ic_msgver_cnt == 0) {
		device_printf(sc->sc_dev, "ic negotiate does not contain "
		    "message version %u\n", nego->ic_msgver_cnt);
		return EINVAL;
	}

	cnt = nego->ic_fwver_cnt + nego->ic_msgver_cnt;
	if (dlen < offsetof(struct vmbus_icmsg_negotiate, ic_ver[cnt])) {
		device_printf(sc->sc_dev, "ic negotiate does not contain "
		    "versions %d\n", dlen);
		return EINVAL;
	}

	error = EOPNOTSUPP;

	/*
	 * Find the best match framework version.
	 */
	has_fw_ver = false;
	for (i = 0; i < nego->ic_fwver_cnt; ++i) {
		if (VMBUS_ICVER_LE(nego->ic_ver[i], fw_ver)) {
			if (!has_fw_ver) {
				sel_fw_ver = nego->ic_ver[i];
				has_fw_ver = true;
			} else if (VMBUS_ICVER_GT(nego->ic_ver[i],
			    sel_fw_ver)) {
				sel_fw_ver = nego->ic_ver[i];
			}
		}
	}
	if (!has_fw_ver) {
		device_printf(sc->sc_dev, "failed to select framework "
		    "version\n");
		goto done;
	}

	/*
	 * Fine the best match message version.
	 */
	has_msg_ver = false;
	for (i = nego->ic_fwver_cnt;
	    i < nego->ic_fwver_cnt + nego->ic_msgver_cnt; ++i) {
		if (VMBUS_ICVER_LE(nego->ic_ver[i], msg_ver)) {
			if (!has_msg_ver) {
				sel_msg_ver = nego->ic_ver[i];
				has_msg_ver = true;
			} else if (VMBUS_ICVER_GT(nego->ic_ver[i],
			    sel_msg_ver)) {
				sel_msg_ver = nego->ic_ver[i];
			}
		}
	}
	if (!has_msg_ver) {
		device_printf(sc->sc_dev, "failed to select message version\n");
		goto done;
	}

	error = 0;
done:
	if (bootverbose || !has_fw_ver || !has_msg_ver) {
		if (has_fw_ver) {
			device_printf(sc->sc_dev,
			    "sel framework version: %u.%u\n",
			    VMBUS_ICVER_MAJOR(sel_fw_ver),
			    VMBUS_ICVER_MINOR(sel_fw_ver));
		}
		for (i = 0; i < nego->ic_fwver_cnt; i++) {
			device_printf(sc->sc_dev,
			    "supp framework version: %u.%u\n",
			    VMBUS_ICVER_MAJOR(nego->ic_ver[i]),
			    VMBUS_ICVER_MINOR(nego->ic_ver[i]));
		}

		if (has_msg_ver) {
			device_printf(sc->sc_dev,
			    "sel message version: %u.%u\n",
			    VMBUS_ICVER_MAJOR(sel_msg_ver),
			    VMBUS_ICVER_MINOR(sel_msg_ver));
		}
		for (i = nego->ic_fwver_cnt;
		    i < nego->ic_fwver_cnt + nego->ic_msgver_cnt; i++) {
			device_printf(sc->sc_dev,
			    "supp message version: %u.%u\n",
			    VMBUS_ICVER_MAJOR(nego->ic_ver[i]),
			    VMBUS_ICVER_MINOR(nego->ic_ver[i]));
		}
	}
	if (error)
		return error;

	/* Record the selected versions. */
	sc->sc_fwver = sel_fw_ver;
	sc->sc_msgver = sel_msg_ver;

	/* One framework version. */
	nego->ic_fwver_cnt = 1;
	nego->ic_ver[0] = sel_fw_ver;

	/* One message version. */
	nego->ic_msgver_cnt = 1;
	nego->ic_ver[1] = sel_msg_ver;

	/* Update data size. */
	nego->ic_hdr.ic_dsize = VMBUS_IC_NEGOSZ -
	    sizeof(struct vmbus_icmsg_hdr);

	/* Update total size, if necessary. */
	if (dlen < VMBUS_IC_NEGOSZ)
		*dlen0 = VMBUS_IC_NEGOSZ;

	return 0;
}

int
vmbusic_sendresp(struct vmbusic_softc *sc, struct vmbus_channel *chan,
    void *data, uint32_t dlen, uint64_t rid)
{
	struct vmbus_icmsg_hdr *hdr;
	int error;

	KASSERTMSG(dlen >= sizeof(*hdr), "invalid data length %d", dlen);
	hdr = data;

	hdr->ic_flags = VMBUS_ICMSG_FLAG_TRANSACTION|VMBUS_ICMSG_FLAG_RESPONSE;
	error = vmbus_channel_send(chan, data, dlen, rid,
	    VMBUS_CHANPKT_TYPE_INBAND, 0);
	if (error != 0)
		device_printf(sc->sc_dev, "resp send failed: %d\n", error);
	return error;
}
