#ifndef CDKTEMPLATE_H
#define CDKTEMPLATE_H	1

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
 * Define the CDK cdktemplate widget structure.
 */
struct STemplate {
   CDKOBJS	obj;
   WINDOW	*parent;
   WINDOW	*win;
   WINDOW	*labelWin;
   WINDOW	*fieldWin;
   WINDOW	*titleWin;
   chtype	*title[MAX_LINES];
   int		titlePos[MAX_LINES];
   int		titleLen[MAX_LINES];
   int		titleLines;
   chtype	*label;
   chtype	*overlay;
   chtype	overlayAttr;
   char		*plate;
   char		*info;
   int		labelLen;
   int		overlayLen;
   int		labelPos;
   int		fieldWidth;
   int		boxWidth;
   int		boxHeight;
   int		platePos;
   int		plateLen;
   int		screenPos;
   int		infoPos;
   int		min;
   chtype	labelAttr;
   chtype	fieldAttr;
   chtype	ULChar;
   chtype	URChar;
   chtype	LLChar;
   chtype	LRChar;
   chtype	VChar;
   chtype	HChar;
   chtype	BoxAttrib;
   EExitType	exitType;
   boolean	shadow;
   void *	callbackfn;
   PROCESSFN	preProcessFunction;
   void *	preProcessData;
   PROCESSFN	postProcessFunction;
   void *	postProcessData;
};
typedef struct STemplate CDKTEMPLATE;
typedef void (*TEMPLATECB) (CDKTEMPLATE *cdktemplate, chtype input);

/*
 * This creates a pointer to a new CDK cdktemplate widget.
 */
CDKTEMPLATE *newCDKTemplate (
		CDKSCREEN *	/* cdkscreen */,
		int 		/* xpos */,
		int 		/* ypos */,
		char *		/* title */,
		char *		/* label */,
		char *		/* plate */,
		char *		/* overlay */,
		boolean 	/* Box */,
		boolean 	/* shadow */);

/*
 * This activates the cdktemplate widget.
 */
char *activateCDKTemplate (
		CDKTEMPLATE *	/* cdktemplate */,
		chtype *	/* actions */);

/*
 * This injects a single character into the widget.
 */
char *injectCDKTemplate (
		CDKTEMPLATE *	/* cdktemplate */,
		chtype 		/* input */);

/*
 * This sets various attributes of the widget.
 */
void setCDKTemplate (
		CDKTEMPLATE *	/* cdktemplate */,
		char *		/* value */,
		boolean 	/* Box */);

/*
 * This sets the value in the cdktemplate widget.
 */
void setCDKTemplateValue (
		CDKTEMPLATE *	/* cdktemplate */,
		char *		/* value */);

char *getCDKTemplateValue (
		CDKTEMPLATE *	/* cdktemplate */);

/*
 * This sets the minimum number of characters to enter.
 */
void setCDKTemplateMin (
		CDKTEMPLATE *	/* cdktemplate */,
		int 		/* min */);

int getCDKTemplateMin (
		CDKTEMPLATE *	/* cdktemplate */);

/*
 * This sets the box attribute of the widget.
 */
void setCDKTemplateBox (
		CDKTEMPLATE *	/* cdktemplate */,
		boolean 	/* Box */);

boolean getCDKTemplateBox (
		CDKTEMPLATE *	/* cdktemplate */);

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKTemplateULChar (
		CDKTEMPLATE *	/* cdktemplate */,
		chtype 		/* character */);

void setCDKTemplateURChar (
		CDKTEMPLATE *	/* cdktemplate */,
		chtype 		/* character */);

void setCDKTemplateLLChar (
		CDKTEMPLATE *	/* cdktemplate */,
		chtype 		/* character */);

void setCDKTemplateLRChar (
		CDKTEMPLATE *	/* cdktemplate */,
		chtype 		/* character */);

void setCDKTemplateVerticalChar (
		CDKTEMPLATE *	/* cdktemplate */,
		chtype 		/* character */);

void setCDKTemplateHorizontalChar (
		CDKTEMPLATE *	/* cdktemplate */,
		chtype 		/* character */);

void setCDKTemplateBoxAttribute (
		CDKTEMPLATE *	/* cdktemplate */,
		chtype 		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKTemplateBackgroundColor (
		CDKTEMPLATE *	/* cdktemplate */,
		char *		/* color */);

/*
 * This draws the cdktemplate on the screen.
 */
#define drawCDKTemplate(obj,Box) drawCDKObject(obj,Box)

/*
 * This erases the widget from the screen.
 */
#define eraseCDKTemplate(obj) eraseCDKObject(obj)

/*
 * This erases the cdktemplates contents.
 */
void cleanCDKTemplate (
		CDKTEMPLATE *	/* cdktemplate */);

/*
 * This moves the widget to the given location on the screen.
 */
#define moveCDKTemplate(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This interactively positions the widget on the screen.
 */
#define positionCDKTemplate(widget) positionCDKObject(ObjOf(widget),widget->win)

/*
 * This destroys the widget and all associated memory.
 */
void destroyCDKTemplate (
		CDKTEMPLATE *	/* cdktemplate */);

/*
 * This sets the main callback function.
 */
void setCDKTemplateCB (
		CDKTEMPLATE *	/* cdktemplate */,
		TEMPLATECB 	/* callback */);

/*
 * This returns a character pointer to the contents of the cdktemplate
 * mixed with the plate.
 */
char *mixCDKTemplate (
		CDKTEMPLATE *	/* cdktemplate */);

/*
 * This returns a character pointer to the cdktemplate with the plate
 * stripped out.
 */
char *unmixCDKTemplate (
		CDKTEMPLATE *	/* cdktemplate */,
		char *		/* string */);

/*
 * These set the pre/post callback functions.
 */
void setCDKTemplatePreProcess (
		CDKTEMPLATE *	/* cdktemplate */,
		PROCESSFN 	/* callback */,
		void *		/* data */);

void setCDKTemplatePostProcess (
		CDKTEMPLATE *	/* cdktemplate */,
		PROCESSFN 	/* callback */,
		void *		/* data */);

#endif /* CDKTEMPLATE_H */
