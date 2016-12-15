/* $NetBSD: spkrvar.h,v 1.5 2016/12/15 06:48:14 pgoyette Exp $ */

#ifndef _SYS_DEV_SPKRVAR_H
#define _SYS_DEV_SPKRVAR_H

#include <sys/module.h>

struct spkr_softc {
	device_t sc_dev;
	int sc_octave;	/* currently selected octave */
	int sc_whole;	/* whole-note time at current tempo, in ticks */
	int sc_value;	/* whole divisor for note time, quarter note = 1 */
	int sc_fill;	/* controls spacing of notes */
	bool sc_octtrack;	/* octave-tracking on? */
	bool sc_octprefix;	/* override current octave-tracking state? */
	char *sc_inbuf;

	/* attachment-specific hooks */
	void (*sc_tone)(device_t, u_int, u_int);
	void (*sc_rest)(device_t, int);
};

void spkr_attach(device_t,
    void (*)(device_t, u_int, u_int), void (*)(device_t, int));

#endif /* _SYS_DEV_SPKRVAR_H */
