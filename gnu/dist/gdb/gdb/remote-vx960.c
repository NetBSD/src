// OBSOLETE /* i80960-dependent portions of the RPC protocol
// OBSOLETE    used with a VxWorks target 
// OBSOLETE 
// OBSOLETE    Contributed by Wind River Systems.
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
// OBSOLETE #include <stdio.h>
// OBSOLETE #include "defs.h"
// OBSOLETE 
// OBSOLETE #include "vx-share/regPacket.h"
// OBSOLETE #include "frame.h"
// OBSOLETE #include "inferior.h"
// OBSOLETE #include "target.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "command.h"
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "symfile.h"		/* for struct complaint */
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE #include "gdb_string.h"
// OBSOLETE #include <errno.h>
// OBSOLETE #include <fcntl.h>
// OBSOLETE #include <sys/types.h>
// OBSOLETE #include <sys/time.h>
// OBSOLETE #include <sys/socket.h>
// OBSOLETE 
// OBSOLETE #ifdef _AIX			/* IBM claims "void *malloc()" not char * */
// OBSOLETE #define malloc bogon_malloc
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #include <rpc/rpc.h>
// OBSOLETE #include <sys/time.h>		/* UTek's <rpc/rpc.h> doesn't #incl this */
// OBSOLETE #include <netdb.h>
// OBSOLETE #include "vx-share/ptrace.h"
// OBSOLETE #include "vx-share/xdr_ptrace.h"
// OBSOLETE #include "vx-share/xdr_ld.h"
// OBSOLETE #include "vx-share/xdr_rdb.h"
// OBSOLETE #include "vx-share/dbgRpcLib.h"
// OBSOLETE 
// OBSOLETE /* get rid of value.h if possible */
// OBSOLETE #include <value.h>
// OBSOLETE #include <symtab.h>
// OBSOLETE 
// OBSOLETE /* Flag set if target has fpu */
// OBSOLETE 
// OBSOLETE extern int target_has_fp;
// OBSOLETE 
// OBSOLETE /* 960 floating point format descriptor, from "i960-tdep.c."  */
// OBSOLETE 
// OBSOLETE extern struct ext_format ext_format_i960;
// OBSOLETE 
// OBSOLETE /* Generic register read/write routines in remote-vx.c.  */
// OBSOLETE 
// OBSOLETE extern void net_read_registers ();
// OBSOLETE extern void net_write_registers ();
// OBSOLETE 
// OBSOLETE /* Read a register or registers from the VxWorks target.
// OBSOLETE    REGNO is the register to read, or -1 for all; currently,
// OBSOLETE    it is ignored.  FIXME look at regno to improve efficiency.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE vx_read_register (int regno)
// OBSOLETE {
// OBSOLETE   char i960_greg_packet[I960_GREG_PLEN];
// OBSOLETE   char i960_fpreg_packet[I960_FPREG_PLEN];
// OBSOLETE 
// OBSOLETE   /* Get general-purpose registers.  When copying values into
// OBSOLETE      registers [], don't assume that a location in registers []
// OBSOLETE      is properly aligned for the target data type.  */
// OBSOLETE 
// OBSOLETE   net_read_registers (i960_greg_packet, I960_GREG_PLEN, PTRACE_GETREGS);
// OBSOLETE 
// OBSOLETE   bcopy (&i960_greg_packet[I960_R_R0],
// OBSOLETE 	 &registers[REGISTER_BYTE (R0_REGNUM)], 16 * I960_GREG_SIZE);
// OBSOLETE   bcopy (&i960_greg_packet[I960_R_G0],
// OBSOLETE 	 &registers[REGISTER_BYTE (G0_REGNUM)], 16 * I960_GREG_SIZE);
// OBSOLETE   bcopy (&i960_greg_packet[I960_R_PCW],
// OBSOLETE 	 &registers[REGISTER_BYTE (PCW_REGNUM)], sizeof (int));
// OBSOLETE   bcopy (&i960_greg_packet[I960_R_ACW],
// OBSOLETE 	 &registers[REGISTER_BYTE (ACW_REGNUM)], sizeof (int));
// OBSOLETE   bcopy (&i960_greg_packet[I960_R_TCW],
// OBSOLETE 	 &registers[REGISTER_BYTE (TCW_REGNUM)], sizeof (int));
// OBSOLETE 
// OBSOLETE   /* If the target has floating point registers, fetch them.
// OBSOLETE      Otherwise, zero the floating point register values in
// OBSOLETE      registers[] for good measure, even though we might not
// OBSOLETE      need to.  */
// OBSOLETE 
// OBSOLETE   if (target_has_fp)
// OBSOLETE     {
// OBSOLETE       net_read_registers (i960_fpreg_packet, I960_FPREG_PLEN,
// OBSOLETE 			  PTRACE_GETFPREGS);
// OBSOLETE       bcopy (&i960_fpreg_packet[I960_R_FP0],
// OBSOLETE 	     &registers[REGISTER_BYTE (FP0_REGNUM)],
// OBSOLETE 	     REGISTER_RAW_SIZE (FP0_REGNUM) * 4);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     bzero (&registers[REGISTER_BYTE (FP0_REGNUM)],
// OBSOLETE 	   REGISTER_RAW_SIZE (FP0_REGNUM) * 4);
// OBSOLETE 
// OBSOLETE   /* Mark the register cache valid.  */
// OBSOLETE 
// OBSOLETE   registers_fetched ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Store a register or registers into the VxWorks target.
// OBSOLETE    REGNO is the register to store, or -1 for all; currently,
// OBSOLETE    it is ignored.  FIXME look at regno to improve efficiency.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE vx_write_register (int regno)
// OBSOLETE {
// OBSOLETE   char i960_greg_packet[I960_GREG_PLEN];
// OBSOLETE   char i960_fpreg_packet[I960_FPREG_PLEN];
// OBSOLETE 
// OBSOLETE   /* Store floating-point registers.  When copying values from
// OBSOLETE      registers [], don't assume that a location in registers []
// OBSOLETE      is properly aligned for the target data type.  */
// OBSOLETE 
// OBSOLETE   bcopy (&registers[REGISTER_BYTE (R0_REGNUM)],
// OBSOLETE 	 &i960_greg_packet[I960_R_R0], 16 * I960_GREG_SIZE);
// OBSOLETE   bcopy (&registers[REGISTER_BYTE (G0_REGNUM)],
// OBSOLETE 	 &i960_greg_packet[I960_R_G0], 16 * I960_GREG_SIZE);
// OBSOLETE   bcopy (&registers[REGISTER_BYTE (PCW_REGNUM)],
// OBSOLETE 	 &i960_greg_packet[I960_R_PCW], sizeof (int));
// OBSOLETE   bcopy (&registers[REGISTER_BYTE (ACW_REGNUM)],
// OBSOLETE 	 &i960_greg_packet[I960_R_ACW], sizeof (int));
// OBSOLETE   bcopy (&registers[REGISTER_BYTE (TCW_REGNUM)],
// OBSOLETE 	 &i960_greg_packet[I960_R_TCW], sizeof (int));
// OBSOLETE 
// OBSOLETE   net_write_registers (i960_greg_packet, I960_GREG_PLEN, PTRACE_SETREGS);
// OBSOLETE 
// OBSOLETE   /* Store floating point registers if the target has them.  */
// OBSOLETE 
// OBSOLETE   if (target_has_fp)
// OBSOLETE     {
// OBSOLETE       bcopy (&registers[REGISTER_BYTE (FP0_REGNUM)],
// OBSOLETE 	     &i960_fpreg_packet[I960_R_FP0],
// OBSOLETE 	     REGISTER_RAW_SIZE (FP0_REGNUM) * 4);
// OBSOLETE 
// OBSOLETE       net_write_registers (i960_fpreg_packet, I960_FPREG_PLEN,
// OBSOLETE 			   PTRACE_SETFPREGS);
// OBSOLETE     }
// OBSOLETE }
