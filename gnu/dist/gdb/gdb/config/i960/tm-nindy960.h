// OBSOLETE /* Parameters for Intel 960 running NINDY monitor, for GDB, the GNU debugger.
// OBSOLETE    Copyright 1990, 1991, 1996, 1999, 2000 Free Software Foundation, Inc.
// OBSOLETE    Contributed by Intel Corporation and Cygnus Support.
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
// OBSOLETE /*****************************************************************************
// OBSOLETE  * Definitions to target GDB to an i960 debugged over a serial line.
// OBSOLETE  ******************************************************************************/
// OBSOLETE 
// OBSOLETE #include "i960/tm-i960.h"
// OBSOLETE 
// OBSOLETE /* forward declarations */
// OBSOLETE struct frame_info;
// OBSOLETE 
// OBSOLETE /* Override the standard gdb prompt when compiled for this target.  */
// OBSOLETE 
// OBSOLETE #define	DEFAULT_PROMPT	"(gdb960) "
// OBSOLETE 
// OBSOLETE /* Additional command line options accepted by nindy gdb's, for handling
// OBSOLETE    the remote-nindy.c interface.  These should really be target-specific
// OBSOLETE    rather than architecture-specific.  */
// OBSOLETE 
// OBSOLETE extern int nindy_old_protocol;	/* nonzero if old NINDY serial protocol */
// OBSOLETE extern int nindy_initial_brk;	/* Send a BREAK to reset board first */
// OBSOLETE extern char *nindy_ttyname;	/* Name of serial port to talk to nindy */
// OBSOLETE 
// OBSOLETE #define	ADDITIONAL_OPTIONS \
// OBSOLETE 	{"O", no_argument, &nindy_old_protocol, 1},	\
// OBSOLETE 	{"brk", no_argument, &nindy_initial_brk, 1},	\
// OBSOLETE 	{"ser", required_argument, 0, 1004},	/* 1004 is magic cookie for ADDL_CASES */
// OBSOLETE 
// OBSOLETE #define	ADDITIONAL_OPTION_CASES	\
// OBSOLETE 	case 1004:	/* -ser option:  remote nindy auto-start */	\
// OBSOLETE 	  nindy_ttyname = optarg;	\
// OBSOLETE 	  break;
// OBSOLETE 
// OBSOLETE #define	ADDITIONAL_OPTION_HELP \
// OBSOLETE 	"\
// OBSOLETE   -O                Use old protocol to talk to a Nindy target\n\
// OBSOLETE   -brk              Send a break to a Nindy target to reset it.\n\
// OBSOLETE   -ser SERIAL       Open remote Nindy session to SERIAL port.\n\
// OBSOLETE "
// OBSOLETE 
// OBSOLETE /* If specified on the command line, open tty for talking to nindy,
// OBSOLETE    and download the executable file if one was specified.  */
// OBSOLETE 
// OBSOLETE extern void nindy_open (char *name, int from_tty);
// OBSOLETE #define	ADDITIONAL_OPTION_HANDLER					\
// OBSOLETE 	if (nindy_ttyname != NULL)					\
// OBSOLETE           {								\
// OBSOLETE             if (catch_command_errors (nindy_open, nindy_ttyname,	\
// OBSOLETE 				      !batch, RETURN_MASK_ALL))		\
// OBSOLETE 	      {								\
// OBSOLETE                 if (execarg != NULL)					\
// OBSOLETE                   catch_command_errors (target_load, execarg, !batch,	\
// OBSOLETE 					RETURN_MASK_ALL);		\
// OBSOLETE 	      }								\
// OBSOLETE 	  }
// OBSOLETE 
// OBSOLETE /* If configured for i960 target, we take control before main loop
// OBSOLETE    and demand that we configure for a nindy target.  */
// OBSOLETE 
// OBSOLETE #define	BEFORE_MAIN_LOOP_HOOK	\
// OBSOLETE   nindy_before_main_loop();
// OBSOLETE 
// OBSOLETE extern void
// OBSOLETE   nindy_before_main_loop ();	/* In remote-nindy.c */
// OBSOLETE 
// OBSOLETE /* FRAME_CHAIN_VALID returns zero if the given frame is the outermost one
// OBSOLETE    and has no caller.
// OBSOLETE 
// OBSOLETE    On the i960, each various target system type defines FRAME_CHAIN_VALID,
// OBSOLETE    since it differs between NINDY and VxWorks, the two currently supported
// OBSOLETE    targets types.  */
// OBSOLETE 
// OBSOLETE extern int nindy_frame_chain_valid (CORE_ADDR, struct frame_info *);
// OBSOLETE #define	FRAME_CHAIN_VALID(chain, thisframe) nindy_frame_chain_valid (chain, thisframe)
// OBSOLETE 
// OBSOLETE extern int
// OBSOLETE   nindy_frame_chain_valid ();	/* See nindy-tdep.c */
// OBSOLETE 
// OBSOLETE /* Sequence of bytes for breakpoint instruction */
// OBSOLETE 
// OBSOLETE #define BREAKPOINT {0x00, 0x3e, 0x00, 0x66}
// OBSOLETE 
// OBSOLETE /* Amount ip must be decremented by after a breakpoint.
// OBSOLETE  * This is often the number of bytes in BREAKPOINT but not always.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE #define DECR_PC_AFTER_BREAK 0
