/*	$NetBSD: curses_private.h,v 1.2 2000/04/12 21:46:27 jdc Exp $	*/

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
};

#define __LDATASIZE	(sizeof(__LDATA))

struct __line {
#define	__ISDIRTY	0x01		/* Line is dirty. */
#define __ISPASTEOL	0x02		/* Cursor is past end of line */
#define __FORCEPAINT	0x04		/* Force a repaint of the line */
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
	wchar_t	bchar;			/* Background character */
	attr_t	battr;			/* Background attributes */
};

/* Private functions. */
#ifdef DEBUG
void	 __CTRACE __P((const char *, ...));
#endif
int	 __delay __P((void));
unsigned int __hash __P((char *, int));
void	 __id_subwins __P((WINDOW *));
void	 __init_getch __P((char *));
void	 __init_acs __P((void));
char	*__longname __P((char *, char *));	/* Original BSD version */
int	 __mvcur __P((int, int, int, int, int));
int	 __nodelay __P((void));
int	 __notimeout __P((void));
char	*__parse_cap __P((const char *, ...));
void	 __restartwin __P((void));
void	 __restore_colors __P((void));
void	 __restore_termios __P((void));
void	 __restore_stophandler __P((void));
void	 __save_termios __P((void));
void	 __set_color __P((attr_t));
void	 __set_stophandler __P((void));
void	 __set_subwin __P((WINDOW *, WINDOW *));
void	 __startwin __P((void));
void	 __stop_signal_handler __P((int));
int	 __stopwin __P((void));
void	 __swflags __P((WINDOW *));
int	 __timeout __P((int));
int	 __touchline __P((WINDOW *, int, int, int, int));
int	 __touchwin __P((WINDOW *));
char	*__tscroll __P((const char *, int, int));
int	 __waddch __P((WINDOW *, __LDATA *));

/* Private #defines. */
#define	min(a,b)	(a < b ? a : b)
#define	max(a,b)	(a > b ? a : b)

/* Private externs. */
extern int	 __echoit;
extern int	 __endwin;
extern int	 __pfast;
extern int	 __rawmode;
extern int	 __noqch;
extern attr_t	 __nca;
