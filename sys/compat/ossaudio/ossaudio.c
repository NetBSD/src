/*	$NetBSD: ossaudio.c,v 1.6 1997/04/04 15:36:01 augustss Exp $	*/
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/audioio.h>

#include <sys/syscallargs.h>

#include <compat/ossaudio/ossaudio.h>
#include <compat/ossaudio/ossaudiovar.h>

static struct audiodevinfo *getdevinfo __P((struct file *, struct proc *));

int
oss_ioctl_audio(p, uap, retval)
	register struct proc *p;
	register struct oss_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap;
	register_t *retval;
{	       
	register struct file *fp;
	register struct filedesc *fdp;
	u_long com;
	struct audio_info tmpinfo;
	int idat;
	int error;
	int (*ioctlf) __P((struct file *, u_long, caddr_t, struct proc *));

	fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL)
		return (EBADF);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0)
		return (EBADF);

	ioctlf = fp->f_ops->fo_ioctl;

	com = SCARG(uap, com);
	retval[0] = 0;

	switch (com) {
	case OSS_SNDCTL_DSP_RESET:
		error = ioctlf(fp, AUDIO_FLUSH, (caddr_t)0, p);
		if (error)
			return error;
		break;
	case OSS_SNDCTL_DSP_SYNC:
	case OSS_SNDCTL_DSP_POST:
		error = ioctlf(fp, AUDIO_DRAIN, (caddr_t)0, p);
		if (error)
			return error;
		break;
	case OSS_SNDCTL_DSP_SPEED:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			return error;
		tmpinfo.play.sample_rate =
		tmpinfo.record.sample_rate = idat;
		(void) ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		/* fall into ... */
	case OSS_SOUND_PCM_READ_RATE:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			return error;
		idat = tmpinfo.play.sample_rate;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			return error;
		break;
	case OSS_SNDCTL_DSP_STEREO:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			return error;
		tmpinfo.play.channels =
		tmpinfo.record.channels = idat ? 2 : 1;
		(void) ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			return error;
		idat = tmpinfo.play.channels - 1;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			return error;
		break;
	case OSS_SNDCTL_DSP_GETBLKSIZE:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			return error;
		idat = tmpinfo.blocksize;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			return error;
		break;
	case OSS_SNDCTL_DSP_SETFMT:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			return error;
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
			tmpinfo.record.encoding = AUDIO_ENCODING_LINEAR;
			break;
		case OSS_AFMT_S16_LE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 16;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_LINEAR;
			break;
		default:
			return EINVAL;
		}
		(void) ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		/* fall into ... */
	case OSS_SOUND_PCM_READ_BITS:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			return error;
		switch (tmpinfo.play.encoding) {
		case AUDIO_ENCODING_ULAW:
			idat = OSS_AFMT_MU_LAW;
			break;
		case AUDIO_ENCODING_ALAW:
			idat = OSS_AFMT_A_LAW;
			break;
		case AUDIO_ENCODING_PCM16:
			idat = OSS_AFMT_S16_LE;
			break;
		case AUDIO_ENCODING_PCM8:
			idat = OSS_AFMT_U8;
			break;
		case AUDIO_ENCODING_ADPCM:
			idat = OSS_AFMT_IMA_ADPCM;
			break;
		}
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			return error;
		break;
	case OSS_SNDCTL_DSP_CHANNELS:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			return error;
		tmpinfo.play.channels =
		tmpinfo.record.channels = idat;
		(void) ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		/* fall into ... */
	case OSS_SOUND_PCM_READ_CHANNELS:
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			return error;
		idat = tmpinfo.play.channels;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			return error;
		break;
	case OSS_SOUND_PCM_WRITE_FILTER:
	case OSS_SOUND_PCM_READ_FILTER:
		return EINVAL; /* XXX unimplemented */
	case OSS_SNDCTL_DSP_SUBDIVIDE:
		return EINVAL; /* XXX unimplemented */
	case OSS_SNDCTL_DSP_SETFRAGMENT:
		AUDIO_INITINFO(&tmpinfo);
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			return error;
		if ((idat & 0xffff) < 4 || (idat & 0xffff) > 17)
			return EINVAL;
		tmpinfo.blocksize = 1 << (idat & 0xffff);
		tmpinfo.hiwat = (idat >> 16) & 0xffff;
		(void) ioctlf(fp, AUDIO_SETINFO, (caddr_t)&tmpinfo, p);
		error = ioctlf(fp, AUDIO_GETINFO, (caddr_t)&tmpinfo, p);
		if (error)
			return error;
		idat = tmpinfo.blocksize;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			return error;
		break;
	case OSS_SNDCTL_DSP_GETFMTS:
		idat = OSS_AFMT_MU_LAW | OSS_AFMT_U8 | OSS_AFMT_S16_LE;
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			return error;
		break;
	case OSS_SNDCTL_DSP_GETOSPACE:
	case OSS_SNDCTL_DSP_GETISPACE:
	case OSS_SNDCTL_DSP_NONBLOCK:
		return EINVAL; /* XXX unimplemented */
	case OSS_SNDCTL_DSP_GETCAPS:
		idat = 0; /* XXX full duplex info should be set */
		error = copyout(&idat, SCARG(uap, data), sizeof idat);
		if (error)
			return error;
		break;
	case OSS_SNDCTL_DSP_GETTRIGGER:
	case OSS_SNDCTL_DSP_SETTRIGGER:
	case OSS_SNDCTL_DSP_GETIPTR:
	case OSS_SNDCTL_DSP_GETOPTR:
	case OSS_SNDCTL_DSP_MAPINBUF:
	case OSS_SNDCTL_DSP_MAPOUTBUF:
	case OSS_SNDCTL_DSP_SETSYNCRO:
	case OSS_SNDCTL_DSP_SETDUPLEX:
	case OSS_SNDCTL_DSP_PROFILE:
		return EINVAL; /* XXX unimplemented */
	default:
		return EINVAL;
	}

	return 0;
}

/* If the NetBSD mixer device should have more than 32 devices
 * some will not be available to Linux */
#define NETBSD_MAXDEVS 32
struct audiodevinfo {
	int done;
	dev_t dev;
	u_int16_t devmap[OSS_SOUND_MIXER_NRDEVICES], 
	          rdevmap[NETBSD_MAXDEVS];
        u_long devmask, recmask, stereomask;
	u_long caps, source;
};

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
	int i;
	static struct {
		char *name;
		int code;
	} *dp, devs[] = {
		{ AudioNmicrophone,	OSS_SOUND_MIXER_MIC },
		{ AudioNline,		OSS_SOUND_MIXER_LINE },
		{ AudioNcd,		OSS_SOUND_MIXER_CD },
		{ AudioNdac,		OSS_SOUND_MIXER_PCM },
		{ AudioNrecord,		OSS_SOUND_MIXER_IMIX },
		{ AudioNvolume,		OSS_SOUND_MIXER_VOLUME },
		{ AudioNtreble,		OSS_SOUND_MIXER_TREBLE },
		{ AudioNbass,		OSS_SOUND_MIXER_BASS },
		{ AudioNspeaker,	OSS_SOUND_MIXER_SPEAKER },
/*		{ AudioNheadphone,	?? },*/
		{ AudioNoutput,		OSS_SOUND_MIXER_OGAIN },
		{ AudioNinput,		OSS_SOUND_MIXER_IGAIN },
/*		{ AudioNmaster,		OSS_SOUND_MIXER_MASTER },*/
/*		{ AudioNstereo,		?? },*/
/*		{ AudioNmono,		?? },*/
/*		{ AudioNsource,		?? },*/
		{ AudioNfmsynth,	OSS_SOUND_MIXER_SYNTH },
/*		{ AudioNwave,		OSS_SOUND_MIXER_PCM },*/
/*		{ AudioNmidi,		?? },*/
/*		{ AudioNmixerout,	?? },*/
		{ 0, -1 }
	};
	int (*ioctlf) __P((struct file *, u_long, caddr_t, struct proc *)) =
	    fp->f_ops->fo_ioctl;
	struct vnode *vp;
	struct vattr va;
	static struct audiodevinfo devcache = { 0 };
	struct audiodevinfo *di = &devcache;

	/* Figure out what device it is so we can check if the
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
	di->source = -1;
	di->caps = 0;
	for(i = 0; i < OSS_SOUND_MIXER_NRDEVICES; i++)
		di->devmap[i] = -1;
	for(i = 0; i < NETBSD_MAXDEVS; i++)
		di->rdevmap[i] = -1;
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
			}
			break;
		case AUDIO_MIXER_ENUM:
			if (strcmp(mi.label.name, AudioNsource) == 0) {
				int j;
				di->source = i;
				for(j = 0; j < mi.un.e.num_mem; j++) {
					di->recmask |= 1 << di->rdevmap[mi.un.e.member[j].ord];
				}
				di->caps = OSS_SOUND_CAP_EXCL_INPUT;
			}
			break;
		case AUDIO_MIXER_SET:
			if (strcmp(mi.label.name, AudioNsource) == 0) {
				int j;
				di->source = i;
				for(j = 0; j < mi.un.s.num_mem; j++) {
					int k, mask = mi.un.s.member[j].mask;
					if (mask) {
						for(k = 0; !(mask & (1<<k)); k++)
							;
						di->recmask |= 1 << di->rdevmap[k];
					}
				}
			}
			break;
		}
	}
	return di;
}

int
oss_ioctl_mixer(p, uap, retval)
	register struct proc *p;
	register struct oss_sys_ioctl_args /* {
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
	int idat;
	int i;
	int error;
	int l, r, n;
	int (*ioctlf) __P((struct file *, u_long, caddr_t, struct proc *));

	fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL)
		return (EBADF);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0)
		return (EBADF);

	com = SCARG(uap, com);
	retval[0] = 0;

	di = getdevinfo(fp, p);
	if (di == 0)
		return EINVAL;

	ioctlf = fp->f_ops->fo_ioctl;
	switch (com) {
	case OSS_SOUND_MIXER_READ_RECSRC:
		if (di->source == -1)
			return EINVAL;
		mc.dev = di->source;
		if (di->caps & OSS_SOUND_CAP_EXCL_INPUT) {
			mc.type = AUDIO_MIXER_ENUM;
			error = ioctlf(fp, AUDIO_MIXER_READ, (caddr_t)&mc, p);
			if (error)
				return error;
			idat = 1 << di->rdevmap[mc.un.ord];
		} else {
			int k;
			unsigned int mask;
			mc.type = AUDIO_MIXER_SET;
			error = ioctlf(fp, AUDIO_MIXER_READ, (caddr_t)&mc, p);
			if (error)
				return error;
			idat = 0;
			for(mask = mc.un.mask, k = 0; mask; mask >>= 1, k++)
				idat |= 1 << di->rdevmap[k];
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
		if (di->source == -1)
			return EINVAL;
		mc.dev = di->source;
		mc.type = AUDIO_MIXER_ENUM;
		error = copyin(SCARG(uap, data), &idat, sizeof idat);
		if (error)
			return error;
		for(i = 0; i < OSS_SOUND_MIXER_NRDEVICES; i++)
			if (idat & (1 << i))
				mc.un.ord = di->devmap[i];
		return ioctlf(fp, AUDIO_MIXER_READ, (caddr_t)&mc, p);
	default:
		if (OSS_MIXER_READ(OSS_SOUND_MIXER_FIRST) <= com &&
		    com < OSS_MIXER_READ(OSS_SOUND_MIXER_NRDEVICES)) {
			n = OSS_GET_DEV(com);
			mc.dev = di->devmap[n];
			mc.type = AUDIO_MIXER_VALUE;
		    doread:
			mc.un.value.num_channels = di->stereomask & (1<<n) ? 2 : 1;
			error = ioctlf(fp, AUDIO_MIXER_READ, (caddr_t)&mc, p);
			if (error)
				return error;
			if (mc.type != AUDIO_MIXER_VALUE)
				return EINVAL;
			if (mc.un.value.num_channels != 2) {
				l = r = mc.un.value.level[AUDIO_MIXER_LEVEL_MONO];
			} else {
				l = mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT];
				r = mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
			}
			l = l * 100 / 255;
			r = r * 100 / 255;
			idat = l | (r << 8);
			break;
		} else if (OSS_MIXER_WRITE(OSS_SOUND_MIXER_FIRST) <= com &&
			   com < OSS_MIXER_WRITE(OSS_SOUND_MIXER_NRDEVICES)) {
			n = OSS_GET_DEV(com);
			error = copyin(SCARG(uap, data), &idat, sizeof idat);
			if (error)
				return error;
			l = (idat & 0xff) * 255 / 100;
			r = (idat >> 8) * 255 / 100;
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
			error = ioctlf(fp, AUDIO_MIXER_WRITE, (caddr_t)&mc, p);
			if (error)
				return error;
			goto doread;
		} else
			return EINVAL;
	}
	return copyout(&idat, SCARG(uap, data), sizeof idat);
}

/* XXX hook for sequencer emulation */
int
oss_ioctl_sequencer(p, uap, retval)
	register struct proc *p;
	register struct oss_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap;
	register_t *retval;
{	       
	register struct file *fp;
	register struct filedesc *fdp;
#if 0
	u_long com;
	int idat;
	int error;
#endif

	fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL)
		return (EBADF);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0)
		return (EBADF);

#if 0
	com = SCARG(uap, com);
#endif
	retval[0] = 0;

	return EINVAL;
}
