/* TUI display registers in window.

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
#include "tui.h"
#include "tuiData.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "frame.h"
#include "regcache.h"
#include "inferior.h"
#include "target.h"
#include "tuiLayout.h"
#include "tuiWin.h"
#include "tuiDataWin.h"
#include "tuiGeneralWin.h"
#include "tui-file.h"

/*****************************************
** LOCAL DEFINITIONS                    **
******************************************/
#define DOUBLE_FLOAT_LABEL_WIDTH    6
#define DOUBLE_FLOAT_LABEL_FMT      "%6.6s: "
#define DOUBLE_FLOAT_VALUE_WIDTH    30	/*min of 16 but may be in sci notation */

#define SINGLE_FLOAT_LABEL_WIDTH    6
#define SINGLE_FLOAT_LABEL_FMT      "%6.6s: "
#define SINGLE_FLOAT_VALUE_WIDTH    25	/* min of 8 but may be in sci notation */

#define SINGLE_LABEL_WIDTH    16
#define SINGLE_LABEL_FMT      "%10.10s: "
#define SINGLE_VALUE_WIDTH    20 /* minimum of 8 but may be in sci notation */

/* In the code HP gave Cygnus, this was actually a function call to a
   PA-specific function, which was supposed to determine whether the
   target was a 64-bit or 32-bit processor.  However, the 64-bit
   support wasn't complete, so we didn't merge that in, so we leave
   this here as a stub.  */
#define IS_64BIT 0

/*****************************************
** STATIC DATA                          **
******************************************/


/*****************************************
** STATIC LOCAL FUNCTIONS FORWARD DECLS    **
******************************************/
static TuiStatus _tuiSetRegsContent
  (int, int, struct frame_info *, TuiRegisterDisplayType, int);
static const char *_tuiRegisterName (int);
static TuiStatus _tuiGetRegisterRawValue (int, char *, struct frame_info *);
static void _tuiSetRegisterElement
  (int, struct frame_info *, TuiDataElementPtr, int);
static void _tuiDisplayRegister (int, TuiGenWinInfoPtr, enum precision_type);
static void _tuiRegisterFormat
  (char *, int, int, TuiDataElementPtr, enum precision_type);
static TuiStatus _tuiSetGeneralRegsContent (int);
static TuiStatus _tuiSetSpecialRegsContent (int);
static TuiStatus _tuiSetGeneralAndSpecialRegsContent (int);
static TuiStatus _tuiSetFloatRegsContent (TuiRegisterDisplayType, int);
static int _tuiRegValueHasChanged
  (TuiDataElementPtr, struct frame_info *, char *);
static void _tuiShowFloat_command (char *, int);
static void _tuiShowGeneral_command (char *, int);
static void _tuiShowSpecial_command (char *, int);
static void _tui_vShowRegisters_commandSupport (TuiRegisterDisplayType);
static void _tuiToggleFloatRegs_command (char *, int);
static void _tuiScrollRegsForward_command (char *, int);
static void _tuiScrollRegsBackward_command (char *, int);



/*****************************************
** PUBLIC FUNCTIONS                     **
******************************************/

/*
   ** tuiLastRegsLineNo()
   **        Answer the number of the last line in the regs display.
   **        If there are no registers (-1) is returned.
 */
int
tuiLastRegsLineNo (void)
{
  register int numLines = (-1);

  if (dataWin->detail.dataDisplayInfo.regsContentCount > 0)
    {
      numLines = (dataWin->detail.dataDisplayInfo.regsContentCount /
		  dataWin->detail.dataDisplayInfo.regsColumnCount);
      if (dataWin->detail.dataDisplayInfo.regsContentCount %
	  dataWin->detail.dataDisplayInfo.regsColumnCount)
	numLines++;
    }
  return numLines;
}				/* tuiLastRegsLineNo */


/*
   ** tuiLineFromRegElementNo()
   **        Answer the line number that the register element at elementNo is
   **        on.  If elementNo is greater than the number of register elements
   **        there are, -1 is returned.
 */
int
tuiLineFromRegElementNo (int elementNo)
{
  if (elementNo < dataWin->detail.dataDisplayInfo.regsContentCount)
    {
      int i, line = (-1);

      i = 1;
      while (line == (-1))
	{
	  if (elementNo <
	      (dataWin->detail.dataDisplayInfo.regsColumnCount * i))
	    line = i - 1;
	  else
	    i++;
	}

      return line;
    }
  else
    return (-1);
}				/* tuiLineFromRegElementNo */


/*
   ** tuiFirstRegElementNoInLine()
   **        Answer the index of the first element in lineNo.  If lineNo is
   **        past the register area (-1) is returned.
 */
int
tuiFirstRegElementNoInLine (int lineNo)
{
  if ((lineNo * dataWin->detail.dataDisplayInfo.regsColumnCount)
      <= dataWin->detail.dataDisplayInfo.regsContentCount)
    return ((lineNo + 1) *
	    dataWin->detail.dataDisplayInfo.regsColumnCount) -
      dataWin->detail.dataDisplayInfo.regsColumnCount;
  else
    return (-1);
}				/* tuiFirstRegElementNoInLine */


/*
   ** tuiLastRegElementNoInLine()
   **        Answer the index of the last element in lineNo.  If lineNo is past
   **        the register area (-1) is returned.
 */
int
tuiLastRegElementNoInLine (int lineNo)
{
  if ((lineNo * dataWin->detail.dataDisplayInfo.regsColumnCount) <=
      dataWin->detail.dataDisplayInfo.regsContentCount)
    return ((lineNo + 1) *
	    dataWin->detail.dataDisplayInfo.regsColumnCount) - 1;
  else
    return (-1);
}				/* tuiLastRegElementNoInLine */


/*
   ** tuiCalculateRegsColumnCount
   **        Calculate the number of columns that should be used to display
   **        the registers.
 */
int
tuiCalculateRegsColumnCount (TuiRegisterDisplayType dpyType)
{
  int colCount, colWidth;

  if (IS_64BIT || dpyType == TUI_DFLOAT_REGS)
    colWidth = DOUBLE_FLOAT_VALUE_WIDTH + DOUBLE_FLOAT_LABEL_WIDTH;
  else
    {
      if (dpyType == TUI_SFLOAT_REGS)
	colWidth = SINGLE_FLOAT_VALUE_WIDTH + SINGLE_FLOAT_LABEL_WIDTH;
      else
	colWidth = SINGLE_VALUE_WIDTH + SINGLE_LABEL_WIDTH;
    }
  colCount = (dataWin->generic.width - 2) / colWidth;

  return colCount;
}				/* tuiCalulateRegsColumnCount */


/*
   ** tuiShowRegisters().
   **        Show the registers int the data window as indicated by dpyType.
   **        If there is any other registers being displayed, then they are
   **        cleared.  What registers are displayed is dependent upon dpyType.
 */
void
tuiShowRegisters (TuiRegisterDisplayType dpyType)
{
  TuiStatus ret = TUI_FAILURE;
  int refreshValuesOnly = FALSE;

  /* Say that registers should be displayed, even if there is a problem */
  dataWin->detail.dataDisplayInfo.displayRegs = TRUE;

  if (target_has_registers)
    {
      refreshValuesOnly =
	(dpyType == dataWin->detail.dataDisplayInfo.regsDisplayType);
      switch (dpyType)
	{
	case TUI_GENERAL_REGS:
	  ret = _tuiSetGeneralRegsContent (refreshValuesOnly);
	  break;
	case TUI_SFLOAT_REGS:
	case TUI_DFLOAT_REGS:
	  ret = _tuiSetFloatRegsContent (dpyType, refreshValuesOnly);
	  break;

/* could ifdef out */

	case TUI_SPECIAL_REGS:
	  ret = _tuiSetSpecialRegsContent (refreshValuesOnly);
	  break;
	case TUI_GENERAL_AND_SPECIAL_REGS:
	  ret = _tuiSetGeneralAndSpecialRegsContent (refreshValuesOnly);
	  break;

/* end of potential if def */

	default:
	  break;
	}
    }
  if (ret == TUI_FAILURE)
    {
      dataWin->detail.dataDisplayInfo.regsDisplayType = TUI_UNDEFINED_REGS;
      tuiEraseDataContent (NO_REGS_STRING);
    }
  else
    {
      int i;

      /* Clear all notation of changed values */
      for (i = 0; (i < dataWin->detail.dataDisplayInfo.regsContentCount); i++)
	{
	  TuiGenWinInfoPtr dataItemWin;

	  dataItemWin = &dataWin->detail.dataDisplayInfo.
	    regsContent[i]->whichElement.dataWindow;
	  (&((TuiWinElementPtr)
	     dataItemWin->content[0])->whichElement.data)->highlight = FALSE;
	}
      dataWin->detail.dataDisplayInfo.regsDisplayType = dpyType;
      tuiDisplayAllData ();
    }
  (tuiLayoutDef ())->regsDisplayType = dpyType;

  return;
}				/* tuiShowRegisters */


/*
   ** tuiDisplayRegistersFrom().
   **        Function to display the registers in the content from
   **        'startElementNo' until the end of the register content or the
   **        end of the display height.  No checking for displaying past
   **        the end of the registers is done here.
 */
void
tuiDisplayRegistersFrom (int startElementNo)
{
  if (dataWin->detail.dataDisplayInfo.regsContent != (TuiWinContent) NULL &&
      dataWin->detail.dataDisplayInfo.regsContentCount > 0)
    {
      register int i = startElementNo;
      int j, valueCharsWide, itemWinWidth, curY, labelWidth;
      enum precision_type precision;

      precision = (dataWin->detail.dataDisplayInfo.regsDisplayType
		   == TUI_DFLOAT_REGS) ?
	double_precision : unspecified_precision;
      if (IS_64BIT ||
	  dataWin->detail.dataDisplayInfo.regsDisplayType == TUI_DFLOAT_REGS)
	{
	  valueCharsWide = DOUBLE_FLOAT_VALUE_WIDTH;
	  labelWidth = DOUBLE_FLOAT_LABEL_WIDTH;
	}
      else
	{
	  if (dataWin->detail.dataDisplayInfo.regsDisplayType ==
	      TUI_SFLOAT_REGS)
	    {
	      valueCharsWide = SINGLE_FLOAT_VALUE_WIDTH;
	      labelWidth = SINGLE_FLOAT_LABEL_WIDTH;
	    }
	  else
	    {
	      valueCharsWide = SINGLE_VALUE_WIDTH;
	      labelWidth = SINGLE_LABEL_WIDTH;
	    }
	}
      itemWinWidth = valueCharsWide + labelWidth;
      /*
         ** Now create each data "sub" window, and write the display into it.
       */
      curY = 1;
      while (i < dataWin->detail.dataDisplayInfo.regsContentCount &&
	     curY <= dataWin->generic.viewportHeight)
	{
	  for (j = 0;
	       (j < dataWin->detail.dataDisplayInfo.regsColumnCount &&
		i < dataWin->detail.dataDisplayInfo.regsContentCount); j++)
	    {
	      TuiGenWinInfoPtr dataItemWin;
	      TuiDataElementPtr dataElementPtr;

	      /* create the window if necessary */
	      dataItemWin = &dataWin->detail.dataDisplayInfo.
		regsContent[i]->whichElement.dataWindow;
	      dataElementPtr = &((TuiWinElementPtr)
				 dataItemWin->content[0])->whichElement.data;
	      if (dataItemWin->handle == (WINDOW *) NULL)
		{
		  dataItemWin->height = 1;
		  dataItemWin->width = (precision == double_precision) ?
		    itemWinWidth + 2 : itemWinWidth + 1;
		  dataItemWin->origin.x = (itemWinWidth * j) + 1;
		  dataItemWin->origin.y = curY;
		  makeWindow (dataItemWin, DONT_BOX_WINDOW);
                  scrollok (dataItemWin->handle, FALSE);
		}
              touchwin (dataItemWin->handle);

	      /*
	         ** Get the printable representation of the register
	         ** and display it
	       */
	      _tuiDisplayRegister (
			    dataElementPtr->itemNo, dataItemWin, precision);
	      i++;		/* next register */
	    }
	  curY++;		/* next row; */
	}
    }

  return;
}				/* tuiDisplayRegistersFrom */


/*
   ** tuiDisplayRegElementAtLine().
   **        Function to display the registers in the content from
   **        'startElementNo' on 'startLineNo' until the end of the
   **        register content or the end of the display height.
   **        This function checks that we won't display off the end
   **        of the register display.
 */
void
tuiDisplayRegElementAtLine (int startElementNo, int startLineNo)
{
  if (dataWin->detail.dataDisplayInfo.regsContent != (TuiWinContent) NULL &&
      dataWin->detail.dataDisplayInfo.regsContentCount > 0)
    {
      register int elementNo = startElementNo;

      if (startElementNo != 0 && startLineNo != 0)
	{
	  register int lastLineNo, firstLineOnLastPage;

	  lastLineNo = tuiLastRegsLineNo ();
	  firstLineOnLastPage = lastLineNo - (dataWin->generic.height - 2);
	  if (firstLineOnLastPage < 0)
	    firstLineOnLastPage = 0;
	  /*
	     ** If there is no other data displayed except registers,
	     ** and the elementNo causes us to scroll past the end of the
	     ** registers, adjust what element to really start the display at.
	   */
	  if (dataWin->detail.dataDisplayInfo.dataContentCount <= 0 &&
	      startLineNo > firstLineOnLastPage)
	    elementNo = tuiFirstRegElementNoInLine (firstLineOnLastPage);
	}
      tuiDisplayRegistersFrom (elementNo);
    }

  return;
}				/* tuiDisplayRegElementAtLine */



/*
   ** tuiDisplayRegistersFromLine().
   **        Function to display the registers starting at line lineNo in
   **        the data window.  Answers the line number that the display
   **        actually started from.  If nothing is displayed (-1) is returned.
 */
int
tuiDisplayRegistersFromLine (int lineNo, int forceDisplay)
{
  if (dataWin->detail.dataDisplayInfo.regsContentCount > 0)
    {
      int line, elementNo;

      if (lineNo < 0)
	line = 0;
      else if (forceDisplay)
	{			/*
				   ** If we must display regs (forceDisplay is true), then make
				   ** sure that we don't display off the end of the registers.
				 */
	  if (lineNo >= tuiLastRegsLineNo ())
	    {
	      if ((line = tuiLineFromRegElementNo (
		 dataWin->detail.dataDisplayInfo.regsContentCount - 1)) < 0)
		line = 0;
	    }
	  else
	    line = lineNo;
	}
      else
	line = lineNo;

      elementNo = tuiFirstRegElementNoInLine (line);
      if (elementNo < dataWin->detail.dataDisplayInfo.regsContentCount)
	tuiDisplayRegElementAtLine (elementNo, line);
      else
	line = (-1);

      return line;
    }

  return (-1);			/* nothing was displayed */
}				/* tuiDisplayRegistersFromLine */


/*
   ** tuiCheckRegisterValues()
   **        This function check all displayed registers for changes in
   **        values, given a particular frame.  If the values have changed,
   **        they are updated with the new value and highlighted.
 */
void
tuiCheckRegisterValues (struct frame_info *frame)
{
  if (m_winPtrNotNull (dataWin) && dataWin->generic.isVisible)
    {
      if (dataWin->detail.dataDisplayInfo.regsContentCount <= 0 &&
	  dataWin->detail.dataDisplayInfo.displayRegs)
	tuiShowRegisters ((tuiLayoutDef ())->regsDisplayType);
      else
	{
	  int i, j;
	  char rawBuf[MAX_REGISTER_RAW_SIZE];

	  for (i = 0;
	       (i < dataWin->detail.dataDisplayInfo.regsContentCount); i++)
	    {
	      TuiDataElementPtr dataElementPtr;
	      TuiGenWinInfoPtr dataItemWinPtr;
	      int wasHilighted;

	      dataItemWinPtr = &dataWin->detail.dataDisplayInfo.
		regsContent[i]->whichElement.dataWindow;
	      dataElementPtr = &((TuiWinElementPtr)
			     dataItemWinPtr->content[0])->whichElement.data;
	      wasHilighted = dataElementPtr->highlight;
	      dataElementPtr->highlight =
		_tuiRegValueHasChanged (dataElementPtr, frame, &rawBuf[0]);
	      if (dataElementPtr->highlight)
		{
                  int size;

                  size = REGISTER_RAW_SIZE (dataElementPtr->itemNo);
		  for (j = 0; j < size; j++)
		    ((char *) dataElementPtr->value)[j] = rawBuf[j];
		  _tuiDisplayRegister (
					dataElementPtr->itemNo,
					dataItemWinPtr,
			((dataWin->detail.dataDisplayInfo.regsDisplayType ==
			  TUI_DFLOAT_REGS) ?
			 double_precision : unspecified_precision));
		}
	      else if (wasHilighted)
		{
		  dataElementPtr->highlight = FALSE;
		  _tuiDisplayRegister (
					dataElementPtr->itemNo,
					dataItemWinPtr,
			((dataWin->detail.dataDisplayInfo.regsDisplayType ==
			  TUI_DFLOAT_REGS) ?
			 double_precision : unspecified_precision));
		}
	    }
	}
    }
  return;
}				/* tuiCheckRegisterValues */


/*
   ** tuiToggleFloatRegs().
 */
void
tuiToggleFloatRegs (void)
{
  TuiLayoutDefPtr layoutDef = tuiLayoutDef ();

  if (layoutDef->floatRegsDisplayType == TUI_SFLOAT_REGS)
    layoutDef->floatRegsDisplayType = TUI_DFLOAT_REGS;
  else
    layoutDef->floatRegsDisplayType = TUI_SFLOAT_REGS;

  if (m_winPtrNotNull (dataWin) && dataWin->generic.isVisible &&
      (dataWin->detail.dataDisplayInfo.regsDisplayType == TUI_SFLOAT_REGS ||
       dataWin->detail.dataDisplayInfo.regsDisplayType == TUI_DFLOAT_REGS))
    tuiShowRegisters (layoutDef->floatRegsDisplayType);

  return;
}				/* tuiToggleFloatRegs */


void
_initialize_tuiRegs (void)
{
  if (xdb_commands)
    {
      add_com ("fr", class_tui, _tuiShowFloat_command,
	       "Display only floating point registers\n");
      add_com ("gr", class_tui, _tuiShowGeneral_command,
	       "Display only general registers\n");
      add_com ("sr", class_tui, _tuiShowSpecial_command,
	       "Display only special registers\n");
      add_com ("+r", class_tui, _tuiScrollRegsForward_command,
	       "Scroll the registers window forward\n");
      add_com ("-r", class_tui, _tuiScrollRegsBackward_command,
	       "Scroll the register window backward\n");
      add_com ("tf", class_tui, _tuiToggleFloatRegs_command,
	       "Toggle between single and double precision floating point registers.\n");
      add_cmd (TUI_FLOAT_REGS_NAME_LOWER,
	       class_tui,
	       _tuiToggleFloatRegs_command,
	       "Toggle between single and double precision floating point \
registers.\n",
	       &togglelist);
    }
}


/*****************************************
** STATIC LOCAL FUNCTIONS                 **
******************************************/


/*
   ** _tuiRegisterName().
   **        Return the register name.
 */
static const char *
_tuiRegisterName (int regNum)
{
  return REGISTER_NAME (regNum);
}
extern int pagination_enabled;

static void
tui_restore_gdbout (void *ui)
{
  ui_file_delete (gdb_stdout);
  gdb_stdout = (struct ui_file*) ui;
  pagination_enabled = 1;
}

/*
   ** _tuiRegisterFormat
   **        Function to format the register name and value into a buffer,
   **        suitable for printing or display
 */
static void
_tuiRegisterFormat (char *buf, int bufLen, int regNum,
                    TuiDataElementPtr dataElement,
                    enum precision_type precision)
{
  struct ui_file *stream;
  struct ui_file *old_stdout;
  const char *name;
  struct cleanup *cleanups;
  char *p;
  int pos;

  name = REGISTER_NAME (regNum);
  if (name == 0)
    {
      strcpy (buf, "");
      return;
    }
  
  pagination_enabled = 0;
  old_stdout = gdb_stdout;
  stream = tui_sfileopen (bufLen);
  gdb_stdout = stream;
  cleanups = make_cleanup (tui_restore_gdbout, (void*) old_stdout);
  gdbarch_print_registers_info (current_gdbarch, stream, selected_frame,
                                regNum, 1);

  /* Save formatted output in the buffer.  */
  p = tui_file_get_strbuf (stream);
  pos = 0;
  while (*p && *p == *name++ && bufLen)
    {
      *buf++ = *p++;
      bufLen--;
      pos++;
    }
  while (*p == ' ')
    p++;
  while (pos < 8 && bufLen)
    {
      *buf++ = ' ';
      bufLen--;
      pos++;
    }
  strncpy (buf, p, bufLen);

  /* Remove the possible \n.  */
  p = strchr (buf, '\n');
  if (p)
    *p = 0;

  do_cleanups (cleanups);
}


#define NUM_GENERAL_REGS    32
/*
   ** _tuiSetGeneralRegsContent().
   **      Set the content of the data window to consist of the general registers.
 */
static TuiStatus
_tuiSetGeneralRegsContent (int refreshValuesOnly)
{
  return (_tuiSetRegsContent (0,
			      NUM_GENERAL_REGS - 1,
			      selected_frame,
			      TUI_GENERAL_REGS,
			      refreshValuesOnly));

}				/* _tuiSetGeneralRegsContent */


#ifndef PCOQ_HEAD_REGNUM
#define START_SPECIAL_REGS  0
#else
#define START_SPECIAL_REGS    PCOQ_HEAD_REGNUM
#endif

/*
   ** _tuiSetSpecialRegsContent().
   **      Set the content of the data window to consist of the special registers.
 */
static TuiStatus
_tuiSetSpecialRegsContent (int refreshValuesOnly)
{
  TuiStatus ret = TUI_FAILURE;
  int endRegNum;

  endRegNum = FP0_REGNUM - 1;
  ret = _tuiSetRegsContent (START_SPECIAL_REGS,
			    endRegNum,
			    selected_frame,
			    TUI_SPECIAL_REGS,
			    refreshValuesOnly);

  return ret;
}				/* _tuiSetSpecialRegsContent */


/*
   ** _tuiSetGeneralAndSpecialRegsContent().
   **      Set the content of the data window to consist of the special registers.
 */
static TuiStatus
_tuiSetGeneralAndSpecialRegsContent (int refreshValuesOnly)
{
  TuiStatus ret = TUI_FAILURE;
  int endRegNum = (-1);

  endRegNum = FP0_REGNUM - 1;
  ret = _tuiSetRegsContent (
	 0, endRegNum, selected_frame, TUI_SPECIAL_REGS, refreshValuesOnly);

  return ret;
}				/* _tuiSetGeneralAndSpecialRegsContent */

/*
   ** _tuiSetFloatRegsContent().
   **        Set the content of the data window to consist of the float registers.
 */
static TuiStatus
_tuiSetFloatRegsContent (TuiRegisterDisplayType dpyType, int refreshValuesOnly)
{
  TuiStatus ret = TUI_FAILURE;
  int startRegNum;

  startRegNum = FP0_REGNUM;
  ret = _tuiSetRegsContent (startRegNum,
			    NUM_REGS - 1,
			    selected_frame,
			    dpyType,
			    refreshValuesOnly);

  return ret;
}				/* _tuiSetFloatRegsContent */


/*
   ** _tuiRegValueHasChanged().
   **        Answer TRUE if the register's value has changed, FALSE otherwise.
   **        If TRUE, newValue is filled in with the new value.
 */
static int
_tuiRegValueHasChanged (TuiDataElementPtr dataElement,
                        struct frame_info *frame,
                        char *newValue)
{
  int hasChanged = FALSE;

  if (dataElement->itemNo != UNDEFINED_ITEM &&
      _tuiRegisterName (dataElement->itemNo) != (char *) NULL)
    {
      char rawBuf[MAX_REGISTER_RAW_SIZE];
      int i;

      if (_tuiGetRegisterRawValue (
			 dataElement->itemNo, rawBuf, frame) == TUI_SUCCESS)
	{
          int size = REGISTER_RAW_SIZE (dataElement->itemNo);
          
	  for (i = 0; (i < size && !hasChanged); i++)
	    hasChanged = (((char *) dataElement->value)[i] != rawBuf[i]);
	  if (hasChanged && newValue != (char *) NULL)
	    {
	      for (i = 0; i < size; i++)
		newValue[i] = rawBuf[i];
	    }
	}
    }
  return hasChanged;
}				/* _tuiRegValueHasChanged */



/*
   ** _tuiGetRegisterRawValue().
   **        Get the register raw value.  The raw value is returned in regValue.
 */
static TuiStatus
_tuiGetRegisterRawValue (int regNum, char *regValue, struct frame_info *frame)
{
  TuiStatus ret = TUI_FAILURE;

  if (target_has_registers)
    {
      int opt;
      
      get_saved_register (regValue, &opt, (CORE_ADDR*) NULL, frame,
			  regNum, (enum lval_type*) NULL);
      if (register_cached (regNum) >= 0)
	ret = TUI_SUCCESS;
    }
  return ret;
}				/* _tuiGetRegisterRawValue */



/*
   ** _tuiSetRegisterElement().
   **       Function to initialize a data element with the input and
   **       the register value.
 */
static void
_tuiSetRegisterElement (int regNum, struct frame_info *frame,
                        TuiDataElementPtr dataElement,
                        int refreshValueOnly)
{
  if (dataElement != (TuiDataElementPtr) NULL)
    {
      if (!refreshValueOnly)
	{
	  dataElement->itemNo = regNum;
	  dataElement->name = _tuiRegisterName (regNum);
	  dataElement->highlight = FALSE;
	}
      if (dataElement->value == (Opaque) NULL)
	dataElement->value = (Opaque) xmalloc (MAX_REGISTER_RAW_SIZE);
      if (dataElement->value != (Opaque) NULL)
	_tuiGetRegisterRawValue (regNum, dataElement->value, frame);
    }

  return;
}				/* _tuiSetRegisterElement */


/*
   ** _tuiSetRegsContent().
   **        Set the content of the data window to consist of the registers
   **        numbered from startRegNum to endRegNum.  Note that if
   **        refreshValuesOnly is TRUE, startRegNum and endRegNum are ignored.
 */
static TuiStatus
_tuiSetRegsContent (int startRegNum, int endRegNum,
                    struct frame_info *frame,
                    TuiRegisterDisplayType dpyType,
                    int refreshValuesOnly)
{
  TuiStatus ret = TUI_FAILURE;
  int numRegs = endRegNum - startRegNum + 1;
  int allocatedHere = FALSE;

  if (dataWin->detail.dataDisplayInfo.regsContentCount > 0 &&
      !refreshValuesOnly)
    {
      freeDataContent (dataWin->detail.dataDisplayInfo.regsContent,
		       dataWin->detail.dataDisplayInfo.regsContentCount);
      dataWin->detail.dataDisplayInfo.regsContentCount = 0;
    }
  if (dataWin->detail.dataDisplayInfo.regsContentCount <= 0)
    {
      dataWin->detail.dataDisplayInfo.regsContent =
	allocContent (numRegs, DATA_WIN);
      allocatedHere = TRUE;
    }

  if (dataWin->detail.dataDisplayInfo.regsContent != (TuiWinContent) NULL)
    {
      int i;

      if (!refreshValuesOnly || allocatedHere)
	{
	  dataWin->generic.content = (OpaquePtr) NULL;
	  dataWin->generic.contentSize = 0;
	  addContentElements (&dataWin->generic, numRegs);
	  dataWin->detail.dataDisplayInfo.regsContent =
	    (TuiWinContent) dataWin->generic.content;
	  dataWin->detail.dataDisplayInfo.regsContentCount = numRegs;
	}
      /*
         ** Now set the register names and values
       */
      for (i = startRegNum; (i <= endRegNum); i++)
	{
	  TuiGenWinInfoPtr dataItemWin;

	  dataItemWin = &dataWin->detail.dataDisplayInfo.
	    regsContent[i - startRegNum]->whichElement.dataWindow;
	  _tuiSetRegisterElement (
				   i,
				   frame,
	   &((TuiWinElementPtr) dataItemWin->content[0])->whichElement.data,
				   !allocatedHere && refreshValuesOnly);
	}
      dataWin->detail.dataDisplayInfo.regsColumnCount =
	tuiCalculateRegsColumnCount (dpyType);
#ifdef LATER
      if (dataWin->detail.dataDisplayInfo.dataContentCount > 0)
	{
	  /* delete all the windows? */
	  /* realloc content equal to dataContentCount + regsContentCount */
	  /* append dataWin->detail.dataDisplayInfo.dataContent to content */
	}
#endif
      dataWin->generic.contentSize =
	dataWin->detail.dataDisplayInfo.regsContentCount +
	dataWin->detail.dataDisplayInfo.dataContentCount;
      ret = TUI_SUCCESS;
    }

  return ret;
}				/* _tuiSetRegsContent */


/*
   ** _tuiDisplayRegister().
   **        Function to display a register in a window.  If hilite is TRUE,
   **        than the value will be displayed in reverse video
 */
static void
_tuiDisplayRegister (int regNum,
                     TuiGenWinInfoPtr winInfo,		/* the data item window */
                     enum precision_type precision)
{
  if (winInfo->handle != (WINDOW *) NULL)
    {
      int i;
      char buf[40];
      int valueCharsWide, labelWidth;
      TuiDataElementPtr dataElementPtr = &((TuiWinContent)
				    winInfo->content)[0]->whichElement.data;

      if (IS_64BIT ||
	  dataWin->detail.dataDisplayInfo.regsDisplayType == TUI_DFLOAT_REGS)
	{
	  valueCharsWide = DOUBLE_FLOAT_VALUE_WIDTH;
	  labelWidth = DOUBLE_FLOAT_LABEL_WIDTH;
	}
      else
	{
	  if (dataWin->detail.dataDisplayInfo.regsDisplayType ==
	      TUI_SFLOAT_REGS)
	    {
	      valueCharsWide = SINGLE_FLOAT_VALUE_WIDTH;
	      labelWidth = SINGLE_FLOAT_LABEL_WIDTH;
	    }
	  else
	    {
	      valueCharsWide = SINGLE_VALUE_WIDTH;
	      labelWidth = SINGLE_LABEL_WIDTH;
	    }
	}

      buf[0] = (char) 0;
      _tuiRegisterFormat (buf,
			  valueCharsWide + labelWidth,
			  regNum,
			  dataElementPtr,
			  precision);

      if (dataElementPtr->highlight)
	wstandout (winInfo->handle);

      wmove (winInfo->handle, 0, 0);
      for (i = 1; i < winInfo->width; i++)
        waddch (winInfo->handle, ' ');
      wmove (winInfo->handle, 0, 0);
      waddstr (winInfo->handle, buf);

      if (dataElementPtr->highlight)
	wstandend (winInfo->handle);
      tuiRefreshWin (winInfo);
    }
  return;
}				/* _tuiDisplayRegister */


static void
_tui_vShowRegisters_commandSupport (TuiRegisterDisplayType dpyType)
{

  if (m_winPtrNotNull (dataWin) && dataWin->generic.isVisible)
    {				/* Data window already displayed, show the registers */
      if (dataWin->detail.dataDisplayInfo.regsDisplayType != dpyType)
	tuiShowRegisters (dpyType);
    }
  else
    (tuiLayoutDef ())->regsDisplayType = dpyType;

  return;
}				/* _tui_vShowRegisters_commandSupport */


static void
_tuiShowFloat_command (char *arg, int fromTTY)
{
  if (m_winPtrIsNull (dataWin) || !dataWin->generic.isVisible ||
      (dataWin->detail.dataDisplayInfo.regsDisplayType != TUI_SFLOAT_REGS &&
       dataWin->detail.dataDisplayInfo.regsDisplayType != TUI_DFLOAT_REGS))
    _tui_vShowRegisters_commandSupport ((tuiLayoutDef ())->floatRegsDisplayType);

  return;
}				/* _tuiShowFloat_command */


static void
_tuiShowGeneral_command (char *arg, int fromTTY)
{
  _tui_vShowRegisters_commandSupport (TUI_GENERAL_REGS);
}


static void
_tuiShowSpecial_command (char *arg, int fromTTY)
{
  _tui_vShowRegisters_commandSupport (TUI_SPECIAL_REGS);
}


static void
_tuiToggleFloatRegs_command (char *arg, int fromTTY)
{
  if (m_winPtrNotNull (dataWin) && dataWin->generic.isVisible)
    tuiToggleFloatRegs ();
  else
    {
      TuiLayoutDefPtr layoutDef = tuiLayoutDef ();

      if (layoutDef->floatRegsDisplayType == TUI_SFLOAT_REGS)
	layoutDef->floatRegsDisplayType = TUI_DFLOAT_REGS;
      else
	layoutDef->floatRegsDisplayType = TUI_SFLOAT_REGS;
    }


  return;
}				/* _tuiToggleFloatRegs_command */


static void
_tuiScrollRegsForward_command (char *arg, int fromTTY)
{
  tui_scroll (FORWARD_SCROLL, dataWin, 1);
}


static void
_tuiScrollRegsBackward_command (char *arg, int fromTTY)
{
  tui_scroll (BACKWARD_SCROLL, dataWin, 1);
}
