/*	$NetBSD: cnmagic.c,v 1.5 2003/08/22 02:01:32 junyoung Exp $	*/

/*
 * Copyright (c) 2000 Eduardo Horvath
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Eduardo Horvath.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cnmagic.c,v 1.5 2003/08/22 02:01:32 junyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#define ENCODE_STATE(c, n) (short)(((c)&0x1ff)|(((n)&0x7f)<<9))

static unsigned short cn_magic[CNS_LEN];

/*
 * Initialize a cnm_state_t.
 */
void
cn_init_magic(cnm_state_t *cnm)
{
	cnm->cnm_state = 0;
	cnm->cnm_magic = cn_magic;
}

/*
 * Destroy a cnm_state_t.
 */
void
cn_destroy_magic(cnm_state_t *cnm)
{
	cnm->cnm_state = 0;
	cnm->cnm_magic = NULL;
}

/*
 * Translate a magic string to a state
 * machine table.
 */
int
cn_set_magic(char *magic)
{
	unsigned int i, c, n;
	unsigned short m[CNS_LEN];

	for (i = 0; i < CNS_LEN; i++) {
		c = (*magic++) & 0xff;
		n = *magic ? i+1 : CNS_TERM;
		switch (c) {
		case 0:
			/* End of string */
			if (i == 0) {
				/* empty string? */
				cn_magic[0] = 0;
#ifdef DEBUG
				printf("cn_set_magic(): empty!\n");
#endif
				return (0);
			}
			do {
				cn_magic[i] = m[i];
			} while (i--);
			return(0);
		case 0x27:
			/* Escape sequence */
			c = (*magic++) & 0xff;
			n = *magic ? i+1 : CNS_TERM;
			switch (c) {
			case 0x27:
				break;
			case 0x01:
				/* BREAK */
				c = CNC_BREAK;
				break;
			case 0x02:
				/* NUL */
				c = 0;
				break;
			}
			/* FALLTHROUGH */
		default:
			/* Transition to the next state. */
#ifdef DEBUG
			if (!cold)
				printf("mag %d %x:%x\n", i, c, n);
#endif
			m[i] = ENCODE_STATE(c, n);
			break;
		}
	}
	return (EINVAL);
}

/*
 * Translatea state machine table back to
 * a magic string.
 */
int
cn_get_magic(char *magic, int maglen)
{
	unsigned int i, c;

	for (i = 0; i < CNS_LEN;) {
		c = cn_magic[i];
		/* Translate a character */
		switch (CNS_MAGIC_VAL(c)) {
		case CNC_BREAK:
			*magic++ = 0x27;
			*magic++ = 0x01;
			break;
		case 0:
			*magic++ = 0x27;
			*magic++ = 0x02;
			break;
		case 0x27:
			*magic++ = 0x27;
			*magic++ = 0x27;
			break;
		default:
			*magic++ = (c & 0x0ff);
			break;
		}
		/* Now go to the next state */
		i = CNS_MAGIC_NEXT(c);
		if (i == CNS_TERM || i == 0) {
			/* Either termination state or empty machine */
			*magic++ = 0;
			return (0);
		}
	}
	return (EINVAL);
}

