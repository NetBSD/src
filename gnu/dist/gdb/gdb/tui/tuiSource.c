/* TUI display source window.

   Copyright 1998, 1999, 2000, 2001, 2002 Free Software Foundation,
   Inc.

   Contributed by Hewlett-Packard Company.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* FIXME: cagney/2002-02-28: The GDB coding standard indicates that
   "defs.h" should be included first.  Unfortunatly some systems
   (currently Debian GNU/Linux) include the <stdbool.h> via <curses.h>
   and they clash with "bfd.h"'s definiton of true/false.  The correct
   fix is to remove true/false from "bfd.h", however, until that
   happens, hack around it by including "config.h" and <curses.h>
   first.  */

#include "config.h"
#ifdef HAVE_NCURSES_H       
#include <ncurses.h>
#else
#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
#endif

#include "defs.h"
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "breakpoint.h"
#include "source.h"
#include "symtab.h"

#include "tui.h"
#include "tuiData.h"
#include "tuiStack.h"
#include "tuiSourceWin.h"
#include "tuiSource.h"


/* Function to display source in the source window.  */
TuiStatus
tuiSetSourceContent (struct symtab *s, int lineNo, int noerror)
{
  TuiStatus ret = TUI_FAILURE;

  if (s != (struct symtab *) NULL && s->filename != (char *) NULL)
    {
      register FILE *stream;
      register int i, desc, c, lineWidth, nlines;
      register char *srcLine = 0;

      if ((ret = tuiAllocSourceBuffer (srcWin)) == TUI_SUCCESS)
	{
	  lineWidth = srcWin->generic.width - 1;
	  /* Take hilite (window border) into account, when calculating
	     the number of lines  */
	  nlines = (lineNo + (srcWin->generic.height - 2)) - lineNo;
	  desc = open_source_file (s);
	  if (desc < 0)
	    {
	      if (!noerror)
		{
		  char *name = alloca (strlen (s->filename) + 100);
		  sprintf (name, "%s:%d", s->filename, lineNo);
		  print_sys_errmsg (name, errno);
		}
	      ret = TUI_FAILURE;
	    }
	  else
	    {
	      if (s->line_charpos == 0)
		find_source_lines (s, desc);

	      if (lineNo < 1 || lineNo > s->nlines)
		{
		  close (desc);
		  printf_unfiltered (
			  "Line number %d out of range; %s has %d lines.\n",
				      lineNo, s->filename, s->nlines);
		}
	      else if (lseek (desc, s->line_charpos[lineNo - 1], 0) < 0)
		{
		  close (desc);
		  perror_with_name (s->filename);
		}
	      else
		{
		  register int offset, curLineNo, curLine, curLen, threshold;
		  TuiGenWinInfoPtr locator = locatorWinInfoPtr ();
                  TuiSourceInfoPtr src = &srcWin->detail.sourceInfo;

                  if (srcWin->generic.title)
                    xfree (srcWin->generic.title);
                  srcWin->generic.title = xstrdup (s->filename);

                  if (src->filename)
                    xfree (src->filename);
                  src->filename = xstrdup (s->filename);

		  /* Determine the threshold for the length of the line
                     and the offset to start the display.  */
		  offset = src->horizontalOffset;
		  threshold = (lineWidth - 1) + offset;
		  stream = fdopen (desc, FOPEN_RT);
		  clearerr (stream);
		  curLine = 0;
		  curLineNo = src->startLineOrAddr.lineNo = lineNo;
		  if (offset > 0)
		    srcLine = (char *) xmalloc (
					   (threshold + 1) * sizeof (char));
		  while (curLine < nlines)
		    {
		      TuiWinElementPtr element = (TuiWinElementPtr)
		      srcWin->generic.content[curLine];

		      /* get the first character in the line */
		      c = fgetc (stream);

		      if (offset == 0)
			srcLine = ((TuiWinElementPtr)
				   srcWin->generic.content[
					curLine])->whichElement.source.line;
		      /* Init the line with the line number */
		      sprintf (srcLine, "%-6d", curLineNo);
		      curLen = strlen (srcLine);
		      i = curLen -
			((curLen / tuiDefaultTabLen ()) * tuiDefaultTabLen ());
		      while (i < tuiDefaultTabLen ())
			{
			  srcLine[curLen] = ' ';
			  i++;
			  curLen++;
			}
		      srcLine[curLen] = (char) 0;

		      /* Set whether element is the execution point and
		         whether there is a break point on it.  */
		      element->whichElement.source.lineOrAddr.lineNo =
			curLineNo;
		      element->whichElement.source.isExecPoint =
			(strcmp (((TuiWinElementPtr)
			locator->content[0])->whichElement.locator.fileName,
				 s->filename) == 0
			 && curLineNo == ((TuiWinElementPtr)
			 locator->content[0])->whichElement.locator.lineNo);
		      if (c != EOF)
			{
			  i = strlen (srcLine) - 1;
			  do
			    {
			      if ((c != '\n') &&
				  (c != '\r') && (++i < threshold))
				{
				  if (c < 040 && c != '\t')
				    {
				      srcLine[i++] = '^';
				      srcLine[i] = c + 0100;
				    }
				  else if (c == 0177)
				    {
				      srcLine[i++] = '^';
				      srcLine[i] = '?';
				    }
				  else
				    {	/* Store the charcter in the line
					   buffer.  If it is a tab, then
					   translate to the correct number of
					   chars so we don't overwrite our
					   buffer.  */
				      if (c == '\t')
					{
					  int j, maxTabLen = tuiDefaultTabLen ();

					  for (j = i - (
					       (i / maxTabLen) * maxTabLen);
					       ((j < maxTabLen) &&
						i < threshold);
					       i++, j++)
					    srcLine[i] = ' ';
					  i--;
					}
				      else
					srcLine[i] = c;
				    }
				  srcLine[i + 1] = 0;
				}
			      else
				{	/* If we have not reached EOL, then eat
                                           chars until we do  */
				  while (c != EOF && c != '\n' && c != '\r')
				    c = fgetc (stream);
				}
			    }
			  while (c != EOF && c != '\n' && c != '\r' &&
				 i < threshold && (c = fgetc (stream)));
			}
		      /* Now copy the line taking the offset into account */
		      if (strlen (srcLine) > offset)
			strcpy (((TuiWinElementPtr) srcWin->generic.content[
					curLine])->whichElement.source.line,
				&srcLine[offset]);
		      else
			((TuiWinElementPtr)
			 srcWin->generic.content[
			  curLine])->whichElement.source.line[0] = (char) 0;
		      curLine++;
		      curLineNo++;
		    }
		  if (offset > 0)
		    tuiFree (srcLine);
		  fclose (stream);
		  srcWin->generic.contentSize = nlines;
		  ret = TUI_SUCCESS;
		}
	    }
	}
    }
  return ret;
}


/* elz: this function sets the contents of the source window to empty
   except for a line in the middle with a warning message about the
   source not being available. This function is called by
   tuiEraseSourceContents, which in turn is invoked when the source files
   cannot be accessed */

void
tuiSetSourceContentNil (TuiWinInfoPtr winInfo, char *warning_string)
{
  int lineWidth;
  int nLines;
  int curr_line = 0;

  lineWidth = winInfo->generic.width - 1;
  nLines = winInfo->generic.height - 2;

  /* set to empty each line in the window, except for the one
     which contains the message */
  while (curr_line < winInfo->generic.contentSize)
    {
      /* set the information related to each displayed line
         to null: i.e. the line number is 0, there is no bp,
         it is not where the program is stopped */

      TuiWinElementPtr element =
      (TuiWinElementPtr) winInfo->generic.content[curr_line];
      element->whichElement.source.lineOrAddr.lineNo = 0;
      element->whichElement.source.isExecPoint = FALSE;
      element->whichElement.source.hasBreak = FALSE;

      /* set the contents of the line to blank */
      element->whichElement.source.line[0] = (char) 0;

      /* if the current line is in the middle of the screen, then we want to
         display the 'no source available' message in it.
         Note: the 'weird' arithmetic with the line width and height comes from
         the function tuiEraseSourceContent. We need to keep the screen and the
         window's actual contents in synch */

      if (curr_line == (nLines / 2 + 1))
	{
	  int i;
	  int xpos;
	  int warning_length = strlen (warning_string);
	  char *srcLine;

	  srcLine = element->whichElement.source.line;

	  if (warning_length >= ((lineWidth - 1) / 2))
	    xpos = 1;
	  else
	    xpos = (lineWidth - 1) / 2 - warning_length;

	  for (i = 0; i < xpos; i++)
	    srcLine[i] = ' ';

	  sprintf (srcLine + i, "%s", warning_string);

	  for (i = xpos + warning_length; i < lineWidth; i++)
	    srcLine[i] = ' ';

	  srcLine[i] = '\n';

	}			/* end if */

      curr_line++;

    }				/* end while */
}


/* Function to display source in the source window.  This function
   initializes the horizontal scroll to 0.  */
void
tuiShowSource (struct symtab *s, TuiLineOrAddress line, int noerror)
{
  srcWin->detail.sourceInfo.horizontalOffset = 0;
  tuiUpdateSourceWindowAsIs(srcWin, s, line, noerror);
}


/* Answer whether the source is currently displayed in the source window.  */
int
tuiSourceIsDisplayed (char *fname)
{
  return (srcWin->generic.contentInUse &&
	  (strcmp (((TuiWinElementPtr) (locatorWinInfoPtr ())->
		  content[0])->whichElement.locator.fileName, fname) == 0));
}


/* Scroll the source forward or backward vertically.  */
void
tuiVerticalSourceScroll (TuiScrollDirection scrollDirection,
                         int numToScroll)
{
  if (srcWin->generic.content != (OpaquePtr) NULL)
    {
      TuiLineOrAddress l;
      struct symtab *s;
      TuiWinContent content = (TuiWinContent) srcWin->generic.content;

      if (current_source_symtab == (struct symtab *) NULL)
	s = find_pc_symtab (selected_frame->pc);
      else
	s = current_source_symtab;

      if (scrollDirection == FORWARD_SCROLL)
	{
	  l.lineNo = content[0]->whichElement.source.lineOrAddr.lineNo +
	    numToScroll;
	  if (l.lineNo > s->nlines)
	    /*line = s->nlines - winInfo->generic.contentSize + 1; */
	    /*elz: fix for dts 23398 */
	    l.lineNo = content[0]->whichElement.source.lineOrAddr.lineNo;
	}
      else
	{
	  l.lineNo = content[0]->whichElement.source.lineOrAddr.lineNo -
	    numToScroll;
	  if (l.lineNo <= 0)
	    l.lineNo = 1;
	}

      print_source_lines (s, l.lineNo, l.lineNo + 1, 0);
    }
}
