// OBSOLETE /* Native support for Motorola 88k running Harris CX/UX.
// OBSOLETE    Copyright 1988, 1990, 1991, 1992, 1993, 1994, 1995, 1998, 1999, 2000,
// OBSOLETE    2001 Free Software Foundation, Inc.
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
// OBSOLETE 
// OBSOLETE #include <sys/types.h>
// OBSOLETE #include <sys/param.h>
// OBSOLETE #include <sys/dir.h>
// OBSOLETE #include <signal.h>
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include <sys/user.h>
// OBSOLETE 
// OBSOLETE #include "bfd.h"
// OBSOLETE #include "symfile.h"
// OBSOLETE #include "objfiles.h"
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "regcache.h"
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
// OBSOLETE #include <sys/ptrace.h>
// OBSOLETE 
// OBSOLETE /* CX/UX provides them already, but as word offsets instead of char offsets */
// OBSOLETE #define SXIP_OFFSET (PT_SXIP * 4)
// OBSOLETE #define SNIP_OFFSET (PT_SNIP * 4)
// OBSOLETE #define SFIP_OFFSET (PT_SFIP * 4)
// OBSOLETE #define PSR_OFFSET  (PT_PSR  * sizeof(int))
// OBSOLETE #define FPSR_OFFSET (PT_FPSR * sizeof(int))
// OBSOLETE #define FPCR_OFFSET (PT_FPCR * sizeof(int))
// OBSOLETE 
// OBSOLETE #define XREGADDR(r) (((char *)&u.pt_x0-(char *)&u) + \
// OBSOLETE                      ((r)-X0_REGNUM)*sizeof(X_REGISTER_RAW_TYPE))
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
// OBSOLETE   for (regno = 0; regno < PC_REGNUM; regno++)
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
// OBSOLETE   /* now load up registers 32-37; special pc registers */
// OBSOLETE   *(int *) &buf[0] = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 			     (PTRACE_ARG3_TYPE) PSR_OFFSET, 0);
// OBSOLETE   supply_register (PSR_REGNUM, buf);
// OBSOLETE   *(int *) &buf[0] = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 			     (PTRACE_ARG3_TYPE) FPSR_OFFSET, 0);
// OBSOLETE   supply_register (FPSR_REGNUM, buf);
// OBSOLETE   *(int *) &buf[0] = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 			     (PTRACE_ARG3_TYPE) FPCR_OFFSET, 0);
// OBSOLETE   supply_register (FPCR_REGNUM, buf);
// OBSOLETE   *(int *) &buf[0] = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 			     (PTRACE_ARG3_TYPE) SXIP_OFFSET, 0);
// OBSOLETE   supply_register (SXIP_REGNUM, buf);
// OBSOLETE   *(int *) &buf[0] = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 			     (PTRACE_ARG3_TYPE) SNIP_OFFSET, 0);
// OBSOLETE   supply_register (SNIP_REGNUM, buf);
// OBSOLETE   *(int *) &buf[0] = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 			     (PTRACE_ARG3_TYPE) SFIP_OFFSET, 0);
// OBSOLETE   supply_register (SFIP_REGNUM, buf);
// OBSOLETE 
// OBSOLETE   if (target_is_m88110)
// OBSOLETE     {
// OBSOLETE       for (regaddr = XREGADDR (X0_REGNUM), regno = X0_REGNUM;
// OBSOLETE 	   regno < NUM_REGS;
// OBSOLETE 	   regno++, regaddr += 16)
// OBSOLETE 	{
// OBSOLETE 	  X_REGISTER_RAW_TYPE xval;
// OBSOLETE 
// OBSOLETE 	  *(int *) &xval.w1 = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 				      (PTRACE_ARG3_TYPE) regaddr, 0);
// OBSOLETE 	  *(int *) &xval.w2 = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 				      (PTRACE_ARG3_TYPE) (regaddr + 4), 0);
// OBSOLETE 	  *(int *) &xval.w3 = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 				      (PTRACE_ARG3_TYPE) (regaddr + 8), 0);
// OBSOLETE 	  *(int *) &xval.w4 = ptrace (3, PIDGET (inferior_ptid),
// OBSOLETE 				      (PTRACE_ARG3_TYPE) (regaddr + 12), 0);
// OBSOLETE 	  supply_register (regno, (void *) &xval);
// OBSOLETE 	}
// OBSOLETE     }
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
// OBSOLETE       else if (regno == PSR_REGNUM)
// OBSOLETE 	ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		(PTRACE_ARG3_TYPE) PSR_OFFSET, read_register (regno));
// OBSOLETE       else if (regno == FPSR_REGNUM)
// OBSOLETE 	ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		(PTRACE_ARG3_TYPE) FPSR_OFFSET, read_register (regno));
// OBSOLETE       else if (regno == FPCR_REGNUM)
// OBSOLETE 	ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		(PTRACE_ARG3_TYPE) FPCR_OFFSET, read_register (regno));
// OBSOLETE       else if (regno == SXIP_REGNUM)
// OBSOLETE 	ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		(PTRACE_ARG3_TYPE) SXIP_OFFSET, read_register (regno));
// OBSOLETE       else if (regno == SNIP_REGNUM)
// OBSOLETE 	ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		(PTRACE_ARG3_TYPE) SNIP_OFFSET, read_register (regno));
// OBSOLETE       else if (regno == SFIP_REGNUM)
// OBSOLETE 	ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 		(PTRACE_ARG3_TYPE) SFIP_OFFSET, read_register (regno));
// OBSOLETE       else if (target_is_m88110 && regno < NUM_REGS)
// OBSOLETE 	{
// OBSOLETE 	  X_REGISTER_RAW_TYPE xval;
// OBSOLETE 
// OBSOLETE 	  read_register_bytes (REGISTER_BYTE (regno), (char *) &xval,
// OBSOLETE 			       sizeof (X_REGISTER_RAW_TYPE));
// OBSOLETE 	  regaddr = XREGADDR (regno);
// OBSOLETE 	  ptrace (6, PIDGET (inferior_ptid), (PTRACE_ARG3_TYPE) regaddr, xval.w1);
// OBSOLETE 	  ptrace (6, PIDGET (inferior_ptid), (PTRACE_ARG3_TYPE) regaddr + 4, xval.w2);
// OBSOLETE 	  ptrace (6, PIDGET (inferior_ptid), (PTRACE_ARG3_TYPE) regaddr + 8, xval.w3);
// OBSOLETE 	  ptrace (6, PIDGET (inferior_ptid), (PTRACE_ARG3_TYPE) regaddr + 12, xval.w4);
// OBSOLETE 	}
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
// OBSOLETE 	      (PTRACE_ARG3_TYPE) PSR_OFFSET, read_register (regno));
// OBSOLETE       ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 	      (PTRACE_ARG3_TYPE) FPSR_OFFSET, read_register (regno));
// OBSOLETE       ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 	      (PTRACE_ARG3_TYPE) FPCR_OFFSET, read_register (regno));
// OBSOLETE       ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 	      (PTRACE_ARG3_TYPE) SXIP_OFFSET, read_register (SXIP_REGNUM));
// OBSOLETE       ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 	      (PTRACE_ARG3_TYPE) SNIP_OFFSET, read_register (SNIP_REGNUM));
// OBSOLETE       ptrace (6, PIDGET (inferior_ptid),
// OBSOLETE 	      (PTRACE_ARG3_TYPE) SFIP_OFFSET, read_register (SFIP_REGNUM));
// OBSOLETE       if (target_is_m88110)
// OBSOLETE 	{
// OBSOLETE 	  for (regno = X0_REGNUM; regno < NUM_REGS; regno++)
// OBSOLETE 	    {
// OBSOLETE 	      X_REGISTER_RAW_TYPE xval;
// OBSOLETE 
// OBSOLETE 	      read_register_bytes (REGISTER_BYTE (regno), (char *) &xval,
// OBSOLETE 				   sizeof (X_REGISTER_RAW_TYPE));
// OBSOLETE 	      regaddr = XREGADDR (regno);
// OBSOLETE 	      ptrace (6, PIDGET (inferior_ptid), (PTRACE_ARG3_TYPE) regaddr, xval.w1);
// OBSOLETE 	      ptrace (6, PIDGET (inferior_ptid), (PTRACE_ARG3_TYPE) (regaddr + 4), xval.w2);
// OBSOLETE 	      ptrace (6, PIDGET (inferior_ptid), (PTRACE_ARG3_TYPE) (regaddr + 8), xval.w3);
// OBSOLETE 	      ptrace (6, PIDGET (inferior_ptid), (PTRACE_ARG3_TYPE) (regaddr + 12), xval.w4);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* blockend is the address of the end of the user structure */
// OBSOLETE 
// OBSOLETE m88k_register_u_addr (int blockend, int regnum)
// OBSOLETE {
// OBSOLETE   struct USER u;
// OBSOLETE   int ustart = blockend - sizeof (struct USER);
// OBSOLETE 
// OBSOLETE   if (regnum < PSR_REGNUM)
// OBSOLETE     return (ustart + ((int) &u.pt_r0 - (int) &u) +
// OBSOLETE 	    REGISTER_SIZE * regnum);
// OBSOLETE   else if (regnum == PSR_REGNUM)
// OBSOLETE     return (ustart + ((int) &u.pt_psr) - (int) &u);
// OBSOLETE   else if (regnum == FPSR_REGNUM)
// OBSOLETE     return (ustart + ((int) &u.pt_fpsr) - (int) &u);
// OBSOLETE   else if (regnum == FPCR_REGNUM)
// OBSOLETE     return (ustart + ((int) &u.pt_fpcr) - (int) &u);
// OBSOLETE   else if (regnum == SXIP_REGNUM)
// OBSOLETE     return (ustart + SXIP_OFFSET);
// OBSOLETE   else if (regnum == SNIP_REGNUM)
// OBSOLETE     return (ustart + SNIP_OFFSET);
// OBSOLETE   else if (regnum == SFIP_REGNUM)
// OBSOLETE     return (ustart + SFIP_OFFSET);
// OBSOLETE   else if (target_is_m88110)
// OBSOLETE     return (ustart + ((int) &u.pt_x0 - (int) &u) +	/* Must be X register */
// OBSOLETE 	    sizeof (u.pt_x0) * (regnum - X0_REGNUM));
// OBSOLETE   else
// OBSOLETE     return (blockend + REGISTER_SIZE * regnum);
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
// OBSOLETE 
// OBSOLETE /* This support adds the equivalent of adb's % command.  When
// OBSOLETE    the `add-shared-symbol-files' command is given, this routine scans 
// OBSOLETE    the dynamic linker's link map and reads the minimal symbols
// OBSOLETE    from each shared object file listed in the map. */
// OBSOLETE 
// OBSOLETE struct link_map
// OBSOLETE {
// OBSOLETE   unsigned long l_addr;		/* address at which object is mapped */
// OBSOLETE   char *l_name;			/* full name of loaded object */
// OBSOLETE   void *l_ld;			/* dynamic structure of object */
// OBSOLETE   struct link_map *l_next;	/* next link object */
// OBSOLETE   struct link_map *l_prev;	/* previous link object */
// OBSOLETE };
// OBSOLETE 
// OBSOLETE #define LINKS_MAP_POINTER "_ld_tail"
// OBSOLETE #define LIBC_FILE "/usr/lib/libc.so.1"
// OBSOLETE #define SHARED_OFFSET 0xf0001000
// OBSOLETE 
// OBSOLETE #ifndef PATH_MAX
// OBSOLETE #define PATH_MAX 1023		/* maximum size of path name on OS */
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE void
// OBSOLETE add_shared_symbol_files (void)
// OBSOLETE {
// OBSOLETE   void *desc;
// OBSOLETE   struct link_map *ld_map, *lm, lms;
// OBSOLETE   struct minimal_symbol *minsym;
// OBSOLETE   struct objfile *objfile;
// OBSOLETE   char *path_name;
// OBSOLETE 
// OBSOLETE   if (ptid_equal (inferior_ptid, null_ptid))
// OBSOLETE     {
// OBSOLETE       warning ("The program has not yet been started.");
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   objfile = symbol_file_add (LIBC_FILE, 0, NULL, 0, OBJF_READNOW);
// OBSOLETE   minsym = lookup_minimal_symbol (LINKS_MAP_POINTER, objfile);
// OBSOLETE 
// OBSOLETE   ld_map = (struct link_map *)
// OBSOLETE     read_memory_integer (((int) SYMBOL_VALUE_ADDRESS (minsym) + SHARED_OFFSET), 4);
// OBSOLETE   lm = ld_map;
// OBSOLETE   while (lm)
// OBSOLETE     {
// OBSOLETE       int local_errno = 0;
// OBSOLETE 
// OBSOLETE       read_memory ((CORE_ADDR) lm, (char *) &lms, sizeof (struct link_map));
// OBSOLETE       if (lms.l_name)
// OBSOLETE 	{
// OBSOLETE 	  if (target_read_string ((CORE_ADDR) lms.l_name, &path_name,
// OBSOLETE 				  PATH_MAX, &local_errno))
// OBSOLETE 	    {
// OBSOLETE 	      struct section_addr_info section_addrs;
// OBSOLETE 	      memset (&section_addrs, 0, sizeof (section_addrs));
// OBSOLETE 	      section_addrs.other[0].addr = lms.l_addr;
// OBSOLETE               section_addrs.other[0].name = ".text";
// OBSOLETE 	      symbol_file_add (path_name, 1, &section_addrs, 0, 0);
// OBSOLETE 	      xfree (path_name);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       /* traverse links in reverse order so that we get the
// OBSOLETE          the symbols the user actually gets. */
// OBSOLETE       lm = lms.l_prev;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Getting new symbols may change our opinion about what is
// OBSOLETE      frameless.  */
// OBSOLETE   reinit_frame_cache ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #if defined(_ES_MP)
// OBSOLETE 
// OBSOLETE #include <sys/regset.h>
// OBSOLETE 
// OBSOLETE unsigned int
// OBSOLETE m88k_harris_core_register_addr (int regno, int reg_ptr)
// OBSOLETE {
// OBSOLETE   unsigned int word_offset;
// OBSOLETE 
// OBSOLETE   switch (regno)
// OBSOLETE     {
// OBSOLETE     case PSR_REGNUM:
// OBSOLETE       word_offset = R_EPSR;
// OBSOLETE       break;
// OBSOLETE     case FPSR_REGNUM:
// OBSOLETE       word_offset = R_FPSR;
// OBSOLETE       break;
// OBSOLETE     case FPCR_REGNUM:
// OBSOLETE       word_offset = R_FPCR;
// OBSOLETE       break;
// OBSOLETE     case SXIP_REGNUM:
// OBSOLETE       word_offset = R_EXIP;
// OBSOLETE       break;
// OBSOLETE     case SNIP_REGNUM:
// OBSOLETE       word_offset = R_ENIP;
// OBSOLETE       break;
// OBSOLETE     case SFIP_REGNUM:
// OBSOLETE       word_offset = R_EFIP;
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE       if (regno <= FP_REGNUM)
// OBSOLETE 	word_offset = regno;
// OBSOLETE       else
// OBSOLETE 	word_offset = ((regno - X0_REGNUM) * 4);
// OBSOLETE     }
// OBSOLETE   return (word_offset * 4);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #endif /* _ES_MP */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_m88k_nat (void)
// OBSOLETE {
// OBSOLETE #ifdef _ES_MP
// OBSOLETE   /* Enable 88110 support, as we don't support the 88100 under ES/MP.  */
// OBSOLETE 
// OBSOLETE   target_is_m88110 = 1;
// OBSOLETE #elif defined(_CX_UX)
// OBSOLETE   /* Determine whether we're running on an 88100 or an 88110.  */
// OBSOLETE   target_is_m88110 = (sinfo (SYSMACHINE, 0) == SYS5800);
// OBSOLETE #endif /* _CX_UX */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #ifdef _ES_MP
// OBSOLETE /* Given a pointer to a general register set in /proc format (gregset_t *),
// OBSOLETE    unpack the register contents and supply them as gdb's idea of the current
// OBSOLETE    register values. */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE supply_gregset (gregset_t *gregsetp)
// OBSOLETE {
// OBSOLETE   register int regi;
// OBSOLETE   register greg_t *regp = (greg_t *) gregsetp;
// OBSOLETE 
// OBSOLETE   for (regi = 0; regi < R_R31; regi++)
// OBSOLETE     {
// OBSOLETE       supply_register (regi, (char *) (regp + regi));
// OBSOLETE     }
// OBSOLETE   supply_register (PSR_REGNUM, (char *) (regp + R_EPSR));
// OBSOLETE   supply_register (FPSR_REGNUM, (char *) (regp + R_FPSR));
// OBSOLETE   supply_register (FPCR_REGNUM, (char *) (regp + R_FPCR));
// OBSOLETE   supply_register (SXIP_REGNUM, (char *) (regp + R_EXIP));
// OBSOLETE   supply_register (SNIP_REGNUM, (char *) (regp + R_ENIP));
// OBSOLETE   supply_register (SFIP_REGNUM, (char *) (regp + R_EFIP));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Given a pointer to a floating point register set in /proc format
// OBSOLETE    (fpregset_t *), unpack the register contents and supply them as gdb's
// OBSOLETE    idea of the current floating point register values.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE supply_fpregset (fpregset_t *fpregsetp)
// OBSOLETE {
// OBSOLETE   register int regi;
// OBSOLETE   char *from;
// OBSOLETE 
// OBSOLETE   for (regi = FP0_REGNUM; regi <= FPLAST_REGNUM; regi++)
// OBSOLETE     {
// OBSOLETE       from = (char *) &((*fpregsetp)[regi - FP0_REGNUM]);
// OBSOLETE       supply_register (regi, from);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #endif /* _ES_MP */
// OBSOLETE 
// OBSOLETE #ifdef _CX_UX
// OBSOLETE 
// OBSOLETE #include <sys/regset.h>
// OBSOLETE 
// OBSOLETE unsigned int
// OBSOLETE m88k_harris_core_register_addr (int regno, int reg_ptr)
// OBSOLETE {
// OBSOLETE   unsigned int word_offset;
// OBSOLETE 
// OBSOLETE   switch (regno)
// OBSOLETE     {
// OBSOLETE     case PSR_REGNUM:
// OBSOLETE       word_offset = R_PSR;
// OBSOLETE       break;
// OBSOLETE     case FPSR_REGNUM:
// OBSOLETE       word_offset = R_FPSR;
// OBSOLETE       break;
// OBSOLETE     case FPCR_REGNUM:
// OBSOLETE       word_offset = R_FPCR;
// OBSOLETE       break;
// OBSOLETE     case SXIP_REGNUM:
// OBSOLETE       word_offset = R_XIP;
// OBSOLETE       break;
// OBSOLETE     case SNIP_REGNUM:
// OBSOLETE       word_offset = R_NIP;
// OBSOLETE       break;
// OBSOLETE     case SFIP_REGNUM:
// OBSOLETE       word_offset = R_FIP;
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE       if (regno <= FP_REGNUM)
// OBSOLETE 	word_offset = regno;
// OBSOLETE       else
// OBSOLETE 	word_offset = ((regno - X0_REGNUM) * 4) + R_X0;
// OBSOLETE     }
// OBSOLETE   return (word_offset * 4);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #endif /* _CX_UX */
