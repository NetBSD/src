/*	$NetBSD: audio.h,v 1.2.26.1 2014/12/24 00:05:16 riz Exp $	*/

/*
 * Header file for audio drivers
 */
#include "ntp_types.h"

#define MAXGAIN		255	/* max codec gain */
#define	MONGAIN		127	/* codec monitor gain */

/*
 * Function prototypes
 */
int	audio_init		(const char *, int, int);
int	audio_gain		(int, int, int);
void	audio_show		(void);
