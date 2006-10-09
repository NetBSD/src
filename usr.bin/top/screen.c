/*	$NetBSD: screen.c,v 1.9 2006/10/09 19:49:51 apb Exp $	*/

/*
 *  Top users/processes display for Unix
 *  Version 3
 *
 * Copyright (c) 1984, 1989, William LeFebvre, Rice University
 * Copyright (c) 1989, 1990, 1992, William LeFebvre, Northwestern University
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS EMPLOYER BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*  This file contains the routines that interface to termcap and stty/gtty.
 *
 *  Paul Vixie, February 1987: converted to use ioctl() instead of stty/gtty.
 *
 *  I put in code to turn on the TOSTOP bit while top was running, but I
 *  didn't really like the results.  If you desire it, turn on the
 *  preprocessor variable "TOStop".   --wnl
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: screen.c,v 1.9 2006/10/09 19:49:51 apb Exp $");
#endif

#include "os.h"
#include "top.h"

#include <sys/ioctl.h>
#ifdef CBREAK
# include <sgtty.h>
# define SGTTY
#else
# ifdef TCGETA
#  define TERMIO
#  include <termio.h>
# else
#  define TERMIOS
#  include <termios.h>
# endif
#endif
#if defined(TERMIO) || defined(TERMIOS)
# ifndef TAB3
#  ifdef OXTABS
#   define TAB3 OXTABS
#  else
#   define TAB3 0
#  endif
# endif
#endif
#include "screen.h"
#include "boolean.h"

extern char *myname;

int  overstrike;
int  screen_length;
int  screen_width;
char ch_erase;
char ch_kill;
char smart_terminal;
char PC;
struct tinfo *info;
char home[15];
char lower_left[15];
char *clear_line;
char *clear_screen;
char *clear_to_end;
char *cursor_motion;
char *start_standout;
char *end_standout;
char *terminal_init;
char *terminal_end;
short ospeed;

#ifdef SGTTY
static struct sgttyb old_settings;
static struct sgttyb new_settings;
#endif
#ifdef TERMIO
static struct termio old_settings;
static struct termio new_settings;
#endif
#ifdef TERMIOS
static struct termios old_settings;
static struct termios new_settings;
#endif
static char is_a_terminal = No;
#ifdef TOStop
static int old_lword;
static int new_lword;
#endif

#define	STDIN	0
#define	STDOUT	1
#define	STDERR	2

void
init_termcap(interactive)

int interactive;

{
    char *PCptr;
    char *term_name;
    int status;

    /* set defaults in case we aren't smart */
    screen_width = MAX_COLS;
    screen_length = 0;

    if (!interactive)
    {
	/* pretend we have a dumb terminal */
	smart_terminal = No;
	return;
    }

    /* assume we have a smart terminal until proven otherwise */
    smart_terminal = Yes;

    /* get the terminal name */
    term_name = getenv("TERM");

    /* if there is no TERM, assume it's a dumb terminal */
    /* patch courtesy of Sam Horrocks at telegraph.ics.uci.edu */
    if (term_name == NULL)
    {
	smart_terminal = No;
	return;
    }

    /* now get the termcap entry */
    if ((status = t_getent(&info, term_name)) != 1)
    {
	if (status == -1)
	{
	    fprintf(stderr, "%s: can't open termcap file\n", myname);
	}
	else
	{
	    fprintf(stderr, "%s: no termcap entry for a `%s' terminal\n",
		    myname, term_name);
	}

	/* pretend it's dumb and proceed */
	smart_terminal = No;
	return;
    }

    /* "hardcopy" immediately indicates a very stupid terminal */
    if (t_getflag(info, "hc"))
    {
	smart_terminal = No;
	return;
    }

    /* set screen_width and screen_length from an ioctl, if possible. */
    get_screensize();

    /* get size from terminal capabilities if not yet set properly. */
    /* screen_width is a little different */
    if (screen_width < 0)
    {
	if ((screen_width = t_getnum(info, "co")) == -1)
	{
	    screen_width = 79;
	}
	else
	{
	    screen_width -= 1;
	}
    }

    if (screen_length <= 0)
    {
	if ((screen_length = t_getnum(info, "li")) <= 0)
	{
	    screen_length = smart_terminal = 0;
	    return;
	}
    }

    /* terminals that overstrike need special attention */
    overstrike = t_getflag(info, "os");

    /* get "ce", clear to end */
    if (!overstrike)
    {
	clear_line = t_agetstr(info, "ce");
    }

    /* get necessary capabilities */
    if ((clear_screen  = t_agetstr(info, "cl")) == NULL ||
	(cursor_motion = t_agetstr(info, "cm")) == NULL)
    {
	smart_terminal = No;
	return;
    }

    /* get some more sophisticated stuff -- these are optional */
    clear_to_end   = t_agetstr(info, "cd");
    terminal_init  = t_agetstr(info, "ti");
    terminal_end   = t_agetstr(info, "te");
    start_standout = t_agetstr(info, "so");
    end_standout   = t_agetstr(info, "se");

    /* pad character */
    PC = (PCptr = t_agetstr(info, "pc")) ? *PCptr : 0;

    /* set convenience strings */
    home[0] = '\0';
    t_goto(info, cursor_motion, 0, 0, home, 14);
    lower_left[0] = '\0';
    t_goto(info, cursor_motion, 0, screen_length - 1, lower_left, 14);

    /* if stdout is not a terminal, pretend we are a dumb terminal */
#ifdef SGTTY
    if (ioctl(STDOUT, TIOCGETP, &old_settings) == -1)
    {
	smart_terminal = No;
    }
#endif
#ifdef TERMIO
    if (ioctl(STDOUT, TCGETA, &old_settings) == -1)
    {
	smart_terminal = No;
    }
#endif
#ifdef TERMIOS
    if (tcgetattr(STDOUT, &old_settings) == -1)
    {
	smart_terminal = No;
    }
#endif
}

void
init_screen()

{
    /* get the old settings for safe keeping */
#ifdef SGTTY
    if (ioctl(STDOUT, TIOCGETP, &old_settings) != -1)
    {
	/* copy the settings so we can modify them */
	new_settings = old_settings;

	/* turn on CBREAK and turn off character echo and tab expansion */
	new_settings.sg_flags |= CBREAK;
	new_settings.sg_flags &= ~(ECHO|XTABS);
	(void) ioctl(STDOUT, TIOCSETP, &new_settings);

	/* remember the erase and kill characters */
	ch_erase = old_settings.sg_erase;
	ch_kill  = old_settings.sg_kill;

#ifdef TOStop
	/* get the local mode word */
	(void) ioctl(STDOUT, TIOCLGET, &old_lword);

	/* modify it */
	new_lword = old_lword | LTOSTOP;
	(void) ioctl(STDOUT, TIOCLSET, &new_lword);
#endif
	/* remember that it really is a terminal */
	is_a_terminal = Yes;

	/* send the termcap initialization string */
	putcap(terminal_init);
    }
#endif
#ifdef TERMIO
    if (ioctl(STDOUT, TCGETA, &old_settings) != -1)
    {
	/* copy the settings so we can modify them */
	new_settings = old_settings;

	/* turn off ICANON, character echo and tab expansion */
	new_settings.c_lflag &= ~(ICANON|ECHO);
	new_settings.c_oflag &= ~(TAB3);
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	(void) ioctl(STDOUT, TCSETA, &new_settings);

	/* remember the erase and kill characters */
	ch_erase = old_settings.c_cc[VERASE];
	ch_kill  = old_settings.c_cc[VKILL];

	/* remember that it really is a terminal */
	is_a_terminal = Yes;

	/* send the termcap initialization string */
	putcap(terminal_init);
    }
#endif
#ifdef TERMIOS
    if (tcgetattr(STDOUT, &old_settings) != -1)
    {
	/* copy the settings so we can modify them */
	new_settings = old_settings;

	/* turn off ICANON, character echo and tab expansion */
	new_settings.c_lflag &= ~(ICANON|ECHO);
	new_settings.c_oflag &= ~(TAB3);
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	(void) tcsetattr(STDOUT, TCSADRAIN, &new_settings);

	/* remember the erase and kill characters */
	ch_erase = old_settings.c_cc[VERASE];
	ch_kill  = old_settings.c_cc[VKILL];

	/* remember that it really is a terminal */
	is_a_terminal = Yes;

	/* send the termcap initialization string */
	putcap(terminal_init);
    }
#endif

    if (!is_a_terminal)
    {
	/* not a terminal at all---consider it dumb */
	smart_terminal = No;
    }
}

void
end_screen()

{
    /* move to the lower left, clear the line and send "te" */
    if (smart_terminal)
    {
	putcap(lower_left);
	putcap(clear_line);
	fflush(stdout);
	putcap(terminal_end);
    }

    /* if we have settings to reset, then do so */
    if (is_a_terminal)
    {
#ifdef SGTTY
	(void) ioctl(STDOUT, TIOCSETP, &old_settings);
#ifdef TOStop
	(void) ioctl(STDOUT, TIOCLSET, &old_lword);
#endif
#endif
#ifdef TERMIO
	(void) ioctl(STDOUT, TCSETA, &old_settings);
#endif
#ifdef TERMIOS
	(void) tcsetattr(STDOUT, TCSADRAIN, &old_settings);
#endif
    }
}

void
reinit_screen()

{
    /* install our settings if it is a terminal */
    if (is_a_terminal)
    {
#ifdef SGTTY
	(void) ioctl(STDOUT, TIOCSETP, &new_settings);
#ifdef TOStop
	(void) ioctl(STDOUT, TIOCLSET, &new_lword);
#endif
#endif
#ifdef TERMIO
	(void) ioctl(STDOUT, TCSETA, &new_settings);
#endif
#ifdef TERMIOS
	(void) tcsetattr(STDOUT, TCSADRAIN, &new_settings);
#endif
    }

    /* send init string */
    if (smart_terminal)
    {
	putcap(terminal_init);
    }
}

void
get_screensize()

{

#ifdef TIOCGWINSZ

    struct winsize ws;

    if (ioctl (1, TIOCGWINSZ, &ws) != -1)
    {
	if (ws.ws_row != 0)
	{
	    screen_length = ws.ws_row;
	}
	if (ws.ws_col != 0)
	{
	    screen_width = ws.ws_col - 1;
	}
    }

#else
#ifdef TIOCGSIZE

    struct ttysize ts;

    if (ioctl (1, TIOCGSIZE, &ts) != -1)
    {
	if (ts.ts_lines != 0)
	{
	    screen_length = ts.ts_lines;
	}
	if (ts.ts_cols != 0)
	{
	    screen_width = ts.ts_cols - 1;
	}
    }

#endif /* TIOCGSIZE */
#endif /* TIOCGWINSZ */
}

void
standout(msg)

char *msg;

{
    if (smart_terminal)
    {
	putcap(start_standout);
	fputs(msg, stdout);
	putcap(end_standout);
    }
    else
    {
	fputs(msg, stdout);
    }
}

void
clear()

{
    if (smart_terminal)
    {
	putcap(clear_screen);
    }
}

int
clear_eol(len)

int len;

{
    if (smart_terminal && !overstrike && len > 0)
    {
	if (clear_line)
	{
	    putcap(clear_line);
	    return(0);
	}
	else
	{
	    while (len-- > 0)
	    {
		putchar(' ');
	    }
	    return(1);
	}
    }
    return(-1);
}

void
go_home()

{
    if (smart_terminal)
    {
	putcap(home);
    }
}

/* This has to be defined as a subroutine for tputs (instead of a macro) */

int
putstdout(ch)

int ch;

{
    return (putchar(ch));
}

void
Move_to(int x, int y)
{
	char buf[256];

	if(t_goto(info,	cursor_motion, x, y, buf, 255) == 0)
		TCputs(buf);
}
