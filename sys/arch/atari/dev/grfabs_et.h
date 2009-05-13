/*	$NetBSD: grfabs_et.h,v 1.1.182.1 2009/05/13 17:16:22 jym Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
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

/*
 * Saved register-state of the ET4000
 */
typedef struct {
	u_char	misc_output;
	u_char	crt[25];	/* Std. VGA crt registers	*/
	u_char	attr[21];	/* Std. VGA attr. registers	*/
	u_char	grf[9];		/* Std. VGA grf. registers	*/
	u_char	seq[5];		/* Std. VGA seq. registers	*/

	/* ET4000 extensions */
	u_char	ext_start;	/* Extra Crt registers		*/
	u_char	compat_6845;
	u_char	overfl_high;
	u_char	hor_overfl;
	u_char	state_ctl;	/* Extra Seq. registers		*/
	u_char	aux_mode;
	u_char	seg_sel;	/* Extra general register	*/
} et_sv_reg_t;

typedef struct {
	et_sv_reg_t	sv_regs;	/* saved card regs	*/
	int		fb_size;	/* # of shorts in sv_fb	*/
	u_short		sv_fb[1];	/* saved frame buffer	*/
	/* actually longer... */
} save_area_t;

void	et_hwrest(et_sv_reg_t *);
void	et_hwsave(et_sv_reg_t *);
int	et_probe_card(void);
void	et_probe_video(MODES *);
