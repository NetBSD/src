/*	$NetBSD: acs.c,v 1.17 2010/02/03 15:34:40 roy Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: acs.c,v 1.17 2010/02/03 15:34:40 roy Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

chtype _acs_char[NUM_ACS];
#ifdef HAVE_WCHAR
#include <assert.h>
#include <langinfo.h>
#include <strings.h>

cchar_t _wacs_char[ NUM_ACS ];
#endif /* HAVE_WCHAR */

/*
 * __init_acs --
 *	Fill in the ACS characters.  The 'ac' termcap entry is a list of
 *	character pairs - ACS definition then terminal representation.
 */
void
__init_acs(SCREEN *screen)
{
	int		count;
	const char	*aofac;	/* Address of 'ac' */
	unsigned char	acs, term;

	/* Default value '+' for all ACS characters */
	for (count=0; count < NUM_ACS; count++)
		_acs_char[count]= '+';

	/* Add the SUSv2 defaults (those that are not '+') */
	ACS_RARROW = '>';
	ACS_LARROW = '<';
	ACS_UARROW = '^';
	ACS_DARROW = 'v';
	ACS_BLOCK = '#';
/*	ACS_DIAMOND = '+';	*/
	ACS_CKBOARD = ':';
	ACS_DEGREE = 39;	/* ' */
	ACS_PLMINUS = '#';
	ACS_BOARD = '#';
	ACS_LANTERN = '#';
/*	ACS_LRCORNER = '+';	*/
/*	ACS_URCORNER = '+';	*/
/*	ACS_ULCORNER = '+';	*/
/*	ACS_LLCORNER = '+';	*/
/*	ACS_PLUS = '+';		*/
	ACS_HLINE = '-';
	ACS_S1 = '-';
	ACS_S9 = '_';
/*	ACS_LTEE = '+';		*/
/*	ACS_RTEE = '+';		*/
/*	ACS_BTEE = '+';		*/
/*	ACS_TTEE = '+';		*/
	ACS_VLINE = '|';
	ACS_BULLET = 'o';
	/* Add the extensions defaults */
	ACS_S3 = '-';
	ACS_S7 = '-';
	ACS_LEQUAL = '<';
	ACS_GEQUAL = '>';
	ACS_PI = '*';
	ACS_NEQUAL = '!';
	ACS_STERLING = 'f';

	if (t_acs_chars(screen->term) == NULL)
		goto out;

	aofac = t_acs_chars(screen->term);

	while (*aofac != '\0') {
		if ((acs = *aofac) == '\0')
			return;
		if (++aofac == '\0')
			return;
		if ((term = *aofac) == '\0')
			return;
	 	/* Only add characters 1 to 127 */
		if (acs < NUM_ACS)
			_acs_char[acs] = term | __ALTCHARSET;
		aofac++;
#ifdef DEBUG
		__CTRACE(__CTRACE_INIT, "__init_acs: %c = %c\n", acs, term);
#endif
	}

	if (t_ena_acs(screen->term) != NULL)
		ti_puts(screen->term, t_ena_acs(screen->term), 0,
		    __cputchar_args, screen->outfd);

out:
	for (count=0; count < NUM_ACS; count++)
		screen->acs_char[count]= _acs_char[count];
}

void
_cursesi_reset_acs(SCREEN *screen)
{
	int count;

	for (count=0; count < NUM_ACS; count++)
		_acs_char[count]= screen->acs_char[count];
}

#ifdef HAVE_WCHAR
/*
 * __init_wacs --
 *	Fill in the ACS characters.  The 'ac' termcap entry is a list of
 *	character pairs - ACS definition then terminal representation.
 */
void
__init_wacs(SCREEN *screen)
{
	int		count;
	const char	*aofac;	/* Address of 'ac' */
	unsigned char	acs, term;
	char	*lstr;

	/* Default value '+' for all ACS characters */
	for (count=0; count < NUM_ACS; count++) {
		_wacs_char[ count ].vals[ 0 ] = ( wchar_t )btowc( '+' );
		_wacs_char[ count ].attributes = 0;
		_wacs_char[ count ].elements = 1;
	}

	/* Add the SUSv2 defaults (those that are not '+') */
	lstr = nl_langinfo(CODESET);
	_DIAGASSERT(lstr);
	if (!strcasecmp(lstr, "UTF-8")) {
#ifdef DEBUG
		__CTRACE(__CTRACE_INIT, "__init_wacs: setting defaults\n" );
#endif /* DEBUG */
		WACS_RARROW  = ( wchar_t )btowc( '>' );
		WACS_LARROW  = ( wchar_t )btowc( '<' );
		WACS_UARROW  = ( wchar_t )btowc( '^' );
		WACS_DARROW  = ( wchar_t )btowc( 'v' );
		WACS_BLOCK   = ( wchar_t )btowc( '#' );
		WACS_CKBOARD = ( wchar_t )btowc( ':' );
		WACS_DEGREE  = ( wchar_t )btowc( 39 );	/* ' */
		WACS_PLMINUS = ( wchar_t )btowc( '#' );
		WACS_BOARD   = ( wchar_t )btowc( '#' );
		WACS_LANTERN = ( wchar_t )btowc( '#' );
		WACS_HLINE   = ( wchar_t )btowc( '-' );
		WACS_S1	  = ( wchar_t )btowc( '-' );
		WACS_S9	  = ( wchar_t )btowc( '_' );
		WACS_VLINE   = ( wchar_t )btowc( '|' );
		WACS_BULLET  = ( wchar_t )btowc( 'o' );
		WACS_S3 = ( wchar_t )btowc( 'p' );
		WACS_S7 = ( wchar_t )btowc( 'r' );
		WACS_LEQUAL = ( wchar_t )btowc( 'y' );
		WACS_GEQUAL = ( wchar_t )btowc( 'z' );
		WACS_PI = ( wchar_t )btowc( '{' );
		WACS_NEQUAL = ( wchar_t )btowc( '|' );
		WACS_STERLING = ( wchar_t )btowc( '}' );
	} else {
		/* Unicode defaults */
#ifdef DEBUG
		__CTRACE(__CTRACE_INIT,
		    "__init_wacs: setting Unicode defaults\n" );
#endif /* DEBUG */
		WACS_RARROW	 = 0x2192;
		WACS_LARROW	 = 0x2190;
		WACS_UARROW	 = 0x2192;
		WACS_DARROW	 = 0x2193;
		WACS_BLOCK	  = 0x25ae;
  		WACS_DIAMOND	= 0x25c6;
		WACS_CKBOARD	= 0x2592;
		WACS_DEGREE	 = 0x00b0;
		WACS_PLMINUS	= 0x00b1;
		WACS_BOARD	  = 0x2592;
		WACS_LANTERN	= 0x2603;
  		WACS_LRCORNER   = 0x2518;
  		WACS_URCORNER   = 0x2510;
  		WACS_ULCORNER   = 0x250c;
  		WACS_LLCORNER   = 0x2514;
  		WACS_PLUS	   = 0x253c;
		WACS_HLINE	  = 0x2500;
		WACS_S1		 = 0x23ba;
		WACS_S9		 = 0x23bd;
  		WACS_LTEE	   = 0x251c;
  		WACS_RTEE	   = 0x2524;
  		WACS_BTEE	   = 0x2534;
  		WACS_TTEE	   = 0x252c;
		WACS_VLINE	  = 0x2502;
		WACS_BULLET	 = 0x00b7;
		WACS_S3		 = 0x23bb;
		WACS_S7		 = 0x23bc;
		WACS_LEQUAL	 = 0x2264;
		WACS_GEQUAL	 = 0x2265;
		WACS_PI		 = 0x03C0;
		WACS_NEQUAL	 = 0x2260;
		WACS_STERLING	 = 0x00A3;
	}

	if (t_acs_chars(screen->term) == NULL) {
#ifdef DEBUG
		__CTRACE(__CTRACE_INIT,
		    "__init_wacs: no alternative characters\n" );
#endif /* DEBUG */
		goto out;
	}

	aofac = t_acs_chars(screen->term);

	while (*aofac != '\0') {
		if ((acs = *aofac) == '\0')
			return;
		if (++aofac == '\0')
			return;
		if ((term = *aofac) == '\0')
			return;
	 	/* Only add characters 1 to 127 */
		if (acs < NUM_ACS) {
			_wacs_char[acs].vals[ 0 ] = term;
			_wacs_char[acs].attributes |= WA_ALTCHARSET;
		}
		aofac++;
#ifdef DEBUG
		__CTRACE(__CTRACE_INIT, "__init_wacs: %c = %c\n", acs, term);
#endif
	}

	if (t_ena_acs(screen->term) != NULL)
		ti_puts(screen->term, t_ena_acs(screen->term), 0,
			   __cputchar_args, screen->outfd);

out:
	for (count=0; count < NUM_ACS; count++)
		memcpy(&screen->wacs_char[count], &_wacs_char[count],
			sizeof(cchar_t));
}

void
_cursesi_reset_wacs(SCREEN *screen)
{
	int count;

	for (count=0; count < NUM_ACS; count++)
		memcpy( &_wacs_char[count], &screen->wacs_char[count],
			sizeof( cchar_t ));
}
#endif /* HAVE_WCHAR */
