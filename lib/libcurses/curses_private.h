/*	$NetBSD: curses_private.h,v 1.11 2000/12/19 21:34:25 jdc Exp $	*/

/*-
 * Copyright (c) 1998-2000 Brett Lymn
 *                         (blymn@baea.com.au, brett_lymn@yahoo.com.au)
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 *
 */

/* Private structure definitions for curses. */
/*
 * A window an array of __LINE structures pointed to by the 'lines' pointer.
 * A line is an array of __LDATA structures pointed to by the 'line' pointer.
 *
 * IMPORTANT: the __LDATA structure must NOT induce any padding, so if new
 * fields are added -- padding fields with *constant values* should ensure
 * that the compiler will not generate any padding when storing an array of
 *  __LDATA structures.  This is to enable consistent use of memcmp, and memcpy
 * for comparing and copying arrays.
 */

struct __ldata {
	wchar_t	ch;			/* Character */
	attr_t	attr;			/* Attributes */
	wchar_t	bch;			/* Background character */
	attr_t	battr;			/* Background attributes */
};

#define __LDATASIZE	(sizeof(__LDATA))

struct __line {
#define	__ISDIRTY	0x01		/* Line is dirty. */
#define __ISPASTEOL	0x02		/* Cursor is past end of line */
	unsigned int flags;
	unsigned int hash;		/* Hash value for the line. */
	int *firstchp, *lastchp;	/* First and last chngd columns ptrs */
	int firstch, lastch;		/* First and last changed columns. */
	__LDATA *line;			/* Pointer to the line text. */
};

struct __window {		/* Window structure. */
	struct __window	*nextp, *orig;	/* Subwindows list and parent. */
	int begy, begx;			/* Window home. */
	int cury, curx;			/* Current x, y coordinates. */
	int maxy, maxx;			/* Maximum values for curx, cury. */
	short ch_off;			/* x offset for firstch/lastch. */
	__LINE **lines;			/* Array of pointers to the lines */
	__LINE  *lspace;		/* line space (for cleanup) */
	__LDATA *wspace;		/* window space (for cleanup) */

#define	__ENDLINE	0x00000001	/* End of screen. */
#define	__FLUSH		0x00000002	/* Fflush(stdout) after refresh. */
#define	__FULLWIN	0x00000004	/* Window is a screen. */
#define	__IDLINE	0x00000008	/* Insert/delete sequences. */
#define	__SCROLLWIN	0x00000010	/* Last char will scroll window. */
#define	__SCROLLOK	0x00000020	/* Scrolling ok. */
#define	__CLEAROK	0x00000040	/* Clear on next refresh. */
#define	__LEAVEOK	0x00000100	/* If cursor left */
#define	__KEYPAD	0x00010000	/* If interpreting keypad codes */
#define	__NOTIMEOUT	0x00020000	/* Wait indefinitely for func keys */
	unsigned int flags;
	int	delay;			/* delay for getch() */
	attr_t	wattr;			/* Character attributes */
	wchar_t	bch;			/* Background character */
	attr_t	battr;			/* Background attributes */
};

/* Set of attributes unset by 'me' - 'mb', 'md', 'mh', 'mk', 'mp' and 'mr'. */
#define	__TERMATTR \
	(__REVERSE | __BLINK | __DIM | __BOLD | __BLANK | __PROTECT)

struct __winlist {
	struct __window		*winp;	/* The window. */
	struct __winlist	*nextp;	/* Next window. */
};

/* Private variables. */
extern char	 __GT;			/* Gtty indicates tabs. */
extern char	 __NONL;		/* Term can't hack LF doing a CR. */
extern char	 __UPPERCASE;		/* Terminal is uppercase only. */

extern int	 My_term;		/* Use Def_term regardless. */
extern const char	*Def_term;	/* Default terminal type. */

/* Termcap capabilities. */
extern char	__tc_pc;
extern char	__tc_am, __tc_bs, __tc_cc, __tc_da, __tc_eo,
		__tc_hc, __tc_hl, __tc_in, __tc_mi, __tc_ms,
		__tc_nc, __tc_ns, __tc_os, __tc_ul, __tc_ut,
		__tc_xb, __tc_xn, __tc_xt, __tc_xs, __tc_xx;
extern char	__CA;
extern int	__tc_pa, __tc_Co, __tc_NC;
extern char	*__tc_ac, *__tc_AB, *__tc_ae, *__tc_AF, *__tc_AL,
		*__tc_al, *__tc_as, *__tc_bc, *__tc_bl, *__tc_bt,
		*__tc_cd, *__tc_ce, *__tc_cl, *__tc_cm, *__tc_cr,
		*__tc_cs, *__tc_dc, *__tc_DL, *__tc_dl, *__tc_dm,
		*__tc_DO, *__tc_do, *__tc_eA, *__tc_ed, *__tc_ei,
		*__tc_ho, *__tc_Ic, *__tc_ic, *__tc_im, *__tc_Ip,
		*__tc_ip, *__tc_k0, *__tc_k1, *__tc_k2, *__tc_k3,
		*__tc_k4, *__tc_k5, *__tc_k6, *__tc_k7, *__tc_k8,
		*__tc_k9, *__tc_kd, *__tc_ke, *__tc_kh, *__tc_kl,
		*__tc_kr, *__tc_ks, *__tc_ku, *__tc_LE, *__tc_ll,
		*__tc_ma, *__tc_mb, *__tc_md, *__tc_me, *__tc_mh,
		*__tc_mk, *__tc_mm, *__tc_mo, *__tc_mp, *__tc_mr,
		*__tc_nd, *__tc_nl, *__tc_oc, *__tc_op,
		*__tc_rc, *__tc_RI, *__tc_Sb, *__tc_sc, *__tc_se,
		*__tc_SF, *__tc_Sf, *__tc_sf, *__tc_so, *__tc_sp,
		*__tc_SR, *__tc_sr, *__tc_ta, *__tc_te, *__tc_ti,
		*__tc_uc, *__tc_ue, *__tc_UP, *__tc_up, *__tc_us,
		*__tc_vb, *__tc_ve, *__tc_vi, *__tc_vs;

/* Private functions. */
#ifdef DEBUG
void	 __CTRACE(const char *fmt, ...);
#endif
int	 __delay(void);
unsigned int __hash(char *s, int len);
void	 __id_subwins(WINDOW *orig);
void	 __init_getch(void);
void	 __init_acs(void);
char	*__longname(char *bp, char *def);	/* Original BSD version */
int	 __mvcur(int ly, int lx, int y, int x, int in_refresh);
int	 __nodelay(void);
int	 __notimeout(void);
char	*__parse_cap(const char *, ...);
void	 __restartwin(void);
void	 __restore_colors(void);
void     __restore_cursor_vis(void);
void     __restore_meta_state(void);
void	 __restore_termios(void);
void	 __restore_stophandler(void);
void	 __save_termios(void);
void	 __set_color(attr_t attr);
void	 __set_stophandler(void);
void	 __set_subwin(WINDOW *orig, WINDOW *win);
void	 __startwin(void);
void	 __stop_signal_handler(int signo);
int	 __stopwin(void);
void	 __swflags(WINDOW *win);
int	 __timeout(int delay);
int	 __touchline(WINDOW *win, int y, int sx, int ex);
int	 __touchwin(WINDOW *win);
char	*__tscroll(const char *cap, int n1, int n2);
void	 __unsetattr(int);
int	 __waddch(WINDOW *win, __LDATA *dp);

/* Private #defines. */
#define	min(a,b)	(a < b ? a : b)
#define	max(a,b)	(a > b ? a : b)

/* Private externs. */
extern int		 __echoit;
extern int		 __endwin;
extern int		 __pfast;
extern int		 __rawmode;
extern int		 __noqch;
extern attr_t		 __nca;
extern attr_t		 __mask_op, __mask_me, __mask_ue, __mask_se;
extern struct __winlist	*__winlistp;
extern WINDOW		*__virtscr;
