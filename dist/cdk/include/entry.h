#ifndef CDKENTRY_H
#define CDKENTRY_H	1

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
 * Define the CDK entry widget structure.
 */
struct SEntry {
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
   int		screenCol;
   int		leftChar;
   int		min;
   int		max;
   int		boxWidth;
   int		boxHeight;
   EExitType	exitType;
   EDisplayType dispType;
   boolean	shadow;
   chtype	filler;
   chtype	hidden;
   chtype	ULChar;
   chtype	URChar;
   chtype	LLChar;
   chtype	LRChar;
   chtype	VChar;
   chtype	HChar;
   chtype	BoxAttrib;
   void *	callbackfn;
   PROCESSFN	preProcessFunction;
   void *	preProcessData;
   PROCESSFN	postProcessFunction;
   void *	postProcessData;
};
typedef struct SEntry CDKENTRY;
typedef void (*ENTRYCB) (CDKENTRY *entry, chtype character);

/*
 * This creates a pointer to a new CDK entry widget.
 */
CDKENTRY *newCDKEntry (
		CDKSCREEN *	/* cdkscreen */,
		int		/* xpos */,
		int		/* ypos */,
		char *		/* title */,
		char *		/* label */,
		chtype		/* fieldAttrib */,
		chtype		/* filler */,
		EDisplayType	/* disptype */,
		int		/* fieldWidth */,
		int		/* min */,
		int		/* max */,
		boolean		/* Box */,
		boolean		/* shadow */);

/*
 * This activates the entry widget.
 */
char *activateCDKEntry (
		CDKENTRY *	/* entry */,
		chtype *	/* actions */);

/*
 * This injects a single character into the widget.
 */
char *injectCDKEntry (
		CDKENTRY *	/* entry */,
		chtype		/* input */);

/*
 * This sets various attributes of the entry field.
 */
void setCDKEntry (
		CDKENTRY *	/* entry */,
		char *		/* value */,
		int		/* min */,
		int		/* max */,
		boolean		/* Box */);

/*
 * This sets the value of the entry field.
 */
void setCDKEntryValue (
		CDKENTRY *	/* entry */,
		char *		/* value */);
char *getCDKEntryValue (
		CDKENTRY *	/* entry */);

/*
 * This sets the maximum length of a string allowable for
 * the given widget.
 */
void setCDKEntryMax (
		CDKENTRY *	/* entry */,
		int		/* max */);
int getCDKEntryMax (
		CDKENTRY *	/* entry */);

/*
 * This sets the minimum length of a string allowable for
 * the given widget.
 */
void setCDKEntryMin (
		CDKENTRY *	/* entry */,
		int		/* min */);
int getCDKEntryMin (
		CDKENTRY *	/* entry */);

/*
 * This sets the filler character of the entry field.
 */
void setCDKEntryFillerChar (
		CDKENTRY *	/* entry */,
		chtype		/* fillerCharacter */);
chtype getCDKEntryFillerChar (
		CDKENTRY *	/* entry */);

/*
 * This sets the character to use when a hidden type is used.
 */
void setCDKEntryHiddenChar (
		CDKENTRY *	/* entry */,
		chtype		/* hiddenCharacter */);
chtype getCDKEntryHiddenChar (
		CDKENTRY *	/* entry */);

/*
 * This sets the box attribute of the widget.
 */
void setCDKEntryBox (
		CDKENTRY *	/* entry */,
		boolean		/* Box */);
boolean getCDKEntryBox (
		CDKENTRY *	/* entry */);

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKEntryULChar (
		CDKENTRY *	/* entry */,
		chtype		/* character */);
void setCDKEntryURChar (
		CDKENTRY *	/* entry */,
		chtype		/* character */);
void setCDKEntryLLChar (
		CDKENTRY *	/* entry */,
		chtype		/* character */);
void setCDKEntryLRChar (
		CDKENTRY *	/* entry */,
		chtype		/* character */);
void setCDKEntryVerticalChar (
		CDKENTRY *	/* entry */,
		chtype		/* character */);
void setCDKEntryHorizontalChar (
		CDKENTRY *	/* entry */,
		chtype		/* character */);
void setCDKEntryBoxAttribute (
		CDKENTRY *	/* entry */,
		chtype		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKEntryBackgroundColor (
		CDKENTRY *	/* entry */,
		char *		/* color */);

/*
 * This draws the entry field.
 */
#define drawCDKEntry(obj,box) drawCDKObject(obj,box)

/*
 * This erases the widget from the screen.
 */
#define eraseCDKEntry(obj) eraseCDKObject(obj)

/*
 * This cleans out the value of the entry field.
 */
void cleanCDKEntry (
		CDKENTRY *	/* entry */);

/*
 * This moves the widget to the given location.
 */
#define moveCDKEntry(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This is an interactive method of moving the widget.
 */
#define positionCDKEntry(widget) positionCDKObject(ObjOf(widget),widget->win)

/*
 * This destroys the widget and all the memory associated with the widget.
 */
void destroyCDKEntry (
		CDKENTRY *	/* entry */);

/*
 * This sets the callback to the entry fields main handler
 */
void setCDKEntryCB (
		CDKENTRY *	/* entry */,
		ENTRYCB		/* callback */);

/*
 * These set the callbacks to the pre and post process functions.
 */
void setCDKEntryPreProcess (
		CDKENTRY *	/* entry */,
		PROCESSFN	/* callback */,
		void *		/* data */);
void setCDKEntryPostProcess (
		CDKENTRY *	/* entry */,
		PROCESSFN	/* callback */,
		void *		/* data */);

#endif /* CDKENTRY_H */
