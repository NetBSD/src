/*	$NetBSD: fwiso.c,v 1.3 2003/07/14 15:47:13 lukem Exp $	*/

/*-
 * Copyright (c) 2001 and 2002
 *      HAYAKAWA Koichi.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fwiso.c,v 1.3 2003/07/14 15:47:13 lukem Exp $");

#include "fwiso.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/mbuf.h>
#include <sys/poll.h>
#include <sys/select.h>

#include <dev/ieee1394/fwiso_ioctl.h>
#include <dev/ieee1394/ieee1394reg.h>
#include <dev/ieee1394/ieee1394var.h>
#include <dev/ieee1394/fwisovar.h>

#ifdef DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

#define	UNIT(dev) minor(dev)&0xff
#define	IOCTL(dev) minor(dev)&0x0100
#define	DVDEV(dev) minor(dev)&0x0200
#define	MPEG2DEV(dev) minor(dev)&0x0300

/*
 * device number rule
 *
 *	Lower 8-bit of minor number represents fwiso pseudo device.
 *	Higher bits represents types of the device.
 *
 *	0x0000:     raw fwiso device
 *	0x0100:     for ioctl (can't read/write)
 *	0x0200:     standard DV type
 *	0x0300:     MPEG2 TS
 */


/*
 * The movement of data.
 *
 *	Isochronous data acquired by device is copied in reservoir
 *	buffer by fwisohandler.  fwisohandler is a callback function,
 *	called in the device drover of an IEEE 1394 host device.  The
 *	copied data in reservoir is deriverd to userland by fwisoread.
 *	So, at least data copy will happen twice.
 */


struct fwiso_pktdata {
	u_int8_t *pkt_base;	/* Base address: NEVER change */
	size_t pkt_len;		/* Length of the data in the buffer */
	size_t pkt_buflen;	/* Length of the buffer: NEVER change */
};

struct fwiso_pkts {
	struct fwiso_pktdata *fp_iov;	/* packet data vector: NEVER change */
	struct fwiso_pktdata *fp_iov_last; /* end packet data vector: NEVER change  */
	/*
	 * Set members below volatile because they will be changed by
	 * both intr handler and read routine.
	 */
	volatile struct fwiso_pktdata *fp_iov_start;	/* pkt data vector */
	struct fwiso_pktdata *fp_iov_end;		/* pkt data vector */
	volatile int fp_iovcnt;				/* vector length */
};

#define FWISO_DEVNAMESIZ 12
struct fwiso_data {
	char fd_devname[FWISO_DEVNAMESIZ];
	struct ieee1394_softc *fd_dev;

	int fd_status;		/* status: share with interrupt hendler */
	int fd_overflow;

	struct uio *fd_uio;	/* for interrupt handler */

	struct fwiso_pkts fd_rsv_pkts; /* temporary buffer for packets */
	u_int8_t *fd_rsv_buf;
	int fd_rsv_size;

	/*
	 * Should those data (fd_mode, fd_channel, fd_tag) be dedicatd
	 * for read and write, or be shared with read and write?
	 */
	int fd_mode;
	int fd_channel;		/* ISO channel (0 - 63) */
	int fd_tag;		/* IEEE1394_ISO_TAG0, _TAG1, _TAG2 */
	int fd_threshold;

	u_int32_t fd_cycletimer;

	ieee1394_ir_tag_t fd_irtag; /* tag for isochronous reception */

	ieee1394_it_tag_t fd_ittag; /* tag for isochronous transmission */
	struct ieee1394_it_datalist *fd_itlist;
	int fd_it_serial;	/* serial number of data block (not packet) */
	int fd_it_frame;	/* frame number */
	int fd_it_dv_insert_empty;
	int fd_it_dv_insert_fraction;
	int fd_it_dv_insert_fractional;

	int fd_flags;
#define FWISO_OPEN		0x01
#define FWISO_SETHANDLER	0x02
#define FWISO_IR_SLEEPING	0x04
#define FWISO_WRITE		0x08
#define FWISO_NONBLOCK		0x10

	size_t fd_uioprev;	/* previous uio value: XXX really needed? */
};


struct fwiso_data fwiso_data_str[NFWISO];

static int fwiso_min_space[FWISO_MODE_MAX] = {1, 480, 488, 1};
static int fwiso_head_offset[FWISO_MODE_MAX] = {0, 8, 0, 0};
#define FWISO_MAX_INTERFACE 10
	/* I hope all the elements of fwiso_interface is initialised as NULL */
static struct ieee1394_softc *fwiso_interface[FWISO_MAX_INTERFACE] = {NULL};

static struct evcnt fwiso_drop_ev = EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "fwiso", "dropframe");
static struct evcnt fwiso_frame_ev = EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "fwiso", "frame");

void fwisoattach(int);

static struct ieee1394_softc *fwiso_lookup_if(const char *);
int fwisowrite_dv(struct fwiso_data *, struct uio *, int);
static int fwiso_set_handler(struct fwiso_data *);

dev_type_open(fwisoopen);
dev_type_close(fwisoclose);
dev_type_read(fwisoread);
dev_type_write(fwisowrite);
dev_type_ioctl(fwisoioctl);
dev_type_poll(fwisopoll);

const struct cdevsw fwiso_cdevsw = {
	fwisoopen, fwisoclose, fwisoread, fwisowrite, fwisoioctl,
	nostop, notty, fwisopoll, nommap,
};


/* XXX: only for experimental */
extern int fwohci_set_isotx(struct ieee1394_softc *);
struct fwohci_it_ctx;
extern int fwohci_it_ctx_clear(ieee1394_it_tag_t);
extern u_int16_t fwohci_it_cycletimer(ieee1394_it_tag_t);
extern ieee1394_it_tag_t fwohci_it_set(struct ieee1394_softc *, int, int);
extern int fwohci_it_ctx_writedata(ieee1394_it_tag_t, int, 
    struct ieee1394_it_datalist *, int);



/* ARGSUSED */
void
fwisoattach(int n)
{
	int i;

	memset(fwiso_data_str, 0, sizeof(fwiso_data_str));

	for (i = 0; i < NFWISO; ++i) {
		snprintf(fwiso_data_str[i].fd_devname, 10, "fwiso%d", i);
		fwiso_data_str[i].fd_channel = -1;
		fwiso_data_str[i].fd_mode = -1;
		fwiso_data_str[i].fd_threshold = 0;
		fwiso_data_str[i].fd_tag = 0;
		fwiso_data_str[i].fd_uioprev = 0;
	}

	evcnt_attach_static(&fwiso_drop_ev);
	evcnt_attach_static(&fwiso_frame_ev);
}




/*
 * int fwisoread(dev_t dev, struct uio *uio, int ioflag)
 *
 *     Read routine
 *
 *     Algorithm:
 *
 *       if interrupt handler is not set
 *           register interrupt handler
 *           prepare reserve buffer;
 *       if some data exists in reserve buffer
 *           copy data in uio
 *           if uio is full
 *               return
 *       set uio in interrupt handler
 *       sleep until interrupt handler wait me
 *       set reserve buffer
 *       return
 */
int
fwisoread(dev_t dev, struct uio *uio, int ioflag)
{
	struct fwiso_data *fd;
	int unit;
	int status = 0;
	int s;

	int minspace;
	int headoffs;
	volatile struct fwiso_pktdata *pkt;
	struct fwiso_pktdata *pkt_last;
	struct ieee1394_softc *isc;
	size_t resid;

	int i;

	unit = UNIT(dev);

	if (unit >= NFWISO) {
		return ENXIO;
	}

	if (IOCTL(dev)) {
		DPRINTF(("fwiso%d cannot read\n", unit));
		return ENXIO;
	}

	fd = &fwiso_data_str[unit];

	if ((fd->fd_flags & FWISO_OPEN) == 0) {
		return EBUSY;
	}

	isc = fd->fd_dev;

	if ((fd->fd_flags & FWISO_SETHANDLER) == 0) {
		int rv;

		if ((rv = fwiso_set_handler(fd)) != 0) {
			return rv;
		}
	}

	minspace = fwiso_min_space[fd->fd_mode];
	headoffs = fwiso_head_offset[fd->fd_mode];

	/* This is constant */
	pkt_last = fd->fd_rsv_pkts.fp_iov_last;
	/* Only fwiso writes.  Others read only */
	pkt = fd->fd_rsv_pkts.fp_iov_start;

	s = splbio();

	i = 0;
	resid = uio->uio_resid;
	DPRINTF(("%s: now reading\n", isc->sc1394_dev.dv_xname));
	while ((status = (*isc->sc1394_ir_read)((struct device *)isc,
	    fd->fd_irtag, uio, headoffs, 0)) == EAGAIN
	    || (status == 0 && uio->uio_resid == resid)) {
		if (fd->fd_flags & FWISO_NONBLOCK) {
			break;
		}
		if (isc->sc1394_ir_wait == NULL) {
			if (++i > 20) {
				status = EIO;
				break;
			}
			delay(100);
		} else {
			status = (*isc->sc1394_ir_wait)((struct device *)isc,
			    fd->fd_irtag, (void *)fd, fd->fd_devname);
			if (status != 0) {
				break;
			}
		}
	}

	splx(s);

	return status;
}





/*
 * int fwisowrite(dev_t dev, struct uio *uio, int ioflag)
 *
 *	Write routine
 */
int
fwisowrite(dev_t dev, struct uio *uio, int ioflag)
{
	struct fwiso_data *fd;
	int unit;
	int rv = ENODEV;

	unit = UNIT(dev);

	if (unit >= NFWISO) {
		return ENXIO;
	}

	if (IOCTL(dev)) {
		DPRINTF(("fwiso%d cannot read\n", unit));
		return ENXIO;
	}

	fd = &fwiso_data_str[unit];

	if ((fd->fd_flags & FWISO_OPEN) == 0) {
		return EBUSY;
	}

	if (fd->fd_ittag == NULL) {
		/* XXX */
		if (fd->fd_channel < 0) {
			fd->fd_channel = 63;
		}
		if (fd->fd_tag <= 0) {
			fd->fd_tag = IEEE1394_ISO_TAG1;
		}
		fd->fd_ittag = fwohci_it_set(fd->fd_dev, fd->fd_channel,
		    fd->fd_tag);
		if (fd->fd_ittag == NULL) {
			return EBUSY;
		}
		fd->fd_flags |= FWISO_WRITE;
		printf("fwiso%d: it_set returns %p\n", unit, fd->fd_ittag);

		fd->fd_it_dv_insert_fractional = 16000;
		fd->fd_it_dv_insert_fraction = 1015;
		fd->fd_it_dv_insert_empty = 0;

#define ITLISTSIZE sizeof(struct ieee1394_it_datalist)*100
		if ((fd->fd_itlist = malloc(ITLISTSIZE, M_DEVBUF,
		    M_WAITOK|M_ZERO)) == NULL) {
			/*  */
			printf("fwiso%d: cannot get memory for it_list\n",
			    unit);
			return EBUSY;
		}

		/* XXX */
		fd->fd_cycletimer = fwohci_it_cycletimer(fd->fd_ittag);
		printf("cycletimer %x %d %d\n", fd->fd_cycletimer,
		    fd->fd_cycletimer >> 13, fd->fd_cycletimer&0x1fff);
	}
#if 1
	switch(fd->fd_mode) {
	case FWISO_MODE_DV:
		rv = fwisowrite_dv(fd, uio, 480);
		break;
	case FWISO_MODE_DV_RAW:
		break;
	case FWISO_MODE_RAW:
		break;
	}

#endif

	return rv;
}




int
fwisowrite_dv(struct fwiso_data *fd, struct uio *uio, int size)
{
	int rv;
	struct ieee1394_it_datalist *itlist;
	struct ieee1394_it_datalist *loopend;
	struct iovec *iov = uio->uio_iov;
	int nodeid;
	const struct iovec *iov_end = iov + uio->uio_iovcnt;
	int iov_offs;		/* offset in iov */
	int ndata;
	int writesize = 0;
	int res = 0;
	int serialno;
	int i;
	int s;

	DPRINTF(("fwisowrite_dv(%p %p %d), resid %d iovcnt %d\n",
	    fd, uio, size, uio->uio_resid, uio->uio_iovcnt));

	iov_offs = 0;
	itlist = fd->fd_itlist;

	if (uio->uio_resid < size) {
		/* Only get rid of small data */
		uio->uio_offset += uio->uio_resid;
		uio->uio_resid = 0;
		return res;
	}

	nodeid = 0;

	s = splbio();

	while (uio->uio_resid >= size) {
		if (uio->uio_resid > 90*size) {
			loopend = fd->fd_itlist + 90;
			DPRINTF(("loop 90\n"));
		} else {
			loopend = fd->fd_itlist + uio->uio_resid/size;
			DPRINTF(("loop %d %p\n", uio->uio_resid/size, loopend));
		}

		serialno = fd->fd_it_serial;

		/* make data list */
		for (itlist = fd->fd_itlist; itlist < loopend; ++itlist) {
			int psize = size;

			/* first data is CIP header */
			itlist->it_cmd[0] = IEEE1394_IT_CMD_IMMED | 8;
			itlist->it_u[0].id_data[0]
			    = IEEE1394_CIP_SET(SID, nodeid)
			    | IEEE1394_CIP_SET(DBS, size)
			    | IEEE1394_CIP_SET(DBC, serialno);
			++serialno;

			itlist->it_u[0].id_data[1]
			    = IEEE1394_CIP_FMT_DV
			    | IEEE1394_CIP_SET(FDF_SYT, 0xffff);

			for (i = 1; i < IEEE1394_IT_CMD_NUM; ++i) {

				if (psize == 0) {
					itlist->it_cmd[i]
					    = IEEE1394_IT_CMD_NOP;
					continue;
				}

				itlist->it_u[i].id_addr
				    = (u_int8_t *)iov->iov_base + iov_offs;

				if (iov->iov_len - iov_offs >= psize) {
					itlist->it_cmd[i]
					    = IEEE1394_IT_CMD_PTR | psize;
					iov_offs += psize;
					psize = 0;
				} else {
					itlist->it_cmd[i]
					    = IEEE1394_IT_CMD_PTR | (iov->iov_len - iov_offs);
					psize -= iov->iov_len - iov_offs;
					iov_offs = 0;
					if (++iov == iov_end) {
						printf("ERROR iov %d\n",
						    iov - uio->uio_iov);
						res = EFAULT;
						goto error_1;
					}
				}
			}
		}
		ndata = itlist - fd->fd_itlist;
		DPRINTF(("calling fwohci_it_ctx_writedata(%p %d %p 0)\n",
		    fd->fd_ittag, ndata, fd->fd_itlist));
		rv = fwohci_it_ctx_writedata(fd->fd_ittag, ndata,
		    fd->fd_itlist, 0);

		writesize = rv * size;
		/* XXX: decrement for empty packet */

		uio->uio_resid -= writesize;
		uio->uio_offset += writesize;
		fd->fd_it_serial += rv;
		itlist += rv;

		if (rv == 0) {
			if (fd->fd_uioprev == uio->uio_resid) {
#if 0
				printf("umm, I cannot write any more (%d)\n",
				    fd->fd_it_serial);
				res = EFAULT;
				goto error_1;
#endif
			}
		}
		fd->fd_uioprev = uio->uio_resid;

		if (rv < ndata) {
			/* reach at the end of DMA buffer */
			break;
		}
	}

 error_1:

	splx(s);

	return res;
}



int
fwisoopen(dev_t dev, int flags, int fmt, struct proc *p)
{
	struct fwiso_data *fd;
	int unit = UNIT(dev);

	if (unit >= NFWISO) {
		return ENXIO;
	}

	if (IOCTL(dev)) {
		return 0;
	}

	fd = &fwiso_data_str[unit];

	if (fd->fd_flags &= FWISO_OPEN) {
		return EBUSY;
	}

	fd->fd_flags = FWISO_OPEN;
	if (flags & O_NONBLOCK) {
		fd->fd_flags |= FWISO_NONBLOCK;
	}

	fd->fd_rsv_size = 0;
	fd->fd_uio = NULL;
	fd->fd_mode = FWISO_MODE_RAW;

	if (DVDEV(dev)) {
		fd->fd_mode = FWISO_MODE_DV;
		fd->fd_channel = 63;
		fd->fd_tag = IEEE1394_ISO_TAG1;
	}

	if (fd->fd_dev == NULL) {
		int i;

		for (i = 0; i < FWISO_MAX_INTERFACE; ++i) {
			if (fwiso_interface[i] != NULL) {
				fd->fd_dev = fwiso_interface[i];
				printf("%s: using %s\n", fd->fd_devname,
				    fwiso_interface[i]->sc1394_dev.dv_xname);
				break;
			}
		}
	}

	fd->fd_overflow = 0;

	return 0;
}





int
fwisoclose(dev_t dev, int flags, int fmt, struct proc *p)
{
	struct fwiso_data *fd;
	int unit = UNIT(dev);

	if (unit >= NFWISO) {
		return ENXIO;
	}

	fd = &fwiso_data_str[unit];

	fd->fd_flags &= ~FWISO_OPEN;

	/* remove interrupt handler */
	if (fd->fd_flags & FWISO_SETHANDLER) {
		int s = splbio();
		/* remove handler */
		(*fd->fd_dev->sc1394_ir_close)((struct device *)fd->fd_dev,
		     fd->fd_irtag);

		fd->fd_flags &= ~FWISO_SETHANDLER;
		splx(s);
	}

	/* When this is opened for write, cleanup it_tag */
	if (fd->fd_flags & FWISO_WRITE) {
		fwohci_it_ctx_clear(fd->fd_ittag);
		fd->fd_ittag = NULL;
	}

	if (fd->fd_overflow > 0) {
		printf("%s: %d frame dropped\n", fd->fd_devname, fd->fd_overflow);
	}

	fd->fd_flags = 0;
	fd->fd_overflow = 0;
	/* keep mode and interface */

	return 0;
}



int
fwisoioctl(dev_t dev, u_long cmd, caddr_t data, int fflag, struct proc *p)
{
	int unit = UNIT(dev);
	struct fwiso_data *fd;
	int error = 0;

	if (unit >= NFWISO) {
		return ENXIO;
	}

	fd = &fwiso_data_str[unit];

	if (!data) {
		return EINVAL;
	}

	switch(cmd) {

	case FWISOSETIF:
	    {
		const struct fwiso_if *fi = (struct fwiso_if *)data;

		if ((fd->fd_dev = fwiso_lookup_if(fi->fi_name)) == NULL) {
			error = EINVAL;
		}
		break;
	    }
	case FWISOGETIF:
	    {
		struct fwiso_if *fi = (struct fwiso_if *)data;

		if (fd->fd_dev != NULL) {
			memcpy(fi->fi_name, fd->fd_dev->sc1394_dev.dv_xname,
			    FWISO_IFNAMESIZ);
		} else {
			memcpy(fi->fi_name, "---", 3);
		}
		break;
	    }
	case FWISOSETMODE:
	    {
		const int mode = *(int *)data;

		if (mode >= 0 && mode < FWISO_MODE_MAX) {
			fd->fd_mode = mode;
		} else {
			error = EINVAL;
		}
		break;
	    }
	case FWISOGETMODE:
		*((int *)data) = fd->fd_mode;
		break;
	case FWISOSETCHANNEL:
	    {
		const int channel = *(int *)data;

		if (channel >= 0 && channel < 64) {
			fd->fd_channel = channel;
		} else if (channel == FWISO_CHANNEL_ANY) {
			fd->fd_channel = IEEE1394_ISO_CHANNEL_ANY;
		} else {
			error = EINVAL;
		}
		break;
	    }
	case FWISOGETCHANNEL:
		*((int *)data) = fd->fd_channel;
		break;
	case FWISOSETTAG:
	    {
		const int tag = *(int *)data;

#define FWISO_TAG_MAX (FWISO_TAG0 | FWISO_TAG1 | FWISO_TAG2 | FWISO_TAG3)

		if (tag >= FWISO_TAG0 || tag <= FWISO_TAG_MAX) {
			/* valid tag */
			fd->fd_tag = tag;
		} else {
			error = EINVAL;
		}
	    }
	case FWISOGETTAG:
		*((int *)data) = fd->fd_tag;
		break;
	default:
		error = EINVAL;
		break;
	}
	
	return error;
}




int
fwisopoll(dev_t dev, int events, struct proc *p)
{
	int unit = UNIT(dev);
	struct fwiso_data *fd;
	int s;
	int revents;
	int (*ir_select)(struct device *, ieee1394_ir_tag_t, struct proc *);

	if (unit >= NFWISO) {
		return ENXIO;
	}

	fd = &fwiso_data_str[unit];

	if ((ir_select = fd->fd_dev->sc1394_ir_select) == NULL) {
		return events;
	}

	s = splbio();
	/*
	 * right now, only select for read is supported.
	 */
	revents = events & (POLLOUT | POLLWRNORM);

	if (events & (POLLIN | POLLRDNORM)) {
		if ((fd->fd_flags & FWISO_SETHANDLER) == 0) {
			int errcode;

			if ((errcode = fwiso_set_handler(fd)) != 0) {
				splx(s);
				return errcode;
			}
		}

		if ((*ir_select)((struct device *)fd->fd_dev,
			fd->fd_irtag, p) > 0) {
			revents |= events & (POLLIN | POLLRDNORM);
		}
#if 0
		if (fd->fd_rsv_pkts.fp_iovcnt > 0) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			selrecord(p, &fd->fd_sel);
		}
#endif
	}

	splx(s);
	return revents;
}



int
fwiso_register_if(struct ieee1394_softc *interface)
{
	int i;

	for (i = 0; i < FWISO_MAX_INTERFACE; ++i) {
		if (fwiso_interface[i] == NULL) {
			fwiso_interface[i] = interface;
			break;
		}
	}

	if (i == FWISO_MAX_INTERFACE) {
		printf("fwiso: too many interfaces %s\n",
		    interface->sc1394_dev.dv_xname);
		return 1;
	}

	return 0;
}




static struct ieee1394_softc *
fwiso_lookup_if(const char *ifname)
{
	int i;

	for (i = 0; i < FWISO_MAX_INTERFACE; ++i) {
		if (!strncmp(fwiso_interface[i]->sc1394_dev.dv_xname,
		    ifname, FWISO_IFNAMESIZ)) {
			return fwiso_interface[i];
		}
	}

	return NULL;
}




/*
 * static int fwiso_set_handler(struct fwiso_data *fd)
 *
 *	This function set interrupt handler for isochronous data read.
 */
static int
fwiso_set_handler(struct fwiso_data *fd)
{
	int bufsize;
	int s;
	struct ieee1394_softc *isc = fd->fd_dev;
	int flags = 0;

	if (isc == NULL || isc->sc1394_ir_open == NULL) {
		return ENXIO;
	}
	switch (fd->fd_mode) {
	case FWISO_MODE_DV:
		if (fd->fd_threshold == 0) {
			fd->fd_threshold = 30;
		}
		fd->fd_rsv_size = 1024; /* buffer for 256 ms DV data */
		bufsize = 488;
		flags |= IEEE1394_IR_TRIGGER_CIP_SYNC;
		break;
	case FWISO_MODE_DV_RAW:
		fd->fd_rsv_size = 128; /* buffer for 16 ms DV data */
		bufsize = 488;
		flags |= IEEE1394_IR_SHORTDELAY;
		break;
	case FWISO_MODE_RAW:
		fd->fd_rsv_size = 64;
		bufsize = 4000;
		flags |= IEEE1394_IR_NEEDHEADER | IEEE1394_IR_SHORTDELAY;
		break;
	case FWISO_MODE_MPEG2TS:
		fd->fd_rsv_size = 64;
		bufsize = 512;
		break;
	}

	s = splbio();

	/* set interrupt handler */
	fd->fd_irtag = (*isc->sc1394_ir_open)((struct device *)isc,
	    fd->fd_channel, fd->fd_tag, fd->fd_rsv_size, bufsize, flags);
	if (fd->fd_irtag == NULL) {
		splx(s);
		return ENXIO;
	}
	fd->fd_flags |= FWISO_SETHANDLER;
 
	splx(s);

	return 0;
}
