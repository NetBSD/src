#ifndef CDKVIEWER_H
#define CDKVIEWER_H	1

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
 * Define the CDK viewer widget structure.
 */
struct SViewer {
   CDKOBJS	obj;
   WINDOW	*parent;
   WINDOW	*win;
   WINDOW	*infoWin;
   int		fieldWidth;
   chtype	*info[MAX_LINES];
   int		infoPos[MAX_LINES];
   int		infoLen[MAX_LINES];
   WINDOW	*buttonWin;
   chtype	*buttonLabel[MAX_BUTTONS];
   int		buttonPos[MAX_BUTTONS];
   int		buttonLen[MAX_BUTTONS];
   int		buttonCount;
   chtype	buttonHighlight;
   WINDOW	*titleWin;
   chtype	*title[MAX_LINES];
   int		titlePos[MAX_LINES];
   int		titleLen[MAX_LINES];
   int		titleLines;
   int		infoSize;
   int		boxHeight;
   int		boxWidth;
   int		viewSize;
   int		currentTop;
   int		currentButton;
   int		leftChar;
   int		length;
   int		maxLeftChar;
   int		maxTopLine;
   int		characters;
   chtype	ULChar;
   chtype	URChar;
   chtype	LLChar;
   chtype	LRChar;
   chtype	VChar;
   chtype	HChar;
   chtype	BoxAttrib;
   boolean	showLineInfo;
   boolean	interpret;
   EExitType	exitType;
   boolean	shadow;
};
typedef struct SViewer CDKVIEWER;

/*
 * This creates a pointer to a new CDK viewer widget.
 */
CDKVIEWER *newCDKViewer (
		CDKSCREEN *	/* cdkscreen */,
		int		/* xpos */,
		int		/* ypos */,
		int		/* height */,
		int		/* width */,
		char **		/* buttons */,
		int		/* buttonCount */,
		chtype		/* buttonHighlight */,
		boolean		/* Box */,
		boolean		/* shadow */);

/*
 * This activates the viewer widget.
 */
int activateCDKViewer (
		CDKVIEWER *	/* viewer */,
		chtype *	/* actions */);

/*
 * This sets various attributes of the viewer widget.
 */
int setCDKViewer (
		CDKVIEWER *	/* viewer */,
		char *		/* title */,
		char **		/* info */,
		int		/* infoSize */,
		chtype		/* buttonHighlight */,
		boolean		/* interpret */,
		boolean		/* showLineInfo */,
		boolean		/* Box */);

/*
 * This sets the contents of the viewer widget.
 */
int setCDKViewerInfo (
		CDKVIEWER *	/* viewer */,
		char **		/* info */,
		int		/* infoSize */,
		boolean		/* interpret */);

chtype **getCDKViewerInfo (
		CDKVIEWER *	/* viewer */,
		int *		/* size */);

/*
 * This sets the title of the viewer widget.
 */
void setCDKViewerTitle (
		CDKVIEWER *	/* viewer */,
		char *		/* title */);

chtype **getCDKViewerTitle (
		CDKVIEWER *	/* viewer */);

/*
 * This sets the highlight bar attribute.
 */
void setCDKViewerHighlight (
		CDKVIEWER *	/* viewer */,
		chtype		/* buttonHighlight */);

chtype getCDKViewerHighlight (
		CDKVIEWER *	/* viewer */);

/*
 * This sets the boolean flag dictating whether a information line
 * will be displayed in the top left corner of the viewer.
 */
void setCDKViewerInfoLine (
		CDKVIEWER *	/* viewer */,
		boolean		/* showLineInfo */);

boolean getCDKViewerInfoLine (
		CDKVIEWER *	/* viewer */);

/*
 * This sets the box attribute of the widget.
 */
void setCDKViewerBox (
		CDKVIEWER *	/* viewer */,
		boolean		/* Box */);

boolean getCDKViewerBox (
		CDKVIEWER *	/* viewer */);

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKViewerULChar (
		CDKVIEWER *	/* viewer */,
		chtype		/* character */);

void setCDKViewerURChar (
		CDKVIEWER *	/* viewer */,
		chtype		/* character */);

void setCDKViewerLLChar (
		CDKVIEWER *	/* viewer */,
		chtype		/* character */);

void setCDKViewerLRChar (
		CDKVIEWER *	/* viewer */,
		chtype		/* character */);

void setCDKViewerVerticalChar (
		CDKVIEWER *	/* viewer */,
		chtype		/* character */);

void setCDKViewerHorizontalChar (
		CDKVIEWER *	/* viewer */,
		chtype		/* character */);

void setCDKViewerBoxAttribute (
		CDKVIEWER *	/* viewer */,
		chtype		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKViewerBackgroundColor (
		CDKVIEWER *	/* viewer */,
		char *		/* color */);

/*
 * This draws the viewer field on the screen.
 */
#define drawCDKViewer(obj,box) drawCDKObject(obj,box)

/*
 * This erases the widget from the screen.
 */
#define eraseCDKViewer(obj) eraseCDKObject(obj)

/*
 * This moves the widget to the given location.
 */
#define moveCDKViewer(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This is an interactive method of moving the widget.
 */
#define positionCDKViewer(widget) positionCDKObject(ObjOf(widget),widget->win)

/*
 * This destroys the widget and all the memory associated with the widget.
 */
void destroyCDKViewer (
		CDKVIEWER *	/* viewer */);

#endif /* CDKVIEWER_H */
