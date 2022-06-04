/*	$NetBSD: midi_seq_mod.c,v 1.1 2022/06/04 03:31:10 pgoyette Exp $	*/

/*
 * Copyright (c) 1998, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@NetBSD.org), (MIDI FST and Active
 * Sense handling) Chapman Flack (chap@NetBSD.org), and Andrew Doran.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: midi_seq_mod.c,v 1.1 2022/06/04 03:31:10 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "midi.h"
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/conf.h>
#include <sys/audioio.h>
#include <sys/midiio.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/module.h>

#include <dev/audio/audio_if.h>
#include <dev/midi_if.h>
#include <dev/midivar.h>

#include "ioconf.h"

extern struct cfdriver sequencer_cd;
extern struct cdevsw midi_cdevsw;
extern struct cdevsw sequencer_cdevsw;

#ifdef _MODULE
#include "ioconf.c"

CFDRIVER_DECL(sequencer, DV_DULL, NULL);

devmajor_t midi_bmajor = -1, midi_cmajor = -1;
devmajor_t sequencer_bmajor = -1, sequencer_cmajor = -1;
#endif

MODULE(MODULE_CLASS_DRIVER, midi_seq, "audio");

static int
midi_seq_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = devsw_attach(midi_cd.cd_name, NULL, &midi_bmajor,
		    &midi_cdevsw, &midi_cmajor);
		if (error)
			break;

		error = devsw_attach(sequencer_cd.cd_name,
		    NULL, &sequencer_bmajor,
		    &sequencer_cdevsw, &sequencer_cmajor);
		if (error) {
			devsw_detach(NULL, &midi_cdevsw);
			break;
		}

		error = config_init_component(cfdriver_ioconf_midi_seq,
		    cfattach_ioconf_midi_seq, cfdata_ioconf_midi_seq);
		if (error) {
			devsw_detach(NULL, &sequencer_cdevsw);
			devsw_detach(NULL, &midi_cdevsw);
		}
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(cfdriver_ioconf_midi_seq,
		   cfattach_ioconf_midi_seq, cfdata_ioconf_midi_seq);
		if (error == 0) {
			devsw_detach(NULL, &sequencer_cdevsw);
			devsw_detach(NULL, &midi_cdevsw);
		}
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
