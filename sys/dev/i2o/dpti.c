/*	$NetBSD: dpti.c,v 1.1.2.2 2001/09/26 19:54:50 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Copyright (c) 1996-2000 Distributed Processing Technology Corporation
 * Copyright (c) 2000 Adaptec Corporation
 * All rights reserved.
 *
 * TERMS AND CONDITIONS OF USE
 *
 * Redistribution and use in source form, with or without modification, are
 * permitted provided that redistributions of source code must retain the
 * above copyright notice, this list of conditions and the following disclaimer.
 *
 * This software is provided `as is' by Adaptec and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose, are disclaimed. In no
 * event shall Adaptec be liable for any direct, indirect, incidental, special,
 * exemplary or consequential damages (including, but not limited to,
 * procurement of substitute goods or services; loss of use, data, or profits;
 * or business interruptions) however caused and on any theory of liability,
 * whether in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this driver software, even
 * if advised of the possibility of such damage.
 */

/*
 * Adaptec/DPT I2O control interface.
 */

#include "opt_i2o.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/endian.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/ioctl.h>

#include <machine/bus.h>

#include <dev/i2o/i2o.h>
#include <dev/i2o/i2odpt.h>
#include <dev/i2o/iopio.h>
#include <dev/i2o/iopvar.h>
#include <dev/i2o/dptivar.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_ioctl.h>

#ifdef I2ODEBUG
#define	DPRINTF(x)		printf x
#else
#define	DPRINTF(x)
#endif

static struct dpt_sig dpti_sig = {
	{ 'd', 'P', 't', 'S', 'i', 'G'}, SIG_VERSION, PROC_INTEL,
	PROC_386 | PROC_486 | PROC_PENTIUM | PROC_SEXIUM, FT_HBADRVR, 0,
	OEM_DPT, OS_FREE_BSD, CAP_ABOVE16MB, DEV_ALL,
	ADF_ALL_SC5,
	0, 0, DPTI_VERSION, DPTI_REVISION, DPTI_SUBREVISION,
	DPTI_MONTH, DPTI_DAY, DPTI_YEAR,
	""
};

void	dpti_attach(struct device *, struct device *, void *);
int	dpti_blinkled(struct dpti_softc *);
int	dpti_ctlrinfo(struct dpti_softc *, u_long, caddr_t);
int	dpti_match(struct device *, struct cfdata *, void *);
int	dpti_passthrough(struct dpti_softc *, caddr_t, struct proc *);
int	dpti_sysinfo(struct dpti_softc *, u_long, caddr_t);

cdev_decl(dpti);

extern struct cfdriver dpti_cd;

struct cfattach dpti_ca = {
	sizeof(struct dpti_softc), dpti_match, dpti_attach
};

int
dpti_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct iop_attach_args *ia;
	struct iop_softc *iop;

	ia = aux;
	iop = (struct iop_softc *)parent;

	if (ia->ia_class != I2O_CLASS_ANY || ia->ia_tid != I2O_TID_IOP)
		return (0);

	if (le16toh(iop->sc_status.orgid) != I2O_ORG_DPT)
		return (0);

	return (1);
}

void
dpti_attach(struct device *parent, struct device *self, void *aux)
{
	struct iop_softc *iop;
	struct dpti_softc *sc;
	struct {
		struct	i2o_param_op_results pr;
		struct	i2o_param_read_results prr;
		struct	i2o_dpt_param_exec_iop_buffers dib;
	} __attribute__ ((__packed__)) param;
	int rv;

	sc = (struct dpti_softc *)self;
	iop = (struct iop_softc *)parent;

	/*
	 * Tell the world what we are.  The description in the signature
	 * must be no more than 46 bytes long (see dptivar.h).
	 */
	printf(": DPT/Adaptec SCSI RAID management interface\n");
	sprintf(dpti_sig.dsDescription, "NetBSD %s I2O OSM", osrelease);

	rv = iop_field_get_all(iop, I2O_TID_IOP,
	    I2O_DPT_PARAM_EXEC_IOP_BUFFERS, &param,
	    sizeof(param), NULL);
	if (rv != 0)
		return;

	sc->sc_blinkled = le32toh(param.dib.serialoutputoff) + 8;
}

int
dptiopen(dev_t dev, int flag, int mode, struct proc *p)
{

	if (securelevel > 1)
		return (EPERM);
	if (device_lookup(&dpti_cd, minor(dev)) == NULL)
		return (ENXIO);

	return (0);
}

int
dpticlose(dev_t dev, int flag, int mode, struct proc *p)
{

	return (0);
}

int
dptiioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct iop_softc *iop;
	struct dpti_softc *sc;
	struct ioctl_pt *pt;
	int i, size;

	sc = device_lookup(&dpti_cd, minor(dev));
	iop = (struct iop_softc *)sc->sc_dv.dv_parent;

	/*
	 * Currently, we only take ioctls passed down from the Linux
	 * emulation layer.
	 */
	if (cmd == PTIOCLINUX) {
		pt = (struct ioctl_pt *)data;
		cmd = pt->com;
		data = pt->data;
	} else
		return (ENOTTY);

	size = IOCPARM_LEN(cmd);

	switch (cmd & 0xffff) {
	case DPT_SIGNATURE:
		if (size > sizeof(dpti_sig))
			size = sizeof(dpti_sig);
		memcpy(data, &dpti_sig, size);
		return (0);

	case DPT_CTRLINFO:
		return (dpti_ctlrinfo(sc, cmd, data));

	case DPT_SYSINFO:
		return (dpti_sysinfo(sc, cmd, data));

	case DPT_BLINKLED:
		if ((i = dpti_blinkled(sc)) == -1)
			i = 0;

		if (size == 0)
			return (copyout(&i, *(caddr_t *)data, sizeof(i)));

		*(int *)data = i;
		return (0);

	case DPT_TARGET_BUSY:
		/*
		 * XXX This is here to stop linux_machdepioctl() from
		 * whining about an unknown ioctl.  Really, it should be
		 * implemented.
		 */
		return (EIO);

	case DPT_I2OUSRCMD:
		return (dpti_passthrough(sc, data, p));

	case DPT_I2ORESETCMD:
		printf("%s: I2ORESETCMD not implemented\n",
		    sc->sc_dv.dv_xname);
		return (ENOTTY);

	case DPT_I2ORESCANCMD:
		return (iop_reconfigure(iop, 0));

	default:
		return (ENOTTY);
	}
}

int
dpti_blinkled(struct dpti_softc *sc)
{
	struct iop_softc *iop;
	u_int v;

	iop = (struct iop_softc *)sc->sc_dv.dv_parent;

	v = bus_space_read_1(iop->sc_iot, iop->sc_ioh, sc->sc_blinkled + 0);
	if (v == 0xbc) {
		v = bus_space_read_1(iop->sc_iot, iop->sc_ioh,
		    sc->sc_blinkled + 1);
		return (v);
	}

	return (-1);
}

int
dpti_ctlrinfo(struct dpti_softc *sc, u_long cmd, caddr_t data)
{
	struct dpt_ctlrinfo CtlrInfo;
	struct iop_softc *iop;
	int rv, i;

	iop = (struct iop_softc *)sc->sc_dv.dv_parent;

	memset(&CtlrInfo, 0, sizeof(CtlrInfo));

	CtlrInfo.length = sizeof(CtlrInfo) - sizeof(u_int16_t);
	CtlrInfo.drvrHBAnum = sc->sc_dv.dv_unit;
	CtlrInfo.baseAddr = iop->sc_memaddr;
	if ((i = dpti_blinkled(sc)) == -1)
		i = 0;
	CtlrInfo.blinkState = i;
	CtlrInfo.pciBusNum = iop->sc_pcibus;
	CtlrInfo.pciDeviceNum = iop->sc_pcidev;
	CtlrInfo.hbaFlags = FLG_OSD_PCI_VALID | FLG_OSD_DMA | FLG_OSD_I2O;
	CtlrInfo.Interrupt = 10;			/* XXX */

	if (IOCPARM_LEN(cmd) > sizeof(*data)) {
		bcopy(&CtlrInfo, data, sizeof(CtlrInfo));
		rv = 0;
	} else
		rv = copyout(&CtlrInfo, *(caddr_t *)data,
		    sizeof(CtlrInfo));

	return (rv);
}

int
dpti_sysinfo(struct dpti_softc *sc, u_long cmd, caddr_t data)
{

	/*
	 * XXX Not currently implemented, since it involves reading from the
	 * IBM PC CMOS and grovelling around in ISA expansion card space,
	 * which is kind of disgusting.  The DPT utilities don't seem to
	 * mind.  :-)
	 */
	return (EIO);
}

int
dpti_passthrough(struct dpti_softc *sc, caddr_t data, struct proc *proc)
{
	struct iop_softc *iop;
	struct i2o_msg mh, *mf;
	struct i2o_reply rh;
	struct iop_msg *im;
	struct dpti_ptbuf bufs[IOP_MAX_MSG_XFERS];
	u_int32_t mbtmp[IOP_MAX_MSG_SIZE / sizeof(u_int32_t)];
	u_int32_t rbtmp[IOP_MAX_MSG_SIZE / sizeof(u_int32_t)];
	int rv, msgsize, repsize, sgoff, i, mapped, nbuf;
	u_int32_t *p, *pmax;

	iop = (struct iop_softc *)sc->sc_dv.dv_parent;

	if ((rv = dpti_blinkled(sc)) != -1) {
		if (rv != 0) {
			printf("%s: adapter blinkled = 0x%02x\n",
			    sc->sc_dv.dv_xname, rv);
			return (EIO);
		}
	}

	/*
	 * Copy in the message frame header and determine the size of the
	 * full message frame.
	 */
	if ((rv = copyin(data, &mh, sizeof(mh))) != 0) {
		DPRINTF(("%s: message copyin failed\n",
		    sc->sc_dv.dv_xname));
		return (rv);
	}

	msgsize = (mh.msgflags >> 14) & ~3;
	if (msgsize < sizeof(mh) || msgsize >= IOP_MAX_MSG_SIZE) {
		DPRINTF(("%s: bad message frame size\n",
		    sc->sc_dv.dv_xname));
		return (EINVAL);
	}

	/*
	 * Handle special commands.
	 *
	 * XXX We'll need to handle all of these later for online firmware
	 * upgrades.  Swish or what, eh?
	 */
	switch (mh.msgfunc >> 24) {
	case I2O_EXEC_IOP_RESET:
		printf("dpti%s: I2O_EXEC_IOP_RESET not implemented\n",
		    sc->sc_dv.dv_xname);
		return (EIO);

	case I2O_EXEC_OUTBOUND_INIT:
		printf("dpti%s: I2O_EXEC_OUTBOUND_INIT not implemented\n",
		    sc->sc_dv.dv_xname);
		return (EIO);

	case I2O_EXEC_SYS_TAB_SET:
		printf("dpti%s: I2O_EXEC_SYS_TAB_SET not implemented\n",
		    sc->sc_dv.dv_xname);
		return (EIO);

	case I2O_EXEC_STATUS_GET:
		iop_status_get(iop, 0);
		rv = copyout(&iop->sc_status, data + msgsize,
		    sizeof(iop->sc_status));
		return (rv);
	}

	/*
	 * Copy in the full message frame.
	 */
	if ((rv = copyin(data, mbtmp, msgsize)) != 0) {
		DPRINTF(("%s: full message copyin failed\n",
		    sc->sc_dv.dv_xname));
		return (rv);
	}

	/*
	 * Determine the size of the reply frame, and copy it in.
	 */
	if ((rv = copyin(data + msgsize, &rh, sizeof(rh))) != 0) {
		DPRINTF(("%s: reply copyin failed\n",
		    sc->sc_dv.dv_xname));
		return (rv);
	}

	repsize = (rh.msgflags >> 14) & ~3;
	if (repsize < sizeof(rh) || repsize >= IOP_MAX_MSG_SIZE) {
		DPRINTF(("%s: bad reply header size\n",
		    sc->sc_dv.dv_xname));
		return (EINVAL);
	}

	if ((rv = copyin(data + msgsize, rbtmp, repsize)) != 0) {
		DPRINTF(("%s: reply too large\n", sc->sc_dv.dv_xname));
		return (rv);
	}

	/*
	 * If the message has a scatter gather list, it must be comprised of
	 * simple elements.  If any one transfer contains multiple segments,
	 * we allocate a temporary buffer for it; otherwise, the buffer will
	 * be mapped directly.  XXX Lies!  We don't do scatter-gather yet.
	 */
	if ((sgoff = ((mh.msgflags >> 4) & 15)) != 0) {
		if ((sgoff + 2) > (msgsize >> 2)) {
			DPRINTF(("%s: invalid message size fields\n",
			    sc->sc_dv.dv_xname));
			return (EINVAL);
		}
	} else
		nbuf = -1;

	if (sgoff != 0) {
		p = mbtmp + sgoff;
		pmax = mbtmp + (msgsize >> 2);

		for (nbuf = 0; nbuf < IOP_MAX_MSG_XFERS; nbuf++) {
			if (p > pmax - 2) {
				DPRINTF(("%s: invalid SGL (1)\n",
				    sc->sc_dv.dv_xname));
				return (EINVAL);
			}

			if ((p[0] & 0x30000000) != I2O_SGL_SIMPLE) {
				DPRINTF(("%s: invalid SGL (2)\n",
				    sc->sc_dv.dv_xname));
				return (EINVAL);
			}

			bufs[nbuf].db_out = (p[0] & I2O_SGL_DATA_OUT) != 0;

			if ((p[0] & I2O_SGL_END_BUFFER) != 0) {
				if ((p[0] & 0x00ffffff) > IOP_MAX_XFER) {
					DPRINTF(("%s: buffer too large\n",
					    sc->sc_dv.dv_xname));
					return (EINVAL);
				}

				bufs[nbuf].db_ptr = (caddr_t)p[1];
				bufs[nbuf].db_proc = proc;
				bufs[nbuf].db_size = p[0] & 0x00ffffff;

				if ((p[0] & I2O_SGL_END) != 0)
					break;

				p += 2;
				continue;
			}

			printf("%s: scatter-gather not implemented\n",
			    sc->sc_dv.dv_xname);
			return (EIO);
		}

		if (nbuf == IOP_MAX_MSG_XFERS) {
			DPRINTF(("%s: too many transfers\n",
			    sc->sc_dv.dv_xname));
			return (EINVAL);
		}
	}

	/*
	 * Allocate a wrapper, and adjust the message header fields to
	 * indicate that no scatter-gather list is currently present.
	 */
	im = iop_msg_alloc(iop, IM_WAIT | IM_NOSTATUS);
	im->im_rb = (struct i2o_reply *)rbtmp;
	mf = (struct i2o_msg *)mbtmp;
	mf->msgictx = IOP_ICTX;
	mf->msgtctx = im->im_tctx;
	mapped = 0;

	if (sgoff != 0)
		mf->msgflags = (mf->msgflags & 0xff0f) | (sgoff << 16);

	/*
	 * Map the data transfer(s).
	 */
	for (i = 0; i <= nbuf; i++) {
		rv = iop_msg_map(iop, im, mbtmp, bufs[i].db_ptr,
		    bufs[i].db_size, bufs[i].db_out, bufs[i].db_proc);
		if (rv != 0) {
			DPRINTF(("%s: msg_map failed\n",
			    sc->sc_dv.dv_xname));
			goto bad;
		}
		mapped = 1;
	}

	/*
	 * Start the command and sleep until it completes.
	 */
	if (sc->sc_nactive++ >= 2)
		tsleep(&sc->sc_nactive, PRIBIO, "dptislp", 0);

	if ((rv = iop_msg_post(iop, im, mbtmp, 5*60*1000)) != 0)
		goto bad;

	/*
	 * Copy out the reply frame.
	 */
	if ((rv = copyout(rbtmp, data + msgsize, repsize)) != 0)
		DPRINTF(("%s: reply copyout() failed\n",
		    sc->sc_dv.dv_xname));

 bad:
	sc->sc_nactive--;
	wakeup_one(&sc->sc_nactive);

	if (mapped)
		iop_msg_unmap(iop, im);

	iop_msg_free(iop, im);
	return (rv);
}
