#ifndef CDKLABEL_H
#define CDKLABEL_H	1

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
 * Declare any definitions you need to make.
 */
#define MAX_LABEL_ROWS	300

/*
 * Declare the CDK label structure.
 */
struct SLabel {
   CDKOBJS	obj;
   WINDOW	*parent;
   WINDOW	*win;
   WINDOW	*infoWin;
   chtype	*info[MAX_LABEL_ROWS];
   int		infoLen[MAX_LABEL_ROWS];
   int		infoPos[MAX_LABEL_ROWS];
   int		boxWidth;
   int		boxHeight;
   int		xpos;
   int		ypos;
   int		rows;
   chtype	ULChar;
   chtype	URChar;
   chtype	LLChar;
   chtype	LRChar;
   chtype	VChar;
   chtype	HChar;
   chtype	BoxAttrib;
   boolean	shadow;
};
typedef struct SLabel CDKLABEL;

/*
 * This creates a new CDK label widget.
 */
CDKLABEL *newCDKLabel (
		CDKSCREEN *	/* screen */,
		int		/* xPos */,
		int		/* yPos */,
		char **		/* mesg */,
		int		/* rows */,
		boolean		/* Box */,
		boolean		/* shadow */);

/*
 * This was added to make the builder simpler. All this will
 * do is call drawCDKLabel.
 */
void activateCDKLabel (
		CDKLABEL *	/* label */,
		chtype *	/* actions */);

/*
 * This sets multiple attributes of the widget.
 */
void setCDKLabel (
		CDKLABEL *	/* label */,
		char **		/* message */,
		int		/* lines */,
		boolean		/* Box */);

/*
 * This sets the contents of the label.
 */
void setCDKLabelMessage (
		CDKLABEL *	/* label */,
		char **		/* mesg */,
		int		/* lines */);
chtype **getCDKLabelMessage (
		CDKLABEL *	/* label */,
		int *		/* size */);

/*
 * This sets the box attribute of the widget.
 */
void setCDKLabelBox (
		CDKLABEL *	/* label */,
		boolean		/* Box */);
boolean getCDKLabelBox (
		CDKLABEL *	/* label */);

/*
 * This draws the label.
 */
#define drawCDKLabel(obj,Box) drawCDKObject(obj,Box)

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKLabelULChar (
		CDKLABEL *	/* label */,
		chtype		/* character */);

void setCDKLabelURChar (
		CDKLABEL *	/* label */,
		chtype		/* character */);

void setCDKLabelLLChar (
		CDKLABEL *	/* label */,
		chtype		/* character */);

void setCDKLabelLRChar (
		CDKLABEL *	/* label */,
		chtype		/* character */);

void setCDKLabelVerticalChar (
		CDKLABEL *	/* label */,
		chtype		/* character */);

void setCDKLabelHorizontalChar (
		CDKLABEL *	/* label */,
		chtype		/* character */);

void setCDKLabelBoxAttribute (
		CDKLABEL *	/* label */,
		chtype		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKLabelBackgroundColor (
		CDKLABEL *	/* label */,
		char *		/* color */);

/*
 * This erases the label.
 */
#define eraseCDKLabel(obj) eraseCDKObject(obj)

/*
 * This destroys the label and the memory used by it.
 */
void destroyCDKLabel (
		CDKLABEL *	/* label */);

/*
 * This draws the label then waits for the user to press
 * the key defined by the 'key' parameter.
 */
char waitCDKLabel (
		CDKLABEL *	/* label */,
		char		/* key */);

/*
 * This moves the label.
 */
#define moveCDKLabel(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This allows the user to interactively position the label.
 */
#define positionCDKLabel(widget) positionCDKObject(ObjOf(widget),widget->win)

#endif /* CDKLABEL_H */
