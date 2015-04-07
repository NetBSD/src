/*	$NetBSD: audio.h,v 1.1.1.2 2015/04/07 16:49:04 christos Exp $	*/

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
