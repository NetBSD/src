/* $NetBSD: spkrvar.h,v 1.9.4.2 2017/12/03 11:36:58 jdolecek Exp $ */

#ifndef _SYS_DEV_SPKRVAR_H
#define _SYS_DEV_SPKRVAR_H

#include <sys/module.h>

struct spkr_softc {
	device_t sc_dev;
	device_t sc_wsbelldev;
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
	u_int sc_vol;	/* volume - only for audio skpr */
};

void spkr_attach(device_t,
    void (*)(device_t, u_int, u_int), void (*)(device_t, int));

int spkr_detach(device_t, int);

int spkr_rescan(device_t, const char *, const int *);

int spkr_childdet(device_t, device_t);

#endif /* _SYS_DEV_SPKRVAR_H */
