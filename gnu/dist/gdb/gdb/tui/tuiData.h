/* TUI data manipulation routines.
   Copyright 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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

#ifndef TUI_DATA_H
#define TUI_DATA_H

#if defined (HAVE_NCURSES_H)
#include <ncurses.h>
#elif defined (HAVE_CURSES_H)
#include <curses.h>
#endif

/* Generic window information */
     typedef struct _TuiGenWinInfo
       {
	 WINDOW *handle;	/* window handle */
	 TuiWinType type;	/* type of window */
	 int width;		/* window width */
	 int height;		/* window height */
	 TuiPoint origin;	/* origin of window */
	 OpaquePtr content;	/* content of window */
	 int contentSize;	/* Size of content (# of elements) */
	 int contentInUse;	/* Can it be used, or is it already used? */
	 int viewportHeight;	/* viewport height */
	 int lastVisibleLine;	/* index of last visible line */
	 int isVisible;		/* whether the window is visible or not */
         char* title;          /* Window title to display.  */
       }
TuiGenWinInfo, *TuiGenWinInfoPtr;

/* Constant definitions */
#define DEFAULT_TAB_LEN                8
#define NO_SRC_STRING                  "[ No Source Available ]"
#define NO_DISASSEM_STRING             "[ No Assembly Available ]"
#define NO_REGS_STRING                 "[ Register Values Unavailable ]"
#define NO_DATA_STRING                 "[ No Data Values Displayed ]"
#define MAX_CONTENT_COUNT              100
#define SRC_NAME                       "SRC"
#define CMD_NAME                       "CMD"
#define DATA_NAME                      "REGS"
#define DISASSEM_NAME                  "ASM"
#define TUI_NULL_STR                   ""
#define DEFAULT_HISTORY_COUNT          25
#define BOX_WINDOW                     TRUE
#define DONT_BOX_WINDOW                FALSE
#define HILITE                         TRUE
#define NO_HILITE                      FALSE
#define WITH_LOCATOR                   TRUE
#define NO_LOCATOR                     FALSE
#define EMPTY_SOURCE_PROMPT            TRUE
#define NO_EMPTY_SOURCE_PROMPT         FALSE
#define UNDEFINED_ITEM                 -1
#define MIN_WIN_HEIGHT                 3
#define MIN_CMD_WIN_HEIGHT             3

/* Strings to display in the TUI status line.  */
#define PROC_PREFIX                    "In: "
#define LINE_PREFIX                    "Line: "
#define PC_PREFIX                      "PC: "
#define SINGLE_KEY                     "(SingleKey)"

/* Minimum/Maximum length of some fields displayed in the TUI status line.  */
#define MIN_LINE_WIDTH     4 /* Use at least 4 digits for line numbers.  */
#define MIN_PROC_WIDTH    12
#define MAX_TARGET_WIDTH  10
#define MAX_PID_WIDTH     14

#define TUI_FLOAT_REGS_NAME                  "$FREGS"
#define TUI_FLOAT_REGS_NAME_LOWER            "$fregs"
#define TUI_GENERAL_REGS_NAME                "$GREGS"
#define TUI_GENERAL_REGS_NAME_LOWER          "$gregs"
#define TUI_SPECIAL_REGS_NAME                "$SREGS"
#define TUI_SPECIAL_REGS_NAME_LOWER          "$sregs"
#define TUI_GENERAL_SPECIAL_REGS_NAME        "$REGS"
#define TUI_GENERAL_SPECIAL_REGS_NAME_LOWER  "$regs"

/* Scroll direction enum */
typedef enum
  {
    FORWARD_SCROLL,
    BACKWARD_SCROLL,
    LEFT_SCROLL,
    RIGHT_SCROLL
  }
TuiScrollDirection, *TuiScrollDirectionPtr;


/* General list struct */
typedef struct _TuiList
  {
    OpaqueList list;
    int count;
  }
TuiList, *TuiListPtr;


/* The kinds of layouts available */
typedef enum
  {
    SRC_COMMAND,
    DISASSEM_COMMAND,
    SRC_DISASSEM_COMMAND,
    SRC_DATA_COMMAND,
    DISASSEM_DATA_COMMAND,
    UNDEFINED_LAYOUT
  }
TuiLayoutType, *TuiLayoutTypePtr;

/* Basic data types that can be displayed in the data window. */
typedef enum _TuiDataType
  {
    TUI_REGISTER,
    TUI_SCALAR,
    TUI_COMPLEX,
    TUI_STRUCT
  }
TuiDataType, TuiDataTypePtr;

/* Types of register displays */
typedef enum _TuiRegisterDisplayType
  {
    TUI_UNDEFINED_REGS,
    TUI_GENERAL_REGS,
    TUI_SFLOAT_REGS,
    TUI_DFLOAT_REGS,
    TUI_SPECIAL_REGS,
    TUI_GENERAL_AND_SPECIAL_REGS
  }
TuiRegisterDisplayType, *TuiRegisterDisplayTypePtr;

/* Structure describing source line or line address */
typedef union _TuiLineOrAddress
  {
    int lineNo;
    CORE_ADDR addr;
  }
TuiLineOrAddress, *TuiLineOrAddressPtr;

/* Current Layout definition */
typedef struct _TuiLayoutDef
  {
    TuiWinType displayMode;
    int split;
    TuiRegisterDisplayType regsDisplayType;
    TuiRegisterDisplayType floatRegsDisplayType;
  }
TuiLayoutDef, *TuiLayoutDefPtr;

/* Elements in the Source/Disassembly Window */
typedef struct _TuiSourceElement
  {
    char *line;
    TuiLineOrAddress lineOrAddr;
    int isExecPoint;
    int hasBreak;
  }
TuiSourceElement, *TuiSourceElementPtr;


/* Elements in the data display window content */
typedef struct _TuiDataElement
  {
    const char *name;
    int itemNo;			/* the register number, or data display number */
    TuiDataType type;
    Opaque value;
    int highlight;
  }
TuiDataElement, *TuiDataElementPtr;


/* Elements in the command window content */
typedef struct _TuiCommandElement
  {
    char *line;
  }
TuiCommandElement, *TuiCommandElementPtr;


#define MAX_LOCATOR_ELEMENT_LEN        100

/* Elements in the locator window content */
typedef struct _TuiLocatorElement
  {
    char fileName[MAX_LOCATOR_ELEMENT_LEN];
    char procName[MAX_LOCATOR_ELEMENT_LEN];
    int lineNo;
    CORE_ADDR addr;
  }
TuiLocatorElement, *TuiLocatorElementPtr;

/* Flags to tell what kind of breakpoint is at current line.  */
#define TUI_BP_ENABLED      0x01
#define TUI_BP_DISABLED     0x02
#define TUI_BP_HIT          0x04
#define TUI_BP_CONDITIONAL  0x08
#define TUI_BP_HARDWARE     0x10

/* Position of breakpoint markers in the exec info string.  */
#define TUI_BP_HIT_POS      0
#define TUI_BP_BREAK_POS    1
#define TUI_EXEC_POS        2
#define TUI_EXECINFO_SIZE   4

typedef char TuiExecInfoContent[TUI_EXECINFO_SIZE];

/* An content element in a window */
typedef union
  {
    TuiSourceElement source;	/* the source elements */
    TuiGenWinInfo dataWindow;	/* data display elements */
    TuiDataElement data;	/* elements of dataWindow */
    TuiCommandElement command;	/* command elements */
    TuiLocatorElement locator;	/* locator elements */
    TuiExecInfoContent simpleString;	/* simple char based elements */
  }
TuiWhichElement, *TuiWhichElementPtr;

typedef struct _TuiWinElement
  {
    int highlight;
    TuiWhichElement whichElement;
  }
TuiWinElement, *TuiWinElementPtr;


/* This describes the content of the window. */
typedef TuiWinElementPtr *TuiWinContent;


/* This struct defines the specific information about a data display window */
typedef struct _TuiDataInfo
  {
    TuiWinContent dataContent;	/* start of data display content */
    int dataContentCount;
    TuiWinContent regsContent;	/* start of regs display content */
    int regsContentCount;
    TuiRegisterDisplayType regsDisplayType;
    int regsColumnCount;
    int displayRegs;		/* Should regs be displayed at all? */
  }
TuiDataInfo, *TuiDataInfoPtr;


typedef struct _TuiSourceInfo
  {
    int hasLocator;		/* Does locator belongs to this window? */
    TuiGenWinInfoPtr executionInfo;	/* execution information window */
    int horizontalOffset;	/* used for horizontal scroll */
    TuiLineOrAddress startLineOrAddr;
    char* filename;
  }
TuiSourceInfo, *TuiSourceInfoPtr;


typedef struct _TuiCommandInfo
  {
    int curLine;		/* The current line position */
    int curch;			/* The current cursor position */
    int start_line;
  }
TuiCommandInfo, *TuiCommandInfoPtr;


/* This defines information about each logical window */
typedef struct _TuiWinInfo
  {
    TuiGenWinInfo generic;	/* general window information */
    union
      {
	TuiSourceInfo sourceInfo;
	TuiDataInfo dataDisplayInfo;
	TuiCommandInfo commandInfo;
	Opaque opaque;
      }
    detail;
    int canHighlight;		/* Can this window ever be highlighted? */
    int isHighlighted;		/* Is this window highlighted? */
  }
TuiWinInfo, *TuiWinInfoPtr;

/* MACROS (prefixed with m_) */

/* Testing macros */
#define        m_genWinPtrIsNull(winInfo) \
                ((winInfo) == (TuiGenWinInfoPtr)NULL)
#define        m_genWinPtrNotNull(winInfo) \
                ((winInfo) != (TuiGenWinInfoPtr)NULL)
#define        m_winPtrIsNull(winInfo) \
                ((winInfo) == (TuiWinInfoPtr)NULL)
#define        m_winPtrNotNull(winInfo) \
                ((winInfo) != (TuiWinInfoPtr)NULL)

#define        m_winIsSourceType(type) \
                (type == SRC_WIN || type == DISASSEM_WIN)
#define        m_winIsAuxillary(winType) \
                (winType > MAX_MAJOR_WINDOWS)
#define        m_hasLocator(winInfo) \
                ( ((winInfo) != (TuiWinInfoPtr)NULL) ? \
                    (winInfo->detail.sourceInfo.hasLocator) : \
                    FALSE )

#define     m_setWinHighlightOn(winInfo) \
                if ((winInfo) != (TuiWinInfoPtr)NULL) \
                              (winInfo)->isHighlighted = TRUE
#define     m_setWinHighlightOff(winInfo) \
                if ((winInfo) != (TuiWinInfoPtr)NULL) \
                              (winInfo)->isHighlighted = FALSE


/* Global Data */
extern TuiWinInfoPtr winList[MAX_MAJOR_WINDOWS];
extern int tui_version;

/* Macros */
#define srcWin            winList[SRC_WIN]
#define disassemWin       winList[DISASSEM_WIN]
#define dataWin           winList[DATA_WIN]
#define cmdWin            winList[CMD_WIN]

/* Data Manipulation Functions */
extern void initializeStaticData (void);
extern TuiGenWinInfoPtr allocGenericWinInfo (void);
extern TuiWinInfoPtr allocWinInfo (TuiWinType);
extern void initGenericPart (TuiGenWinInfoPtr);
extern void initWinInfo (TuiWinInfoPtr);
extern TuiWinContent allocContent (int, TuiWinType);
extern int addContentElements (TuiGenWinInfoPtr, int);
extern void initContentElement (TuiWinElementPtr, TuiWinType);
extern void freeWindow (TuiWinInfoPtr);
extern void freeWinContent (TuiGenWinInfoPtr);
extern void freeDataContent (TuiWinContent, int);
extern void freeAllSourceWinsContent (void);
extern void tuiDelWindow (TuiWinInfoPtr);
extern void tuiDelDataWindows (TuiWinContent, int);
extern TuiWinInfoPtr partialWinByName (char *);
extern char *winName (TuiGenWinInfoPtr);
extern TuiLayoutType currentLayout (void);
extern void setCurrentLayoutTo (TuiLayoutType);
extern int termHeight (void);
extern void setTermHeightTo (int);
extern int termWidth (void);
extern void setTermWidthTo (int);
extern void setGenWinOrigin (TuiGenWinInfoPtr, int, int);
extern TuiGenWinInfoPtr locatorWinInfoPtr (void);
extern TuiGenWinInfoPtr sourceExecInfoWinPtr (void);
extern TuiGenWinInfoPtr disassemExecInfoWinPtr (void);
extern TuiListPtr sourceWindows (void);
extern void clearSourceWindows (void);
extern void clearSourceWindowsDetail (void);
extern void clearWinDetail (TuiWinInfoPtr winInfo);
extern void tuiAddToSourceWindows (TuiWinInfoPtr);
extern int tuiDefaultTabLen (void);
extern void tuiSetDefaultTabLen (int);
extern TuiWinInfoPtr tuiWinWithFocus (void);
extern void tuiSetWinWithFocus (TuiWinInfoPtr);
extern TuiLayoutDefPtr tuiLayoutDef (void);
extern int tuiWinResized (void);
extern void tuiSetWinResizedTo (int);

extern TuiWinInfoPtr tuiNextWin (TuiWinInfoPtr);
extern TuiWinInfoPtr tuiPrevWin (TuiWinInfoPtr);

extern void addToSourceWindows (TuiWinInfoPtr winInfo);

#endif /* TUI_DATA_H */
