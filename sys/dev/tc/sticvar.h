/* 	$NetBSD: sticvar.h,v 1.6 2000/12/22 13:30:32 ad Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _TC_STICVAR_H_
#define	_TC_STICVAR_H_

#ifdef _KERNEL

#if defined(pmax)
#define	STIC_KSEG_TO_PHYS(x)	MIPS_KSEG0_TO_PHYS(x)
#elif defined(alpha)
#define	STIC_KSEG_TO_PHYS(x)	ALPHA_K0SEG_TO_PHYS(x)
#else No support for your architecture
#endif

struct stic_hwcmap256 {
#define	CMAP_SIZE	256	/* 256 R/G/B entries */
	u_int8_t	r[CMAP_SIZE];
	u_int8_t	g[CMAP_SIZE];
	u_int8_t	b[CMAP_SIZE];
};

struct stic_hwcursor64 {
	struct	wsdisplay_curpos cc_pos;
	struct	wsdisplay_curpos cc_hot;
	struct	wsdisplay_curpos cc_size;
#define	CURSOR_MAX_SIZE	64
	u_int8_t	cc_color[6];
	u_int64_t	cc_image[64 + 64];
};

struct stic_screen {
	struct	stic_info *ss_si;
	u_int16_t	*ss_backing;
	int	ss_flags;
	int	ss_curx;
	int	ss_cury;
	struct	stic_hwcursor64 ss_cursor;
	struct	stic_hwcmap256 ss_cmap;
};

#define	SS_CURENB_CHANGED	0x0001
#define	SS_CURCMAP_CHANGED	0x0002
#define	SS_CURSHAPE_CHANGED	0x0004
#define	SS_CMAP_CHANGED		0x0008
#define	SS_ALL_CHANGED		0x000f

#define	SS_ALLOCED		0x0010
#define	SS_ACTIVE		0x0020
#define	SS_CURENB		0x0040

struct stic_info {
	u_int32_t	*si_slotkva;
	u_int32_t	*si_stamp;
	u_int32_t	*si_buf;
	u_int32_t	*si_vdac;
	u_int32_t	*si_vdac_reset;
	volatile struct	stic_regs *si_stic;

	u_int32_t	*(*si_pbuf_get)(struct stic_info *);
	int	(*si_pbuf_post)(struct stic_info *, u_int32_t *);
	int	(*si_ioctl)(struct stic_info *, u_long, caddr_t, int,
			   struct proc *);
	int	si_pbuf_select;

	struct	stic_screen *si_curscreen;
	struct	wsdisplay_font *si_font;

	int	si_consw;
	int	si_consh;
	int	si_fontw;
	int	si_fonth;
	int	si_stamph;
	int	si_stampw;
	int	si_depth;

	tc_addr_t si_slotbase;
	int	si_disptype;
	paddr_t	si_buf_phys;
	size_t	si_buf_size;

	int	si_vdacctl;

	struct	callout si_switch_callout;
	void	(*si_switchcb)(void *, int, int);
	void	*si_switchcbarg;
};

#define	STIC_VDAC_BLINK		0x01
#define	STIC_VDAC_24BIT		0x02

void	stic_init(struct stic_info *);
void	stic_attach(struct device *, struct stic_info *, int);
void	stic_cnattach(struct stic_info *);
void	stic_reset(struct stic_info *);
void	stic_flush(struct stic_info *);

extern struct stic_info stic_consinfo;

#endif	/* _KERNEL */

#define	STIC_PACKET_SIZE	4096
#define	STIC_IMGBUF_SIZE	8192

struct stic_xinfo {
	int	sxi_stampw;
	int	sxi_stamph;
	u_long	sxi_buf_size;
	u_long	sxi_buf_phys;
};

#define	STICIO_GXINFO	_IOR('s', 0, struct stic_xinfo)
#define	STICIO_SBLINK	_IOW('s', 1, int)
#define	STICIO_S24BIT	_IOW('s', 2, int)
#define	STICIO_START860	_IO('s', 3)
#define	STICIO_RESET860	_IO('s', 4)

struct stic_xmap {
	u_int32_t	sxm_stic[PAGE_SIZE];
	u_int32_t	sxm_poll[0x0c0000 / sizeof(u_int32_t)];
	u_int32_t	sxm_buf[256 * 1024 / sizeof(u_int32_t)];
};

#endif	/* !_TC_STICVAR_H_ */
