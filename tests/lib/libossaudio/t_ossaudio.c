/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
#include <soundcard.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <atf-c.h>

ATF_TC(oss_dsp_init);
ATF_TC_HEAD(oss_dsp_init, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests dsp init ioctls");
}

ATF_TC_BODY(oss_dsp_init, tc)
{
	struct audio_info hwinfo;
	struct audio_info info;
	int fd, channels, fmt, rate;

	if ((fd = open("/dev/audio", O_WRONLY)) == -1)
		atf_tc_skip("Audio device unavailable for playback");

	if (ioctl(fd, AUDIO_GETFORMAT, &hwinfo) < 0) {
		atf_tc_fail("ioctl AUDIO_GETFORMAT failed");
		close(fd);
	}

	/* Verify SNDCTL_DSP_CHANNELS sets the device to mono. */

	channels = 1;
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &channels) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_CHANNELS (1) failed");
	ATF_REQUIRE_EQ(channels, 1);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.channels, 1);

	/* Verify SNDCTL_DSP_CHANNELS sets the device to stereo. */

	channels = 2;
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &channels) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_CHANNELS (2) failed");
	ATF_REQUIRE_EQ(channels, 2);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.channels, 2);

	/*
	 * Verify an invalid argument to SNDCTL_DSP_CHANNELS leaves the device
	 * at the hardware channel count.
	 */

	channels = 0;
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &channels) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_CHANNELS (0) failed");
	ATF_REQUIRE_EQ(channels, (int)hwinfo.play.channels);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.channels, hwinfo.play.channels);

	/*
	 * SNDCTL_DSP_STEREO is an older alternative to SNDCTL_DSP_CHANNELS
	 * that simply takes a boolean argument.
	 */

	/* Set the device to mono with SNDCTL_DSP_STEREO = 0 */

	channels = 0;
	if (ioctl(fd, SNDCTL_DSP_STEREO, &channels) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_STEREO (0) failed");
	ATF_REQUIRE_EQ(channels, 0);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");

	ATF_REQUIRE_EQ(info.play.channels, 1);

	/* Set the device to stereo with SNDCTL_DSP_STEREO = 1 */

	channels = 1;
	if (ioctl(fd, SNDCTL_DSP_STEREO, &channels) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_STEREO (1) failed");
	ATF_REQUIRE_EQ(channels, 1);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.channels, 2);

	/* Verify SNDCTL_DSP_SETFMT works with common audio formats */

	fmt = AFMT_MU_LAW;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &fmt) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETFMT (AFMT_MU_LAW) failed");
	ATF_REQUIRE_EQ(fmt, AFMT_MU_LAW);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.encoding, AUDIO_ENCODING_ULAW);
	ATF_REQUIRE_EQ(info.play.precision, 8);

	fmt = AFMT_A_LAW;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &fmt) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETFMT (AFMT_A_LAW) failed");
	ATF_REQUIRE_EQ(fmt, AFMT_A_LAW);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.encoding, AUDIO_ENCODING_ALAW);
	ATF_REQUIRE_EQ(info.play.precision, 8);

	fmt = AFMT_S16_LE;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &fmt) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETFMT (AFMT_S16_LE) failed");
	ATF_REQUIRE_EQ(fmt, AFMT_S16_LE);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.encoding, AUDIO_ENCODING_SLINEAR_LE);
	ATF_REQUIRE_EQ(info.play.precision, 16);

	fmt = AFMT_S16_BE;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &fmt) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETFMT (AFMT_S16_BE) failed");
	ATF_REQUIRE_EQ(fmt, AFMT_S16_BE);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.encoding, AUDIO_ENCODING_SLINEAR_BE);
	ATF_REQUIRE_EQ(info.play.precision, 16);

	fmt = AFMT_U16_LE;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &fmt) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETFMT (AFMT_U16_LE) failed");
	ATF_REQUIRE_EQ(fmt, AFMT_U16_LE);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.encoding, AUDIO_ENCODING_ULINEAR_LE);
	ATF_REQUIRE_EQ(info.play.precision, 16);

	fmt = AFMT_U16_BE;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &fmt) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETFMT (AFMT_U16_BE) failed");
	ATF_REQUIRE_EQ(fmt, AFMT_U16_BE);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.encoding, AUDIO_ENCODING_ULINEAR_BE);
	ATF_REQUIRE_EQ(info.play.precision, 16);

	fmt = AFMT_S32_LE;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &fmt) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETFMT (AFMT_S32_LE) failed");
	ATF_REQUIRE_EQ(fmt, AFMT_S32_LE);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.encoding, AUDIO_ENCODING_SLINEAR_LE);
	ATF_REQUIRE_EQ(info.play.precision, 32);

	/* Verify some supported sample rates. */

	rate = 8000;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SPEED (8000) failed");

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(rate, (int)info.play.sample_rate);

	rate = 32000;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SPEED (32000) failed");

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.sample_rate, 32000);
	ATF_REQUIRE_EQ(rate, (int)info.play.sample_rate);

	rate = 44100;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SPEED (44100) failed");

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.sample_rate, 44100);
	ATF_REQUIRE_EQ(rate, (int)info.play.sample_rate);

	rate = 48000;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SPEED (48000) failed");

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.sample_rate, 48000);
	ATF_REQUIRE_EQ(rate, (int)info.play.sample_rate);

	rate = 96000;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SPEED (96000) failed");

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.sample_rate, 96000);
	ATF_REQUIRE_EQ(rate, (int)info.play.sample_rate);

	rate = 192000;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SPEED (192000) failed");

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(info.play.sample_rate, 192000);
	ATF_REQUIRE_EQ(rate, (int)info.play.sample_rate);

	/*
	 * and some unsupported sample rates... "best effort"
	 */

	/* closest supported rate is 1000 */
	rate = 900;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SPEED (900) failed");
	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE((fabs(900.0 - info.play.sample_rate) / 900.0) < 0.2);
	ATF_REQUIRE_EQ(rate, (int)info.play.sample_rate);

	/* closest supported rate is 192000 */
	rate = 197000;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SPEED (197000) failed");
	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE((fabs(197000.0 - info.play.sample_rate) / 197000.0) < 0.2);
	ATF_REQUIRE_EQ(rate, (int)info.play.sample_rate);

	/* 0 should return the hardware rate. */

	rate = 0;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SPEED (0) failed");
	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");
	ATF_REQUIRE_EQ(hwinfo.play.sample_rate, info.play.sample_rate);

	close(fd);
}

ATF_TC(oss_dsp_trigger_read);
ATF_TC_HEAD(oss_dsp_trigger_read, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests SNDCTL_DSP_SETTRIGGER correctly "
	    "changes the recording pause state");
}

ATF_TC_BODY(oss_dsp_trigger_read, tc)
{
	struct audio_info info;
	int fd, bits;

#if defined(__sparc__)
	atf_tc_skip("PR port-sparc/55876");
#endif

	if ((fd = open("/dev/audio", O_RDONLY)) == -1)
		atf_tc_skip("Audio device unavailable for recording");

	/* pause everything ... */

	bits = 0;
	if (ioctl(fd, SNDCTL_DSP_SETTRIGGER, &bits) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETTRIGGER (0) failed");

	if (ioctl(fd, SNDCTL_DSP_GETTRIGGER, &bits) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_GETTRIGGER failed");
	ATF_REQUIRE_EQ(bits, 0);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");

	ATF_REQUIRE_EQ(info.record.pause, 1);

	/* unpause everything ... */

	bits = PCM_ENABLE_INPUT;
	if (ioctl(fd, SNDCTL_DSP_SETTRIGGER, &bits) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETTRIGGER "
		    "(PCM_ENABLE_INPUT) failed");

	if (ioctl(fd, SNDCTL_DSP_GETTRIGGER, &bits) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_GETTRIGGER failed");
	ATF_REQUIRE_EQ(bits, PCM_ENABLE_INPUT);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");

	ATF_REQUIRE_EQ(info.record.pause, 0);

	close(fd);
}

ATF_TC(oss_dsp_trigger_write);
ATF_TC_HEAD(oss_dsp_trigger_write, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests SNDCTL_DSP_SETTRIGGER correctly "
	    "changes the playback pause state");
}

ATF_TC_BODY(oss_dsp_trigger_write, tc)
{
	struct audio_info info;
	int fd, bits;

	if ((fd = open("/dev/audio", O_WRONLY)) == -1)
		atf_tc_skip("Audio device unavailable for playback");

	/* pause everything ... */

	bits = 0;
	if (ioctl(fd, SNDCTL_DSP_SETTRIGGER, &bits) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETTRIGGER (0) failed");

	if (ioctl(fd, SNDCTL_DSP_GETTRIGGER, &bits) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_GETTRIGGER failed");
	ATF_REQUIRE_EQ(bits, 0);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");

	ATF_REQUIRE_EQ(info.play.pause, 1);

	/* unpause everything ... */

	bits = PCM_ENABLE_OUTPUT;
	if (ioctl(fd, SNDCTL_DSP_SETTRIGGER, &bits) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_SETTRIGGER "
		    "(PCM_ENABLE_OUTPUT) failed");

	if (ioctl(fd, SNDCTL_DSP_GETTRIGGER, &bits) < 0)
		atf_tc_fail("ioctl SNDCTL_DSP_GETTRIGGER failed");
	ATF_REQUIRE_EQ(bits, PCM_ENABLE_OUTPUT);

	if (ioctl(fd, AUDIO_GETBUFINFO, &info) < 0)
		atf_tc_fail("ioctl AUDIO_GETBUFINFO failed");

	ATF_REQUIRE_EQ(info.play.pause, 0);

	close(fd);
}

ATF_TC(oss_dsp_caps);
ATF_TC_HEAD(oss_dsp_caps, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verifies that OSS device capabilities "
	    "are the same as native capabilities");
}

ATF_TC_BODY(oss_dsp_caps, tc)
{
	unsigned i;
	char dev[16];
	bool dev_tested = false;
	int fd;
	int caps, props, fmts;

	for (i = 0; i < 16; ++i) {
		(void)snprintf(dev, sizeof(dev), "/dev/audio%u", i);

		if ((fd = open(dev, O_WRONLY)) == -1) {
			if ((fd = open(dev, O_RDONLY)) == -1)
				break;
		}

		if (ioctl(fd, SNDCTL_DSP_GETCAPS, &caps) < 0)
			atf_tc_fail("ioctl SNDCTL_DSP_GETCAPS failed");

		if (ioctl(fd, AUDIO_GETPROPS, &props) < 0)
			atf_tc_fail("ioctl AUDIO_GETPROPS failed");

		ATF_REQUIRE(!(caps & DSP_CAP_DUPLEX) ==
		    !(props & AUDIO_PROP_FULLDUPLEX));

		ATF_REQUIRE(!(caps & DSP_CAP_MMAP) ==
		    !(props & AUDIO_PROP_MMAP));

		ATF_REQUIRE(!(caps & DSP_CAP_INPUT) ==
		    !(props & AUDIO_PROP_CAPTURE));

		ATF_REQUIRE(!(caps & DSP_CAP_OUTPUT) ==
		    !(props & AUDIO_PROP_PLAYBACK));

		/* Trigger is always supported in this implementation. */
		ATF_REQUIRE(caps & DSP_CAP_TRIGGER);

		if (ioctl(fd, SNDCTL_DSP_GETFMTS, &fmts) < 0)
			atf_tc_fail("ioctl SNDCTL_DSP_GETFMTS failed");

		/* All supported by the kernel mixer. */
		ATF_REQUIRE(fmts & AFMT_MU_LAW);
		ATF_REQUIRE(fmts & AFMT_A_LAW);
		ATF_REQUIRE(fmts & AFMT_S8);
		ATF_REQUIRE(fmts & AFMT_U8);
		ATF_REQUIRE(fmts & AFMT_S16_LE);
		ATF_REQUIRE(fmts & AFMT_S16_BE);
		ATF_REQUIRE(fmts & AFMT_U16_LE);
		ATF_REQUIRE(fmts & AFMT_U16_BE);
		ATF_REQUIRE(fmts & AFMT_S32_LE);
		ATF_REQUIRE(fmts & AFMT_S32_BE);

		/* Sanity test... */
		ATF_REQUIRE_EQ(fmts & AFMT_MPEG, 0);

		close(fd);

		dev_tested = true;
	}

	if (!dev_tested)
		atf_tc_skip("No testable audio device available");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, oss_dsp_init);
	ATF_TP_ADD_TC(tp, oss_dsp_caps);
	ATF_TP_ADD_TC(tp, oss_dsp_trigger_read);
	ATF_TP_ADD_TC(tp, oss_dsp_trigger_write);

	return atf_no_error();
}

