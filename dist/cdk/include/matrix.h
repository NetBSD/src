#ifndef CDKMATRIX_H
#define CDKMATRIX_H	1

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
 * Declare some matrix definitions.
 */
#define MAX_MATRIX_ROWS 1000
#define MAX_MATRIX_COLS 1000

/*
 * Define the CDK matrix widget structure.
 */
struct SMatrix {
   CDKOBJS	obj;
   WINDOW *	parent;
   WINDOW *	win;
   WINDOW *	cell[MAX_MATRIX_ROWS][MAX_MATRIX_COLS];
   char *	info[MAX_MATRIX_ROWS][MAX_MATRIX_COLS];
   chtype *	title[MAX_LINES];
   int		titlePos[MAX_LINES];
   int		titleLen[MAX_LINES];
   int		titleAdj;
   int		titleLines;
   int		rows;
   int		cols;
   int		vrows;
   int		vcols;
   int		colwidths[MAX_MATRIX_COLS];
   int		colvalues[MAX_MATRIX_COLS];
   chtype *	coltitle[MAX_MATRIX_COLS];
   int		coltitleLen[MAX_MATRIX_ROWS];
   int		coltitlePos[MAX_MATRIX_ROWS];
   int		maxct;
   chtype *	rowtitle[MAX_MATRIX_ROWS];
   int		rowtitleLen[MAX_MATRIX_ROWS];
   int		rowtitlePos[MAX_MATRIX_ROWS];
   int		maxrt;
   int		boxHeight;
   int		boxWidth;
   int		rowSpace;
   int		colSpace;
   int		row;
   int		col;
   int		crow;
   int		ccol;
   int		trow;
   int		lcol;
   int		oldcrow;
   int		oldccol;
   int		oldvrow;
   int		oldvcol;
   EExitType	exitType;
   boolean	boxCell;
   boolean	shadow;
   chtype	ULChar;
   chtype	URChar;
   chtype	LLChar;
   chtype	LRChar;
   chtype	VChar;
   chtype	HChar;
   chtype	BoxAttrib;
   chtype	highlight;
   int		dominant;
   chtype	filler;
   void *	callbackfn;
   PROCESSFN	preProcessFunction;
   void *	preProcessData;
   PROCESSFN	postProcessFunction;
   void *	postProcessData;
};
typedef struct SMatrix CDKMATRIX;
typedef void (*MATRIXCB) (CDKMATRIX *matrix, chtype input);

/*
 * This creates a new pointer to a matrix widget.
 */
CDKMATRIX *newCDKMatrix (
		CDKSCREEN *	/* cdkscreen */,
		int		/* xpos */,
		int		/* ypos */,
		int		/* rows */,
		int		/* cols */,
		int		/* vrows */,
		int		/* vcols */,
		char *		/* title */,
		char **		/* rowtitles */,
		char **		/* coltitles */,
		int *		/* colwidths */,
		int *		/* coltypes */,
		int		/* rowspace */,
		int		/* colspace */,
		chtype		/* filler */,
		int		/* dominantAttrib */,
		boolean		/* boxMatrix */,
		boolean		/* boxCell */,
		boolean		/* shadow */);

/*
 * This activates the matrix.
 */
int activateCDKMatrix (
		CDKMATRIX *	/* matrix */,
		chtype *	/* actions */);

/*
 * This injects a single character into the matrix widget.
 */
int injectCDKMatrix (
		CDKMATRIX *	/* matrix */,
		chtype		/* input */);

/*
 * These set specific attributes of the matrix widget.
 */
void setCDKMatrix (
		CDKMATRIX *	/* matrix */,
		char *		/* info */ [MAX_MATRIX_ROWS][MAX_MATRIX_COLS],
		int		/* rows */,
		int *		/* subSize */);

/*
 * This sets the value of a given cell.
 */
int setCDKMatrixCell (
		CDKMATRIX *	/* matrix */,
		int		/* row */,
		int		/* col */,
		char *		/* value */);

char *getCDKMatrixCell (
		CDKMATRIX *	/* matrix */,
		int		/* row */,
		int		/* col */);

/*
 * This returns the row/col of the matrix.
 */
int getCDKMatrixCol (
		CDKMATRIX *	/* matrix */);

int getCDKMatrixRow (
		CDKMATRIX *	/* matrix */);

/*
 * These functions set the drawing characters of the widget.
 */
void setCDKMatrixULChar (
		CDKMATRIX *	/* matrix */,
		chtype		/* character */);

void setCDKMatrixURChar (
		CDKMATRIX *	/* matrix */,
		chtype		/* character */);

void setCDKMatrixLLChar (
		CDKMATRIX *	/* matrix */,
		chtype		/* character */);

void setCDKMatrixLRChar (
		CDKMATRIX *	/* matrix */,
		chtype		/* character */);

void setCDKMatrixVerticalChar (
		CDKMATRIX *	/* matrix */,
		chtype		/* character */);

void setCDKMatrixHorizontalChar (
		CDKMATRIX *	/* matrix */,
		chtype		/* character */);

void setCDKMatrixBoxAttribute (
		CDKMATRIX *	/* matrix */,
		chtype		/* character */);

/*
 * This sets the background color of the widget.
 */
void setCDKMatrixBackgroundColor (
		CDKMATRIX *	/* matrix */,
		char *		/* color */);

/*
 * This draws the matrix on the screen.
 */
#define drawCDKMatrix(obj,Box) drawCDKObject(obj,Box)

/*
 * This removes the matrix from the screen.
 */
#define eraseCDKMatrix(obj) eraseCDKObject(obj)

/*
 * This cleans out all the cells from the matrix.
 */
void cleanCDKMatrix (
		CDKMATRIX *	/* matrix */);

/*
 * This sets the main callback in the matrix.
 */
void setCDKMatrixCB (
		CDKMATRIX *	/* matrix */,
		MATRIXCB	/* callback */);

/*
 * This moves the matrix to the given cell.
 */
int moveToCDKMatrixCell (
		CDKMATRIX *	/* matrix */,
		int		/* newrow */,
		int		/* newcol */);

/*
 * This moves the matrix on the screen to the given location.
 */
#define moveCDKMatrix(obj,xpos,ypos,relative,refresh) moveCDKObject(obj,xpos,ypos,relative,refresh)

/*
 * This allows the user to interactively position the matrix.
 */
#define positionCDKMatrix(widget) positionCDKObject(ObjOf(widget),widget->win)

/*
 * This destroys the matrix widget and associated memory.
 */
void destroyCDKMatrix (
		CDKMATRIX *	/* matrix */);

/*
 * This jumps to the given matrix cell. You can pass in
 * -1 for both the row/col if you want to interactively
 * pick the cell.
 */
int jumpToCell (
		CDKMATRIX *	/* matrix */,
		int		/* row */,
		int		/* col */);

/*
 * These set the pre/post process callback functions.
 */
void setCDKMatrixPreProcess (
		CDKMATRIX *	/* matrix */,
		PROCESSFN	/* callback */,
		void *		/* data */);

void setCDKMatrixPostProcess (
		CDKMATRIX *	/* matrix */,
		PROCESSFN	/* callback */,
		void *		/* data */);

#endif /* CDKMATRIX_H */
