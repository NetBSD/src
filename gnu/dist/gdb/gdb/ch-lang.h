// OBSOLETE /* Chill language support definitions for GDB, the GNU debugger.
// OBSOLETE    Copyright 1992, 1994, 1996, 1998, 1999, 2000
// OBSOLETE    Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE /* Forward decls for prototypes */
// OBSOLETE struct value;
// OBSOLETE 
// OBSOLETE extern int chill_parse (void);	/* Defined in ch-exp.y */
// OBSOLETE 
// OBSOLETE extern void chill_error (char *);	/* Defined in ch-exp.y */
// OBSOLETE 
// OBSOLETE /* Defined in ch-typeprint.c */
// OBSOLETE extern void chill_print_type (struct type *, char *, struct ui_file *, int,
// OBSOLETE 			      int);
// OBSOLETE 
// OBSOLETE extern int chill_val_print (struct type *, char *, int, CORE_ADDR,
// OBSOLETE 			    struct ui_file *, int, int, int,
// OBSOLETE 			    enum val_prettyprint);
// OBSOLETE 
// OBSOLETE extern int chill_value_print (struct value *, struct ui_file *,
// OBSOLETE 			      int, enum val_prettyprint);
// OBSOLETE 
// OBSOLETE extern LONGEST
// OBSOLETE type_lower_upper (enum exp_opcode, struct type *, struct type **);
