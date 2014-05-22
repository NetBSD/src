/*	$NetBSD: output.c,v 1.3.2.1 2014/05/22 15:45:43 yamt Exp $	*/

/*
 * Copyright (C) 1984-2012  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */


/*
 * High level routines dealing with the output to the screen.
 */

#include "less.h"
#if MSDOS_COMPILER==WIN32C
#include "windows.h"
#endif

public int errmsgs;	/* Count of messages displayed by error() */
public int need_clr;
public int final_attr;
public int at_prompt;

extern int sigs;
extern int sc_width;
extern int so_s_width, so_e_width;
extern int screen_trashed;
extern int any_display;
extern int is_tty;
extern int oldbot;

#if MSDOS_COMPILER==WIN32C || MSDOS_COMPILER==BORLANDC || MSDOS_COMPILER==DJGPPC
extern int ctldisp;
extern int nm_fg_color, nm_bg_color;
extern int bo_fg_color, bo_bg_color;
extern int ul_fg_color, ul_bg_color;
extern int so_fg_color, so_bg_color;
extern int bl_fg_color, bl_bg_color;
#endif

/*
 * Display the line which is in the line buffer.
 */
	public void
put_line()
{
	register int c;
	register int i;
	int a;

	if (ABORT_SIGS())
	{
		/*
		 * Don't output if a signal is pending.
		 */
		screen_trashed = 1;
		return;
	}

	final_attr = AT_NORMAL;

	for (i = 0;  (c = gline(i, &a)) != '\0';  i++)
	{
		at_switch(a);
		final_attr = a;
		if (c == '\b')
			putbs();
		else
			putchr(c);
	}

	at_exit();
}

static char obuf[OUTBUF_SIZE];
static char *ob = obuf;

/*
 * Flush buffered output.
 *
 * If we haven't displayed any file data yet,
 * output messages on error output (file descriptor 2),
 * otherwise output on standard output (file descriptor 1).
 *
 * This has the desirable effect of producing all
 * error messages on error output if standard output
 * is directed to a file.  It also does the same if
 * we never produce any real output; for example, if
 * the input file(s) cannot be opened.  If we do
 * eventually produce output, code in edit() makes
 * sure these messages can be seen before they are
 * overwritten or scrolled away.
 */
	public void
flush()
{
	register int n;
	register int fd;

	n = ob - obuf;
	if (n == 0)
		return;

#if MSDOS_COMPILER==MSOFTC
	if (is_tty && any_display)
	{
		*ob = '\0';
		_outtext(obuf);
		ob = obuf;
		return;
	}
#else
#if MSDOS_COMPILER==WIN32C || MSDOS_COMPILER==BORLANDC || MSDOS_COMPILER==DJGPPC
	if (is_tty && any_display)
	{
		*ob = '\0';
		if (ctldisp != OPT_ONPLUS)
			WIN32textout(obuf, ob - obuf);
		else
		{
			/*
			 * Look for SGR escape sequences, and convert them
			 * to color commands.  Replace bold, underline,
			 * and italic escapes into colors specified via
			 * the -D command-line option.
			 */
			char *anchor, *p, *p_next;
			unsigned char fg, bg;
			static unsigned char at;
#if MSDOS_COMPILER==WIN32C
			/* Screen colors used by 3x and 4x SGR commands. */
			static unsigned char screen_color[] = {
				0, /* BLACK */
				FOREGROUND_RED,
				FOREGROUND_GREEN,
				FOREGROUND_RED|FOREGROUND_GREEN,
				FOREGROUND_BLUE, 
				FOREGROUND_BLUE|FOREGROUND_RED,
				FOREGROUND_BLUE|FOREGROUND_GREEN,
				FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED
			};
#else
			static enum COLORS screen_color[] = {
				BLACK, RED, GREEN, BROWN,
				BLUE, MAGENTA, CYAN, LIGHTGRAY
			};
#endif

			for (anchor = p_next = obuf;
			     (p_next = memchr(p_next, ESC, ob - p_next)) != NULL; )
			{
				p = p_next;
				if (p[1] == '[')  /* "ESC-[" sequence */
				{
					if (p > anchor)
					{
						/*
						 * If some chars seen since
						 * the last escape sequence,
						 * write them out to the screen.
						 */
						WIN32textout(anchor, p-anchor);
						anchor = p;
					}
					p += 2;  /* Skip the "ESC-[" */
					if (is_ansi_end(*p))
					{
						/*
						 * Handle null escape sequence
						 * "ESC[m", which restores
						 * the normal color.
						 */
						p++;
						anchor = p_next = p;
						at = 0;
						WIN32setcolors(nm_fg_color, nm_bg_color);
						continue;
					}
					p_next = p;

					/*
					 * Select foreground/background colors
					 * based on the escape sequence. 
					 */
					fg = nm_fg_color;
					bg = nm_bg_color;
					while (!is_ansi_end(*p))
					{
						char *q;
						long code = strtol(p, &q, 10);

						if (*q == '\0')
						{
							/*
							 * Incomplete sequence.
							 * Leave it unprocessed
							 * in the buffer.
							 */
							int slop = q - anchor;
							/* {{ strcpy args overlap! }} */
							strcpy(obuf, anchor);
							ob = &obuf[slop];
							return;
						}

						if (q == p ||
						    code > 49 || code < 0 ||
						    (!is_ansi_end(*q) && *q != ';'))
						{
							p_next = q;
							break;
						}
						if (*q == ';')
							q++;

						switch (code)
						{
						default:
						/* case 0: all attrs off */
							fg = nm_fg_color;
							bg = nm_bg_color;
							at = 0;
							break;
						case 1:	/* bold on */
							at |= 1;
							break;
						case 3:	/* italic on */
						case 7: /* inverse on */
							at |= 2;
							break;
						case 4:	/* underline on */
							at |= 4;
							break;
						case 5: /* slow blink on */
						case 6: /* fast blink on */
							at |= 8;
							break;
						case 8:	/* concealed on */
							fg = (bg & 7) | 8;
							break;
						case 22: /* bold off */
							at &= ~1;
							break;
						case 23: /* italic off */
						case 27: /* inverse off */
							at &= ~2;
							break;
						case 24: /* underline off */
							at &= ~4;
							break;
						case 30: case 31: case 32:
						case 33: case 34: case 35:
						case 36: case 37:
							fg = (fg & 8) | (screen_color[code - 30]);
							break;
						case 39: /* default fg */
							fg = nm_fg_color;
							break;
						case 40: case 41: case 42:
						case 43: case 44: case 45:
						case 46: case 47:
							bg = (bg & 8) | (screen_color[code - 40]);
							break;
						case 49: /* default fg */
							bg = nm_bg_color;
							break;
						}
						p = q;
					}
					if (!is_ansi_end(*p) || p == p_next)
						break;
					if (at & 1)
					{
						/*
						 * If \e[1m use defined bold
						 * color, else set intensity.
						 */
						if (p[-2] == '[')
						{
#if MSDOS_COMPILER==WIN32C
							fg |= FOREGROUND_INTENSITY;
							bg |= BACKGROUND_INTENSITY;
#else
							fg = bo_fg_color;
							bg = bo_bg_color;
#endif
						} else
							fg |= 8;
					} else if (at & 2)
					{
						fg = so_fg_color;
						bg = so_bg_color;
					} else if (at & 4)
					{
						fg = ul_fg_color;
						bg = ul_bg_color;
					} else if (at & 8)
					{
						fg = bl_fg_color;
						bg = bl_bg_color;
					}
					fg &= 0xf;
					bg &= 0xf;
					WIN32setcolors(fg, bg);
					p_next = anchor = p + 1;
				} else
					p_next++;
			}

			/* Output what's left in the buffer.  */
			WIN32textout(anchor, ob - anchor);
		}
		ob = obuf;
		return;
	}
#endif
#endif
	fd = (any_display) ? 1 : 2;
	if (write(fd, obuf, n) != n)
		screen_trashed = 1;
	ob = obuf;
}

/*
 * Output a character.
 */
	public int
putchr(c)
	int c;
{
#if 0 /* fake UTF-8 output for testing */
	extern int utf_mode;
	if (utf_mode)
	{
		static char ubuf[MAX_UTF_CHAR_LEN];
		static int ubuf_len = 0;
		static int ubuf_index = 0;
		if (ubuf_len == 0)
		{
			ubuf_len = utf_len(c);
			ubuf_index = 0;
		}
		ubuf[ubuf_index++] = c;
		if (ubuf_index < ubuf_len)
			return c;
		c = get_wchar(ubuf) & 0xFF;
		ubuf_len = 0;
	}
#endif
	if (need_clr)
	{
		need_clr = 0;
		clear_bot();
	}
#if MSDOS_COMPILER
	if (c == '\n' && is_tty)
	{
		/* remove_top(1); */
		putchr('\r');
	}
#else
#ifdef _OSK
	if (c == '\n' && is_tty)  /* In OS-9, '\n' == 0x0D */
		putchr(0x0A);
#endif
#endif
	/*
	 * Some versions of flush() write to *ob, so we must flush
	 * when we are still one char from the end of obuf.
	 */
	if (ob >= &obuf[sizeof(obuf)-1])
		flush();
	*ob++ = c;
	at_prompt = 0;
	return (c);
}

/*
 * Output a string.
 */
	public void
putstr(s)
	register char *s;
{
	while (*s != '\0')
		putchr(*s++);
}


/*
 * Convert an integral type to a string.
 */
#define TYPE_TO_A_FUNC(funcname, type) \
void funcname(num, buf) \
	type num; \
	char *buf; \
{ \
	int neg = (num < 0); \
	char tbuf[INT_STRLEN_BOUND(num)+2]; \
	register char *s = tbuf + sizeof(tbuf); \
	if (neg) num = -num; \
	*--s = '\0'; \
	do { \
		*--s = (num % 10) + '0'; \
	} while ((num /= 10) != 0); \
	if (neg) *--s = '-'; \
	strcpy(buf, s); \
}

TYPE_TO_A_FUNC(postoa, POSITION)
TYPE_TO_A_FUNC(linenumtoa, LINENUM)
TYPE_TO_A_FUNC(inttoa, int)

/*
 * Output an integer in a given radix.
 */
	static int
iprint_int(num)
	int num;
{
	char buf[INT_STRLEN_BOUND(num)];

	inttoa(num, buf);
	putstr(buf);
	return (strlen(buf));
}

/*
 * Output a line number in a given radix.
 */
	static int
iprint_linenum(num)
	LINENUM num;
{
	char buf[INT_STRLEN_BOUND(num)];

	linenumtoa(num, buf);
	putstr(buf);
	return (strlen(buf));
}

/*
 * This function implements printf-like functionality
 * using a more portable argument list mechanism than printf's.
 */
	static int
less_printf(fmt, parg)
	register char *fmt;
	PARG *parg;
{
	register constant char *s;
	register int col;

	col = 0;
	while (*fmt != '\0')
	{
		if (*fmt != '%')
		{
			putchr(*fmt++);
			col++;
		} else
		{
			++fmt;
			switch (*fmt++)
			{
			case 's':
				s = parg->p_string;
				parg++;
				while (*s != '\0')
				{
					putchr(*s++);
					col++;
				}
				break;
			case 'd':
				col += iprint_int(parg->p_int);
				parg++;
				break;
			case 'n':
				col += iprint_linenum(parg->p_linenum);
				parg++;
				break;
			}
		}
	}
	return (col);
}

/*
 * Get a RETURN.
 * If some other non-trivial char is pressed, unget it, so it will
 * become the next command.
 */
	public void
get_return()
{
	int c;

#if ONLY_RETURN
	while ((c = getchr()) != '\n' && c != '\r')
		bell();
#else
	c = getchr();
	if (c != '\n' && c != '\r' && c != ' ' && c != READ_INTR)
		ungetcc(c);
#endif
}

/*
 * Output a message in the lower left corner of the screen
 * and wait for carriage return.
 */
	public void
error(fmt, parg)
	char *fmt;
	PARG *parg;
{
	int col = 0;
	static char return_to_continue[] = "  (press RETURN)";

	errmsgs++;

	if (any_display && is_tty)
	{
		if (!oldbot)
			squish_check();
		at_exit();
		clear_bot();
		at_enter(AT_STANDOUT);
		col += so_s_width;
	}

	col += less_printf(fmt, parg);

	if (!(any_display && is_tty))
	{
		putchr('\n');
		return;
	}

	putstr(return_to_continue);
	at_exit();
	col += sizeof(return_to_continue) + so_e_width;

	get_return();
	lower_left();
    clear_eol();

	if (col >= sc_width)
		/*
		 * Printing the message has probably scrolled the screen.
		 * {{ Unless the terminal doesn't have auto margins,
		 *    in which case we just hammered on the right margin. }}
		 */
		screen_trashed = 1;

	flush();
}

static char intr_to_abort[] = "... (interrupt to abort)";

/*
 * Output a message in the lower left corner of the screen
 * and don't wait for carriage return.
 * Usually used to warn that we are beginning a potentially
 * time-consuming operation.
 */
	public void
ierror(fmt, parg)
	char *fmt;
	PARG *parg;
{
	at_exit();
	clear_bot();
	at_enter(AT_STANDOUT);
	(void) less_printf(fmt, parg);
	putstr(intr_to_abort);
	at_exit();
	flush();
	need_clr = 1;
}

/*
 * Output a message in the lower left corner of the screen
 * and return a single-character response.
 */
	public int
query(fmt, parg)
	char *fmt;
	PARG *parg;
{
	register int c;
	int col = 0;

	if (any_display && is_tty)
		clear_bot();

	(void) less_printf(fmt, parg);
	c = getchr();

	if (!(any_display && is_tty))
	{
		putchr('\n');
		return (c);
	}

	lower_left();
	if (col >= sc_width)
		screen_trashed = 1;
	flush();

	return (c);
}
