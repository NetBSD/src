/* $NetBSD: spkrvar.h,v 1.1 2016/12/08 11:31:08 nat Exp $ */

#ifndef _SYS_DEV_SPKRVAR_H
#define _SYS_DEV_SPKRVAR_H

device_t speakerattach_mi(device_t);
void speaker_play(u_int, u_int, u_int);

#endif /* _SYS_DEV_SPKRVAR_H */
