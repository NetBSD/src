/*	$NetBSD: linux_cdrom.c,v 1.4.14.1 1999/11/15 00:40:04 fvdl Exp $ */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/cdio.h>
#include <sys/dvdio.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_cdrom.h>

#include <compat/linux/linux_syscallargs.h>

#if 0
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

int
linux_ioctl_cdrom(p, uap, retval)
	struct proc *p;
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
	int (*ioctlf) __P((struct file *, u_long, caddr_t, struct proc *));

	struct linux_cdrom_blk l_blk;
	struct linux_cdrom_msf l_msf;
	struct linux_cdrom_ti l_ti;
	struct linux_cdrom_tochdr l_tochdr;
	struct linux_cdrom_tocentry l_tocentry;
	struct linux_cdrom_subchnl l_subchnl;
	struct linux_cdrom_volctrl l_volctrl;

	struct ioc_play_blocks t_blocks;
	struct ioc_play_msf t_msf;
	struct ioc_play_track t_track;
	struct ioc_toc_header t_header;
	struct cd_toc_entry *entry, t_entry;
	struct ioc_read_toc_entry t_toc_entry;
	struct cd_sub_channel_info *info, t_info;
	struct ioc_read_subchannel t_subchannel;
	struct ioc_vol t_vol;

	dvd_struct ds;
	dvd_authinfo dai;

	fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL)
		return (EBADF);

	com = SCARG(uap, com);
	ioctlf = fp->f_ops->fo_ioctl;
	retval[0] = 0;

	switch(com) {
	case LINUX_CDROMPLAYMSF:
		error = copyin(SCARG(uap, data), &l_msf, sizeof l_msf);
		if (error)
			return error;

		t_msf.start_m = l_msf.cdmsf_min0;
		t_msf.start_s = l_msf.cdmsf_sec0;
		t_msf.start_f = l_msf.cdmsf_frame0;
		t_msf.end_m = l_msf.cdmsf_min1;
		t_msf.end_s = l_msf.cdmsf_sec1;
		t_msf.end_f = l_msf.cdmsf_frame1;

		return ioctlf(fp, CDIOCPLAYMSF, (caddr_t)&t_msf, p);

	case LINUX_CDROMPLAYTRKIND:
		error = copyin(SCARG(uap, data), &l_ti, sizeof l_ti);
		if (error)
			return error;

		t_track.start_track = l_ti.cdti_trk0;
		t_track.start_index = l_ti.cdti_ind0;
		t_track.end_track = l_ti.cdti_trk1;
		t_track.end_index = l_ti.cdti_ind1;

		return ioctlf(fp, CDIOCPLAYTRACKS, (caddr_t)&t_track, p);

	case LINUX_CDROMREADTOCHDR:
		error = ioctlf(fp, CDIOREADTOCHEADER, (caddr_t)&t_header, p);
		if (error)
			return error;

		l_tochdr.cdth_trk0 = t_header.starting_track;
		l_tochdr.cdth_trk1 = t_header.ending_track;

		return copyout(&l_tochdr, SCARG(uap, data), sizeof l_tochdr);

	case LINUX_CDROMREADTOCENTRY:
		error = copyin(SCARG(uap, data), &l_tocentry,
			       sizeof l_tocentry);
		if (error)
			return error;
	    
		sg = stackgap_init(p->p_emul);
		entry = stackgap_alloc(&sg, sizeof *entry);
		t_toc_entry.address_format = l_tocentry.cdte_format;
		t_toc_entry.starting_track = l_tocentry.cdte_track;
		t_toc_entry.data_len = sizeof *entry;
		t_toc_entry.data = entry;

		error = ioctlf(fp, CDIOREADTOCENTRYS, (caddr_t)&t_toc_entry,
			       p);
		if (error)
			return error;

		error = copyin(entry, &t_entry, sizeof t_entry);
		if (error)
			return error;
	    
		l_tocentry.cdte_adr = t_entry.addr_type;
		l_tocentry.cdte_ctrl = t_entry.control;
		switch (t_toc_entry.address_format) {
		case CD_LBA_FORMAT:
			l_tocentry.cdte_addr.lba = t_entry.addr.lba;
			break;
		case CD_MSF_FORMAT:
			l_tocentry.cdte_addr.msf.minute =
			    t_entry.addr.msf.minute;
			l_tocentry.cdte_addr.msf.second =
			    t_entry.addr.msf.second;
			l_tocentry.cdte_addr.msf.frame =
			    t_entry.addr.msf.frame;
			break;
		default:
			printf("linux_ioctl: unknown format msf/lba\n");
			return EINVAL;
		}
		
		return copyout(&l_tocentry, SCARG(uap, data),
			       sizeof l_tocentry);
		
	case LINUX_CDROMVOLCTRL:
		error = copyin(SCARG(uap, data), &l_volctrl, sizeof l_volctrl);
		if (error)
			return error;

		t_vol.vol[0] = l_volctrl.channel0;
		t_vol.vol[1] = l_volctrl.channel1;
		t_vol.vol[2] = l_volctrl.channel2;
		t_vol.vol[3] = l_volctrl.channel3;

		return ioctlf(fp, CDIOCSETVOL, (caddr_t)&t_vol, p);

	case LINUX_CDROMVOLREAD:
		error = ioctlf(fp, CDIOCGETVOL, (caddr_t)&t_vol, p);
		if (error)
			return error;

		l_volctrl.channel0 = t_vol.vol[0];
		l_volctrl.channel1 = t_vol.vol[1];
		l_volctrl.channel2 = t_vol.vol[2];
		l_volctrl.channel3 = t_vol.vol[3];

		return copyout(&l_volctrl, SCARG(uap, data), sizeof l_volctrl);

	case LINUX_CDROMSUBCHNL:
		error = copyin(SCARG(uap, data), &l_subchnl, sizeof l_subchnl);
		if (error)
			return error;
	    
		sg = stackgap_init(p->p_emul);
		info = stackgap_alloc(&sg, sizeof *info);
		t_subchannel.address_format = l_subchnl.cdsc_format;
		t_subchannel.data_format = CD_CURRENT_POSITION;
		t_subchannel.track = l_subchnl.cdsc_trk;
		t_subchannel.data_len = sizeof *info;
		t_subchannel.data = info;
		DPRINTF(("linux_ioctl: CDROMSUBCHNL %d %d\n",
			 l_subchnl.cdsc_format, l_subchnl.cdsc_trk));

		error = ioctlf(fp, CDIOCREADSUBCHANNEL, (caddr_t)&t_subchannel,
			       p);
		if (error)
		    return error;

		error = copyin(info, &t_info, sizeof t_info);
		if (error)
			return error;
	    
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

	    switch (t_subchannel.address_format) {
	    case CD_LBA_FORMAT:
		    l_subchnl.cdsc_absaddr.lba =
			t_info.what.position.absaddr.lba;
		    l_subchnl.cdsc_reladdr.lba =
			t_info.what.position.reladdr.lba;
		    DPRINTF(("LBA: %d %d\n",
			     t_info.what.position.absaddr.lba,
			     t_info.what.position.reladdr.lba));
		    break;

	    case CD_MSF_FORMAT:
		    l_subchnl.cdsc_absaddr.msf.minute =
			t_info.what.position.absaddr.msf.minute;
		    l_subchnl.cdsc_absaddr.msf.second =
			t_info.what.position.absaddr.msf.second;
		    l_subchnl.cdsc_absaddr.msf.frame =
			t_info.what.position.absaddr.msf.frame;

		    l_subchnl.cdsc_reladdr.msf.minute =
			t_info.what.position.reladdr.msf.minute;
		    l_subchnl.cdsc_reladdr.msf.second =
			t_info.what.position.reladdr.msf.second;
		    l_subchnl.cdsc_reladdr.msf.frame =
			t_info.what.position.reladdr.msf.frame;
		    DPRINTF(("MSF: %d %d %d %d\n",
			     t_info.what.position.absaddr.msf.minute,
			     t_info.what.position.absaddr.msf.second,
			     t_info.what.position.reladdr.msf.minute,
			     t_info.what.position.reladdr.msf.second));
		    break;

	    default:
		    DPRINTF(("linux_ioctl: unknown format msf/lba\n"));
		    return EINVAL;
	    }

	    return copyout(&l_subchnl, SCARG(uap, data), sizeof l_subchnl);

	case LINUX_CDROMPLAYBLK:
		error = copyin(SCARG(uap, data), &l_blk, sizeof l_blk);
		if (error)
			return error;

		t_blocks.blk = l_blk.from;
		t_blocks.len = l_blk.len;

		return ioctlf(fp, CDIOCPLAYBLOCKS, (caddr_t)&t_blocks, p);

	case LINUX_CDROMEJECT_SW:
		error = copyin(SCARG(uap, data), &idata, sizeof idata);
		if (error)
			return error;

		if (idata == 1)
			ncom = CDIOCALLOW;
		else
			ncom = CDIOCPREVENT;
		break;

	case LINUX_CDROMPAUSE:
		ncom = CDIOCPAUSE;
		break;

	case LINUX_CDROMRESUME:
		ncom = CDIOCRESUME;
		break;

	case LINUX_CDROMSTOP:
		ncom = CDIOCSTOP;
		break;

	case LINUX_CDROMSTART:
		ncom = CDIOCSTART;
		break;

	case LINUX_CDROMEJECT:
		ncom = CDIOCEJECT;
		break;

	case LINUX_CDROMRESET:
		ncom = CDIOCRESET;
		break;

	case LINUX_DVD_READ_STRUCT:
		error = copyin(SCARG(uap, data), &ds, sizeof ds);
		if (error)
			return (error);
		error = ioctlf(fp, DVD_READ_STRUCT, (caddr_t)&ds, p);
		if (error)
			return (error);
		error = copyout(&ds, SCARG(uap, data), sizeof ds);
		if (error)
			return (error);
		return (0);

	case LINUX_DVD_WRITE_STRUCT:
		error = copyin(SCARG(uap, data), &ds, sizeof ds);
		if (error)
			return (error);
		error = ioctlf(fp, DVD_WRITE_STRUCT, (caddr_t)&ds, p);
		if (error)
			return (error);
		error = copyout(&ds, SCARG(uap, data), sizeof ds);
		if (error)
			return (error);
		return (0);

	case LINUX_DVD_AUTH:
		error = copyin(SCARG(uap, data), &dai, sizeof dai);
		if (error)
			return (error);
		error = ioctlf(fp, DVD_AUTH, (caddr_t)&dai, p);
		if (error)
			return (error);
		error = copyout(&dai, SCARG(uap, data), sizeof dai);
		if (error)
			return (error);
		return (0);


	default:
		DPRINTF(("linux_ioctl: unimplemented ioctl %08lx\n", com));
		return EINVAL;
	}

	return ioctlf(fp, ncom, NULL, p);
}
