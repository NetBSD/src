#ifndef CDKMENTRY_H
#define CDKMENTRY_H	1

#include <cdk/cdk.h>

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
 * Define the CDK multiple line entry field widget.
 */
struct SMentry {
   CDKOBJS	obj;
   WINDOW	*parent;
   WINDOW	*win;
   WINDOW	*titleWin;
   chtype	*title[MAX_LINES];
   int		titlePos[MAX_LINES];
   int		titleLen[MAX_LINES];
   int		titleLines;
   WINDOW	*labelWin;
   chtype	*label;
   int		labelLen;
   WINDOW	*fieldWin;
   chtype	fieldAttr;
   int		fieldWidth;
   char		*info;
   int		infoWidth;
   int		boxHeight;
   int		boxWidth;
   int		totalWidth;
   int		rows;
   int		currentRow;
   int		currentCol;
   int		topRow;
   EDisplayType dispType;
   int		min;
   int		logicalRows;
   EExitType	exitType;
   boolean	shadow;
   chtype	ULChar;
   chtype	URChar;
   chtype	LLChar;
   chtype	LRChar;
   chtype	VChar;
   chtype	HChar;
   chtype	BoxAttrib;
   chtype	filler;
   chtype	hidden;
   void *	callbackfn;
   PROCESSFN	preProcessFunction;
   void *	preProcessData;
   PROCESSFN	postProcessFunction;
   void *	postProcessData;
};
typedef struct SMentry CDKMENTRY;
typedef void (*MENTRYCB) (CDKMENTRY *mentry, chtype character);

/*
 * This creates a new CDK mentry field pointer.
 */
CDKMENTRY *newCDKMentry (
		CDKSCREEN *	/* cdkscreen */,
		int		/* xpos */,
		int		/* ypos */,
		char *		/* title */,
		char *		/* label */,
		chtype		/* fieldAttr */,
		chtype		/* filler */,
		EDisplayType	/* disptype */,
		int		/* fieldWidth */,
		int		/* fieldrows */,
		int		/* logicalRows */,
		int		/* min */,
		boolean		/* Box */,
		boolean		/* shadow */);

/*
 * This activates the widget.
 */
char *activateCDKMentry (
		CDKMENTRY *	/* mentry */,
		chtype *	/* input */);

/*
 * This injects a single character into the widget.
 */
char *injectCDKMentry (
		CDKMENTRY *	/* mentry */,
		chtype		/* input */);

/*
 * These set specific attributes of the widget.
 */
void setCDKMentry (
		CDKMENTRY *	/* mentry */,
		char *		/* value */,
		int		/* min */,
		boolean		/* Box */);

/*
 * This sets the value of the widget.
 */
void setCDKMentryValue (
		CDKMENTRY *	/* mentry */,
		char *		/* value */);

char *getCDKMentryValue (
		CDKMENTRY *	/* mentry */);

/*
 * This sets the minimum length of the value for the widget.
 */
void setCDKMentryMin (
		CDKMENTRY *	/* mentry */,
		int		/* min */);

int getCDKMentryMin (
		CDKMENTRY *	/* mentry */);

/*
 * This sets the filler character to use when drawing the widget.
 */
void setCDKMentryFillerChar (
		CDKMENTRY *	/* mentry */,
		chtype		/* filler */);

chtype getCDKMentryFillerChar (
		CDKMENTRY *	/* mentry */);

/*
 * This sets the character to use when a hidden character type is used.
 */
void setCDKMentryHiddenChar (
		CDKMENTRY *	/* mentry */,
		chtype		/* character */);

chtype getCDKMentryHiddenChar (
		CDKMENTRY *	/* mentry */);

/*
 * This sets the box attribute of the mentry widget.
 */
void setCDKMentryBox (
		CDKMENTRY *	/* mentry */,
		boolean		/* Box */);

boolean getCDKMentryBox (
		CDKMENTRY *	/* mentry */);

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKMentryULChar (
		CDKMENTRY *	/* mentry */,
		chtype		/* character */);

void setCDKMentryURChar (
		CDKMENTRY *	/* mentry */,
		chtype		/* character */);

void setCDKMentryLLChar (
		CDKMENTRY *	/* mentry */,
		chtype		/* character */);

void setCDKMentryLRChar (
		CDKMENTRY *	/* mentry */,
		chtype		/* character */);

void setCDKMentryVerticalChar (
		CDKMENTRY *	/* mentry */,
		chtype		/* character */);

void setCDKMentryHorizontalChar (
		CDKMENTRY *	/* mentry */,
		chtype		/* character */);

void setCDKMentryBoxAttribute (
		CDKMENTRY *	/* mentry */,
		chtype		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKMentryBackgroundColor (
		CDKMENTRY *	/* mentry */,
		char *		/* color */);

/*
 * This draws the widget on the screen.
 */
#define drawCDKMentry(obj,Box) drawCDKObject(obj,Box)

/*
 * This erases the widget from the screen.
 */
#define eraseCDKMentry(obj) eraseCDKObject(obj)

/*
 * This cleans out the information in the widget.
 */
void cleanCDKMentry (
		CDKMENTRY *	/* mentry */);

/*
 * This moves the widget to the given location.
 */
#define moveCDKMentry(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This interactively moves the widget on the screen.
 */
#define positionCDKMentry(widget) positionCDKObject(ObjOf(widget),widget->win)

/*
 * This destroys the widget pointer.
 */
void destroyCDKMentry (
		CDKMENTRY *	/* mentry */);

/*
 * This draws the field of the widget.
 */
void drawCDKMentryField (
		CDKMENTRY *	/* mentry */);

/*
 * This sets the widgets main callback.
 */
void setCDKMentryCB (
		CDKMENTRY *	/* mentry */,
		MENTRYCB	/* callback */);

/*
 * These set the pre/post process callback functions.
 */
void setCDKMentryPreProcess (
		CDKMENTRY *	/* mentry */,
		PROCESSFN	/* callback */,
		void *		/* data */);

void setCDKMentryPostProcess (
		CDKMENTRY *	/* mentry */,
		PROCESSFN	/* callback */,
		void *		/* data */);

#endif /* CDKMENTRY_H */
