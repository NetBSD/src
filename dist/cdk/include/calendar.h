#ifndef CDKCALENDAR_H
#define CDKCALENDAR_H	1

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
 * Declare some definitions needed for this widget.
 */
#define MAX_DAYS	50
#define MAX_MONTHS	13
#define MAX_YEARS	502

/*
 * Define the CDK calendar widget structure.
 */
struct SCalendar {
   CDKOBJS	obj;
   WINDOW *	parent;
   WINDOW *	win;
   WINDOW *	labelWin;
   WINDOW *	fieldWin;
   chtype *	title[MAX_LINES];
   int		titlePos[MAX_LINES];
   int		titleLen[MAX_LINES];
   int		titleAdj;
   int		titleLines;
   int		xpos;
   int		ypos;
   int		height;
   int		width;
   int		fieldWidth;
   int		labelLen;
   chtype	yearAttrib;
   chtype	monthAttrib;
   chtype	dayAttrib;
   chtype	highlight;
   chtype	marker[MAX_DAYS][MAX_MONTHS][MAX_YEARS];
   int		day;
   int		month;
   int		year;
   int		weekDay;
   int		boxWidth;
   int		boxHeight;
   int		xOffset;
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
typedef struct SCalendar CDKCALENDAR;

/*
 * This creates a pointer to a new calendar widget.
 */
CDKCALENDAR *newCDKCalendar (
		CDKSCREEN *	/* screen */,
		int		/* xPos */,
		int		/* yPos */,
		char *		/* title */,
		int		/* day */,
		int		/* month */,
		int		/* year */,
		chtype		/* dayAttrib */,
		chtype		/* monthAttrib */,
		chtype		/* yearAttrib */,
		chtype		/* highlight */,
		boolean		/* Box */,
		boolean		/* shadow */);

/*
 * This activates the calendar widget.
 */
time_t activateCDKCalendar (
		CDKCALENDAR *	/* calendar */,
		chtype *	/* actions */);

/*
 * This injects a single character into the widget.
 */
time_t injectCDKCalendar (
		CDKCALENDAR *	/* calendar */,
		chtype		/* input */);

/*
 * This sets multiple attributes of the widget.
 */
void setCDKCalendar (
		CDKCALENDAR *	/* calendar */,
		int		/* day */,
		int		/* month */,
		int		/* year */,
		chtype		/* dayAttrib */,
		chtype		/* monthAttrib */,
		chtype		/* yearAttrib */,
		chtype		/* highlight */,
		boolean		/* Box */);

/*
 * This sets the date of the calendar.
 */
void setCDKCalendarDate (
		CDKCALENDAR *	/* calendar */,
		int		/* day */,
		int		/* month */,
		int		/* year */);

void getCDKCalendarDate (
		CDKCALENDAR *	/* calendar */,
		int *		/* day */,
		int *		/* month */,
		int *		/* year */);

/*
 * This sets the attribute of the days in the calendar.
 */
void setCDKCalendarDayAttribute (
		CDKCALENDAR *	/* calendar */,
		chtype		/* attribute */);

chtype getCDKCalendarDayAttribute (
		CDKCALENDAR *	/* calendar */);

/*
 * This sets the attribute of the month names in the calendar.
 */
void setCDKCalendarMonthAttribute (
		CDKCALENDAR *	/* calendar */,
		chtype		/* attribute */);

chtype getCDKCalendarMonthAttribute (
		CDKCALENDAR *	/* calendar */);

/*
 * This sets the attribute of the year in the calendar.
 */
void setCDKCalendarYearAttribute (
		CDKCALENDAR *	/* calendar */,
		chtype		/* attribute */);

chtype getCDKCalendarYearAttribute (
		CDKCALENDAR *	/* calendar */);

/*
 * This sets the attribute of the highlight bar.
 */
void setCDKCalendarHighlight (
		CDKCALENDAR *	/* calendar */,
		chtype		/* highlight */);

chtype getCDKCalendarHighlight (
		CDKCALENDAR *	/* calendar */);

/*
  * This sets the box attribute of the widget.
 */
void setCDKCalendarBox (
		CDKCALENDAR *	/* calendar */,
		boolean		/* Box */);

boolean getCDKCalendarBox (
		CDKCALENDAR *	/* calendar */);

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKCalendarULChar (
		CDKCALENDAR *	/* calendar */,
		chtype		/* character */);

void setCDKCalendarURChar (
		CDKCALENDAR *	/* calendar */,
		chtype		/* character */);

void setCDKCalendarLLChar (
		CDKCALENDAR *	/* calendar */,
		chtype		/* character */);

void setCDKCalendarLRChar (
		CDKCALENDAR *	/* calendar */,
		chtype		/* character */);

void setCDKCalendarVerticalChar (
		CDKCALENDAR *	/* calendar */,
		chtype		/* character */);

void setCDKCalendarHorizontalChar (
		CDKCALENDAR *	/* calendar */,
		chtype		/* character */);

void setCDKCalendarBoxAttribute (
		CDKCALENDAR *	/* calendar */,
		chtype		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKCalendarBackgroundColor (
		CDKCALENDAR *	/* calendar */,
		char *		/* color */);

/*
 * This sets a marker on the calendar.
 */
void setCDKCalendarMarker (
		CDKCALENDAR *	/* calendar */,
		int		/* day */,
		int		/* month */,
		int		/* year */,
		chtype		/* markerChar */);

/*
 * This removes a marker from the calendar.
 */
void removeCDKCalendarMarker (
		CDKCALENDAR *	/* calendar */,
		int		/* day */,
		int		/* month */,
		int		/* year */);

/*
 * This draws the widget on the screen.
 */
#define drawCDKCalendar(obj,box) drawCDKObject(obj,box)

/*
 * This removes the widget from the screen.
 */
#define eraseCDKCalendar(obj) eraseCDKObject(obj)

/*
 * This moves the widget to the given location.
 */
#define moveCDKCalendar(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This is an interactive method of moving the widget.
 */
#define positionCDKCalendar(widget) positionCDKObject(ObjOf(widget),widget->win)

/*
 * This destroys the calendar widget and all associated memory.
 */
void destroyCDKCalendar (
		CDKCALENDAR *	/* calendar */);

/*
 * This sets the pre and post process functions.
 */
void setCDKCalendarPreProcess (
		CDKCALENDAR *	/* calendar */,
		PROCESSFN	/* callback */,
		void *		/* data */);

void setCDKCalendarPostProcess (
		CDKCALENDAR *	/* calendar */,
		PROCESSFN	/* callback */,
		void *		/* data */);

#endif /* CDKCALENDAR_H */
