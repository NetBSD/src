/* TUI display registers in window.
   Copyright 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
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

#ifndef _TUI_REGS_H
#define _TUI_REGS_H

/*****************************************
** TYPE DEFINITIONS                        **
******************************************/



/*****************************************
** PUBLIC FUNCTION EXTERNAL DECLS        **
******************************************/
extern void tuiCheckRegisterValues (struct frame_info *);
extern void tuiShowRegisters (TuiRegisterDisplayType);
extern void tuiDisplayRegistersFrom (int);
extern int tuiDisplayRegistersFromLine (int, int);
extern int tuiLastRegsLineNo (void);
extern int tuiFirstRegElementInLine (int);
extern int tuiLastRegElementInLine (int);
extern int tuiLineFromRegElementNo (int);
extern void tuiToggleFloatRegs (void);
extern int tuiCalculateRegsColumnCount (TuiRegisterDisplayType);
extern int tuiFirstRegElementNoInLine (int lineno);

#endif
/*_TUI_REGS_H*/
