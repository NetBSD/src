/*	$NetBSD: audio.c,v 1.1.1.1 2000/03/29 12:38:51 simonb Exp $	*/

/*
 * audio.c - audio interface for reference clock audio drivers
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "audio.h"
#include <unistd.h>
#include <stdio.h>

#ifdef HAVE_SYS_AUDIOIO_H
#include <sys/audioio.h>
#endif /* HAVE_SYS_AUDIOIO_H */
#ifdef HAVE_SUN_AUDIOIO_H
#include <sun/audioio.h>
#endif /* HAVE_SUN_AUDIOIO_H */
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */

#include <fcntl.h>

/*
 * Global variables
 */
#ifdef HAVE_SYS_AUDIOIO_H
static struct audio_device device; /* audio device ident */
static struct audio_info info;	/* audio device info */
static int ctl_fd;		/* audio control file descriptor */
#endif /* HAVE_SYS_AUDIOIO_H */


/*
 * audio_init - open and initialize audio device
 *
 * This code works with SunOS 4.x and Solaris 2.x; however, it is
 * believed generic and applicable to other systems with a minor twid
 * or two. All it does is open the device, set the buffer size (Solaris
 * only), preset the gain and set the input port. It assumes that the
 * codec sample rate (8000 Hz), precision (8 bits), number of channels
 * (1) and encoding (ITU-T G.711 mu-law companded) have been set by
 * default.
 */
int
audio_init(void)
{
	int fd;
#ifdef HAVE_SYS_AUDIOIO_H
	int rval;
#endif /* HAVE_SYS_AUDIOIO_H */

	/*
	 * Open audio device
	 */
	fd = open("/dev/audio", O_RDWR | O_NONBLOCK, 0777);
	if (fd < 0) {
		perror("audio:");
		return (fd);
	}

#ifdef HAVE_SYS_AUDIOIO_H
	/*
	 * Open audio control device
	 */
	ctl_fd = open("/dev/audioctl", O_RDWR);
	if (ctl_fd < 0) {
		perror("audioctl:");
		close(fd);
		return(ctl_fd);
	}

	/*
	 * Set audio device parameters.
	 */
	rval = audio_gain((AUDIO_MAX_GAIN - AUDIO_MIN_GAIN) / 2,
	    AUDIO_MICROPHONE);
	if (rval < 0) {
		close(ctl_fd);
		close(fd);
		return(rval);
	}
#endif /* HAVE_SYS_AUDIOIO_H */
	return (fd);
}


/*
 * audio_gain - adjust codec gain and port
 */
int
audio_gain(
	int gain,		/* gain 0-255 */
	int port		/* port */
	)
{
#ifdef HAVE_SYS_AUDIOIO_H
	int rval;
#endif /* HAVE_SYS_AUDIOIO_H */

#ifdef HAVE_SYS_AUDIOIO_H
	AUDIO_INITINFO(&info);
	info.record.buffer_size = AUDIO_BUFSIZ;
	info.record.gain = gain;
	info.record.port = port;
	info.record.error = 0;
	rval = ioctl(ctl_fd, (int)AUDIO_SETINFO, &info);
	if (rval < 0) {
		perror("audio:");
		return (rval);
	}
	return (info.record.error);
#else
	return (0);
#endif /* HAVE_SYS_AUDIOIO_H */
}


/*
 * audio_show - display audio parameters
 *
 * This code doesn't really do anything, except satisfy curiousity and
 * verify the ioctl's work.
 */
void
audio_show(void)
{
#ifdef HAVE_SYS_AUDIOIO_H
	ioctl(ctl_fd, (int)AUDIO_GETDEV, &device);
	printf("audio: name %s, version %s, config %s\n",
	    device.name, device.version, device.config);
	ioctl(ctl_fd, (int)AUDIO_GETINFO, &info);
	printf(
	    "audio: samples %d, channels %d, precision %d, encoding %d\n",
	    info.record.sample_rate, info.record.channels,
	    info.record.precision, info.record.encoding);
	printf("audio: gain %d, port %d, buffer %d\n",
	    info.record.gain, info.record.port,
	    info.record.buffer_size);
	printf("audio: gain %d, port %d\n",
	    info.record.gain, info.record.port);
	printf(
	    "audio: samples %d, eof %d, pause %d, error %d, waiting %d, balance %d\n",
	    info.record.samples, info.record.eof,
	    info.record.pause, info.record.error,
	    info.record.waiting, info.record.balance);

	{
		int cnt = 0;	/* this will handle clean line wraps someday */
		/* Right now, I think we're under 75 columns... */

		printf("audio: monitor %d", info.monitor_gain);
		cnt += 17;

#ifdef HAVE_STRUCT_AUDIO_INFO_OUTPUT_MUTED
		printf(", muted %d", info.output_muted);
		cnt += 9;
#endif

#ifdef HAVE_STRUCT_AUDIO_INFO_BLOCKSIZE
		printf(", blocksize %d", info.blocksize);
		cnt += 16;
#endif

#ifdef HAVE_STRUCT_AUDIO_INFO_HIWAT
		printf(", hiwat %d", info.hiwat);
		cnt += 11;
#endif

#ifdef HAVE_STRUCT_AUDIO_INFO_LOWAT
		printf(", lowat %d", info.lowat);
		cnt += 11;
#endif

#ifdef HAVE_STRUCT_AUDIO_INFO_MODE
		printf(", mode %d", info.mode);
		cnt += 8;
#endif
		if (cnt) {
			printf("\n");
			cnt = 0;
		}
	}
#endif /* HAVE_SYS_AUDIOIO_H */
}
