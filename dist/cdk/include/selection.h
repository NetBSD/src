#ifndef CDKSELECTION_H
#define CDKSELECTION_H	1

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
 *	This product includes software developed by Mike Glover
 *	and contributors.
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
 * Declare selection list definitions.
 */
#define MAX_CHOICES	100

/*
 * Define the CDK selection widget structure.
 */
struct SSelection {
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
   int		itemLen[MAX_ITEMS];
   int		itemPos[MAX_ITEMS];
   chtype	*choice[MAX_CHOICES];
   int		choicelen[MAX_CHOICES];
   int		choiceCount;
   int		maxchoicelen;
   int		selections[MAX_ITEMS];
   int		mode[MAX_ITEMS];
   int		maxTopItem;
   int		maxLeftChar;
   int		leftChar;
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
typedef struct SSelection CDKSELECTION;

/*
 * This creates a new pointer to a selection widget.
 */
CDKSELECTION *newCDKSelection (
		CDKSCREEN *	/* cdkscreen */,
		int		/* xpos */,
		int		/* ypos */,
		int		/* spos */,
		int		/* height */,
		int		/* width */,
		char *		/* title */,
		char **		/* list */,
		int		/* listSize */,
		char **		/* choices */,
		int		/* choiceSize */,
		chtype		/* highlight */,
		boolean		/* Box */,
		boolean		/* shadow */);

/*
 * This activates the selection widget.
 */
int activateCDKSelection (
		CDKSELECTION *	/* selection */,
		chtype *	/* actions */);

/*
 * This injects a single character into the widget.
 */
int injectCDKSelection (
		CDKSELECTION *	/* selection */,
		chtype		/* input */);

/*
 * This draws the selection widget.
 */
#define drawCDKSelection(obj,Box) drawCDKObject(obj,Box)

/*
 * This erases the selection widget from the screen.
 */
#define eraseCDKSelection(obj) eraseCDKObject(obj)

/*
 * This moves the widget to the given location.
 */
#define moveCDKSelection(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This interactively moves the widget on the screen.
 */
#define positionCDKSelection(widget) positionCDKObject(ObjOf(widget),widget->win)

/*
 * This destroys the widget and all associated memory.
 */
void destroyCDKSelection (
		CDKSELECTION *	/* selection */);

/*
 * This sets various attributes of the selection widget.
 */
void setCDKSelection (
		CDKSELECTION *	/* selection */,
		chtype		/* highlight */,
		int		/* defChoices */ [],
		boolean		/* Box */);

/*
 * This sets the contents of the selection list.
 */
void setCDKSelectionItems (
		CDKSELECTION *	/* selection */,
		char **		/* list */,
		int		/* listSize */);

int getCDKSelectionItems (
		CDKSELECTION *	/* selection */,
		char *		/* list */ []);

/*
 * This sets the selection list highlight bar.
 */
void setCDKSelectionHighlight (
		CDKSELECTION *	/* selection */,
		chtype		/* highlight */);

chtype getCDKSelectionHighlight (
		CDKSELECTION *	/* selection */);

/*
 * This sets the choices of the selection list.
 */
void setCDKSelectionChoices (
		CDKSELECTION *	/* selection */,
		int		/* choices */ []);

int *getCDKSelectionChoices (
		CDKSELECTION *	/* selection */);

void setCDKSelectionChoice (
		CDKSELECTION *	/* selection */,
		int		/* index */,
		int		/* choice */);

int getCDKSelectionChoice (
		CDKSELECTION *	/* selection */,
		int		/* index */);

/*
 * This sets the modes of the items in the selection list. Currently
 * there are only two: editable=0 and read-only=1
 */
void setCDKSelectionModes (
		CDKSELECTION *	/* selection */,
		int		/* modes */ []);

int *getCDKSelectionModes (
		CDKSELECTION *	/* selection */);

void setCDKSelectionMode (
		CDKSELECTION *	/* selection */,
		int		/* index */,
		int		/* mode */);

int getCDKSelectionMode (
		CDKSELECTION *	/* selection */,
		int		/* index */);

/*
 * This sets the box attribute of the widget.
 */
void setCDKSelectionBox (
		CDKSELECTION *	/* selection */,
		boolean		/* Box */);

boolean getCDKSelectionBox (
		CDKSELECTION *	/* selection */);

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKSelectionULChar (
		CDKSELECTION *	/* selection */,
		chtype		/* character */);

void setCDKSelectionURChar (
		CDKSELECTION *	/* selection */,
		chtype		/* character */);

void setCDKSelectionLLChar (
		CDKSELECTION *	/* selection */,
		chtype		/* character */);

void setCDKSelectionLRChar (
		CDKSELECTION *	/* selection */,
		chtype		/* character */);

void setCDKSelectionVerticalChar (
		CDKSELECTION *	/* selection */,
		chtype		/* character */);

void setCDKSelectionHorizontalChar (
		CDKSELECTION *	/* selection */,
		chtype		/* character */);

void setCDKSelectionBoxAttribute (
		CDKSELECTION *	/* selection */,
		chtype		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKSelectionBackgroundColor (
		CDKSELECTION *	/* selection */,
		char *		/* color */);

/*
 * These set the pre/post process callback functions.
 */
void setCDKSelectionPreProcess (
		CDKSELECTION *	/* selection */,
		PROCESSFN	/* callback */,
		void *		/* data */);

void setCDKSelectionPostProcess (
		CDKSELECTION *	/* selection */,
		PROCESSFN	/* callback */,
		void *		/* data */);

#endif /* CDKSELECTION_H */
