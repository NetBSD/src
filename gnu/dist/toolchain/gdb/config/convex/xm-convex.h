/* OBSOLETE /* Definitions to make GDB run on Convex Unix (4bsd) */
/* OBSOLETE    Copyright 1989, 1991, 1992, 1996  Free Software Foundation, Inc. */
/* OBSOLETE  */
/* OBSOLETE This file is part of GDB. */
/* OBSOLETE  */
/* OBSOLETE This program is free software; you can redistribute it and/or modify */
/* OBSOLETE it under the terms of the GNU General Public License as published by */
/* OBSOLETE the Free Software Foundation; either version 2 of the License, or */
/* OBSOLETE (at your option) any later version. */
/* OBSOLETE  */
/* OBSOLETE This program is distributed in the hope that it will be useful, */
/* OBSOLETE but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* OBSOLETE MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* OBSOLETE GNU General Public License for more details. */
/* OBSOLETE  */
/* OBSOLETE You should have received a copy of the GNU General Public License */
/* OBSOLETE along with this program; if not, write to the Free Software */
/* OBSOLETE Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define HOST_BYTE_ORDER BIG_ENDIAN */
/* OBSOLETE  */
/* OBSOLETE #define ATTACH_DETACH */
/* OBSOLETE #define HAVE_WAIT_STRUCT */
/* OBSOLETE #define NO_SIGINTERRUPT */
/* OBSOLETE  */
/* OBSOLETE /* Use SIGCONT rather than SIGTSTP because convex Unix occasionally */
/* OBSOLETE    turkeys SIGTSTP.  I think.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define STOP_SIGNAL SIGCONT */
/* OBSOLETE  */
/* OBSOLETE /* Hook to call after creating inferior process.  Now init_trace_fun */
/* OBSOLETE    is in the same place.  So re-write this to use the init_trace_fun */
/* OBSOLETE    (making convex a debugging target).  FIXME.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define CREATE_INFERIOR_HOOK create_inferior_hook */
