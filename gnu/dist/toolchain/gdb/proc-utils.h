/* Machine independent support for SVR4 /proc (process file system) for GDB.
   Copyright 1999 Free Software Foundation, Inc.

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
along with this program; if not, write to the Free Software Foundation, 
Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


extern void 
proc_prettyprint_why (unsigned long why, unsigned long what, int verbose);

extern void proc_prettyprint_syscalls (sysset_t *sysset, int verbose);

extern void proc_prettyprint_syscall (int num, int verbose);

extern void proc_prettyprint_flags (unsigned long flags, int verbose);

extern void
proc_prettyfprint_signalset (FILE *file, sigset_t *sigset, int verbose);

extern void
proc_prettyfprint_faultset (FILE *file, fltset_t *fltset, int verbose);

extern void
proc_prettyfprint_syscall (FILE *file, int num, int verbose);

extern void
proc_prettyfprint_signal (FILE *file, int signo, int verbose);

extern void
proc_prettyfprint_flags (FILE *file, unsigned long flags, int verbose);

extern void
proc_prettyfprint_why (FILE *file, unsigned long why, unsigned long what, int verbose);

extern void
proc_prettyfprint_fault (FILE *file, int faultno, int verbose);

extern void
proc_prettyfprint_syscalls (FILE *file, sysset_t *sysset, int verbose);
