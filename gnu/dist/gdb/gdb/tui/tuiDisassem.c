/* Disassembly display.

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
#include "symtab.h"
#include "breakpoint.h"
#include "frame.h"
#include "value.h"

#include "tui.h"
#include "tuiData.h"
#include "tuiWin.h"
#include "tuiLayout.h"
#include "tuiSourceWin.h"
#include "tuiStack.h"
#include "tui-file.h"

struct tui_asm_line 
{
  CORE_ADDR addr;
  char* addr_string;
  char* insn;
};

/* Function to set the disassembly window's content.
   Disassemble count lines starting at pc.
   Return address of the count'th instruction after pc.  */
static CORE_ADDR
tui_disassemble (struct tui_asm_line* lines, CORE_ADDR pc, int count)
{
  struct ui_file *gdb_dis_out;
  disassemble_info asm_info;

  /* now init the ui_file structure */
  gdb_dis_out = tui_sfileopen (256);

  memcpy (&asm_info, TARGET_PRINT_INSN_INFO, sizeof (asm_info));
  asm_info.stream = gdb_dis_out;

  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
    asm_info.endian = BFD_ENDIAN_BIG;
  else
    asm_info.endian = BFD_ENDIAN_LITTLE;

  if (TARGET_ARCHITECTURE != NULL)
    asm_info.mach = TARGET_ARCHITECTURE->mach;

  /* Now construct each line */
  for (; count > 0; count--, lines++)
    {
      if (lines->addr_string)
        xfree (lines->addr_string);
      if (lines->insn)
        xfree (lines->insn);
      
      print_address (pc, gdb_dis_out);
      lines->addr = pc;
      lines->addr_string = xstrdup (tui_file_get_strbuf (gdb_dis_out));

      ui_file_rewind (gdb_dis_out);

      pc = pc + TARGET_PRINT_INSN (pc, &asm_info);

      lines->insn = xstrdup (tui_file_get_strbuf (gdb_dis_out));

      /* reset the buffer to empty */
      ui_file_rewind (gdb_dis_out);
    }
  ui_file_delete (gdb_dis_out);
  return pc;
}

/* Find the disassembly address that corresponds to FROM lines
   above or below the PC.  Variable sized instructions are taken
   into account by the algorithm.  */
static CORE_ADDR
tui_find_disassembly_address (CORE_ADDR pc, int from)
{
  register CORE_ADDR newLow;
  int maxLines;
  int i;
  struct tui_asm_line* lines;

  maxLines = (from > 0) ? from : - from;
  if (maxLines <= 1)
     return pc;

  lines = (struct tui_asm_line*) alloca (sizeof (struct tui_asm_line)
                                         * maxLines);
  memset (lines, 0, sizeof (struct tui_asm_line) * maxLines);

  newLow = pc;
  if (from > 0)
    {
      tui_disassemble (lines, pc, maxLines);
      newLow = lines[maxLines - 1].addr;
    }
  else
    {
      CORE_ADDR last_addr;
      int pos;
      struct minimal_symbol* msymbol;
              
      /* Find backward an address which is a symbol
         and for which disassembling from that address will fill
         completely the window.  */
      pos = maxLines - 1;
      do {
         newLow -= 1 * maxLines;
         msymbol = lookup_minimal_symbol_by_pc_section (newLow, 0);

         if (msymbol)
            newLow = SYMBOL_VALUE_ADDRESS (msymbol);
         else
            newLow += 1 * maxLines;

         tui_disassemble (lines, newLow, maxLines);
         last_addr = lines[pos].addr;
      } while (last_addr > pc && msymbol);

      /* Scan forward disassembling one instruction at a time
         until the last visible instruction of the window
         matches the pc.  We keep the disassembled instructions
         in the 'lines' window and shift it downward (increasing
         its addresses).  */
      if (last_addr < pc)
        do
          {
            CORE_ADDR next_addr;
                 
            pos++;
            if (pos >= maxLines)
              pos = 0;

            next_addr = tui_disassemble (&lines[pos], last_addr, 1);

            /* If there are some problems while disassembling exit.  */
            if (next_addr <= last_addr)
              break;
            last_addr = next_addr;
          } while (last_addr <= pc);
      pos++;
      if (pos >= maxLines)
         pos = 0;
      newLow = lines[pos].addr;
    }
  for (i = 0; i < maxLines; i++)
    {
      xfree (lines[i].addr_string);
      xfree (lines[i].insn);
    }
  return newLow;
}

/* Function to set the disassembly window's content.  */
TuiStatus
tuiSetDisassemContent (CORE_ADDR pc)
{
  TuiStatus ret = TUI_FAILURE;
  register int i;
  register int offset = disassemWin->detail.sourceInfo.horizontalOffset;
  register int lineWidth, maxLines;
  CORE_ADDR cur_pc;
  TuiGenWinInfoPtr locator = locatorWinInfoPtr ();
  int tab_len = tuiDefaultTabLen ();
  struct tui_asm_line* lines;
  int insn_pos;
  int addr_size, max_size;
  char* line;
  
  if (pc == 0)
    return TUI_FAILURE;

  ret = tuiAllocSourceBuffer (disassemWin);
  if (ret != TUI_SUCCESS)
    return ret;

  disassemWin->detail.sourceInfo.startLineOrAddr.addr = pc;
  cur_pc = (CORE_ADDR)
    (((TuiWinElementPtr) locator->content[0])->whichElement.locator.addr);

  maxLines = disassemWin->generic.height - 2;	/* account for hilite */

  /* Get temporary table that will hold all strings (addr & insn).  */
  lines = (struct tui_asm_line*) alloca (sizeof (struct tui_asm_line)
                                         * maxLines);
  memset (lines, 0, sizeof (struct tui_asm_line) * maxLines);

  lineWidth = disassemWin->generic.width - 1;

  tui_disassemble (lines, pc, maxLines);

  /* See what is the maximum length of an address and of a line.  */
  addr_size = 0;
  max_size = 0;
  for (i = 0; i < maxLines; i++)
    {
      size_t len = strlen (lines[i].addr_string);
      if (len > addr_size)
        addr_size = len;

      len = strlen (lines[i].insn) + tab_len;
      if (len > max_size)
        max_size = len;
    }
  max_size += addr_size + tab_len;

  /* Allocate memory to create each line.  */
  line = (char*) alloca (max_size);
  insn_pos = (1 + (addr_size / tab_len)) * tab_len;

  /* Now construct each line */
  for (i = 0; i < maxLines; i++)
    {
      TuiWinElementPtr element;
      TuiSourceElement* src;
      int curLen;

      element = (TuiWinElementPtr) disassemWin->generic.content[i];
      src = &element->whichElement.source;
      strcpy (line, lines[i].addr_string);
      curLen = strlen (line);

      /* Add spaces to make the instructions start on the same column */
      while (curLen < insn_pos)
        {
          strcat (line, " ");
          curLen++;
        }

      strcat (line, lines[i].insn);

      /* Now copy the line taking the offset into account */
      if (strlen (line) > offset)
        strcpy (src->line, &line[offset]);
      else
        src->line[0] = '\0';

      src->lineOrAddr.addr = lines[i].addr;
      src->isExecPoint = lines[i].addr == cur_pc;

      /* See whether there is a breakpoint installed.  */
      src->hasBreak = (!src->isExecPoint
                       && breakpoint_here_p (pc) != no_breakpoint_here);

      xfree (lines[i].addr_string);
      xfree (lines[i].insn);
    }
  disassemWin->generic.contentSize = i;
  return TUI_SUCCESS;
}


/*
   ** tuiShowDisassem().
   **        Function to display the disassembly window with disassembled code.
 */
void
tuiShowDisassem (CORE_ADDR startAddr)
{
  struct symtab *s = find_pc_symtab (startAddr);
  TuiWinInfoPtr winWithFocus = tuiWinWithFocus ();
  TuiLineOrAddress val;

  val.addr = startAddr;
  tuiAddWinToLayout (DISASSEM_WIN);
  tuiUpdateSourceWindow (disassemWin, s, val, FALSE);
  /*
     ** if the focus was in the src win, put it in the asm win, if the
     ** source view isn't split
   */
  if (currentLayout () != SRC_DISASSEM_COMMAND && winWithFocus == srcWin)
    tuiSetWinFocusTo (disassemWin);

  return;
}				/* tuiShowDisassem */


/*
   ** tuiShowDisassemAndUpdateSource().
   **        Function to display the disassembly window.
 */
void
tuiShowDisassemAndUpdateSource (CORE_ADDR startAddr)
{
  struct symtab_and_line sal;

  tuiShowDisassem (startAddr);
  if (currentLayout () == SRC_DISASSEM_COMMAND)
    {
      TuiLineOrAddress val;

      /*
         ** Update what is in the source window if it is displayed too,
         ** note that it follows what is in the disassembly window and visa-versa
       */
      sal = find_pc_line (startAddr, 0);
      val.lineNo = sal.line;
      tuiUpdateSourceWindow (srcWin, sal.symtab, val, TRUE);
      if (sal.symtab)
	{
	  current_source_symtab = sal.symtab;
	  tuiUpdateLocatorFilename (sal.symtab->filename);
	}
      else
	tuiUpdateLocatorFilename ("?");
    }

  return;
}				/* tuiShowDisassemAndUpdateSource */

/*
   ** tuiGetBeginAsmAddress().
 */
CORE_ADDR
tuiGetBeginAsmAddress (void)
{
  TuiGenWinInfoPtr locator;
  TuiLocatorElementPtr element;
  CORE_ADDR addr;

  locator = locatorWinInfoPtr ();
  element = &((TuiWinElementPtr) locator->content[0])->whichElement.locator;

  if (element->addr == 0)
    {
      struct minimal_symbol *main_symbol;

      /* Find address of the start of program.
         Note: this should be language specific.  */
      main_symbol = lookup_minimal_symbol ("main", NULL, NULL);
      if (main_symbol == 0)
        main_symbol = lookup_minimal_symbol ("MAIN", NULL, NULL);
      if (main_symbol == 0)
        main_symbol = lookup_minimal_symbol ("_start", NULL, NULL);
      if (main_symbol)
        addr = SYMBOL_VALUE_ADDRESS (main_symbol);
      else
        addr = 0;
    }
  else				/* the target is executing */
    addr = element->addr;

  return addr;
}				/* tuiGetBeginAsmAddress */

/* Determine what the low address will be to display in the TUI's
   disassembly window.  This may or may not be the same as the
   low address input.  */
CORE_ADDR
tuiGetLowDisassemblyAddress (CORE_ADDR low, CORE_ADDR pc)
{
  int pos;

  /* Determine where to start the disassembly so that the pc is about in the
     middle of the viewport.  */
  pos = tuiDefaultWinViewportHeight (DISASSEM_WIN, DISASSEM_COMMAND) / 2;
  pc = tui_find_disassembly_address (pc, -pos);

  if (pc < low)
    pc = low;
  return pc;
}

/*
   ** tuiVerticalDisassemScroll().
   **      Scroll the disassembly forward or backward vertically
 */
void
tuiVerticalDisassemScroll (TuiScrollDirection scrollDirection,
                           int numToScroll)
{
  if (disassemWin->generic.content != (OpaquePtr) NULL)
    {
      CORE_ADDR pc;
      TuiWinContent content;
      struct symtab *s;
      TuiLineOrAddress val;
      int maxLines, dir;

      content = (TuiWinContent) disassemWin->generic.content;
      if (current_source_symtab == (struct symtab *) NULL)
	s = find_pc_symtab (selected_frame->pc);
      else
	s = current_source_symtab;

      /* account for hilite */
      maxLines = disassemWin->generic.height - 2;
      pc = content[0]->whichElement.source.lineOrAddr.addr;
      dir = (scrollDirection == FORWARD_SCROLL) ? maxLines : - maxLines;

      val.addr = tui_find_disassembly_address (pc, dir);
      tuiUpdateSourceWindowAsIs (disassemWin, s, val, FALSE);
    }
}
