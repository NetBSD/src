/*	$NetBSD: oss4_mixer.c,v 1.1 2021/06/08 18:43:54 nia Exp $	*/

/*-
 * Copyright (c) 2020-2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nia Alarie.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
#include <sys/audioio.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include "internal.h"

static int get_audio_count(void);
static int get_mixer_count(void);
static int get_mixer_control_count(int);

oss_private int
_oss4_mixer_ioctl(int fd, unsigned long com, void *argp)
{
	oss_audioinfo *tmpai;
	oss_card_info *cardinfo;
	oss_mixext *ext;
	oss_mixext_root root;
	oss_mixer_enuminfo *ei;
	oss_mixer_value *mv;
	oss_mixerinfo *mi;
	oss_sysinfo sysinfo;
	dev_t devno;
	struct stat tmpstat;
	struct audio_device dev;
	struct audio_format_query fmtq;
	struct mixer_devinfo mdi;
	struct mixer_ctrl mc;
	char devname[32];
	size_t len;
	int newfd = -1, tmperrno;
	int i, noffs;
	int retval;

	/*
	 * Note: it is difficult to translate the NetBSD concept of a "set"
	 * mixer control type to the OSSv4 API, as far as I can tell.
	 *
	 * This means they are treated like enums, i.e. only one entry in the
	 * set can be selected at a time.
	 */

	switch (com) {
	case SNDCTL_AUDIOINFO:
	/*
	 * SNDCTL_AUDIOINFO_EX is intended for underlying hardware devices
	 * that are to be opened in "exclusive mode" (bypassing the normal
	 * kernel mixer for exclusive control). NetBSD does not support
	 * bypassing the kernel mixer, so it's an alias of SNDCTL_AUDIOINFO.
	 */
	case SNDCTL_AUDIOINFO_EX:
	case SNDCTL_ENGINEINFO:
		devno = 0;
		tmpai = (struct oss_audioinfo*)argp;
		if (tmpai == NULL) {
			errno = EINVAL;
			return -1;
		}

		/*
		 * If the input device is -1, guess the device related to
		 * the open mixer device.
		 */
		if (tmpai->dev < 0) {
			fstat(fd, &tmpstat);
			if ((tmpstat.st_rdev & 0xff00) == 0x2a00)
				devno = tmpstat.st_rdev & 0xff;
			if (devno >= 0x80)
				tmpai->dev = devno & 0x7f;
		}
		if (tmpai->dev < 0)
			tmpai->dev = 0;

		snprintf(tmpai->devnode, sizeof(tmpai->devnode),
		    "/dev/audio%d", tmpai->dev);

		if ((newfd = open(tmpai->devnode, O_WRONLY)) < 0) {
			if ((newfd = open(tmpai->devnode, O_RDONLY)) < 0) {
				return newfd;
			}
		}

		retval = ioctl(newfd, AUDIO_GETDEV, &dev);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		if (_oss_get_caps(newfd, &tmpai->caps) < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		snprintf(tmpai->name, sizeof(tmpai->name),
		    "%s %s", dev.name, dev.version);
		tmpai->busy = 0;
		tmpai->pid = -1;
		_oss_dsp_ioctl(newfd, SNDCTL_DSP_GETFMTS, &tmpai->iformats);
		tmpai->oformats = tmpai->iformats;
		tmpai->magic = -1; /* reserved for "internal use" */
		memset(tmpai->cmd, 0, sizeof(tmpai->cmd));
		tmpai->card_number = -1;
		memset(tmpai->song_name, 0,
		    sizeof(tmpai->song_name));
		memset(tmpai->label, 0, sizeof(tmpai->label));
		tmpai->port_number = 0;
		tmpai->mixer_dev = tmpai->dev;
		tmpai->legacy_device = tmpai->dev;
		tmpai->enabled = 1;
		tmpai->flags = -1; /* reserved for "future versions" */
		tmpai->min_rate = 1000;
		tmpai->max_rate = 192000;
		tmpai->nrates = 0;
		tmpai->min_channels = 1;
		tmpai->max_channels = 2;
		for (fmtq.index = 0;
		    ioctl(newfd, AUDIO_QUERYFORMAT, &fmtq) != -1; ++fmtq.index) {
			if (fmtq.fmt.channels > (unsigned)tmpai->max_channels)
				tmpai->max_channels = fmtq.fmt.channels;
		}
		tmpai->binding = -1; /* reserved for "future versions" */
		tmpai->rate_source = -1;
		/*
		 * 'handle' is supposed to be globally unique. The closest
		 * we have to that is probably device nodes.
		 */
		strlcpy(tmpai->handle, tmpai->devnode,
		    sizeof(tmpai->handle));
		tmpai->next_play_engine = 0;
		tmpai->next_rec_engine = 0;
		argp = tmpai;
		close(newfd);
		break;
	case SNDCTL_CARDINFO:
		cardinfo = (oss_card_info *)argp;
		if (cardinfo == NULL) {
			errno = EINVAL;
			return -1;
		}
		if (cardinfo->card != -1) {
			snprintf(devname, sizeof(devname),
			    "/dev/audio%d", cardinfo->card);
			newfd = open(devname, O_RDONLY);
			if (newfd < 0)
				return newfd;
		} else {
			newfd = fd;
		}
		retval = ioctl(newfd, AUDIO_GETDEV, &dev);
		tmperrno = errno;
		if (newfd != fd)
			close(newfd);
		if (retval < 0) {
			errno = tmperrno;
			return retval;
		}
		strlcpy(cardinfo->shortname, dev.name,
		    sizeof(cardinfo->shortname));
		snprintf(cardinfo->longname, sizeof(cardinfo->longname),
		    "%s %s %s", dev.name, dev.version, dev.config);
		memset(cardinfo->hw_info, 0, sizeof(cardinfo->hw_info));
		/*
		 * OSSv4 does not document this ioctl, and claims it should
		 * not be used by applications and is provided for "utiltiy
		 * programs included in OSS". We follow the Solaris
		 * implementation (which is documented) and leave these fields
		 * unset.
		 */
		cardinfo->flags = 0;
		cardinfo->intr_count = 0;
		cardinfo->ack_count = 0;
		break;
	case SNDCTL_SYSINFO:
		memset(&sysinfo, 0, sizeof(sysinfo));
		strlcpy(sysinfo.product,
		    "OSS/NetBSD", sizeof(sysinfo.product));
		strlcpy(sysinfo.version,
		    "4.01", sizeof(sysinfo.version));
		strlcpy(sysinfo.license,
		    "BSD", sizeof(sysinfo.license));
		sysinfo.versionnum = SOUND_VERSION;
		sysinfo.numaudios = 
		    sysinfo.numcards =
			get_audio_count();
		sysinfo.numaudioengines = 1;
		sysinfo.numsynths = 1;
		sysinfo.nummidis = -1;
		sysinfo.numtimers = -1;
		sysinfo.nummixers = get_mixer_count();
		*(struct oss_sysinfo *)argp = sysinfo;
		break;
	case SNDCTL_MIXERINFO:
		mi = (oss_mixerinfo *)argp;
		if (mi == NULL) {
			errno = EINVAL;
			return -1;
		}
		snprintf(devname, sizeof(devname), "/dev/mixer%d", mi->dev);
		if ((newfd = open(devname, O_RDONLY)) < 0)
			return newfd;
		retval = ioctl(newfd, AUDIO_GETDEV, &dev);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		strlcpy(mi->id, devname, sizeof(mi->id));
		strlcpy(mi->handle, devname, sizeof(mi->handle));
		snprintf(mi->name, sizeof(mi->name),
		    "%s %s", dev.name, dev.version);
		mi->card_number = mi->dev;
		mi->port_number = 0;
		mi->magic = 0;
		mi->enabled = 1;
		mi->caps = 0;
		mi->flags = 0;
		mi->nrext = get_mixer_control_count(newfd) + 1;
		mi->priority = UCHAR_MAX - mi->dev;
		strlcpy(mi->devnode, devname, sizeof(mi->devnode));
		mi->legacy_device = mi->dev;
		break;
	case SNDCTL_MIX_DESCRIPTION:
		/* No description available. */
		errno = ENOSYS;
		return -1;
	case SNDCTL_MIX_NRMIX:
		INTARG = get_mixer_count();
		break;
	case SNDCTL_MIX_NREXT:
		snprintf(devname, sizeof(devname), "/dev/mixer%d", INTARG);
		if ((newfd = open(devname, O_RDONLY)) < 0)
			return newfd;
		INTARG = get_mixer_control_count(newfd) + 1;
		close(newfd);
		break;
	case SNDCTL_MIX_EXTINFO:
		ext = (oss_mixext *)argp;
		snprintf(devname, sizeof(devname), "/dev/mixer%d", ext->dev);
		if ((newfd = open(devname, O_RDONLY)) < 0)
			return newfd;
		if (ext->ctrl == 0) {
			/*
			 * NetBSD has no concept of a "root mixer control", but
 			 * OSSv4 requires one to work. We fake one at 0 and
			 * simply add 1 to all real control indexes.
			 */
			retval = ioctl(newfd, AUDIO_GETDEV, &dev);
			tmperrno = errno;
			close(newfd);
			if (retval < 0) {
				errno = tmperrno;
				return -1;
			}
			memset(&root, 0, sizeof(root));
			strlcpy(root.id, devname, sizeof(root.id));
			snprintf(root.name, sizeof(root.name),
			    "%s %s", dev.name, dev.version);
			strlcpy(ext->id, devname, sizeof(ext->id));
			snprintf(ext->extname, sizeof(ext->extname),
			    "%s %s", dev.name, dev.version);
			strlcpy(ext->extname, "root", sizeof(ext->extname));
			ext->type = MIXT_DEVROOT;
			ext->minvalue = 0;
			ext->maxvalue = 0;
			ext->flags = 0;
			ext->parent = -1;
			ext->control_no = -1;
			ext->update_counter = 0;
			ext->rgbcolor = 0;
			memcpy(&ext->data, &root,
			    sizeof(root) > sizeof(ext->data) ?
			    sizeof(ext->data) : sizeof(root));
			return 0;
		}
		mdi.index = ext->ctrl - 1;
		retval = ioctl(newfd, AUDIO_MIXER_DEVINFO, &mdi);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		ext->flags = MIXF_READABLE | MIXF_WRITEABLE | MIXF_POLL;
		ext->parent = mdi.mixer_class + 1;
		strlcpy(ext->id, mdi.label.name, sizeof(ext->id));
		strlcpy(ext->extname, mdi.label.name, sizeof(ext->extname));
		len = strlen(ext->extname);
		memset(ext->data, 0, sizeof(ext->data));
		ext->control_no = -1;
		ext->update_counter = 0;
		ext->rgbcolor = 0;
		switch (mdi.type) {
		case AUDIO_MIXER_CLASS:
			ext->type = MIXT_GROUP;
			ext->parent = 0;
			ext->minvalue = 0;
			ext->maxvalue = 0;
			break;
		case AUDIO_MIXER_ENUM:
			ext->maxvalue = mdi.un.e.num_mem;
			ext->minvalue = 0;
			for (i = 0; i < mdi.un.e.num_mem; ++i) {
				ext->enum_present[i / 8] |= (1 << (i % 8));
			}
			if (mdi.un.e.num_mem == 2) {
				if (!strcmp(mdi.un.e.member[0].label.name, AudioNoff) &&
				    !strcmp(mdi.un.e.member[1].label.name, AudioNon)) {
					ext->type = MIXT_MUTE;
				} else {
					ext->type = MIXT_ENUM;
				}
			} else {
				ext->type = MIXT_ENUM;
			}
			break;
		case AUDIO_MIXER_SET:
			ext->maxvalue = mdi.un.s.num_mem;
			ext->minvalue = 0;
#ifdef notyet
			/*
			 * XXX: This is actually the correct type for "set"
			 * controls, but it seems no real world software
			 * supports it. The only documentation exists in
			 * the OSSv4 headers and describes it as "reserved
			 * for Sun's implementation".
			 */
			ext->type = MIXT_ENUM_MULTI;
#else
			ext->type = MIXT_ENUM;
#endif
			for (i = 0; i < mdi.un.s.num_mem; ++i) {
				ext->enum_present[i / 8] |= (1 << (i % 8));
			}
			break;
		case AUDIO_MIXER_VALUE:
			ext->maxvalue = UCHAR_MAX + 1;
			ext->minvalue = 0;
			if (mdi.un.v.num_channels == 2) {
				ext->type = MIXT_STEREOSLIDER;
			} else {
				ext->type = MIXT_MONOSLIDER;
			}
			break;
		}
		close(newfd);
		break;
	case SNDCTL_MIX_ENUMINFO:
		ei = (oss_mixer_enuminfo *)argp;
		if (ei == NULL) {
			errno = EINVAL;
			return -1;
		}
		if (ei->ctrl == 0) {
			errno = EINVAL;
			return -1;
		}
		snprintf(devname, sizeof(devname), "/dev/mixer%d", ei->dev);
		if ((newfd = open(devname, O_RDONLY)) < 0)
			return newfd;
		mdi.index = ei->ctrl - 1;
		retval = ioctl(newfd, AUDIO_MIXER_DEVINFO, &mdi);
		tmperrno = errno;
		close(newfd);
		if (retval < 0) {
			errno = tmperrno;
			return retval;
		}
		ei->version = 0;
		switch (mdi.type) {
		case AUDIO_MIXER_ENUM:
			ei->nvalues = mdi.un.e.num_mem;
			noffs = 0;
			for (i = 0; i < ei->nvalues; ++i) {
				ei->strindex[i] = noffs;
				len = strlen(mdi.un.e.member[i].label.name) + 1;
				if ((noffs + len) >= sizeof(ei->strings)) {
				    errno = ENOMEM;
				    return -1;
				}
				memcpy(ei->strings + noffs,
				    mdi.un.e.member[i].label.name, len);
				noffs += len;
			}
			break;
		case AUDIO_MIXER_SET:
			ei->nvalues = mdi.un.s.num_mem;
			noffs = 0;
			for (i = 0; i < ei->nvalues; ++i) {
				ei->strindex[i] = noffs;
				len = strlen(mdi.un.s.member[i].label.name) + 1;
				if ((noffs + len) >= sizeof(ei->strings)) {
				    errno = ENOMEM;
				    return -1;
				}
				memcpy(ei->strings + noffs,
				    mdi.un.s.member[i].label.name, len);
				noffs += len;
			}
			break;
		default:
			errno = EINVAL;
			return -1;
		}
		break;
	case SNDCTL_MIX_WRITE:
		mv = (oss_mixer_value *)argp;
		if (mv == NULL) {
			errno = EINVAL;
			return -1;
		}
		if (mv->ctrl == 0) {
			errno = EINVAL;
			return -1;
		}
		snprintf(devname, sizeof(devname), "/dev/mixer%d", mv->dev);
		if ((newfd = open(devname, O_RDWR)) < 0)
			return newfd;
		mdi.index = mc.dev = mv->ctrl - 1;
		retval = ioctl(newfd, AUDIO_MIXER_DEVINFO, &mdi);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		mc.type = mdi.type;
		switch (mdi.type) {
		case AUDIO_MIXER_ENUM:
			if (mv->value >= mdi.un.e.num_mem) {
				close(newfd);
				errno = EINVAL;
				return -1;
			}
			mc.un.ord = mdi.un.e.member[mv->value].ord;
			break;
		case AUDIO_MIXER_SET:
			if (mv->value >= mdi.un.s.num_mem) {
				close(newfd);
				errno = EINVAL;
				return -1;
			}
#ifdef notyet
			mc.un.mask = 0;
			for (i = 0; i < mdi.un.s.num_mem; ++i) {
				if (mv->value & (1 << i)) {
					mc.un.mask |= mdi.un.s.member[mv->value].mask;
				}
			}
#else
			mc.un.mask = mdi.un.s.member[mv->value].mask;
#endif
			break;
		case AUDIO_MIXER_VALUE:
			mc.un.value.num_channels = mdi.un.v.num_channels;
			if (mdi.un.v.num_channels != 2) {
				for (i = 0; i < mdi.un.v.num_channels; ++i) {
					mc.un.value.level[i] = mv->value;
				}
			} else {
			    mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT] = 
				(mv->value >> 0) & 0xFF;
			    mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
				(mv->value >> 8) & 0xFF;
			}
			break;
		}
		retval = ioctl(newfd, AUDIO_MIXER_WRITE, &mc);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		close(newfd);
		break;
	case SNDCTL_MIX_READ:
		mv = (oss_mixer_value *)argp;
		if (mv == NULL) {
			errno = EINVAL;
			return -1;
		}
		if (mv->ctrl == 0) {
			errno = EINVAL;
			return -1;
		}
		snprintf(devname, sizeof(devname), "/dev/mixer%d", mv->dev);
		if ((newfd = open(devname, O_RDWR)) < 0)
			return newfd;
		mdi.index = mc.dev = (mv->ctrl - 1);
		retval = ioctl(newfd, AUDIO_MIXER_DEVINFO, &mdi);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		mc.dev = mdi.index;
		mc.type = mdi.type;
		if (mdi.type == AUDIO_MIXER_VALUE)
			mc.un.value.num_channels = mdi.un.v.num_channels;
		retval = ioctl(newfd, AUDIO_MIXER_READ, &mc);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		close(newfd);
		mv->value = 0;
		switch (mdi.type) {
		case AUDIO_MIXER_ENUM:
			for (i = 0; i < mdi.un.e.num_mem; ++i) {
				if (mc.un.ord == mdi.un.e.member[i].ord) {
					mv->value = i;
					break;
				}
			}
			break;
		case AUDIO_MIXER_SET:
			for (i = 0; i < mdi.un.s.num_mem; ++i) {
#ifdef notyet
				if (mc.un.mask & mdi.un.s.member[i].mask)
					mv->value |= (1 << i);
#else
				if (mc.un.mask == mdi.un.s.member[i].mask) {
					mv->value = i;
					break;
				}
#endif
			}
			break;
		case AUDIO_MIXER_VALUE:
			if (mdi.un.v.num_channels != 2) {
				mv->value = mc.un.value.level[0];
			} else {
				mv->value = \
				    ((mc.un.value.level[1] & 0xFF) << 8) |
				    ((mc.un.value.level[0] & 0xFF) << 0);
			}
			break;
		default:
			errno = EINVAL;
			return -1;
		}
		break;
	default:
		errno = EINVAL;
		return -1;
	}
	return 0;
}

static int
get_audio_count(void)
{
	char devname[32];
	int ndevs = 0;
	int tmpfd;
	int tmperrno = errno;

	do {
		snprintf(devname, sizeof(devname),
		    "/dev/audio%d", ndevs);
		if ((tmpfd = open(devname, O_RDONLY)) != -1 ||
		    (tmpfd = open(devname, O_WRONLY)) != -1) {
			ndevs++;
			close(tmpfd);
		}
	} while (tmpfd != -1);
	errno = tmperrno;
	return ndevs;
}

static int
get_mixer_count(void)
{
	char devname[32];
	int ndevs = 0;
	int tmpfd;
	int tmperrno = errno;

	do {
		snprintf(devname, sizeof(devname),
		    "/dev/mixer%d", ndevs);
		if ((tmpfd = open(devname, O_RDONLY)) != -1) {
			ndevs++;
			close(tmpfd);
		}
	} while (tmpfd != -1);
	errno = tmperrno;
	return ndevs;
}

static int
get_mixer_control_count(int fd)
{
	struct mixer_devinfo mdi;
	int ndevs = 0;

	do {
		mdi.index = ndevs++;
	} while (ioctl(fd, AUDIO_MIXER_DEVINFO, &mdi) != -1);

	return ndevs > 0 ? ndevs - 1 : 0;
}
