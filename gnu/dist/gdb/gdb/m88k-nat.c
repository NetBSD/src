// OBSOLETE /* Native-dependent Motorola 88xxx support for GDB, the GNU Debugger.
// OBSOLETE    Copyright 1988, 1990, 1991, 1992, 1993, 1995, 1999, 2000, 2001
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
// OBSOLETE #include "defs.h"
// OBSOLETE #include "frame.h"
// OBSOLETE #include "inferior.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE #include <sys/types.h>
// OBSOLETE #include <sys/param.h>
// OBSOLETE #include <sys/dir.h>
// OBSOLETE #include <signal.h>
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include <sys/user.h>
// OBSOLETE 
// OBSOLETE #ifndef USER			/* added to support BCS ptrace_user */
// OBSOLETE #define USER ptrace_user
// OBSOLETE #endif
// OBSOLETE #include <sys/ioctl.h>
// OBSOLETE #include <fcntl.h>
// OBSOLETE #include <sys/file.h>
// OBSOLETE #include "gdb_stat.h"
// OBSOLETE 
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "setjmp.h"
// OBSOLETE #include "value.h"
// OBSOLETE 
// OBSOLETE #ifdef DELTA88
// OBSOLETE #include <sys/ptrace.h>
// OBSOLETE 
// OBSOLETE /* define offsets to the pc instruction offsets in ptrace_user struct */
// OBSOLETE #define SXIP_OFFSET ((char *)&u.pt_sigframe.sig_sxip - (char *)&u)
// OBSOLETE #define SNIP_OFFSET ((char *)&u.pt_sigframe.sig_snip - (char *)&u)
// OBSOLETE #define SFIP_OFFSET ((char *)&u.pt_sigframe.sig_sfip - (char *)&u)
// OBSOLETE #else
// OBSOLETE /* define offsets to the pc instruction offsets in ptrace_user struct */
// OBSOLETE #define SXIP_OFFSET ((char *)&u.pt_sigframe.dg_sigframe.sc_sxip - (char *)&u)
// OBSOLETE #define SNIP_OFFSET ((char *)&u.pt_sigframe.dg_sigframe.sc_snip - (char *)&u)
// OBSOLETE #define SFIP_OFFSET ((char *)&u.pt_sigframe.dg_sigframe.sc_sfip - (char *)&u)
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE extern int have_symbol_file_p ();
// OBSOLETE 
// OBSOLETE extern jmp_buf stack_jmp;
// OBSOLETE 
// OBSOLETE extern int errno;
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fetch_inferior_registers (int regno)
// OBSOLETE {
// OBSOLETE   register unsigned int regaddr;
// OBSOLETE   char buf[MAX_REGISTER_RAW_SIZE];
// OBSOLETE   register int i;
// OBSOLETE 
// OBSOLETE   struct USER u;
// OBSOLETE   unsigned int offset;
// OBSOLETE 
// OBSOLETE   offset = (char *) &u.pt_r0 - (char *) &u;
// OBSOLETE   regaddr = offset;		/* byte offset to r0; */
// OBSOLETE 
// OBSOLETE /*  offset = ptrace (3, PIDGET (inferior_ptid), (PTRACE_ARG3_TYPE) offset, 0) - KERNEL_U_ADDR; */
// OBSOLETE   for (regno = 0; regno < NUM_REGS; regno++)
// OBSOLETE     {
// OBSOLETE       /*regaddr = register_addr (regno, offset); */
// OBSOLETE       /* 88k enhancement  */
// OBSOLETE 
// OBSOLETE       for (i = 0; i < REGISTER_RAW_SIZE (regno); i += sizeof (int))
// OBSOLETE 	{
// OBSOLETE 	  *(int *) &buf[i] = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 				     (PTRACE_ARG3_TYPE) regaddr, 0);
// OBSOLETE 	  regaddr += sizeof (int);
// OBSOLETE 	}
// OBSOLETE       supply_register (regno, buf);
// OBSOLETE     }
// OBSOLETE   /* now load up registers 36 - 38; special pc registers */
// OBSOLETE   *(int *) &buf[0] = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 			     (PTRACE_ARG3_TYPE) SXIP_OFFSET, 0);
// OBSOLETE   supply_register (SXIP_REGNUM, buf);
// OBSOLETE   *(int *) &buf[0] = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 			     (PTRACE_ARG3_TYPE) SNIP_OFFSET, 0);
// OBSOLETE   supply_register (SNIP_REGNUM, buf);
// OBSOLETE   *(int *) &buf[0] = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 			     (PTRACE_ARG3_TYPE) SFIP_OFFSET, 0);
// OBSOLETE   supply_register (SFIP_REGNUM, buf);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Store our register values back into the inferior.
// OBSOLETE    If REGNO is -1, do this for all registers.
// OBSOLETE    Otherwise, REGNO specifies which register (so we can save time).  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE store_inferior_registers (int regno)
// OBSOLETE {
// OBSOLETE   register unsigned int regaddr;
// OBSOLETE   char buf[80];
// OBSOLETE 
// OBSOLETE   struct USER u;
// OBSOLETE 
// OBSOLETE   unsigned int offset = (char *) &u.pt_r0 - (char *) &u;
// OBSOLETE 
// OBSOLETE   regaddr = offset;
// OBSOLETE 
// OBSOLETE   /* Don't try to deal with EXIP_REGNUM or ENIP_REGNUM, because I think either
// OBSOLETE      svr3 doesn't run on an 88110, or the kernel isolates the different (not
// OBSOLETE      completely sure this is true, but seems to be.  */
// OBSOLETE   if (regno >= 0)
// OBSOLETE     {
// OBSOLETE       /*      regaddr = register_addr (regno, offset); */
// OBSOLETE       if (regno < PC_REGNUM)
// OBSOLETE 	{
// OBSOLETE 	  regaddr = offset + regno * sizeof (int);
// OBSOLETE 	  errno = 0;
// OBSOLETE 	  ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		  (PTRACE_ARG3_TYPE) regaddr, read_register (regno));
// OBSOLETE 	  if (errno != 0)
// OBSOLETE 	    {
// OBSOLETE 	      sprintf (buf, "writing register number %d", regno);
// OBSOLETE 	      perror_with_name (buf);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       else if (regno == SXIP_REGNUM)
// OBSOLETE 	ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		(PTRACE_ARG3_TYPE) SXIP_OFFSET, read_register (regno));
// OBSOLETE       else if (regno == SNIP_REGNUM)
// OBSOLETE 	ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		(PTRACE_ARG3_TYPE) SNIP_OFFSET, read_register (regno));
// OBSOLETE       else if (regno == SFIP_REGNUM)
// OBSOLETE 	ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		(PTRACE_ARG3_TYPE) SFIP_OFFSET, read_register (regno));
// OBSOLETE       else
// OBSOLETE 	printf_unfiltered ("Bad register number for store_inferior routine\n");
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       for (regno = 0; regno < PC_REGNUM; regno++)
// OBSOLETE 	{
// OBSOLETE 	  /*      regaddr = register_addr (regno, offset); */
// OBSOLETE 	  errno = 0;
// OBSOLETE 	  regaddr = offset + regno * sizeof (int);
// OBSOLETE 	  ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		  (PTRACE_ARG3_TYPE) regaddr, read_register (regno));
// OBSOLETE 	  if (errno != 0)
// OBSOLETE 	    {
// OBSOLETE 	      sprintf (buf, "writing register number %d", regno);
// OBSOLETE 	      perror_with_name (buf);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 	      (PTRACE_ARG3_TYPE) SXIP_OFFSET, read_register (SXIP_REGNUM));
// OBSOLETE       ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 	      (PTRACE_ARG3_TYPE) SNIP_OFFSET, read_register (SNIP_REGNUM));
// OBSOLETE       ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 	      (PTRACE_ARG3_TYPE) SFIP_OFFSET, read_register (SFIP_REGNUM));
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* blockend is the address of the end of the user structure */
// OBSOLETE m88k_register_u_addr (int blockend, int regnum)
// OBSOLETE {
// OBSOLETE   struct USER u;
// OBSOLETE   int ustart = blockend - sizeof (struct USER);
// OBSOLETE   switch (regnum)
// OBSOLETE     {
// OBSOLETE     case 0:
// OBSOLETE     case 1:
// OBSOLETE     case 2:
// OBSOLETE     case 3:
// OBSOLETE     case 4:
// OBSOLETE     case 5:
// OBSOLETE     case 6:
// OBSOLETE     case 7:
// OBSOLETE     case 8:
// OBSOLETE     case 9:
// OBSOLETE     case 10:
// OBSOLETE     case 11:
// OBSOLETE     case 12:
// OBSOLETE     case 13:
// OBSOLETE     case 14:
// OBSOLETE     case 15:
// OBSOLETE     case 16:
// OBSOLETE     case 17:
// OBSOLETE     case 18:
// OBSOLETE     case 19:
// OBSOLETE     case 20:
// OBSOLETE     case 21:
// OBSOLETE     case 22:
// OBSOLETE     case 23:
// OBSOLETE     case 24:
// OBSOLETE     case 25:
// OBSOLETE     case 26:
// OBSOLETE     case 27:
// OBSOLETE     case 28:
// OBSOLETE     case 29:
// OBSOLETE     case 30:
// OBSOLETE     case 31:
// OBSOLETE       return (ustart + ((int) &u.pt_r0 - (int) &u) + REGISTER_SIZE * regnum);
// OBSOLETE     case PSR_REGNUM:
// OBSOLETE       return (ustart + ((int) &u.pt_psr - (int) &u));
// OBSOLETE     case FPSR_REGNUM:
// OBSOLETE       return (ustart + ((int) &u.pt_fpsr - (int) &u));
// OBSOLETE     case FPCR_REGNUM:
// OBSOLETE       return (ustart + ((int) &u.pt_fpcr - (int) &u));
// OBSOLETE     case SXIP_REGNUM:
// OBSOLETE       return (ustart + SXIP_OFFSET);
// OBSOLETE     case SNIP_REGNUM:
// OBSOLETE       return (ustart + SNIP_OFFSET);
// OBSOLETE     case SFIP_REGNUM:
// OBSOLETE       return (ustart + SFIP_OFFSET);
// OBSOLETE     default:
// OBSOLETE       if (regnum < NUM_REGS)
// OBSOLETE 	/* The register is one of those which is not defined...
// OBSOLETE 	   give it zero */
// OBSOLETE 	return (ustart + ((int) &u.pt_r0 - (int) &u));
// OBSOLETE       else
// OBSOLETE 	return (blockend + REGISTER_SIZE * regnum);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #ifdef USE_PROC_FS
// OBSOLETE 
// OBSOLETE #include <sys/procfs.h>
// OBSOLETE 
// OBSOLETE /* Prototypes for supply_gregset etc. */
// OBSOLETE #include "gregset.h"
// OBSOLETE 
// OBSOLETE /*  Given a pointer to a general register set in /proc format (gregset_t *),
// OBSOLETE    unpack the register contents and supply them as gdb's idea of the current
// OBSOLETE    register values. */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE supply_gregset (gregset_t *gregsetp)
// OBSOLETE {
// OBSOLETE   register int regi;
// OBSOLETE   register greg_t *regp = (greg_t *) gregsetp;
// OBSOLETE 
// OBSOLETE   for (regi = 0; regi <= SP_REGNUM; regi++)
// OBSOLETE     supply_register (regi, (char *) (regp + regi));
// OBSOLETE 
// OBSOLETE   supply_register (SXIP_REGNUM, (char *) (regp + R_XIP));
// OBSOLETE   supply_register (SNIP_REGNUM, (char *) (regp + R_NIP));
// OBSOLETE   supply_register (SFIP_REGNUM, (char *) (regp + R_FIP));
// OBSOLETE   supply_register (PSR_REGNUM, (char *) (regp + R_PSR));
// OBSOLETE   supply_register (FPSR_REGNUM, (char *) (regp + R_FPSR));
// OBSOLETE   supply_register (FPCR_REGNUM, (char *) (regp + R_FPCR));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fill_gregset (gregset_t *gregsetp, int regno)
// OBSOLETE {
// OBSOLETE   int regi;
// OBSOLETE   register greg_t *regp = (greg_t *) gregsetp;
// OBSOLETE 
// OBSOLETE   for (regi = 0; regi <= R_R31; regi++)
// OBSOLETE     if ((regno == -1) || (regno == regi))
// OBSOLETE       *(regp + regi) = *(int *) &registers[REGISTER_BYTE (regi)];
// OBSOLETE 
// OBSOLETE   if ((regno == -1) || (regno == SXIP_REGNUM))
// OBSOLETE     *(regp + R_XIP) = *(int *) &registers[REGISTER_BYTE (SXIP_REGNUM)];
// OBSOLETE   if ((regno == -1) || (regno == SNIP_REGNUM))
// OBSOLETE     *(regp + R_NIP) = *(int *) &registers[REGISTER_BYTE (SNIP_REGNUM)];
// OBSOLETE   if ((regno == -1) || (regno == SFIP_REGNUM))
// OBSOLETE     *(regp + R_FIP) = *(int *) &registers[REGISTER_BYTE (SFIP_REGNUM)];
// OBSOLETE   if ((regno == -1) || (regno == PSR_REGNUM))
// OBSOLETE     *(regp + R_PSR) = *(int *) &registers[REGISTER_BYTE (PSR_REGNUM)];
// OBSOLETE   if ((regno == -1) || (regno == FPSR_REGNUM))
// OBSOLETE     *(regp + R_FPSR) = *(int *) &registers[REGISTER_BYTE (FPSR_REGNUM)];
// OBSOLETE   if ((regno == -1) || (regno == FPCR_REGNUM))
// OBSOLETE     *(regp + R_FPCR) = *(int *) &registers[REGISTER_BYTE (FPCR_REGNUM)];
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #endif /* USE_PROC_FS */
