/* $NetBSD: linux_sg.c,v 1.5.6.6 2008/01/21 09:41:28 yamt Exp $ */

/*
 * Copyright (c) 2004 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_sg.c,v 1.5.6.6 2008/01/21 09:41:28 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/device.h>

#include <sys/scsiio.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_sg.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>

int linux_sg_version = 30125;

#ifdef LINUX_SG_DEBUG
#define DPRINTF(a)	printf a
#else
#define DPRINTF(a)
#endif

#ifdef LINUX_SG_DEBUG
static void dump_sg_io(struct linux_sg_io_hdr *);
static void dump_scsireq(struct scsireq *);
#endif

static int bsd_to_linux_host_status(int);
static int bsd_to_linux_driver_status(int);

int
linux_ioctl_sg(struct lwp *l, const struct linux_sys_ioctl_args *uap,
    register_t *retval)
{
	struct file *fp;
	struct filedesc *fdp;
	u_long com = SCARG(uap, com);
	int error = 0;
	int (*ioctlf)(struct file *, u_long, void *, struct lwp *);
	struct linux_sg_io_hdr lreq;
	struct scsireq req;

        fdp = l->l_proc->p_fd;
	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return EBADF;

	FILE_USE(fp);
	ioctlf = fp->f_ops->fo_ioctl;

	*retval = 0;
	DPRINTF(("Command = %lx\n", com));
	switch (com) {
	case LINUX_SG_GET_VERSION_NUM: {
		error = copyout(&version, SCARG(uap, data),
		    sizeof(linux_sg_version));
		break;
	}
	case LINUX_SG_IO:
		error = copyin(SCARG(uap, data), &lreq, sizeof(lreq));
		if (error) {
			DPRINTF(("failed to copy in request data %d\n", error));
			break;
		}

#ifdef LINUX_SG_DEBUG
		dump_sg_io(&lreq);
#endif
		(void)memset(&req, 0, sizeof(req));
		switch (lreq.dxfer_direction) {
		case SG_DXFER_TO_DEV:
			req.flags = SCCMD_WRITE;
			break;
		case SG_DXFER_FROM_DEV:
			req.flags = SCCMD_READ;
			break;
		default:
			DPRINTF(("unknown direction %d\n",
				lreq.dxfer_direction));
			error = EINVAL;
			goto done;
		}
		if (lreq.iovec_count != 0) {
			/* XXX: Not yet */
			error = EOPNOTSUPP;
			DPRINTF(("scatter/gather not supported\n"));
			break;
		}

		if (lreq.cmd_len > sizeof(req.cmd)) {
			DPRINTF(("invalid command length %d\n", lreq.cmd_len));
			error = EINVAL;
			break;
		}

		error = copyin(lreq.cmdp, req.cmd, lreq.cmd_len);
		if (error) {
			DPRINTF(("failed to copy in cmd data %d\n", error));
			break;
		}

		req.timeout = lreq.timeout;
		req.cmdlen = lreq.cmd_len;
		req.datalen = lreq.dxfer_len;
		req.databuf = lreq.dxferp;

		error = ioctlf(fp, SCIOCCOMMAND, (void *)&req, l);
		if (error) {
			DPRINTF(("SCIOCCOMMAND failed %d\n", error));
			break;
		}
#ifdef LINUX_SG_DEBUG
		dump_scsireq(&req);
#endif
		if (req.senselen_used) {
			if (req.senselen > lreq.mx_sb_len)
				req.senselen = lreq.mx_sb_len;
			lreq.sb_len_wr = req.senselen;
			error = copyout(req.sense, lreq.sbp, req.senselen);
			if (error) {
				DPRINTF(("sense copyout failed %d\n", error));
				break;
			}
		} else {
			lreq.sb_len_wr = 0;
		}

		lreq.status = req.status;
		lreq.masked_status = 0; 	/* XXX */
		lreq.host_status = bsd_to_linux_host_status(req.retsts);
		lreq.sb_len_wr = req.datalen_used;
		lreq.driver_status = bsd_to_linux_driver_status(req.error);
		lreq.resid = req.datalen - req.datalen_used;
		lreq.duration = req.timeout;	/* XXX */
		lreq.info = 0;			/* XXX */
		error = copyout(&lreq, SCARG(uap, data), sizeof(lreq));
		if (error) {
			DPRINTF(("failed to copy out req data %d\n", error));
		}
		break;
	case LINUX_SG_EMULATED_HOST:
	case LINUX_SG_SET_TRANSFORM:
	case LINUX_SG_GET_TRANSFORM:
	case LINUX_SG_GET_NUM_WAITING:
	case LINUX_SG_SCSI_RESET:
	case LINUX_SG_GET_REQUEST_TABLE:
	case LINUX_SG_SET_KEEP_ORPHAN:
	case LINUX_SG_GET_KEEP_ORPHAN:
	case LINUX_SG_GET_ACCESS_COUNT:
	case LINUX_SG_SET_FORCE_LOW_DMA:
	case LINUX_SG_GET_LOW_DMA:
	case LINUX_SG_GET_SG_TABLESIZE:
	case LINUX_SG_GET_SCSI_ID:
	case LINUX_SG_SET_FORCE_PACK_ID:
	case LINUX_SG_GET_PACK_ID:
	case LINUX_SG_SET_RESERVED_SIZE:
	case LINUX_SG_GET_RESERVED_SIZE:
		error = ENODEV;
		break;

	/* version 2 interfaces */
	case LINUX_SG_SET_TIMEOUT:
		break;
	case LINUX_SG_GET_TIMEOUT:
		/* ioctl returns value..., grr. */
		*retval = 60;
		break;
	case LINUX_SG_GET_COMMAND_Q:
	case LINUX_SG_SET_COMMAND_Q:
	case LINUX_SG_SET_DEBUG:
	case LINUX_SG_NEXT_CMD_LEN:
		error = ENODEV;
		break;
	}

done:
	FILE_UNUSE(fp, l);

	DPRINTF(("Return=%d\n", error));
	return error;
}

static int
bsd_to_linux_driver_status(int bs)
{
	switch (bs) {
	default:
	case XS_NOERROR:
		return 0;
	case XS_SENSE:
	case XS_SHORTSENSE:
		return LINUX_DRIVER_SENSE;
	case XS_RESOURCE_SHORTAGE:
		return LINUX_DRIVER_SOFT;
	case XS_DRIVER_STUFFUP:
		return LINUX_DRIVER_ERROR;
	case XS_SELTIMEOUT:
	case XS_TIMEOUT:
		return LINUX_DRIVER_TIMEOUT;
	case XS_BUSY:
		return LINUX_DRIVER_BUSY;
	case XS_RESET:
		return LINUX_SUGGEST_ABORT;
	case XS_REQUEUE:
		return LINUX_SUGGEST_RETRY;
	}
}

static int
bsd_to_linux_host_status(int bs)
{
	switch (bs) {
	case SCCMD_OK:
	case SCCMD_SENSE:
		return LINUX_DID_OK;
	case SCCMD_TIMEOUT:
		return LINUX_DID_TIME_OUT;
	case SCCMD_BUSY:
		return LINUX_DID_BUS_BUSY;
	case SCCMD_UNKNOWN:
	default:
		return LINUX_DID_ERROR;
	}
}



#ifdef LINUX_SG_DEBUG
static void
dump_sg_io(struct linux_sg_io_hdr *lr)
{
	printf("linuxreq [interface_id=%x, dxfer_direction=%d, cmd_len=%d, "
	    "mx_sb_len=%d, iovec_count=%d, dxfer_len=%d, dxferp=%p, "
	    "cmdp=%p, sbp=%p, timeout=%u, flags=%d, pack_id=%d, "
	    "usr_ptr=%p, status=%u, masked_status=%u, sb_len_wr=%u, "
	    "host_status=%u, driver_status=%u, resid=%d, duration=%u, "
	    "info=%u]\n",
	    lr->interface_id, lr->dxfer_direction, lr->cmd_len,
	    lr->mx_sb_len, lr->iovec_count, lr->dxfer_len, lr->dxferp,
	    lr->cmdp, lr->sbp, lr->timeout, lr->flags, lr->pack_id,
	    lr->usr_ptr, lr->status, lr->masked_status, lr->sb_len_wr,
	    lr->host_status, lr->driver_status, lr->resid, lr->duration,
	    lr->info);
}

static void
dump_scsireq(struct scsireq *br)
{
	int i;
	printf("bsdreq [flags=%lx, timeout=%lu, cmd=[ ",
	    br->flags, br->timeout);
	for (i = 0; i < sizeof(br->cmd) / sizeof(br->cmd[0]); i++)
		printf("%.2u ", br->cmd[i]);
	printf("], cmdlen=%u, databuf=%p, datalen=%lu, datalen_used=%lu, "
	    "sense=[ ",
	    br->cmdlen, br->databuf, br->datalen, br->datalen_used);
	for (i = 0; i < sizeof(br->sense) / sizeof(br->sense[0]); i++)
		printf("%.2u ", br->sense[i]);
	printf("], senselen=%u, senselen_used=%u, status=%u, retsts=%u, "
	    "error=%d]\n",
	    br->senselen, br->senselen_used, br->status, br->retsts, br->error);
}
#endif
