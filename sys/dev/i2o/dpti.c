/*	$NetBSD: dpti.c,v 1.1.2.5 2002/06/20 03:44:24 nathanw Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dpti.c,v 1.1.2.5 2002/06/20 03:44:24 nathanw Exp $");

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

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#ifdef i386
#include <machine/pio.h>
#endif

#include <dev/i2o/i2o.h>
#include <dev/i2o/i2odpt.h>
#include <dev/i2o/iopio.h>
#include <dev/i2o/iopvar.h>
#include <dev/i2o/dptivar.h>

#ifdef I2ODEBUG
#define	DPRINTF(x)		printf x
#else
#define	DPRINTF(x)
#endif

static struct dpt_sig dpti_sig = {
	{ 'd', 'P', 't', 'S', 'i', 'G'},
	SIG_VERSION,
#if defined(i386)
	PROC_INTEL,
#elif defined(powerpc)
	PROC_POWERPC,
#elif defined(alpha)
	PROC_ALPHA,
#elif defined(__mips__)
	PROC_MIPS,
#elif defined(sparc64)
	PROC_ULTRASPARC,
#endif
#if defined(i386)
	PROC_386 | PROC_486 | PROC_PENTIUM | PROC_SEXIUM,
#else
	0,
#endif
	FT_HBADRVR,
	0,
	OEM_DPT,
	OS_FREE_BSD,	/* XXX */
	CAP_ABOVE16MB,
	DEV_ALL,
	ADF_ALL_SC5,
	0,
	0,
	DPTI_VERSION,
	DPTI_REVISION,
	DPTI_SUBREVISION,
	DPTI_MONTH,
	DPTI_DAY,
	DPTI_YEAR,
	""		/* Will be filled later */
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
	printf(": DPT/Adaptec RAID management interface\n");
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
	int i, size, rv;

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
		if (sc->sc_nactive++ >= 2)
			tsleep(&sc->sc_nactive, PRIBIO, "dptislp", 0);

		rv = dpti_passthrough(sc, data, p);

		sc->sc_nactive--;
		wakeup_one(&sc->sc_nactive);
		return (rv);

	case DPT_I2ORESETCMD:
		printf("%s: I2ORESETCMD not implemented\n",
		    sc->sc_dv.dv_xname);
		return (EOPNOTSUPP);

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
	struct dpt_ctlrinfo info;
	struct iop_softc *iop;
	int rv, i;

	iop = (struct iop_softc *)sc->sc_dv.dv_parent;

	memset(&info, 0, sizeof(info));

	info.length = sizeof(info) - sizeof(u_int16_t);
	info.drvrHBAnum = sc->sc_dv.dv_unit;
	info.baseAddr = iop->sc_memaddr;
	if ((i = dpti_blinkled(sc)) == -1)
		i = 0;
	info.blinkState = i;
	info.pciBusNum = iop->sc_pcibus;
	info.pciDeviceNum = iop->sc_pcidev;
	info.hbaFlags = FLG_OSD_PCI_VALID | FLG_OSD_DMA | FLG_OSD_I2O;
	info.Interrupt = 10;			/* XXX */

	if (IOCPARM_LEN(cmd) > sizeof(*data)) {
		memcpy(data, &info, min(sizeof(info), IOCPARM_LEN(cmd)));
		rv = 0;
	} else
		rv = copyout(&info, *(caddr_t *)data, sizeof(info));

	return (rv);
}

int
dpti_sysinfo(struct dpti_softc *sc, u_long cmd, caddr_t data)
{
	struct dpt_sysinfo info;
	int rv;
#ifdef i386
	int i, j;
#endif

	memset(&info, 0, sizeof(info));

#ifdef i386
	outb (0x70, 0x12);
	i = inb(0x71);
	j = i >> 4;
	if (i == 0x0f) {
		outb (0x70, 0x19);
		j = inb (0x71);
	}
	info.drive0CMOS = j;

	j = i & 0x0f;
	if (i == 0x0f) {
		outb (0x70, 0x1a);
		j = inb (0x71);
	}
	info.drive1CMOS = j;
	info.processorFamily = dpti_sig.dsProcessorFamily;

	/*
	 * Get the conventional memory size from CMOS.
	 */
	outb(0x70, 0x16);
	j = inb(0x71);
	j <<= 8;
	outb(0x70, 0x15);
	j |= inb(0x71);
	info.conventionalMemSize = j;

	/*
	 * Get the extended memory size from CMOS.
	 */
	outb(0x70, 0x31);
	j = inb(0x71);
	j <<= 8;
	outb(0x70, 0x30);
	j |= inb(0x71);
	info.extendedMemSize = j;

	switch (cpu_class) {
	case CPUCLASS_386:
		info.processorType = PROC_386;
		break;
	case CPUCLASS_486:
		info.processorType = PROC_486;
		break;
	case CPUCLASS_586:
		info.processorType = PROC_PENTIUM;
		break;
	case CPUCLASS_686:
	default:
		info.processorType = PROC_SEXIUM;
		break;
	}

	info.flags = SI_CMOS_Valid | SI_BusTypeValid |
	    SI_MemorySizeValid | SI_NO_SmartROM;
#else
	info.flags = SI_BusTypeValid | SI_NO_SmartROM;
#endif

	info.busType = SI_PCI_BUS;

	/*
	 * Copy out the info structure to the user.
	 */
	if (IOCPARM_LEN(cmd) > sizeof(*data)) {
		memcpy(data, &info, min(sizeof(info), IOCPARM_LEN(cmd)));
		rv = 0;
	} else
		rv = copyout(&info, *(caddr_t *)data, sizeof(info));

	return (rv);
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
	int rv, msgsize, repsize, sgoff, i, mapped, nbuf, nfrag, j, sz;
	u_int32_t *p, *pmax, *pstart;

	iop = (struct iop_softc *)sc->sc_dv.dv_parent;
	im = NULL;

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
	 */
	switch (mh.msgfunc >> 24) {
	case I2O_EXEC_IOP_RESET:
		printf("%s: I2O_EXEC_IOP_RESET not implemented\n",
		    sc->sc_dv.dv_xname);
		return (EOPNOTSUPP);

	case I2O_EXEC_OUTBOUND_INIT:
		printf("%s: I2O_EXEC_OUTBOUND_INIT not implemented\n",
		    sc->sc_dv.dv_xname);
		return (EOPNOTSUPP);

	case I2O_EXEC_SYS_TAB_SET:
		printf("%s: I2O_EXEC_SYS_TAB_SET not implemented\n",
		    sc->sc_dv.dv_xname);
		return (EOPNOTSUPP);

	case I2O_EXEC_STATUS_GET:
		if ((rv = iop_status_get(iop, 0)) == 0)
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
	 * be mapped directly.
	 */
	if ((sgoff = ((mh.msgflags >> 4) & 15)) != 0) {
		if ((sgoff + 2) > (msgsize >> 2)) {
			DPRINTF(("%s: invalid message size fields\n",
			    sc->sc_dv.dv_xname));
			return (EINVAL);
		}

		memset(bufs, 0, sizeof(bufs));
	} else
		nbuf = -1;

	rv = EINVAL;

	if (sgoff != 0) {
		p = mbtmp + sgoff;
		pmax = mbtmp + (msgsize >> 2) - 2;

		for (nbuf = 0; nbuf < IOP_MAX_MSG_XFERS; nbuf++, p += 2) {
			if (p > pmax) {
				DPRINTF(("%s: invalid SGL (1)\n",
				    sc->sc_dv.dv_xname));
				goto bad;
			}

			if ((p[0] & 0x30000000) != I2O_SGL_SIMPLE) {
				DPRINTF(("%s: invalid SGL (2)\n",
				    sc->sc_dv.dv_xname));
				goto bad;
			}

			bufs[nbuf].db_out = (p[0] & I2O_SGL_DATA_OUT) != 0;
			bufs[nbuf].db_ptr = NULL;

			if ((p[0] & I2O_SGL_END_BUFFER) != 0) {
				if ((p[0] & 0x00ffffff) > IOP_MAX_XFER) {
					DPRINTF(("%s: buffer too large\n",
					    sc->sc_dv.dv_xname));
					goto bad;
				}

				bufs[nbuf].db_ptr = (caddr_t)p[1];
				bufs[nbuf].db_proc = proc;
				bufs[nbuf].db_size = p[0] & 0x00ffffff;

				if ((p[0] & I2O_SGL_END) != 0)
					break;

				continue;
			}

			/*
			 * The buffer has multiple segments.  Determine the
			 * total size.
			 */
			nfrag = 0;
			sz = 0;
			for (pstart = p; p <= pmax; p += 2) {
				if (nfrag == DPTI_MAX_SEGS) {
					DPRINTF(("%s: too many segments\n",
					    sc->sc_dv.dv_xname));
					goto bad;
				}

				bufs[nbuf].db_frags[nfrag].iov_len =
				    p[0] & 0x00ffffff;
				bufs[nbuf].db_frags[nfrag].iov_base = 
				    (void *)p[1];

				sz += p[0] & 0x00ffffff;
				nfrag++;

				if ((p[0] & I2O_SGL_END) != 0) {
					if ((p[0] & I2O_SGL_END_BUFFER) == 0) {
						DPRINTF((
						    "%s: invalid SGL (3)\n",
						    sc->sc_dv.dv_xname));
						goto bad;
					}
					break;
				}
				if ((p[0] & I2O_SGL_END_BUFFER) != 0)
					break;
			}
			bufs[nbuf].db_nfrag = nfrag;

			if (p > pmax) {
				DPRINTF(("%s: invalid SGL (4)\n",
				    sc->sc_dv.dv_xname));
				goto bad;
			}

			if (sz > IOP_MAX_XFER) {
				DPRINTF(("%s: buffer too large\n",
				    sc->sc_dv.dv_xname));
				goto bad;
			}

			bufs[nbuf].db_size = sz;
			bufs[nbuf].db_ptr = malloc(sz, M_DEVBUF, M_WAITOK);
			if (bufs[nbuf].db_ptr == NULL) {
				DPRINTF(("%s: allocation failure\n",
				    sc->sc_dv.dv_xname));
				rv = ENOMEM;
				goto bad;
			}

			for (i = 0, sz = 0; i < bufs[nbuf].db_nfrag; i++) {
				rv = copyin(bufs[nbuf].db_frags[i].iov_base,
				    bufs[nbuf].db_ptr + sz,
				    bufs[nbuf].db_frags[i].iov_len);
				if (rv != 0) {
					DPRINTF(("%s: frag copyin\n",
					    sc->sc_dv.dv_xname));
					goto bad;
				}
				sz += bufs[nbuf].db_frags[i].iov_len;
			}

			if ((p[0] & I2O_SGL_END) != 0)
				break;
		}

		if (nbuf == IOP_MAX_MSG_XFERS) {
			DPRINTF(("%s: too many transfers\n",
			    sc->sc_dv.dv_xname));
			goto bad;
		}
	}

	/*
	 * Allocate a wrapper, and adjust the message header fields to
	 * indicate that no scatter-gather list is currently present.
	 */
	mapped = 0;

	im = iop_msg_alloc(iop, IM_WAIT | IM_NOSTATUS);
	im->im_rb = (struct i2o_reply *)rbtmp;
	mf = (struct i2o_msg *)mbtmp;
	mf->msgictx = IOP_ICTX;
	mf->msgtctx = im->im_tctx;

	if (sgoff != 0)
		mf->msgflags = (mf->msgflags & 0xff0f) | (sgoff << 16);

	/*
	 * Map the data transfer(s).
	 */
	for (i = 0; i <= nbuf; i++) {
		rv = iop_msg_map(iop, im, mbtmp, bufs[i].db_ptr,
		    bufs[i].db_size, bufs[i].db_out, bufs[i].db_proc);
		if (rv != 0) {
			DPRINTF(("%s: msg_map failed, rv = %d\n",
			    sc->sc_dv.dv_xname, rv));
			goto bad;
		}
		mapped = 1;
	}

	/*
	 * Start the command and sleep until it completes.
	 */
	if ((rv = iop_msg_post(iop, im, mbtmp, 5*60*1000)) != 0)
		goto bad;

	/*
	 * Copy out the reply frame.
	 */
	if ((rv = copyout(rbtmp, data + msgsize, repsize)) != 0)
		DPRINTF(("%s: reply copyout() failed\n",
		    sc->sc_dv.dv_xname));

 bad:
	/*
	 * Free resources and return to the caller.
	 */
	if (im != NULL) {
		if (mapped)
			iop_msg_unmap(iop, im);
		iop_msg_free(iop, im);
	}

	for (i = 0; i <= nbuf; i++) {
		if (bufs[i].db_proc != NULL)
			continue;

		if (!bufs[i].db_out && rv == 0) {
			for (j = 0, sz = 0; j < bufs[i].db_nfrag; j++) {
				rv = copyout(bufs[i].db_ptr + sz,
				    bufs[i].db_frags[j].iov_base,
				    bufs[i].db_frags[j].iov_len);
				if (rv != 0)
					break;
				sz += bufs[i].db_frags[j].iov_len;
			}
		}

		if (bufs[i].db_ptr != NULL)
			free(bufs[i].db_ptr, M_DEVBUF);
	}

	return (rv);
}
