#ifndef CDKDRAW_H
#define CDKDRAW_H	1

#include <cdk/cdk.h>

/*
 * Copyright 1999, Mike Glover
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
 *    must display the following acknowledgment:
 *      This product includes software developed by Mike Glover
 *      and contributors.
 * 4. Neither the name of Mike Glover, nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MIKE GLOVER AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL MIKE GLOVER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This boxes a window.
 */
void boxWindow (
		WINDOW *	/* window */,
		chtype		/* attr */);

/*
 * This draws a single line with the character supplied by 'character'
 */
void drawLine (
		WINDOW *	/* window */,
		int		/* startx */,
		int		/* starty */,
		int		/* endx */,
		int		/* endy */,
		chtype		/* character */);

/*
 * This writes a string segment on the given window.
 */
void writeChar (
		WINDOW *	/* window */,
		int		/* xpos */,
		int		/* ypos */,
		char *		/* string */,
		int		/* align */,
		int		/* start */,
		int		/* len */);

/*
 * This writes a string segment on the given window.
 */
void writeCharAttrib (
		WINDOW *	/* window */,
		int		/* xpos */,
		int		/* ypos */,
		char *		/* string */,
		chtype		/* attr */,
		int		/* align */,
		int		/* start */,
		int		/* len */);

/*
 * This writes a string segment on the given window.
 */
void writeChtype (
		WINDOW *	/* window */,
		int		/* xpos */,
		int		/* ypos */,
		chtype *	/* string */,
		int		/* align */,
		int		/* start */,
		int		/* len */);

/*
 * This writes a string segment on the given window.
 */
void writeChtypeAttrib (
		WINDOW *	/* window */,
		int		/* xpos */,
		int		/* ypos */,
		chtype *	/* string */,
		chtype		/* attr */,
		int		/* align */,
		int		/* start */,
		int		/* len */);

/*
 * This boxes a window using the given characters.
 */
void attrbox (
		WINDOW *	/* window */,
		chtype		/* tlc */,
		chtype		/* trc */,
		chtype		/* blc */,
		chtype		/* brc */,
		chtype		/* hor */,
		chtype		/* vert */,
		chtype		/* type */,
		int		/* shadow */);

/*
 * This draws the shadow window of the widget.
 */
void drawShadow (
		WINDOW *	/* window */);

#endif /* CDKDRAW_H */
