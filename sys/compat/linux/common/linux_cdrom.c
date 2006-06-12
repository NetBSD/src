/*	$NetBSD: linux_cdrom.c,v 1.20 2006/06/12 00:42:18 christos Exp $ */

/*
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_cdrom.c,v 1.20 2006/06/12 00:42:18 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/cdio.h>
#include <sys/dvdio.h>
#include <sys/malloc.h>

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_cdrom.h>

#include <compat/linux/linux_syscallargs.h>

static int bsd_to_linux_msf_lba(unsigned address_format, union msf_lba *bml,
				union linux_cdrom_addr *llml);

#if DEBUG_LINUX
#define DPRINTF(x) uprintf x
#else
#define DPRINTF(x)
#endif

/*
 * XXX from dev/scsipi/cd.c
 */
#define MAXTRACK 99

static int
bsd_to_linux_msf_lba(unsigned address_format, union msf_lba *bml,
		      union linux_cdrom_addr *llml)
{
	switch (address_format) {
	case CD_LBA_FORMAT:
		llml->lba = bml->lba;
		break;
	case CD_MSF_FORMAT:
		llml->msf.minute = bml->msf.minute;
		llml->msf.second = bml->msf.second;
		llml->msf.frame = bml->msf.frame;
		break;
	default:
		return -1;
	}
	return 0;
}

int
linux_ioctl_cdrom(l, uap, retval)
	struct lwp *l;
	struct linux_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap;
	register_t *retval;
{
	int error, idata;
	u_long com, ncom;
	caddr_t sg;
	struct file *fp;
	struct filedesc *fdp;
	int (*ioctlf)(struct file *, u_long, void *, struct lwp *);

	union { 
		struct linux_cdrom_blk ll_blk;
		struct linux_cdrom_msf ll_msf;
		struct linux_cdrom_ti ll_ti;
		struct linux_cdrom_tochdr ll_tochdr;
		struct linux_cdrom_tocentry ll_tocentry;
		struct linux_cdrom_subchnl ll_subchnl;
		struct linux_cdrom_volctrl ll_volctrl;
		struct linux_cdrom_multisession ll_session;
		dvd_struct ll_ds;
		dvd_authinfo ll_dai;
	} *u1;

#define l_blk u1->ll_blk
#define l_msf u1->ll_msf
#define l_ti u1->ll_ti
#define l_tochdr u1->ll_tochdr
#define l_tocentry u1->ll_tocentry
#define l_subchnl u1->ll_subchnl
#define l_volctrl u1->ll_volctrl
#define l_session u1->ll_session
#define ds u1->ll_ds
#define dai u1->ll_dai

	union {
		struct ioc_play_blocks tt_blocks;
		struct ioc_play_msf tt_msf;
		struct ioc_play_track tt_track;
		struct ioc_toc_header tt_header;
		struct cd_toc_entry tt_entry;
		struct ioc_read_toc_entry tt_toc_entry;
		struct cd_sub_channel_info tt_info;
		struct ioc_read_subchannel tt_subchannel;
		struct ioc_vol tt_vol;
	} *u2;

#define	t_blocks u2->tt_blocks
#define	t_msf u2->tt_msf
#define	t_track u2->tt_track
#define	t_header u2->tt_header
#define	t_entry u2->tt_entry
#define	t_toc_entry u2->tt_toc_entry
#define	t_info u2->tt_info
#define	t_subchannel u2->tt_subchannel
#define	t_vol u2->tt_vol

	struct cd_toc_entry *entry;
	struct cd_sub_channel_info *info;
	struct proc *p = l->l_proc;

	fdp = p->p_fd;
	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);

	FILE_USE(fp);

	com = SCARG(uap, com);
	ioctlf = fp->f_ops->fo_ioctl;
	retval[0] = error = 0;

	u1 = malloc(sizeof(*u1), M_TEMP, M_WAITOK);
	u2 = malloc(sizeof(*u2), M_TEMP, M_WAITOK);

	switch(com) {
	case LINUX_CDROMPLAYMSF:
		error = copyin(SCARG(uap, data), &l_msf, sizeof l_msf);
		if (error)
			break;

		t_msf.start_m = l_msf.cdmsf_min0;
		t_msf.start_s = l_msf.cdmsf_sec0;
		t_msf.start_f = l_msf.cdmsf_frame0;
		t_msf.end_m = l_msf.cdmsf_min1;
		t_msf.end_s = l_msf.cdmsf_sec1;
		t_msf.end_f = l_msf.cdmsf_frame1;

		error = ioctlf(fp, CDIOCPLAYMSF, (caddr_t)&t_msf, l);
		break;

	case LINUX_CDROMPLAYTRKIND:
		error = copyin(SCARG(uap, data), &l_ti, sizeof l_ti);
		if (error)
			break;

		t_track.start_track = l_ti.cdti_trk0;
		t_track.start_index = l_ti.cdti_ind0;
		t_track.end_track = l_ti.cdti_trk1;
		t_track.end_index = l_ti.cdti_ind1;

		error = ioctlf(fp, CDIOCPLAYTRACKS, (caddr_t)&t_track, l);
		break;

	case LINUX_CDROMREADTOCHDR:
		error = ioctlf(fp, CDIOREADTOCHEADER, (caddr_t)&t_header, l);
		if (error)
			break;

		l_tochdr.cdth_trk0 = t_header.starting_track;
		l_tochdr.cdth_trk1 = t_header.ending_track;

		error = copyout(&l_tochdr, SCARG(uap, data), sizeof l_tochdr);
		break;

	case LINUX_CDROMREADTOCENTRY:
		error = copyin(SCARG(uap, data), &l_tocentry,
			       sizeof l_tocentry);
		if (error)
			break;

		sg = stackgap_init(p, 0);
		entry = stackgap_alloc(p, &sg, sizeof *entry);
		t_toc_entry.address_format = l_tocentry.cdte_format;
		t_toc_entry.starting_track = l_tocentry.cdte_track;
		t_toc_entry.data_len = sizeof *entry;
		t_toc_entry.data = entry;

		error = ioctlf(fp, CDIOREADTOCENTRIES, (caddr_t)&t_toc_entry,
			       l);
		if (error)
			break;

		error = copyin(entry, &t_entry, sizeof t_entry);
		if (error)
			break;

		l_tocentry.cdte_adr = t_entry.addr_type;
		l_tocentry.cdte_ctrl = t_entry.control;
		if (bsd_to_linux_msf_lba(t_entry.addr_type, &t_entry.addr,
		    &l_tocentry.cdte_addr) < 0) {
			DPRINTF(("linux_ioctl: unknown format msf/lba\n"));
			error = EINVAL;
			break;
		}

		error = copyout(&l_tocentry, SCARG(uap, data),
			       sizeof l_tocentry);
		break;

	case LINUX_CDROMVOLCTRL:
		error = copyin(SCARG(uap, data), &l_volctrl, sizeof l_volctrl);
		if (error)
			break;

		t_vol.vol[0] = l_volctrl.channel0;
		t_vol.vol[1] = l_volctrl.channel1;
		t_vol.vol[2] = l_volctrl.channel2;
		t_vol.vol[3] = l_volctrl.channel3;

		error = ioctlf(fp, CDIOCSETVOL, (caddr_t)&t_vol, l);
		break;

	case LINUX_CDROMVOLREAD:
		error = ioctlf(fp, CDIOCGETVOL, (caddr_t)&t_vol, l);
		if (error)
			break;

		l_volctrl.channel0 = t_vol.vol[0];
		l_volctrl.channel1 = t_vol.vol[1];
		l_volctrl.channel2 = t_vol.vol[2];
		l_volctrl.channel3 = t_vol.vol[3];

		error = copyout(&l_volctrl, SCARG(uap, data), sizeof l_volctrl);
		break;

	case LINUX_CDROMSUBCHNL:
		error = copyin(SCARG(uap, data), &l_subchnl, sizeof l_subchnl);
		if (error)
			break;

		sg = stackgap_init(p, 0);
		info = stackgap_alloc(p, &sg, sizeof *info);
		t_subchannel.address_format = CD_MSF_FORMAT;
		t_subchannel.track = 0;
		t_subchannel.data_format = l_subchnl.cdsc_format;
		t_subchannel.data_len = sizeof *info;
		t_subchannel.data = info;
		DPRINTF(("linux_ioctl: CDROMSUBCHNL %d %d\n",
			 l_subchnl.cdsc_format, l_subchnl.cdsc_trk));

		error = ioctlf(fp, CDIOCREADSUBCHANNEL, (caddr_t)&t_subchannel,
			       l);
		if (error)
			break;

		error = copyin(info, &t_info, sizeof t_info);
		if (error)
			break;

		l_subchnl.cdsc_audiostatus = t_info.header.audio_status;
		l_subchnl.cdsc_adr = t_info.what.position.addr_type;
		l_subchnl.cdsc_ctrl = t_info.what.position.control;
		l_subchnl.cdsc_ind = t_info.what.position.index_number;

		DPRINTF(("linux_ioctl: CDIOCREADSUBCHANNEL %d %d %d\n",
			t_info.header.audio_status,
			t_info.header.data_len[0],
			t_info.header.data_len[1]));
		DPRINTF(("(more) %d %d %d %d %d\n",
			t_info.what.position.data_format,
			t_info.what.position.control,
			t_info.what.position.addr_type,
			t_info.what.position.track_number,
			t_info.what.position.index_number));

		if (bsd_to_linux_msf_lba(t_subchannel.address_format,
					 &t_info.what.position.absaddr,
					 &l_subchnl.cdsc_absaddr) < 0 ||
		    bsd_to_linux_msf_lba(t_subchannel.address_format,
					 &t_info.what.position.reladdr,
					 &l_subchnl.cdsc_reladdr) < 0) {
			DPRINTF(("linux_ioctl: unknown format msf/lba\n"));
			error = EINVAL;
			break;
		}

		error = copyout(&l_subchnl, SCARG(uap, data), sizeof l_subchnl);
		break;

	case LINUX_CDROMPLAYBLK:
		error = copyin(SCARG(uap, data), &l_blk, sizeof l_blk);
		if (error)
			break;

		t_blocks.blk = l_blk.from;
		t_blocks.len = l_blk.len;

		error = ioctlf(fp, CDIOCPLAYBLOCKS, (caddr_t)&t_blocks, l);
		break;

	case LINUX_CDROMEJECT_SW:
		error = copyin(SCARG(uap, data), &idata, sizeof idata);
		if (error)
			break;

		if (idata == 1)
			ncom = CDIOCALLOW;
		else
			ncom = CDIOCPREVENT;
		error = ioctlf(fp, ncom, NULL, l);
		break;

	case LINUX_CDROMPAUSE:
		error = ioctlf(fp, CDIOCPAUSE, NULL, l);
		break;

	case LINUX_CDROMRESUME:
		error = ioctlf(fp, CDIOCRESUME, NULL, l);
		break;

	case LINUX_CDROMSTOP:
		error = ioctlf(fp, CDIOCSTOP, NULL, l);
		break;

	case LINUX_CDROMSTART:
		error = ioctlf(fp, CDIOCSTART, NULL, l);
		break;

	case LINUX_CDROMEJECT:
		error = ioctlf(fp, CDIOCEJECT, NULL, l);
		break;

	case LINUX_CDROMRESET:
		error = ioctlf(fp, CDIOCRESET, NULL, l);
		break;

	case LINUX_CDROMMULTISESSION:
		error = copyin(SCARG(uap, data), &l_session, sizeof l_session);
		if (error)
			break;

		error = ioctlf(fp, CDIOREADTOCHEADER, (caddr_t)&t_header, l);
		if (error)
			break;

		sg = stackgap_init(p, 0);
		entry = stackgap_alloc(p, &sg, sizeof *entry);
		t_toc_entry.address_format = l_session.addr_format;
		t_toc_entry.starting_track = 0;
		t_toc_entry.data_len = sizeof *entry;
		t_toc_entry.data = entry;

		error = ioctlf(fp, CDIOREADTOCENTRIES,
		    (caddr_t)&t_toc_entry, l);
		if (error)
			break;

		error = copyin(entry, &t_entry, sizeof t_entry);
		if (error)
			break;

		if (bsd_to_linux_msf_lba(l_session.addr_format,
		    &t_entry.addr, &l_session.addr) < 0) {
			error = EINVAL;
			break;
		}

		l_session.xa_flag =
		    t_header.starting_track != t_header.ending_track;

		error = copyout(&l_session, SCARG(uap, data), sizeof l_session);
		break;

	case LINUX_CDROMCLOSETRAY:
		error = ioctlf(fp, CDIOCCLOSE, NULL, l);
		break;

	case LINUX_CDROM_LOCKDOOR:
		ncom = SCARG(uap, data) != 0 ? CDIOCPREVENT : CDIOCALLOW;
		error = ioctlf(fp, ncom, NULL, l);
		break;

	case LINUX_CDROM_SET_OPTIONS:
	case LINUX_CDROM_CLEAR_OPTIONS:
		/* whatever you say */
		break;

	case LINUX_CDROM_DEBUG:
		ncom = SCARG(uap, data) != 0 ? CDIOCSETDEBUG : CDIOCCLRDEBUG;
		error = ioctlf(fp, ncom, NULL, l);
		break;

	case LINUX_CDROM_SELECT_SPEED:
	case LINUX_CDROM_SELECT_DISC:
	case LINUX_CDROM_MEDIA_CHANGED:
	case LINUX_CDROM_DRIVE_STATUS:
	case LINUX_CDROM_DISC_STATUS:
	case LINUX_CDROM_CHANGER_NSLOTS:
	case LINUX_CDROM_GET_CAPABILITY:
		error = ENOSYS;
		break;

	case LINUX_DVD_READ_STRUCT:
		error = copyin(SCARG(uap, data), &ds, sizeof ds);
		if (error)
			break;
		error = ioctlf(fp, DVD_READ_STRUCT, (caddr_t)&ds, l);
		if (error)
			break;
		error = copyout(&ds, SCARG(uap, data), sizeof ds);
		break;

	case LINUX_DVD_WRITE_STRUCT:
		error = copyin(SCARG(uap, data), &ds, sizeof ds);
		if (error)
			break;
		error = ioctlf(fp, DVD_WRITE_STRUCT, (caddr_t)&ds, l);
		if (error)
			break;
		error = copyout(&ds, SCARG(uap, data), sizeof ds);
		break;

	case LINUX_DVD_AUTH:
		error = copyin(SCARG(uap, data), &dai, sizeof dai);
		if (error)
			break;
		error = ioctlf(fp, DVD_AUTH, (caddr_t)&dai, l);
		if (error)
			break;
		error = copyout(&dai, SCARG(uap, data), sizeof dai);
		break;


	default:
		DPRINTF(("linux_ioctl: unimplemented ioctl %08lx\n", com));
		error = EINVAL;
	}

	FILE_UNUSE(fp, l);
	free(u1, M_TEMP);
	free(u2, M_TEMP);
	return error;
}
