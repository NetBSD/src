/*	$NetBSD: ossaudio.c,v 1.35 2001/05/10 01:54:30 augustss Exp $	*/

/*-
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
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/audioio.h>
#include <sys/midiio.h>

#include <sys/syscallargs.h>

#include <compat/ossaudio/ossaudio.h>
#include <compat/ossaudio/ossaudiovar.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x) if (ossdebug) printf x
int ossdebug = 0;
#else
#define DPRINTF(x)
#endif

#define TO_OSSVOL(x)	(((x) * 100 + 127) / 255)
#define FROM_OSSVOL(x)	((((x) > 100 ? 100 : (x)) * 255 + 50) / 100)

static struct audiodevinfo *getdevinfo __P((struct file *, struct proc *));
static int opaque_to_enum(struct audiodevinfo *di, audio_mixer_name_t *label, int opq);
static int enum_to_ord(struct audiodevinfo *di, int enm);
static int enum_to_mask(struct audiodevinfo *di, int enm);

static void setblocksize __P((struct file *, struct audio_info *, struct proc *));


int
oss_ioctl_audio(p, uap, retval)
	struct proc *p;
	struct oss_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap;
	register_t *retval;
{	       
	struct file *fp;
	struct filedesc *fdp;
	u_long com;
	struct audio_info tmpinfo;
	struct audio_offset tmpoffs;
	struct oss_audio_buf_info bufinfo;
	struct oss_count_info cntinfo;
	struct audio_encoding tmpenc;
	u_int u;
	int idat, idata;
	int error = 0;
	int (*ioctlf) __P((struct file *, u_long, caddr_t, struct proc *));

	fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
		return (EBADF);

	FILE_USE(fp);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	com = SCARG(uap, com);
	DPRINTF(("oss_ioctl_audio: com=%08lx\n", com));

	retval[0] = 0;

	ioctlf = fp->f_ops->fo_ioctl;
	switch (com) {
	case OSS_SNDCTL_DSP_RESET:
		error = ioctlf(fp, AUDIO_FLUSH, (caddr_t)0, p);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_SYNC:
	case OSS_SNDCTL_DSP_POST:
		error = ioctlf(fp, AUDIO_DRAIN, (caddr_t)0, p);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_SPEED:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		tmpinfo.play.sample_rate =
		tmpinfo.record.sample_rate = idat;
		error = ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		DPRINTF(("oss_sys_ioctl: SNDCTL_DSP_SPEED %d = %d\n",
			 idat, error));
		if (error)
			goto out;
		/* fall into ... */
	case OSS_SOUND_PCM_READ_RATE:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			goto out;
		idat = tmpinfo.play.sample_rate;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_STEREO:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		tmpinfo.play.channels =
		tmpinfo.record.channels = idat ? 2 : 1;
		(void) ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			goto out;
		idat = tmpinfo.play.channels - 1;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_GETBLKSIZE:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			goto out;
		setblocksize(fp, &tmpinfo, p);
		idat = tmpinfo.blocksize;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_SETFMT:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		switch (idat) {
		case OSS_AFMT_MU_LAW:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 8;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_ULAW;
			break;
		case OSS_AFMT_A_LAW:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 8;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_ALAW;
			break;
		case OSS_AFMT_U8:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 8;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_ULINEAR;
			break;
		case OSS_AFMT_S8:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 8;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_SLINEAR;
			break;
		case OSS_AFMT_S16_LE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 16;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_SLINEAR_LE;
			break;
		case OSS_AFMT_S16_BE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 16;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_SLINEAR_BE;
			break;
		case OSS_AFMT_U16_LE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 16;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_ULINEAR_LE;
			break;
		case OSS_AFMT_U16_BE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 16;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_ULINEAR_BE;
			break;
		default:
			error = EINVAL;
			goto out;
		}
		(void) ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		/* fall into ... */
	case OSS_SOUND_PCM_READ_BITS:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			goto out;
		switch (tmpinfo.play.encoding) {
		case AUDIO_ENCODING_ULAW:
			idat = OSS_AFMT_MU_LAW;
			break;
		case AUDIO_ENCODING_ALAW:
			idat = OSS_AFMT_A_LAW;
			break;
		case AUDIO_ENCODING_SLINEAR_LE:
			if (tmpinfo.play.precision == 16)
				idat = OSS_AFMT_S16_LE;
			else
				idat = OSS_AFMT_S8;
			break;
		case AUDIO_ENCODING_SLINEAR_BE:
			if (tmpinfo.play.precision == 16)
				idat = OSS_AFMT_S16_BE;
			else
				idat = OSS_AFMT_S8;
			break;
		case AUDIO_ENCODING_ULINEAR_LE:
			if (tmpinfo.play.precision == 16)
				idat = OSS_AFMT_U16_LE;
			else
				idat = OSS_AFMT_U8;
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
			if (tmpinfo.play.precision == 16)
				idat = OSS_AFMT_U16_BE;
			else
				idat = OSS_AFMT_U8;
			break;
		case AUDIO_ENCODING_ADPCM:
			idat = OSS_AFMT_IMA_ADPCM;
			break;
		}
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_CHANNELS:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		tmpinfo.play.channels =
		tmpinfo.record.channels = idat;
		(void) ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		/* fall into ... */
	case OSS_SOUND_PCM_READ_CHANNELS:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			goto out;
		idat = tmpinfo.play.channels;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
	case OSS_SOUND_PCM_WRITE_FILTER:
	case OSS_SOUND_PCM_READ_FILTER:
		error = EINVAL; /* XXX unimplemented */
		goto out;
	case OSS_SNDCTL_DSP_SUBDIVIDE:
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		setblocksize(fp, &tmpinfo, p);
		if (error)
			goto out;
		if (idat == 0)
			idat = tmpinfo.play.buffer_size / tmpinfo.blocksize;
		idat = (tmpinfo.play.buffer_size / idat) & -4;
		AUDIO_INITINFO(&tmpinfo);
		tmpinfo.blocksize = idat;
		error = ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			goto out;
		idat = tmpinfo.play.buffer_size / tmpinfo.blocksize;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_SETFRAGMENT:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		if ((idat & 0xffff) < 4 || (idat & 0xffff) > 17) {
			error = EINVAL;
			goto out;
		}
		tmpinfo.blocksize = 1 << (idat & 0xffff);
		tmpinfo.hiwat = (idat >> 16) & 0x7fff;
		DPRINTF(("oss_audio: SETFRAGMENT blksize=%d, hiwat=%d\n",
			 tmpinfo.blocksize, tmpinfo.hiwat));
		if (tmpinfo.hiwat == 0)	/* 0 means set to max */
			tmpinfo.hiwat = 65536;
		(void) ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			goto out;
		u = tmpinfo.blocksize;
		for(idat = 0; u > 1; idat++, u >>= 1)
			;
		idat |= (tmpinfo.hiwat & 0x7fff) << 16;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_GETFMTS:
		for(idat = 0, tmpenc.index = 0; 
		    ioctlf(fp, AUDIO_GETENC, (caddr_t)&tmpenc, p) == 0; 
		    tmpenc.index++) {
			switch(tmpenc.encoding) {
			case AUDIO_ENCODING_ULAW:
				idat |= OSS_AFMT_MU_LAW;
				break;
			case AUDIO_ENCODING_ALAW:
				idat |= OSS_AFMT_A_LAW;
				break;
			case AUDIO_ENCODING_SLINEAR:
				idat |= OSS_AFMT_S8;
				break;
			case AUDIO_ENCODING_SLINEAR_LE:
				if (tmpenc.precision == 16)
					idat |= OSS_AFMT_S16_LE;
				else
					idat |= OSS_AFMT_S8;
				break;
			case AUDIO_ENCODING_SLINEAR_BE:
				if (tmpenc.precision == 16)
					idat |= OSS_AFMT_S16_BE;
				else
					idat |= OSS_AFMT_S8;
				break;
			case AUDIO_ENCODING_ULINEAR:
				idat |= OSS_AFMT_U8;
				break;
			case AUDIO_ENCODING_ULINEAR_LE:
				if (tmpenc.precision == 16)
					idat |= OSS_AFMT_U16_LE;
				else
					idat |= OSS_AFMT_U8;
				break;
			case AUDIO_ENCODING_ULINEAR_BE:
				if (tmpenc.precision == 16)
					idat |= OSS_AFMT_U16_BE;
				else
					idat |= OSS_AFMT_U8;
				break;
			case AUDIO_ENCODING_ADPCM:
				idat |= OSS_AFMT_IMA_ADPCM;
				break;
			default:
				break;
			}
		}
		DPRINTF(("oss_sys_ioctl: SNDCTL_DSP_GETFMTS = %x\n", idat));
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_GETOSPACE:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			goto out;
		setblocksize(fp, &tmpinfo, p);
		bufinfo.fragsize = tmpinfo.blocksize;
		bufinfo.fragments = tmpinfo.hiwat -
		    (tmpinfo.play.seek + tmpinfo.blocksize - 1) /
		    tmpinfo.blocksize;
		bufinfo.fragstotal = tmpinfo.hiwat;
		bufinfo.bytes =
		    tmpinfo.hiwat * tmpinfo.blocksize - tmpinfo.play.seek;
		error = copyout(&bufinfo, SCARG(uap, data), sizeof bufinfo);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_GETISPACE:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			goto out;
		setblocksize(fp, &tmpinfo, p);
		bufinfo.fragsize = tmpinfo.blocksize;
		bufinfo.fragments = tmpinfo.hiwat -
		    (tmpinfo.record.seek + tmpinfo.blocksize - 1) /
		    tmpinfo.blocksize;
                bufinfo.fragstotal = tmpinfo.hiwat;
		bufinfo.bytes =
		    tmpinfo.hiwat * tmpinfo.blocksize - tmpinfo.record.seek;
		DPRINTF(("oss_sys_ioctl: SNDCTL_DSP_GETxSPACE = %d %d %d %d\n",
			 bufinfo.fragsize, bufinfo.fragments, 
			 bufinfo.fragstotal, bufinfo.bytes));
		error = copyout(&bufinfo, SCARG(uap, data), sizeof bufinfo);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_NONBLOCK:
		idat = 1;
		error = ioctlf(fp, FIONBIO, (caddr_t)&idat, p);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_GETCAPS:
		error = ioctlf(fp, AUDIO_GETPROPS, (caddr_t)&idata, p);
		if (error)
			goto out;
		idat = OSS_DSP_CAP_TRIGGER; /* pretend we have trigger */
		if (idata & AUDIO_PROP_FULLDUPLEX)
			idat |= OSS_DSP_CAP_DUPLEX;
		if (idata & AUDIO_PROP_MMAP)
			idat |= OSS_DSP_CAP_MMAP;
		DPRINTF(("oss_sys_ioctl: SNDCTL_DSP_GETCAPS = %x\n", idat));
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
#if 0
	case OSS_SNDCTL_DSP_GETTRIGGER:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			goto out;
		idat = (tmpinfo.play.pause ? 0 : OSS_PCM_ENABLE_OUTPUT) |
		       (tmpinfo.record.pause ? 0 : OSS_PCM_ENABLE_INPUT);
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_SETTRIGGER:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		tmpinfo.play.pause = (idat & OSS_PCM_ENABLE_OUTPUT) == 0;
		tmpinfo.record.pause = (idat & OSS_PCM_ENABLE_INPUT) == 0;
		(void) ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			goto out;
		break;
#else
	case OSS_SNDCTL_DSP_GETTRIGGER:
	case OSS_SNDCTL_DSP_SETTRIGGER:
		/* XXX Do nothing for now. */
		idat = OSS_PCM_ENABLE_OUTPUT;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		goto out;
#endif
	case OSS_SNDCTL_DSP_GETIPTR:
		error = ioctlf(fp, AUDIO_GETIOFFS, (caddr_t)&tmpoffs, p);
		if (error)
			goto out;
		cntinfo.bytes = tmpoffs.samples;
		cntinfo.blocks = tmpoffs.deltablks;
		cntinfo.ptr = tmpoffs.offset;
		error = copyout(&cntinfo, SCARG(uap, data), sizeof cntinfo);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_GETOPTR:
		error = ioctlf(fp, AUDIO_GETOOFFS, (caddr_t)&tmpoffs, p);
		if (error)
			goto out;
		cntinfo.bytes = tmpoffs.samples;
		cntinfo.blocks = tmpoffs.deltablks;
		cntinfo.ptr = tmpoffs.offset;
		error = copyout(&cntinfo, SCARG(uap, data), sizeof cntinfo);
		if (error)
			goto out;
		break;
	case OSS_SNDCTL_DSP_MAPINBUF:
	case OSS_SNDCTL_DSP_MAPOUTBUF:
	case OSS_SNDCTL_DSP_SETSYNCRO:
	case OSS_SNDCTL_DSP_SETDUPLEX:
	case OSS_SNDCTL_DSP_PROFILE:
		error = EINVAL;
		goto out;
	default:
		error = EINVAL;
		goto out;
	}

 out:
	FILE_UNUSE(fp, p);
	return error;
}

/* If the NetBSD mixer device should have more than 32 devices
 * some will not be available to Linux */
#define NETBSD_MAXDEVS 64
struct audiodevinfo {
	int done;
	dev_t dev;
	int16_t devmap[OSS_SOUND_MIXER_NRDEVICES], 
	        rdevmap[NETBSD_MAXDEVS];
	char names[NETBSD_MAXDEVS][MAX_AUDIO_DEV_LEN];
	int enum2opaque[NETBSD_MAXDEVS];
        u_long devmask, recmask, stereomask;
	u_long caps, source;
};

static int
opaque_to_enum(struct audiodevinfo *di, audio_mixer_name_t *label, int opq)
{
	int i, o;

	for (i = 0; i < NETBSD_MAXDEVS; i++) {
		o = di->enum2opaque[i];
		if (o == opq)
			break;
		if (o == -1 && label != NULL &&
		    !strncmp(di->names[i], label->name, sizeof di->names[i])) {
			di->enum2opaque[i] = opq;
			break;
		}
	}
	if (i >= NETBSD_MAXDEVS)
		i = -1;
	/*printf("opq_to_enum %s %d -> %d\n", label->name, opq, i);*/
	return (i);
}

static int
enum_to_ord(struct audiodevinfo *di, int enm)
{
	if (enm >= NETBSD_MAXDEVS)
		return (-1);

	/*printf("enum_to_ord %d -> %d\n", enm, di->enum2opaque[enm]);*/
	return (di->enum2opaque[enm]);
}

static int
enum_to_mask(struct audiodevinfo *di, int enm)
{
	int m;
	if (enm >= NETBSD_MAXDEVS)
		return (0);

	m = di->enum2opaque[enm];
	if (m == -1)
		m = 0;
	/*printf("enum_to_mask %d -> %d\n", enm, di->enum2opaque[enm]);*/
	return (m);
}

/* 
 * Collect the audio device information to allow faster
 * emulation of the Linux mixer ioctls.  Cache the information
 * to eliminate the overhead of repeating all the ioctls needed
 * to collect the information.
 */
static struct audiodevinfo *
getdevinfo(fp, p)
	struct file *fp;
	struct proc *p;
{
	mixer_devinfo_t mi;
	int i, j, e;
	static const struct {
		const char *name;
		int code;
	} *dp, devs[] = {
		{ AudioNmicrophone,	OSS_SOUND_MIXER_MIC },
		{ AudioNline,		OSS_SOUND_MIXER_LINE },
		{ AudioNcd,		OSS_SOUND_MIXER_CD },
		{ AudioNdac,		OSS_SOUND_MIXER_PCM },
		{ AudioNrecord,		OSS_SOUND_MIXER_IMIX },
		{ AudioNmaster,		OSS_SOUND_MIXER_VOLUME },
		{ AudioNtreble,		OSS_SOUND_MIXER_TREBLE },
		{ AudioNbass,		OSS_SOUND_MIXER_BASS },
		{ AudioNspeaker,	OSS_SOUND_MIXER_SPEAKER },
/*		{ AudioNheadphone,	?? },*/
		{ AudioNoutput,		OSS_SOUND_MIXER_OGAIN },
		{ AudioNinput,		OSS_SOUND_MIXER_IGAIN },
/*		{ AudioNmaster,		OSS_SOUND_MIXER_SPEAKER },*/
/*		{ AudioNstereo,		?? },*/
/*		{ AudioNmono,		?? },*/
		{ AudioNfmsynth,	OSS_SOUND_MIXER_SYNTH },
/*		{ AudioNwave,		OSS_SOUND_MIXER_PCM },*/
		{ AudioNmidi,		OSS_SOUND_MIXER_SYNTH },
/*		{ AudioNmixerout,	?? },*/
		{ 0, -1 }
	};
	int (*ioctlf) __P((struct file *, u_long, caddr_t, struct proc *)) =
	    fp->f_ops->fo_ioctl;
	struct vnode *vp;
	struct vattr va;
	static struct audiodevinfo devcache = { 0 };
	struct audiodevinfo *di = &devcache;

	/* 
	 * Figure out what device it is so we can check if the
	 * cached data is valid.
	 */
	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VCHR)
		return 0;
	if (VOP_GETATTR(vp, &va, p->p_ucred, p))
		return 0;
	if (di->done && di->dev == va.va_rdev)
		return di;

	di->done = 1;
	di->dev = va.va_rdev;
	di->devmask = 0;
	di->recmask = 0;
	di->stereomask = 0;
	di->source = ~0;
	di->caps = 0;
	for(i = 0; i < OSS_SOUND_MIXER_NRDEVICES; i++)
		di->devmap[i] = -1;
	for(i = 0; i < NETBSD_MAXDEVS; i++) {
		di->rdevmap[i] = -1;
		di->names[i][0] = '\0';
		di->enum2opaque[i] = -1;
	}
	for(i = 0; i < NETBSD_MAXDEVS; i++) {
		mi.index = i;
		if (ioctlf(fp, AUDIO_MIXER_DEVINFO, (caddr_t)&mi, p) < 0)
			break;
		switch(mi.type) {
		case AUDIO_MIXER_VALUE:
			for(dp = devs; dp->name; dp++)
		    		if (strcmp(dp->name, mi.label.name) == 0)
					break;
			if (dp->code >= 0) {
				di->devmap[dp->code] = i;
				di->rdevmap[i] = dp->code;
				di->devmask |= 1 << dp->code;
				if (mi.un.v.num_channels == 2)
					di->stereomask |= 1 << dp->code;
				strncpy(di->names[i], mi.label.name, 
					sizeof di->names[i]);
			}
			break;
		}
	}
	for(i = 0; i < NETBSD_MAXDEVS; i++) {
		mi.index = i;
		if (ioctlf(fp, AUDIO_MIXER_DEVINFO, (caddr_t)&mi, p) < 0)
			break;
		if (strcmp(mi.label.name, AudioNsource) != 0)
			continue;
		di->source = i;
		switch(mi.type) {
		case AUDIO_MIXER_ENUM:
			for(j = 0; j < mi.un.e.num_mem; j++) {
				e = opaque_to_enum(di,
						   &mi.un.e.member[j].label,
						   mi.un.e.member[j].ord);
				if (e >= 0)
					di->recmask |= 1 << di->rdevmap[e];
			}
			di->caps = OSS_SOUND_CAP_EXCL_INPUT;
			break;
		case AUDIO_MIXER_SET:
			for(j = 0; j < mi.un.s.num_mem; j++) {
				e = opaque_to_enum(di,
						   &mi.un.s.member[j].label,
						   mi.un.s.member[j].mask);
				if (e >= 0)
					di->recmask |= 1 << di->rdevmap[e];
			}
			break;
		}
	}
	return di;
}

int
oss_ioctl_mixer(p, uap, retval)
	struct proc *p;
	struct oss_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap;
	register_t *retval;
{	       
	struct file *fp;
	struct filedesc *fdp;
	u_long com;
	struct audiodevinfo *di;
	mixer_ctrl_t mc;
	struct oss_mixer_info omi;
	struct audio_device adev;
	int idat;
	int i;
	int error;
	int l, r, n, e;
	int (*ioctlf) __P((struct file *, u_long, caddr_t, struct proc *));

	fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
		return (EBADF);

	FILE_USE(fp);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	com = SCARG(uap, com);
	DPRINTF(("oss_ioctl_mixer: com=%08lx\n", com));

	retval[0] = 0;

	di = getdevinfo(fp, p);
	if (di == 0) {
		error = EINVAL;
		goto out;
	}

	ioctlf = fp->f_ops->fo_ioctl;
	switch (com) {
	case OSS_GET_VERSION:
		idat = OSS_SOUND_VERSION;
		break;
	case OSS_SOUND_MIXER_INFO:
	case OSS_SOUND_OLD_MIXER_INFO:
		error = ioctlf(fp, AUDIO_GETDEV, (caddr_t)&adev, p);
		if (error)
			return (error);
		omi.modify_counter = 1;
		strncpy(omi.id, adev.name, sizeof omi.id);
		strncpy(omi.name, adev.name, sizeof omi.name);
		return copyout(&omi, SCARG(uap, data), OSS_IOCTL_SIZE(com));
	case OSS_SOUND_MIXER_READ_RECSRC:
		if (di->source == -1) {
			error = EINVAL;
			goto out;
		}
		mc.dev = di->source;
		if (di->caps & OSS_SOUND_CAP_EXCL_INPUT) {
			mc.type = AUDIO_MIXER_ENUM;
			error = ioctlf(fp, AUDIO_MIXER_READ, (caddr_t)&mc, p);
			if (error)
				goto out;
			e = opaque_to_enum(di, NULL, mc.un.ord);
			if (e >= 0)
				idat = 1 << di->rdevmap[e];
		} else {
			mc.type = AUDIO_MIXER_SET;
			error = ioctlf(fp, AUDIO_MIXER_READ, (caddr_t)&mc, p);
			if (error)
				goto out;
			e = opaque_to_enum(di, NULL, mc.un.mask);
			if (e >= 0)
				idat = 1 << di->rdevmap[e];
		}
		break;
	case OSS_SOUND_MIXER_READ_DEVMASK:
		idat = di->devmask;
		break;
	case OSS_SOUND_MIXER_READ_RECMASK:
		idat = di->recmask;
		break;
	case OSS_SOUND_MIXER_READ_STEREODEVS:
		idat = di->stereomask;
		break;
	case OSS_SOUND_MIXER_READ_CAPS:
		idat = di->caps;
		break;
	case OSS_SOUND_MIXER_WRITE_RECSRC:
	case OSS_SOUND_MIXER_WRITE_R_RECSRC:
		if (di->source == -1) {
			error = EINVAL;
			goto out;
		}
		mc.dev = di->source;
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		if (di->caps & OSS_SOUND_CAP_EXCL_INPUT) {
			mc.type = AUDIO_MIXER_ENUM;
			for(i = 0; i < OSS_SOUND_MIXER_NRDEVICES; i++)
				if (idat & (1 << i))
					break;
			if (i >= OSS_SOUND_MIXER_NRDEVICES ||
			    di->devmap[i] == -1)
				return EINVAL;
			mc.un.ord = enum_to_ord(di, di->devmap[i]);
		} else {
			mc.type = AUDIO_MIXER_SET;
			mc.un.mask = 0;
			for(i = 0; i < OSS_SOUND_MIXER_NRDEVICES; i++) {
				if (idat & (1 << i)) {
					if (di->devmap[i] == -1)
						return EINVAL;
					mc.un.mask |= enum_to_mask(di, di->devmap[i]);
				}
			}
		}
		error = ioctlf(fp, AUDIO_MIXER_WRITE, (caddr_t)&mc, p);
		goto out;
	default:
		if (OSS_MIXER_READ(OSS_SOUND_MIXER_FIRST) <= com &&
		    com < OSS_MIXER_READ(OSS_SOUND_MIXER_NRDEVICES)) {
			n = OSS_GET_DEV(com);
			if (di->devmap[n] == -1) {
				error = EINVAL;
				goto out;
			}
		    doread:
			mc.dev = di->devmap[n];
			mc.type = AUDIO_MIXER_VALUE;
			mc.un.value.num_channels = di->stereomask & (1<<n) ? 2 : 1;
			error = ioctlf(fp, AUDIO_MIXER_READ, (caddr_t)&mc, p);
			if (error)
				goto out;
			if (mc.un.value.num_channels != 2) {
				l = r = mc.un.value.level[AUDIO_MIXER_LEVEL_MONO];
			} else {
				l = mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT];
				r = mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
			}
			idat = TO_OSSVOL(l) | (TO_OSSVOL(r) << 8);
			DPRINTF(("OSS_MIXER_READ  n=%d (dev=%d) l=%d, r=%d, idat=%04x\n", 
				 n, di->devmap[n], l, r, idat));
			break;
		} else if ((OSS_MIXER_WRITE_R(OSS_SOUND_MIXER_FIRST) <= com &&
			   com < OSS_MIXER_WRITE_R(OSS_SOUND_MIXER_NRDEVICES)) ||
			   (OSS_MIXER_WRITE(OSS_SOUND_MIXER_FIRST) <= com &&
			   com < OSS_MIXER_WRITE(OSS_SOUND_MIXER_NRDEVICES))) {
			n = OSS_GET_DEV(com);
			if (di->devmap[n] == -1) {
				error = EINVAL;
				goto out;
			}
			error = copyin(SCARG(uap, data), &idat, sizeof idat);
			if (error)
				goto out;
			l = FROM_OSSVOL( idat       & 0xff);
			r = FROM_OSSVOL((idat >> 8) & 0xff);
			mc.dev = di->devmap[n];
			mc.type = AUDIO_MIXER_VALUE;
			if (di->stereomask & (1<<n)) {
				mc.un.value.num_channels = 2;
				mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT] = l;
				mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = r;
			} else {
				mc.un.value.num_channels = 1;
				mc.un.value.level[AUDIO_MIXER_LEVEL_MONO] = (l+r)/2;
			}
			DPRINTF(("OSS_MIXER_WRITE n=%d (dev=%d) l=%d, r=%d, idat=%04x\n", 
				 n, di->devmap[n], l, r, idat));
			error = ioctlf(fp, AUDIO_MIXER_WRITE, (caddr_t)&mc, p);
			if (error)
				goto out;
			if (OSS_MIXER_WRITE(OSS_SOUND_MIXER_FIRST) <= com &&
			   com < OSS_MIXER_WRITE(OSS_SOUND_MIXER_NRDEVICES)) {
				error = 0;
				goto out;
			}
			goto doread;
		} else {
#ifdef AUDIO_DEBUG
			printf("oss_audio: unknown mixer ioctl %04lx\n", com);
#endif
			error = EINVAL;
			goto out;
		}
	}
	error = copyout(&idat, SCARG(uap, data), sizeof idat);
 out:
	FILE_UNUSE(fp, p);
	return error;
}

/* Sequencer emulation */
int
oss_ioctl_sequencer(p, uap, retval)
	struct proc *p;
	struct oss_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap;
	register_t *retval;
{	       
	struct file *fp;
	struct filedesc *fdp;
	u_long com;
	int idat, idat1;
	struct synth_info si;
	struct oss_synth_info osi;
	struct oss_seq_event_rec oser;
	int error;
	int (*ioctlf) __P((struct file *, u_long, caddr_t, struct proc *));

	fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
		return (EBADF);

	FILE_USE(fp);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	com = SCARG(uap, com);
	DPRINTF(("oss_ioctl_sequencer: com=%08lx\n", com));

	retval[0] = 0;

	ioctlf = fp->f_ops->fo_ioctl;
	switch (com) {
	case OSS_SEQ_RESET:
		error = ioctlf(fp, SEQUENCER_RESET, (caddr_t)&idat, p);
		goto out;
	case OSS_SEQ_SYNC:
		error = ioctlf(fp, SEQUENCER_SYNC, (caddr_t)&idat, p);
		goto out;
	case OSS_SYNTH_INFO:
		error = copyin(SCARG(uap, data), &osi, sizeof osi);
		if (error)
			goto out;
		si.device = osi.device;
		error = ioctlf(fp, SEQUENCER_INFO, (caddr_t)&si, p);
		if (error)
			goto out;
		strncpy(osi.name, si.name, sizeof osi.name);
		osi.device = si.device;
		switch(si.synth_type) {
		case SYNTH_TYPE_FM: 
			osi.synth_type = OSS_SYNTH_TYPE_FM; break;
		case SYNTH_TYPE_SAMPLE: 
			osi.synth_type = OSS_SYNTH_TYPE_SAMPLE; break;
		case SYNTH_TYPE_MIDI: 
			osi.synth_type = OSS_SYNTH_TYPE_MIDI; break;
		default:
			osi.synth_type = 0; break;
		}
		switch(si.synth_subtype) {
		case SYNTH_SUB_FM_TYPE_ADLIB: 
			osi.synth_subtype = OSS_FM_TYPE_ADLIB; break;
		case SYNTH_SUB_FM_TYPE_OPL3: 
			osi.synth_subtype = OSS_FM_TYPE_OPL3; break;
		case SYNTH_SUB_MIDI_TYPE_MPU401: 
			osi.synth_subtype = OSS_MIDI_TYPE_MPU401; break;
		case SYNTH_SUB_SAMPLE_TYPE_BASIC: 
			osi.synth_subtype = OSS_SAMPLE_TYPE_BASIC; break;
		default:
			osi.synth_subtype = 0; break;
		}
		osi.perc_mode = 0;
		osi.nr_voices = si.nr_voices;
		osi.nr_drums = 0;
		osi.instr_bank_size = si.instr_bank_size;
		osi.capabilities = 0;
		if (si.capabilities & SYNTH_CAP_OPL3) 
			osi.capabilities |= OSS_SYNTH_CAP_OPL3;
		if (si.capabilities & SYNTH_CAP_INPUT)
			osi.capabilities |= OSS_SYNTH_CAP_INPUT;
		error = copyout(&osi, SCARG(uap, data), sizeof osi);
		goto out;
	case OSS_SEQ_CTRLRATE:
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		error = ioctlf(fp, SEQUENCER_CTRLRATE, (caddr_t)&idat, p);
		if (error)
			goto out;
		retval[0] = idat;
		break;
	case OSS_SEQ_GETOUTCOUNT:
		error = ioctlf(fp, SEQUENCER_GETOUTCOUNT, (caddr_t)&idat, p);
		if (error)
			goto out;
		retval[0] = idat;
		break;
	case OSS_SEQ_GETINCOUNT:
		error = ioctlf(fp, SEQUENCER_GETINCOUNT, (caddr_t)&idat, p);
		if (error)
			goto out;
		retval[0] = idat;
		break;
	case OSS_SEQ_NRSYNTHS:
		error = ioctlf(fp, SEQUENCER_NRSYNTHS, (caddr_t)&idat, p);
		if (error)
			goto out;
		retval[0] = idat;
		break;
	case OSS_SEQ_NRMIDIS:
		error = ioctlf(fp, SEQUENCER_NRMIDIS, (caddr_t)&idat, p);
		if (error)
			goto out;
		retval[0] = idat;
		break;
	case OSS_SEQ_THRESHOLD:
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		error = ioctlf(fp, SEQUENCER_THRESHOLD, (caddr_t)&idat, p);
		goto out;
	case OSS_MEMAVL:
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		error = ioctlf(fp, SEQUENCER_MEMAVL, (caddr_t)&idat, p);
		if (error)
			goto out;
		retval[0] = idat;
		break;
	case OSS_SEQ_PANIC:
		error = ioctlf(fp, SEQUENCER_PANIC, (caddr_t)&idat, p);
		goto out;
	case OSS_SEQ_OUTOFBAND:
		error = copyin(SCARG(uap, data), &oser, sizeof oser);
		if (error)
			goto out;
		error = ioctlf(fp, SEQUENCER_OUTOFBAND, (caddr_t)&oser, p);
		if (error)
			goto out;
		break;
	case OSS_SEQ_GETTIME:
		error = ioctlf(fp, SEQUENCER_GETTIME, (caddr_t)&idat, p);
		if (error)
			goto out;
		retval[0] = idat;
		break;
	case OSS_TMR_TIMEBASE:
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		error = ioctlf(fp, SEQUENCER_TMR_TIMEBASE, (caddr_t)&idat, p);
		if (error)
			goto out;
		retval[0] = idat;
		break;
	case OSS_TMR_START:
		error = ioctlf(fp, SEQUENCER_TMR_START, (caddr_t)&idat, p);
		goto out;
	case OSS_TMR_STOP:
		error = ioctlf(fp, SEQUENCER_TMR_STOP, (caddr_t)&idat, p);
		goto out;
	case OSS_TMR_CONTINUE:
		error = ioctlf(fp, SEQUENCER_TMR_CONTINUE, (caddr_t)&idat, p);
		goto out;
	case OSS_TMR_TEMPO:
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		error = ioctlf(fp, SEQUENCER_TMR_TEMPO, (caddr_t)&idat, p);
		if (error)
			goto out;
		retval[0] = idat;
		break;
	case OSS_TMR_SOURCE:
		error = copyin(SCARG(uap, data), &idat1, sizeof idat);
		if (error)
			goto out;
		idat = 0;
		if (idat1 & OSS_TMR_INTERNAL) idat |= SEQUENCER_TMR_INTERNAL;
		error = ioctlf(fp, SEQUENCER_TMR_SOURCE, (caddr_t)&idat, p);
		if (error)
			goto out;
		idat1 = idat;
		if (idat1 & SEQUENCER_TMR_INTERNAL) idat |= OSS_TMR_INTERNAL;
		retval[0] = idat;
		break;
	case OSS_TMR_METRONOME:
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		error = ioctlf(fp, SEQUENCER_TMR_METRONOME, (caddr_t)&idat, p);
		goto out;
	case OSS_TMR_SELECT:
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			goto out;
		retval[0] = idat;
		error = ioctlf(fp, SEQUENCER_TMR_SELECT, (caddr_t)&idat, p);
		goto out;
	default:
		error = EINVAL;
		goto out;
	}

	error = copyout(&idat, SCARG(uap, data), sizeof idat);
 out:
	FILE_UNUSE(fp, p);
	return error;
}

/*
 * Check that the blocksize is a power of 2 as OSS wants.
 * If not, set it to be.
 */
static void
setblocksize(fp, info, p)
	struct file *fp;
	struct audio_info *info;
	struct proc *p;
{
	struct audio_info set;
	int s;

	if (info->blocksize & (info->blocksize-1)) {
		for(s = 32; s < info->blocksize; s <<= 1)
			;
		AUDIO_INITINFO(&set);
		set.blocksize = s;
		fp->f_ops->fo_ioctl(fp, AUDIO_SETINFO, (caddr_t)&set, p);
		fp->f_ops->fo_ioctl(fp, AUDIO_GETINFO, (caddr_t)info, p);
	}
}
