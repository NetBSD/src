#ifndef CDKMARQUEE_H
#define CDKMARQUEE_H	1

#include <cdk.h>

/*
 * Description of the widget:
 *
 */

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
 * Define the CDK marquee widget structure.
 */
struct SMarquee {
   CDKOBJS	obj;
   WINDOW *	parent;
   WINDOW *	win;
   int		active;
   int		width;
   int		boxWidth;
   int		boxHeight;
   boolean	shadow;
   chtype	ULChar;
   chtype	URChar;
   chtype	LLChar;
   chtype	LRChar;
   chtype	VChar;
   chtype	HChar;
   chtype	BoxAttrib;
};
typedef struct SMarquee CDKMARQUEE;

/*
 * This creates a new marquee widget pointer.
 */
CDKMARQUEE *newCDKMarquee (
		CDKSCREEN *	/* cdkscreen */,
		int		/* xpos */,
		int		/* ypos */,
		int		/* width */,
		boolean		/* Box */,
		boolean		/* shadow */);

/*
 * This turns the marquee 'on'.
 */
int activateCDKMarquee (
		CDKMARQUEE *	/* marquee */,
		char *		/* message */,
		int		/* delay */,
		int		/* repeat */,
		boolean		/* Box */);

/*
 * This turns 'off' the marquee.
 */
void deactivateCDKMarquee (
		CDKMARQUEE *	/* marquee */);

/*
 * This draws the marquee on the screen.
 */
#define drawCDKMarquee(obj,Box) drawCDKObject(obj,Box)

/*
 * This removes the widget from the screen.
 */
#define eraseCDKMarquee(obj) eraseCDKObject(obj)

/*
 * This moves the widget to the given location.
 */
#define moveCDKMarquee(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This interactively positions the widget on the screen.
 */
#define positionCDKMarquee(widget) positionCDKObject(ObjOf(widget),widget->win)

/*
 * This destroys the marquee widget.
 */
void destroyCDKMarquee (
		CDKMARQUEE *	/* marquee */);

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKMarqueeULChar (
		CDKMARQUEE *	/* marquee */,
		chtype		/* character */);

void setCDKMarqueeURChar (
		CDKMARQUEE *	/* marquee */,
		chtype		/* character */);

void setCDKMarqueeLLChar (
		CDKMARQUEE *	/* marquee */,
		chtype		/* character */);

void setCDKMarqueeLRChar (
		CDKMARQUEE *	/* marquee */,
		chtype		/* character */);

void setCDKMarqueeVerticalChar (
		CDKMARQUEE *	/* marquee */,
		chtype		/* character */);

void setCDKMarqueeHorizontalChar (
		CDKMARQUEE *	/* marquee */,
		chtype		/* character */);

void setCDKMarqueeBoxAttribute (
		CDKMARQUEE *	/* marquee */,
		chtype		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKMarqueeBackgroundColor (
		CDKMARQUEE *	/* marquee */,
		char *		/* color */);

#endif /* CDKMARQUEE_H */
