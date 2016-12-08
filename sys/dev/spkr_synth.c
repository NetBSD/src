/*	$NetBSD: spkr_synth.c,v 1.1 2016/12/08 11:31:08 nat Exp $	*/

/*-
 * Copyright (c) 2016 Nathanial Sloss <nathanialsloss@yahoo.com.au>
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

#ifdef VAUDIOSPEAKER
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spkr_synth.c,v 1.1 2016/12/08 11:31:08 nat Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/uio.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/kthread.h>
#include <sys/sysctl.h>
#include <dev/audio_if.h>
#include <dev/audiovar.h>

struct vbell_args {
	device_t *cookie;
	u_int pitch;
	u_int period;
	u_int volume;
	bool dying;
};

void bell_thread(void *);
static int beep_sysctl_device(SYSCTLFN_PROTO);

#include <dev/audiobellvar.h>

#include <dev/spkrvar.h>
#include <dev/isa/spkrio.h>

#include "isa/spkr.c"

int spkrprobe(device_t, cfdata_t, void *);
void spkrattach(device_t, device_t, void *);
int spkrdetach(device_t, int);
device_t speakerattach_mi(device_t);

#include "ioconf.h"

MODULE(MODULE_CLASS_DRIVER, spkr, NULL /* "audio" */);

#ifdef _MODULE
#include "ioconf.c"
#endif


CFATTACH_DECL3_NEW(spkr_synth, 0,
    spkrprobe, spkrattach, spkrdetach, NULL, NULL, NULL, DVF_DETACH_SHUTDOWN);

extern struct cfdriver audio_cd;

static struct sysctllog	*spkr_sc_log;	/* sysctl log */
static int beep_index = 0;

struct vbell_args sc_bell_args;
lwp_t		*sc_bellthread;
kmutex_t	sc_bellock;
kcondvar_t	sc_bellcv;

struct spkr_attach_args {
	device_t dev;
};

static void
tone(u_int xhz, u_int ticks)
{
	audiobell(beep_index, xhz, ticks * (1000 / hz), 80, 0);
}

static void
rest(int ticks)
{
#ifdef SPKRDEBUG
    printf("rest: %d\n", ticks);
#endif /* SPKRDEBUG */
    if (ticks > 0)
	audiobell(beep_index, 0, ticks * (1000 / hz), 80, 0);
}

device_t
speakerattach_mi(device_t dev)
{
	struct spkr_attach_args sa;
	sa.dev = dev;
	return config_found(dev, &sa, NULL);
}

void
spkrattach(device_t parent, device_t self, void *aux)
{
	const struct sysctlnode *node;

	printf("\n");
	beep_index = 0;
	spkr_attached = 1;
	
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n"); 
	mutex_init(&sc_bellock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc_bellcv, "bellcv");

	/* sysctl set-up for default audio device */
	sysctl_createv(&spkr_sc_log, 0, NULL, &node,
		0,
		CTLTYPE_NODE, "beep",
		SYSCTL_DESCR("synthesized beeper information"),
		NULL, 0,
		NULL, 0,
		CTL_HW,
		CTL_CREATE, CTL_EOL);

	if (node != NULL) {
		sysctl_createv(&spkr_sc_log, 0, NULL, NULL,
			CTLFLAG_READWRITE,
			CTLTYPE_INT, "device",
			SYSCTL_DESCR("default device"),
			beep_sysctl_device, 0,
			NULL, 0,
			CTL_HW, node->sysctl_num,
			CTL_CREATE, CTL_EOL);
	}

	kthread_create(PRI_BIO, KTHREAD_MPSAFE | KTHREAD_MUSTJOIN, NULL,
	    bell_thread, &sc_bell_args, &sc_bellthread, "vbell");
}

int
spkrdetach(device_t self, int flags)
{

	pmf_device_deregister(self);

	mutex_enter(&sc_bellock);
	sc_bell_args.dying = true;

	cv_broadcast(&sc_bellcv);
	mutex_exit(&sc_bellock);

	kthread_join(sc_bellthread);
	cv_destroy(&sc_bellcv);
	mutex_destroy(&sc_bellock);

	/* delete sysctl nodes */
	sysctl_teardown(&spkr_sc_log);

	spkr_attached = 0;

	return 0;
}

void
bell_thread(void *arg)
{
	struct vbell_args *vb = arg;
	u_int bpitch;
	u_int bperiod;
	u_int bvolume;
	
	for (;;) {
		mutex_enter(&sc_bellock);
		cv_wait_sig(&sc_bellcv, &sc_bellock);
		
		if (vb->dying == true) {
			mutex_exit(&sc_bellock);
			kthread_exit(0);
		}
		
		bpitch = vb->pitch;
		bperiod = vb->period;
		bvolume = vb->volume;
		mutex_exit(&sc_bellock);
		audiobell(beep_index, bpitch, bperiod, bvolume, 0);
	}
}

void
speaker_play(u_int pitch, u_int period, u_int volume)
{
	if (spkr_attached == 0 || beep_index == -1)
		return;

	mutex_enter(&sc_bellock);
	sc_bell_args.dying = false;
	sc_bell_args.pitch = pitch;
	sc_bell_args.period = period;
	sc_bell_args.volume = volume;

	cv_broadcast(&sc_bellcv);
	mutex_exit(&sc_bellock);
}

/* sysctl helper to set common audio channels */
static int
beep_sysctl_device(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct audio_softc *ac;
	int t, error;

	node = *rnode;

	t = beep_index;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	
	if (t < -1 || (t != -1 && (ac = device_lookup_private(&audio_cd, t)) ==
	    NULL))
		return EINVAL;

	beep_index = t;

	return error;
}

#endif /* VAUDIOSPEAKER */
/* spkr.c ends here */
