/*
 * Copyright (c) 1992,1993,1994 Hellmuth Michaelis, Brian Dunford-Shore
 *                              and Joerg Wunsch.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	This product includes software developed by Hellmuth Michaelis,
 *	Brian Dunford-Shore and Joerg Wunsch.
 * 4. The name authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @(#)pcvt_vtf.h, 3.00, Last Edit-Date: [Sun Feb 27 18:47:30 1994]
 */

#define INT_BITS	(sizeof(unsigned int) * 8)
#define INT_INDEX(n)	((n) / INT_BITS)
#define BIT_INDEX(n)	((n) % INT_BITS)

/*---------------------------------------------------------------------------*
 *	set selective attribute if appropriate
 *---------------------------------------------------------------------------*/
static __inline void vt_selattr(struct video_state *svsp)
{
	int i;

	i = (svsp->Crtat + svsp->cur_offset) - svsp->Crtat;

	if(svsp->selchar)
		svsp->decsca[INT_INDEX(i)] |=  (1 << BIT_INDEX(i));
	else
		svsp->decsca[INT_INDEX(i)] &= ~(1 << BIT_INDEX(i));
}
