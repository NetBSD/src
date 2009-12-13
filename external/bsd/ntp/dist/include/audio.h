/*	$NetBSD: audio.h,v 1.1.1.1 2009/12/13 16:54:48 kardel Exp $	*/

/*
 * Header file for audio drivers
 */
#include "ntp_types.h"

#define MAXGAIN		255	/* max codec gain */
#define	MONGAIN		127	/* codec monitor gain */

/*
 * Function prototypes
 */
int	audio_init		(char *, int, int);
int	audio_gain		(int, int, int);
void	audio_show		(void);
