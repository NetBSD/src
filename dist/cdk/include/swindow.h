#ifndef CDKSWINDOW_H
#define CDKSWINDOW_H	1

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
 * Define the CDK scrolling window widget structure.
 */
struct SSwindow {
   CDKOBJS	obj;
   WINDOW	*parent;
   WINDOW	*win;
   WINDOW	*titleWin;
   chtype	*title[MAX_LINES];
   int		titlePos[MAX_LINES];
   int		titleLen[MAX_LINES];
   int		titleLines;
   WINDOW	*fieldWin;
   int		fieldWidth;
   chtype	*info[MAX_LINES];
   int		infoPos[MAX_LINES];
   int		infoLen[MAX_LINES];
   int		maxTopLine;
   int		maxLeftChar;
   int		leftChar;
   int		currentTop;
   int		itemCount;
   int		boxWidth;
   int		boxHeight;
   int		viewSize;
   int		saveLines;
   chtype	ULChar;
   chtype	URChar;
   chtype	LLChar;
   chtype	LRChar;
   chtype	VChar;
   chtype	HChar;
   chtype	BoxAttrib;
   EExitType	exitType;
   boolean	shadow;
   PROCESSFN	preProcessFunction;
   void *	preProcessData;
   PROCESSFN	postProcessFunction;
   void *	postProcessData;
};
typedef struct SSwindow CDKSWINDOW;
typedef void (*SWINDOWCB) (CDKSWINDOW *swindow, chtype input);

/*
 * This creates a new pointer to a scrolling window widget.
 */
CDKSWINDOW *newCDKSwindow (
		CDKSCREEN *	/* cdkscreen */,
		int		/* xpos */,
		int		/* ypos */,
		int		/* height */,
		int		/* width */,
		char *		/* title */,
		int		/* saveLines */,
		boolean		/* Box */,
		boolean		/* shadow */);

/*
 * This activates the scrolling window.
 */
void activateCDKSwindow (
		CDKSWINDOW *	/* swindow */,
		chtype *	/* actions */);

/*
 * This injects a single character into the scrolling window.
 */
int injectCDKSwindow (
		CDKSWINDOW *	/* swindow */,
		chtype		/* input */);

/*
 * This executes the given command and puts the output of the
 * command into the scrolling window.
 */
int execCDKSwindow (
		CDKSWINDOW *	/* swindow */,
		char *		/* command */,
		int		/* insertPos */);

/*
 * This dumps the contents of the scrolling window to the given filename.
 */
int dumpCDKSwindow (
		CDKSWINDOW *	/* swindow */,
		char *		/* filename */);

/*
 * This jumps to the given line in the window.
 */
void jumpToLineCDKSwindow (
		CDKSWINDOW *	/* swindow */,
		int		/* line */);

/*
 * This saves the contents of the scrolling window via an
 * interactive window.
 */
void saveCDKSwindowInformation (
		CDKSWINDOW *	/* swindow */);

/*
 * This loads the window up with information from a filename
 * interactively provided.
 */
void loadCDKSwindowInformation (
		CDKSWINDOW *	/* swindow */);

/*
 * These functions set the attributes of the scrolling window.
 */
void setCDKSwindow (
		CDKSWINDOW *	/* swindow */,
		char **		/* info */,
		int		/* lines */,
		boolean		/* Box */);

/*
 * This sets the contents of the scrolling window.
 */
void setCDKSwindowContents (
		CDKSWINDOW *	/* swindow */,
		char **		/* info */,
		int		/* lines */);

chtype **getCDKSwindowContents (
		CDKSWINDOW *	/* swindow */,
		int *		/* size */);

/*
 * This sets the box attribute of the scrolling window.
 */
void setCDKSwindowBox (
		CDKSWINDOW *	/* swindow */,
		boolean		/* Box */);

boolean getCDKSwindowBox (
		CDKSWINDOW *	/* swindow */);

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKSwindowULChar (
		CDKSWINDOW *	/* swindow */,
		chtype		/* character */);

void setCDKSwindowURChar (
		CDKSWINDOW *	/* swindow */,
		chtype		/* character */);

void setCDKSwindowLLChar (
		CDKSWINDOW *	/* swindow */,
		chtype		/* character */);

void setCDKSwindowLRChar (
		CDKSWINDOW *	/* swindow */,
		chtype		/* character */);

void setCDKSwindowVerticalChar (
		CDKSWINDOW *	/* swindow */,
		chtype		/* character */);

void setCDKSwindowHorizontalChar (
		CDKSWINDOW *	/* swindow */,
		chtype		/* character */);

void setCDKSwindowBoxAttribute (
		CDKSWINDOW *	/* swindow */,
		chtype		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKSwindowBackgroundColor (
		CDKSWINDOW *	/* swindow */,
		char *		/* color */);

/*
 * This draws the scrolling window on the screen.
 */
#define drawCDKSwindow(obj,Box) drawCDKObject(obj,Box)

/*
 * This removes the widget from the screen.
 */
#define eraseCDKSwindow(obj) eraseCDKObject(obj)

/*
 * This cleans out all of the information from the window.
 */
void cleanCDKSwindow (
		CDKSWINDOW *	/* swindow */);

/*
 * This adds a line to the window.
 */
void addCDKSwindow (
		CDKSWINDOW *	/* swindow */,
		char *		/* info */,
		int		/* insertPos */);

/*
 * This trims lines from the window.
 */
void trimCDKSwindow (
		CDKSWINDOW *	/* swindow */,
		int		/* start */,
		int		/* finish */);

/*
 * This moves the window to the given location.
 */
#define moveCDKSwindow(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This interactively positions the widget on the screen.
 */
#define positionCDKSwindow(widget) positionCDKObject(ObjOf(widget),widget->win)

/*
 * This destroys the widget and all associated memory.
 */
void destroyCDKSwindow (
		CDKSWINDOW *	/* swindow */);

/*
 * These set the pre/post process callback functions.
 */
void setCDKSwindowPreProcess (
		CDKSWINDOW *	/* swindow */,
		PROCESSFN	/* callback */,
		void *		/* data */);

void setCDKSwindowPostProcess (
		CDKSWINDOW *	/* swindow */,
		PROCESSFN	/* callback */,
		void *		/* data */);

#endif /* CDKSWINDOW_H */
