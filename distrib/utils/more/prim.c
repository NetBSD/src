/*	$NetBSD: prim.c,v 1.7 2003/08/06 13:36:54 itojun Exp $	*/

/*
 * Copyright (c) 1988 Mark Nudleman
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)prim.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: prim.c,v 1.7 2003/08/06 13:36:54 itojun Exp $");
#endif
#endif /* not lint */

/*
 * Primitives for displaying the file on the screen.
 */

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "less.h"
#include "extern.h"

int back_scroll = -1;
int hit_eof;		/* keeps track of how many times we hit end of file */
int screen_trashed;

static int squished;


static int match(char *, char *);
static int badmark __P((int));

/*
 * Check to see if the end of file is currently "displayed".
 */
void
eof_check()
/*###72 [cc] conflicting types for `eof_check'%%%*/
{
	off_t pos;

	if (sigs)
		return;
	/*
	 * If the bottom line is empty, we are at EOF.
	 * If the bottom line ends at the file length,
	 * we must be just at EOF.
	 */
	pos = position(BOTTOM_PLUS_ONE);
	if (pos == NULL_POSITION || pos == ch_length())
		hit_eof++;
}

/*
 * If the screen is "squished", repaint it.
 * "Squished" means the first displayed line is not at the top
 * of the screen; this can happen when we display a short file
 * for the first time.
 */
void
squish_check()
/*###95 [cc] conflicting types for `squish_check'%%%*/
{
	if (squished) {
		squished = 0;
		repaint();
	}
}

/*
 * Display n lines, scrolling forward, starting at position pos in the
 * input file.  "only_last" means display only the last screenful if
 * n > screen size.
 */
void
forw(n, pos, only_last)
/*###109 [cc] conflicting types for `forw'%%%*/
	int n;
	off_t pos;
	int only_last;
{
	static int first_time = 1;
	int eof = 0, do_repaint;

	squish_check();

	/*
	 * do_repaint tells us not to display anything till the end, 
	 * then just repaint the entire screen.
	 */
	do_repaint = (only_last && n > sc_height-1);

	if (!do_repaint) {
		if (top_scroll && n >= sc_height - 1) {
			/*
			 * Start a new screen.
			 * {{ This is not really desirable if we happen
			 *    to hit eof in the middle of this screen,
			 *    but we don't yet know if that will happen. }}
			 */
			clear();
			home();
		} else {
			lower_left();
			clear_eol();
		}

		/*
		 * This is not contiguous with what is currently displayed.
		 * Clear the screen image (position table) and start a new
		 * screen.
		 */
		if (pos != position(BOTTOM_PLUS_ONE)) {
			pos_clear();
			add_forw_pos(pos);
			if (top_scroll) {
				clear();
				home();
			} else if (!first_time)
				putstr("...skipping...\n");
		}
	}

	for (short_file = 0; --n >= 0;) {
		/*
		 * Read the next line of input.
		 */
		pos = forw_line(pos);
		if (pos == NULL_POSITION) {
			/*
			 * end of file; copy the table if the file was
			 * too small for an entire screen.
			 */
			eof = 1;
			if (position(TOP) == NULL_POSITION) {
				copytable();
				if (!position(TOP))
					short_file = 1;
			}
			break;
		}
		/*
		 * Add the position of the next line to the position table.
		 * Display the current line on the screen.
		 */
		add_forw_pos(pos);
		if (do_repaint)
			continue;
		/*
		 * If this is the first screen displayed and we hit an early
		 * EOF (i.e. before the requested number of lines), we
		 * "squish" the display down at the bottom of the screen.
		 */
		if (first_time && line == NULL && !top_scroll) {
			squished = 1;
			continue;
		}
		put_line();
	}

	if (eof && !sigs)
		hit_eof++;
	else
		eof_check();
	if (do_repaint)
		repaint();
	first_time = 0;
	(void) currline(BOTTOM);
}

/*
 * Display n lines, scrolling backward.
 */
void
back(n, pos, only_last)
/*###207 [cc] conflicting types for `back'%%%*/
	int n;
	off_t pos;
	int only_last;
{
	int do_repaint;

	squish_check();
	do_repaint = (n > get_back_scroll() || (only_last && n > sc_height-1));
	hit_eof = 0;
	while (--n >= 0)
	{
		/*
		 * Get the previous line of input.
		 */
		pos = back_line(pos);
		if (pos == NULL_POSITION)
			break;
		/*
		 * Add the position of the previous line to the position table.
		 * Display the line on the screen.
		 */
		add_back_pos(pos);
		if (!do_repaint)
		{
			if (retain_below)
			{
				lower_left();
				clear_eol();
			}
			home();
			add_line();
			put_line();
		}
	}

	eof_check();
	if (do_repaint)
		repaint();
	(void) currline(BOTTOM);
}

/*
 * Display n more lines, forward.
 * Start just after the line currently displayed at the bottom of the screen.
 */
void
forward(n, only_last)
/*###254 [cc] conflicting types for `forward'%%%*/
	int n;
	int only_last;
{
	off_t pos;

	if (hit_eof) {
		/*
		 * If we're trying to go forward from end-of-file,
		 * go on to the next file.
		 */
		next_file(1);
		return;
	}

	pos = position(BOTTOM_PLUS_ONE);
	if (pos == NULL_POSITION)
	{
		hit_eof++;
		return;
	}
	forw(n, pos, only_last);
}

/*
 * Display n more lines, backward.
 * Start just before the line currently displayed at the top of the screen.
 */
void
backward(n, only_last)
/*###283 [cc] conflicting types for `backward'%%%*/
	int n;
	int only_last;
{
	off_t pos;

	pos = position(TOP);
	/*
	 * This will almost never happen, because the top line is almost
	 * never empty.
	 */
	if (pos == NULL_POSITION)
		return;   
	back(n, pos, only_last);
}

/*
 * Repaint the screen, starting from a specified position.
 */
void
prepaint(pos)
/*###303 [cc] conflicting types for `prepaint'%%%*/
	off_t pos;
{
	hit_eof = 0;
	forw(sc_height-1, pos, 0);
	screen_trashed = 0;
}

/*
 * Repaint the screen.
 */
void
repaint()
/*###315 [cc] conflicting types for `repaint'%%%*/
{
	/*
	 * Start at the line currently at the top of the screen
	 * and redisplay the screen.
	 */
	prepaint(position(TOP));
}

/*
 * Jump to the end of the file.
 * It is more convenient to paint the screen backward,
 * from the end of the file toward the beginning.
 */
void
jump_forw()
/*###330 [cc] conflicting types for `jump_forw'%%%*/
{
	off_t pos;

	if (ch_end_seek())
	{
		error("Cannot seek to end of file");
		return;
	}
	lastmark();
	pos = ch_tell();
	clear();
	pos_clear();
	add_back_pos(pos);
	back(sc_height - 1, pos, 0);
}

/*
 * Jump to line n in the file.
 */
void
jump_back(n)
/*###351 [cc] conflicting types for `jump_back'%%%*/
	int n;
{
	int c, nlines;

	/*
	 * This is done the slow way, by starting at the beginning
	 * of the file and counting newlines.
	 *
	 * {{ Now that we have line numbering (in linenum.c),
	 *    we could improve on this by starting at the
	 *    nearest known line rather than at the beginning. }}
	 */
	if (ch_seek((off_t)0)) {
		/* 
		 * Probably a pipe with beginning of file no longer buffered. 
		 * If he wants to go to line 1, we do the best we can, 
		 * by going to the first line which is still buffered.
		 */
		if (n <= 1 && ch_beg_seek() == 0)
			jump_loc(ch_tell());
		error("Cannot get to beginning of file");
		return;
	}

	/*
	 * Start counting lines.
	 */
	for (nlines = 1;  nlines < n;  nlines++)
		while ((c = ch_forw_get()) != '\n')
			if (c == EOI) {
				char message[40];
				(void)sprintf(message, "File has only %d lines",
				    nlines - 1);
				error(message);
				return;
			}
	jump_loc(ch_tell());
}

/*
 * Jump to a specified percentage into the file.
 * This is a poor compensation for not being able to
 * quickly jump to a specific line number.
 */
void
jump_percent(percent)
/*###397 [cc] conflicting types for `jump_percent'%%%*/
	int percent;
{
	off_t pos, len;
	int c;

	/*
	 * Determine the position in the file
	 * (the specified percentage of the file's length).
	 */
	if ((len = ch_length()) == NULL_POSITION)
	{
		error("Don't know length of file");
		return;
	}
	pos = (percent * len) / 100;

	/*
	 * Back up to the beginning of the line.
	 */
	if (ch_seek(pos) == 0)
	{
		while ((c = ch_back_get()) != '\n' && c != EOI)
			;
		if (c == '\n')
			(void) ch_forw_get();
		pos = ch_tell();
	}
	jump_loc(pos);
}

/*
 * Jump to a specified position in the file.
 */
void
jump_loc(pos)
/*###432 [cc] conflicting types for `jump_loc'%%%*/
	off_t pos;
{
	int nline;
	off_t tpos;

	if ((nline = onscreen(pos)) >= 0) {
		/*
		 * The line is currently displayed.  
		 * Just scroll there.
		 */
		forw(nline, position(BOTTOM_PLUS_ONE), 0);
		return;
	}

	/*
	 * Line is not on screen.
	 * Seek to the desired location.
	 */
	if (ch_seek(pos)) {
		error("Cannot seek to that position");
		return;
	}

	/*
	 * See if the desired line is BEFORE the currently displayed screen.
	 * If so, then move forward far enough so the line we're on will be
	 * at the bottom of the screen, in order to be able to call back()
	 * to make the screen scroll backwards & put the line at the top of
	 * the screen.
	 * {{ This seems inefficient, but it's not so bad,
	 *    since we can never move forward more than a
	 *    screenful before we stop to redraw the screen. }}
	 */
	tpos = position(TOP);
	if (tpos != NULL_POSITION && pos < tpos) {
		off_t npos = pos;
		/*
		 * Note that we can't forw_line() past tpos here,
		 * so there should be no EOI at this stage.
		 */
		for (nline = 0;  npos < tpos && nline < sc_height - 1;  nline++)
			npos = forw_line(npos);

		if (npos < tpos) {
			/*
			 * More than a screenful back.
			 */
			lastmark();
			clear();
			pos_clear();
			add_back_pos(npos);
		}

		/*
		 * Note that back() will repaint() if nline > back_scroll.
		 */
		back(nline, npos, 0);
		return;
	}
	/*
	 * Remember where we were; clear and paint the screen.
	 */
	lastmark();
	prepaint(pos);
}

/*
 * The table of marks.
 * A mark is simply a position in the file.
 */
#define	NMARKS		(27)		/* 26 for a-z plus one for quote */
#define	LASTMARK	(NMARKS-1)	/* For quote */
static off_t marks[NMARKS];

/*
 * Initialize the mark table to show no marks are set.
 */
void
init_mark()
/*###511 [cc] conflicting types for `init_mark'%%%*/
{
	int i;

	for (i = 0;  i < NMARKS;  i++)
		marks[i] = NULL_POSITION;
}

/*
 * See if a mark letter is valid (between a and z).
 */
static int
badmark(c)
	int c;
{
	if (c < 'a' || c > 'z')
	{
		error("Choose a letter between 'a' and 'z'");
		return (1);
	}
	return (0);
}

/*
 * Set a mark.
 */
void
setmark(c)
/*###538 [cc] conflicting types for `setmark'%%%*/
	int c;
{
	if (badmark(c))
		return;
	marks[c-'a'] = position(TOP);
}

void
lastmark()
/*###547 [cc] conflicting types for `lastmark'%%%*/
{
	marks[LASTMARK] = position(TOP);
}

/*
 * Go to a previously set mark.
 */
void
gomark(c)
/*###556 [cc] conflicting types for `gomark'%%%*/
	int c;
{
	off_t pos;

	if (c == '\'') {
		pos = marks[LASTMARK];
		if (pos == NULL_POSITION)
			pos = 0;
	}
	else {
		if (badmark(c))
			return;
		pos = marks[c-'a'];
		if (pos == NULL_POSITION) {
			error("mark not set");
			return;
		}
	}
	jump_loc(pos);
}

/*
 * Get the backwards scroll limit.
 * Must call this function instead of just using the value of
 * back_scroll, because the default case depends on sc_height and
 * top_scroll, as well as back_scroll.
 */
int
get_back_scroll()
{
	if (back_scroll >= 0)
		return (back_scroll);
	if (top_scroll)
		return (sc_height - 2);
	return (sc_height - 1);
}

/*
 * Search for the n-th occurence of a specified pattern, 
 * either forward or backward.
 */
int
search(search_forward, pattern, n, wantmatch)
	int search_forward;
	char *pattern;
	int n;
	int wantmatch;
{
	off_t pos, linepos;
	char *p;
	char *q;
	int linenum;
	int linematch;
#ifdef RECOMP
	char *re_comp();
	char *errmsg;
#else
#ifdef REGCMP
	char *regcmp();
	static char *cpattern = NULL;
#else
	static char lpbuf[100];
	static char *last_pattern = NULL;
#endif
#endif

	/*
	 * For a caseless search, convert any uppercase in the pattern to
	 * lowercase.
	 */
	if (caseless && pattern != NULL)
		for (p = pattern;  *p;  p++)
			if (isupper(*p))
				*p = tolower(*p);
#ifdef RECOMP

	/*
	 * (re_comp handles a null pattern internally, 
	 *  so there is no need to check for a null pattern here.)
	 */
	if ((errmsg = re_comp(pattern)) != NULL)
	{
		error(errmsg);
		return(0);
	}
#else
#ifdef REGCMP
	if (pattern == NULL || *pattern == '\0')
	{
		/*
		 * A null pattern means use the previous pattern.
		 * The compiled previous pattern is in cpattern, so just use it.
		 */
		if (cpattern == NULL)
		{
			error("No previous regular expression");
			return(0);
		}
	} else
	{
		/*
		 * Otherwise compile the given pattern.
		 */
		char *s;
		if ((s = regcmp(pattern, 0)) == NULL)
		{
			error("Invalid pattern");
			return(0);
		}
		if (cpattern != NULL)
			free(cpattern);
		cpattern = s;
	}
#else
	if (pattern == NULL || *pattern == '\0')
	{
		/*
		 * Null pattern means use the previous pattern.
		 */
		if (last_pattern == NULL)
		{
			error("No previous regular expression");
			return(0);
		}
		pattern = last_pattern;
	} else
	{
		(void)strlcpy(lpbuf, pattern, sizeof(lpbuf));
		last_pattern = lpbuf;
	}
#endif
#endif

	/*
	 * Figure out where to start the search.
	 */

	if (position(TOP) == NULL_POSITION) {
		/*
		 * Nothing is currently displayed.  Start at the beginning
		 * of the file.  (This case is mainly for searches from the
		 * command line.
		 */
		pos = (off_t)0;
	} else if (!search_forward) {
		/*
		 * Backward search: start just before the top line
		 * displayed on the screen.
		 */
		pos = position(TOP);
	} else {
		/*
		 * Start at the second screen line displayed on the screen.
		 */
		pos = position(TOP_PLUS_ONE);
	}

	if (pos == NULL_POSITION)
	{
		/*
		 * Can't find anyplace to start searching from.
		 */
		error("Nothing to search");
		return(0);
	}

	linenum = find_linenum(pos);
	for (;;)
	{
		/*
		 * Get lines until we find a matching one or 
		 * until we hit end-of-file (or beginning-of-file 
		 * if we're going backwards).
		 */
		if (sigs)
			/*
			 * A signal aborts the search.
			 */
			return(0);

		if (search_forward)
		{
			/*
			 * Read the next line, and save the 
			 * starting position of that line in linepos.
			 */
			linepos = pos;
			pos = forw_raw_line(pos);
			if (linenum != 0)
				linenum++;
		} else
		{
			/*
			 * Read the previous line and save the
			 * starting position of that line in linepos.
			 */
			pos = back_raw_line(pos);
			linepos = pos;
			if (linenum != 0)
				linenum--;
		}

		if (pos == NULL_POSITION)
		{
			/*
			 * We hit EOF/BOF without a match.
			 */
			error("Pattern not found");
			return(0);
		}

		/*
		 * If we're using line numbers, we might as well
		 * remember the information we have now (the position
		 * and line number of the current line).
		 */
		if (linenums)
			add_lnum(linenum, pos);

		/*
		 * If this is a caseless search, convert uppercase in the
		 * input line to lowercase.
		 */
		if (caseless)
			for (p = q = line;  *p;  p++, q++)
				*q = isupper(*p) ? tolower(*p) : *p;

		/*
		 * Remove any backspaces along with the preceding char.
		 * This allows us to match text which is underlined or
		 * overstruck.
		 */
		for (p = q = line;  *p;  p++, q++)
			if (q > line && *p == '\b')
				/* Delete BS and preceding char. */
				q -= 2;
			else
				/* Otherwise, just copy. */
				*q = *p;

		/*
		 * Test the next line to see if we have a match.
		 * This is done in a variety of ways, depending
		 * on what pattern matching functions are available.
		 */
#ifdef REGCMP
		linematch = (regex(cpattern, line) != NULL);
#else
#ifdef RECOMP
		linematch = (re_exec(line) == 1);
#else
		linematch = match(pattern, line);
#endif
#endif
		/*
		 * We are successful if wantmatch and linematch are
		 * both true (want a match and got it),
		 * or both false (want a non-match and got it).
		 */
		if (((wantmatch && linematch) || (!wantmatch && !linematch)) &&
		      --n <= 0)
			/*
			 * Found the line.
			 */
			break;
	}
	jump_loc(linepos);
	return(1);
}

#if !defined(REGCMP) && !defined(RECOMP)
/*
 * We have neither regcmp() nor re_comp().
 * We use this function to do simple pattern matching.
 * It supports no metacharacters like *, etc.
 */
static int
match(pattern, buf)
	char *pattern, *buf;
{
	char *pp, *lp;

	for ( ;  *buf != '\0';  buf++)
	{
		for (pp = pattern, lp = buf;  *pp == *lp;  pp++, lp++)
			if (*pp == '\0' || *lp == '\0')
				break;
		if (*pp == '\0')
			return (1);
	}
	return (0);
}
#endif
