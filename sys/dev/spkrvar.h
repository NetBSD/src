/* $NetBSD: spkrvar.h,v 1.2 2016/12/09 04:46:39 christos Exp $ */

#ifndef _SYS_DEV_SPKRVAR_H
#define _SYS_DEV_SPKRVAR_H

device_t speakerattach_mi(device_t);
void speaker_play(u_int, u_int, u_int);

// XXX:
void spkr_tone(u_int, u_int);
void spkr_rest(int);
int spkr__modcmd(modcmd_t, void *);
int spkr_probe(device_t, cfdata_t, void *);
extern int spkr_attached;

#endif /* _SYS_DEV_SPKRVAR_H */
