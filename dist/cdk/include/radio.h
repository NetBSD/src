#ifndef CDKRADIO_H
#define CDKRADIO_H	1

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
 * Define the CDK radio list widget structure.
 */
struct SRadio {
   CDKOBJS	obj;
   WINDOW	*parent;
   WINDOW	*win;
   WINDOW	*scrollbarWin;
   WINDOW	*titleWin;
   chtype	*title[MAX_LINES];
   int		titlePos[MAX_LINES];
   int		titleLen[MAX_LINES];
   int		titleLines;
   WINDOW	*fieldWin;
   int		fieldWidth;
   chtype	*item[MAX_ITEMS];
   int		itemPos[MAX_ITEMS];
   int		itemLen[MAX_ITEMS];
   chtype	choiceChar;
   chtype	leftBoxChar;
   chtype	rightBoxChar;
   int		maxTopItem;
   int		maxLeftChar;
   int		leftChar;
   int		selectedItem;
   int		currentTop;
   int		currentItem;
   int		currentHigh;
   int		listSize;
   int		boxWidth;
   int		boxHeight;
   int		viewSize;
   boolean	scrollbar;
   int		toggleSize;
   chtype	ULChar;
   chtype	URChar;
   chtype	LLChar;
   chtype	LRChar;
   chtype	VChar;
   chtype	HChar;
   chtype	BoxAttrib;
   EExitType	exitType;
   boolean	shadow;
   chtype	highlight;
   PROCESSFN	preProcessFunction;
   void *	preProcessData;
   PROCESSFN	postProcessFunction;
   void *	postProcessData;
};
typedef struct SRadio CDKRADIO;

/*
 * This creates a new radio widget pointer.
 */
CDKRADIO *newCDKRadio (
		CDKSCREEN *	/* cdkscreen */,
		int		/* xpos */,
		int		/* ypos */,
		int		/* spos */,
		int		/* height */,
		int		/* width */,
		char *		/* title */,
		char **		/* mesg */,
		int		/* items */,
		chtype		/* choiceChar */,
		int		/* defItem */,
		chtype		/* highlight */,
		boolean		/* Box */,
		boolean		/* shadow */);

/*
 * This activates the widget.
 */
int activateCDKRadio (
		CDKRADIO *	/* radio */,
		chtype *	/* actions */);

/*
 * This injects a single character into the widget.
 */
int injectCDKRadio (
		CDKRADIO *	/* radio */,
		chtype		/* input */);

/*
 * These set various attributes of the widget.
 */
void setCDKRadio (
		CDKRADIO *	/* radio */,
		chtype		/* highlight */,
		chtype		/* character */,
		boolean		/* Box */);

/*
 * This sets the contents of the radio list.
 */
void setCDKRadioItems (
		CDKRADIO *	/* radio */,
		char **		/* list */,
		int		/* listSize */);

int getCDKRadioItems (
		CDKRADIO *	/* radio */,
		char *		/* list */ []);

/*
 * This sets the highlight bar attribute.
 */
void setCDKRadioHighlight (
		CDKRADIO *	/* radio */,
		chtype		/* highlight */);

chtype getCDKRadioHighlight (
		CDKRADIO *	/* radio */);

/*
 * This sets the 'selected' character.
 */
void setCDKRadioChoiceCharacter (
		CDKRADIO *	/* radio */,
		chtype		/* character */);

chtype getCDKRadioChoiceCharacter (
		CDKRADIO *	/* radio */);

/*
 * This sets the character to draw on the left/right side of
 * the choice box.
 */
void setCDKRadioLeftBrace (
		CDKRADIO *	/* radio */,
		chtype		/* character */);

chtype getCDKRadioLeftBrace (
		CDKRADIO *	/* radio */);

void setCDKRadioRightBrace (
		CDKRADIO *	/* radio */,
		chtype		/* character */);

chtype getCDKRadioRightBrace (
		CDKRADIO *	/* radio */);

/*
 * This sets the box attribute of the widget.
 */
void setCDKRadioBox (
		CDKRADIO *	/* radio */,
		boolean		/* Box */);

boolean getCDKRadioBox (
		CDKRADIO *	/* radio */);

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKRadioULChar (
		CDKRADIO *	/* radio */,
		chtype		/* character */);

void setCDKRadioURChar (
		CDKRADIO *	/* radio */,
		chtype		/* character */);

void setCDKRadioLLChar (
		CDKRADIO *	/* radio */,
		chtype		/* character */);

void setCDKRadioLRChar (
		CDKRADIO *	/* radio */,
		chtype		/* character */);

void setCDKRadioVerticalChar (
		CDKRADIO *	/* radio */,
		chtype		/* character */);

void setCDKRadioHorizontalChar (
		CDKRADIO *	/* radio */,
		chtype		/* character */);

void setCDKRadioBoxAttribute (
		CDKRADIO *	/* radio */,
		chtype		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKRadioBackgroundColor (
		CDKRADIO *	/* radio */,
		char *		/* color */);

/*
 * This draws the widget on the screen.
 */
#define drawCDKRadio(obj,Box) drawCDKObject(obj,Box)

/*
 * This erases the widget from the screen.
 */
#define eraseCDKRadio(obj) eraseCDKObject(obj)

/*
 * This moves the widget to the given screen location.
 */
#define moveCDKRadio(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This interactively moves the widget to a new location on the screen.
 */
#define positionCDKRadio(widget) positionCDKObject(ObjOf(widget),widget->win)

/*
 * This destroys a widget pointer.
 */
void destroyCDKRadio (
		CDKRADIO *	/* radio */);

/*
 * These set the pre/post process callback functions.
 */
void setCDKRadioPreProcess (
		CDKRADIO *	/* radio */,
		PROCESSFN	/* callback */,
		void *		/* data */);

void setCDKRadioPostProcess (
		CDKRADIO *	/* radio */,
		PROCESSFN	/* callback */,
		void *		/* data */);

#endif /* CDKRADIO_H */
