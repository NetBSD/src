#ifndef CDKUTIL_H
#define CDKUTIL_H	1

#include <cdk.h>

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
 * This beeps at the user. The standard curses beep() does not
 * flush the stream, so it will only beep until a force is made.
 * This forces a flush after issuing the beep command.
 */
void Beep(void);

/*
 * This aligns a 'box' on the given window with the height and
 * width given.
 */
void alignxy (
		WINDOW *	/* window */,
		int *		/* xpos */,
		int *		/* ypos */,
		int		/* boxWidth */,
		int		/* boxHeight */);

/*
 * This returns a substring of the given string.
 */
char *substring (
		char *		/* string */,
		int		/* start */,
		int		/* width */);

/*
 * This takes a string, a field width and a justification type and returns the
 * justification adjustment to make, to fill the justification requirement.
 */
int justifyString (
		int		/* fieldWidth */,
		int		/* mesglen */,
		int		/* justify */);

/*
 * This is a quick little popup label widget.
 */
void popupLabel (
		CDKSCREEN *	/* win */,
		char **		/* mesg */,
		int		/* count */);

/*
 * This is a quick little popup dialog box.
 */
int popupDialog (
		CDKSCREEN *	/* screen */,
		char **		/* mesg */,
		int		/* mesgCount */,
		char **		/* buttons */,
		int		/* buttonCount */);

/*
 * This pops up a scrolling list and returns the value selected.
 */
int getListIndex (
		CDKSCREEN *	/* screen */,
		char *		/* title */,
		char **		/* list */,
		int		/* listSize */,
		boolean		/* numbers */);

/*
 * This gets a simple string value from a user.
 */
char *getString (
		CDKSCREEN *	/* screen */,
		char *		/* title */,
		char *		/* label */,
		char *		/* init */);

/*
 * This allows a user to view a file.
 */
int viewFile (
		CDKSCREEN *	/* screen */,
		char *		/* title */,
		char *		/* filename */,
		char **		/* buttons */,
		int		/* buttonCount */);

/*
 * This allows a person to select a file.
 */
char *selectFile (
		CDKSCREEN *	/* screen */,
		char *		/* title */);

/*
 * This allows people to view information in an array.
 */
int viewInfo (
		CDKSCREEN *	/* screen */,
		char *		/* title */,
		char **		/* info */,
		int		/* size */,
		char **		/* buttons */,
		int		/* buttonCount */,
		boolean		/* interpret */);

/*
 * This is needed for calling the quick sort routines. (I may kill
 * this and use the libc qsort)
 */
void quickSort (
		char *		/* list */ [],
		int		/* left */,
		int		/* right */);
void swapIndex (
		char *		/* list */ [],
		int		/* i */,
		int		/* j */);

/*
 * This reads a file, loads the contents into info and
 * returns the number of lines read.
 */
int readFile (
		char *		/* filename */,
		char **		/* info */,
		int		/* maxlines */);

/*
 * This strips which space from the front/back of the given
 * string. The stripType is one of: vFRONT, vBACK, vBOTH.
 */
void stripWhiteSpace (
		EStripType	/* stripType */,
		char *		/* string */);

/*
 * These functions are used to manage a string which is split into parts, e.g.,
 * a file which is read into memory.
 */
char **CDKsplitString(
   		char *		/* string */,
		int		/* separator */);

unsigned CDKcountStrings(
   		char **		/* list */);

void CDKfreeStrings(
   		char **		/* list */);

/*
 * This returns the length of an integer.
 */
int intlen (
		int		/* value */);

/*
 * This opens the given directory and reads in the contents. It stores
 * the results in 'list' and returns the number of elements found.
 */
int getDirectoryContents (
		char *		/* directory */,
		char **		/* list */,
		int		/* maxListSize */);

/*
 * This looks for the given pattern in the given list.
 */
int searchList (
		char **		/* list */,
		int		/* listSize */,
		char *		/* pattern */);

/*
 * This returns the basename of a file.
 */
char *baseName (
		char *		/* filename */);

/*
 * This returns the directory name of a file.
 */
char *dirName (
		char *		/* filename */);

/*
 * This frees the memory used by the given string.
 */
void freeChar (
		char *		/* string */);

/*
 * This frees the memory used by the given string.
 */
void freeChtype (
		chtype *	/* string */);

/*
 * This frees the memory used by the given list of strings.
 */
void freeCharList (
		char **		/* list */,
		unsigned	/* size */);

/*
 * This sets the elements of the given string to 'character'
 */
void cleanChar (
		char *		/* string */,
		int		/* length */,
		char		/* character */);

/*
 * This sets the elements of the given string to 'character'
 */
void cleanChtype (
		chtype *	/* string */,
		int		/* length */,
		chtype		/* character */);

/*
 * This takes a chtype pointer and returns a char pointer.
 */
char *chtype2Char (
		chtype *	/* string */);

/*
 * This takes a char pointer and returns a chtype pointer.
 */
chtype *char2Chtype (
		char *		/* string */,
		int *		/* length */,
		int *		/* align */);

/*
 * This takes a character pointer and returns the equivalent
 * display type.
 */
EDisplayType char2DisplayType (
		char *		/* string */);

/*
 * This copies the given string.
 */
chtype *copyChtype (
		chtype *	/* string */);

/*
 * This copies the given string.
 */
char *copyChar (
		char *		/* string */);

/*
 * This returns the length of the given string.
 */
int chlen (
		chtype *	/* string */);

/*
 * This takes a file mode and returns the first character of the file
 * permissions string.
 */
int mode2Filetype (
		mode_t		/* fileMode */);

/*
 * This takes a file mode and stores the character representation
 * of the mode in 'string'. This also returns the octal value
 * of the file mode.
 */
int mode2Char (
		char *		/* string */,
		mode_t		/* fileMode */);

/*
 * This looks for a link. (used by the </L> pattern)
 */
int checkForLink (
		char *		/* line */,
		char *		/* filename */);

/*
 * This function help set the height/width values of a widget.
 */
int setWidgetDimension (
		int		/* parentDim */,
		int		/* proposedDim */,
		int		/* adjustment */);

/*
 * This safely erases a given window.
 */
void eraseCursesWindow (
		WINDOW *	/* window */);

/*
 * This safely deletes a given window.
 */
void deleteCursesWindow (
		WINDOW *	/* window */);

/*
 * This moves a given window
 */
void moveCursesWindow (
		WINDOW *	/* window */,
		int		/* xdiff */,
		int		/* ydiff */);

/*
 * Return an integer like 'floor()', which returns a double.
 */
int floorCDK(double);

/*
 * Return an integer like 'ceil()', which returns a double.
 */
int ceilCDK(double);

#endif /* CDKUTIL_H */
