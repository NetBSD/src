// OBSOLETE /* Intel 386 native support.
// OBSOLETE    Copyright 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1996, 1998, 1999,
// OBSOLETE    2000, 2001 Free Software Foundation, Inc.
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
// OBSOLETE #include "language.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE #ifdef USG
// OBSOLETE #include <sys/types.h>
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #include <sys/param.h>
// OBSOLETE #include <sys/dir.h>
// OBSOLETE #include <signal.h>
// OBSOLETE #include <sys/user.h>
// OBSOLETE #include <sys/ioctl.h>
// OBSOLETE #include <fcntl.h>
// OBSOLETE 
// OBSOLETE #include <sys/file.h>
// OBSOLETE #include "gdb_stat.h"
// OBSOLETE 
// OBSOLETE #include <stddef.h>
// OBSOLETE #include <sys/ptrace.h>
// OBSOLETE 
// OBSOLETE /* Does AIX define this in <errno.h>?  */
// OBSOLETE extern int errno;
// OBSOLETE 
// OBSOLETE #ifdef HAVE_SYS_REG_H
// OBSOLETE #include <sys/reg.h>
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #include "floatformat.h"
// OBSOLETE 
// OBSOLETE #include "target.h"
// OBSOLETE 
// OBSOLETE static void fetch_core_registers (char *, unsigned, int, CORE_ADDR);
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* this table must line up with REGISTER_NAMES in tm-i386v.h */
// OBSOLETE /* symbols like 'EAX' come from <sys/reg.h> */
// OBSOLETE static int regmap[] =
// OBSOLETE {
// OBSOLETE   EAX, ECX, EDX, EBX,
// OBSOLETE   USP, EBP, ESI, EDI,
// OBSOLETE   EIP, EFL, CS, SS,
// OBSOLETE   DS, ES, FS, GS,
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* blockend is the value of u.u_ar0, and points to the
// OBSOLETE  * place where GS is stored
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE i386_register_u_addr (int blockend, int regnum)
// OBSOLETE {
// OBSOLETE #if 0
// OBSOLETE   /* this will be needed if fp registers are reinstated */
// OBSOLETE   /* for now, you can look at them with 'info float'
// OBSOLETE    * sys5 wont let you change them with ptrace anyway
// OBSOLETE    */
// OBSOLETE   if (regnum >= FP0_REGNUM && regnum <= FP7_REGNUM)
// OBSOLETE     {
// OBSOLETE       int ubase, fpstate;
// OBSOLETE       struct user u;
// OBSOLETE       ubase = blockend + 4 * (SS + 1) - KSTKSZ;
// OBSOLETE       fpstate = ubase + ((char *) &u.u_fpstate - (char *) &u);
// OBSOLETE       return (fpstate + 0x1c + 10 * (regnum - FP0_REGNUM));
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE #endif
// OBSOLETE     return (blockend + 4 * regmap[regnum]);
// OBSOLETE 
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* The code below only work on the aix ps/2 (i386-ibm-aix) -
// OBSOLETE  * mtranle@paris - Sat Apr 11 10:34:12 1992
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE struct env387
// OBSOLETE {
// OBSOLETE   unsigned short control;
// OBSOLETE   unsigned short r0;
// OBSOLETE   unsigned short status;
// OBSOLETE   unsigned short r1;
// OBSOLETE   unsigned short tag;
// OBSOLETE   unsigned short r2;
// OBSOLETE   unsigned long eip;
// OBSOLETE   unsigned short code_seg;
// OBSOLETE   unsigned short opcode;
// OBSOLETE   unsigned long operand;
// OBSOLETE   unsigned short operand_seg;
// OBSOLETE   unsigned short r3;
// OBSOLETE   unsigned char regs[8][10];
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static
// OBSOLETE print_387_status (unsigned short status, struct env387 *ep)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE   int bothstatus;
// OBSOLETE   int top;
// OBSOLETE   int fpreg;
// OBSOLETE   unsigned char *p;
// OBSOLETE 
// OBSOLETE   bothstatus = ((status != 0) && (ep->status != 0));
// OBSOLETE   if (status != 0)
// OBSOLETE     {
// OBSOLETE       if (bothstatus)
// OBSOLETE 	printf_unfiltered ("u: ");
// OBSOLETE       print_387_status_word (status);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (ep->status != 0)
// OBSOLETE     {
// OBSOLETE       if (bothstatus)
// OBSOLETE 	printf_unfiltered ("e: ");
// OBSOLETE       print_387_status_word (ep->status);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   print_387_control_word (ep->control);
// OBSOLETE   printf_unfiltered ("last exception: ");
// OBSOLETE   printf_unfiltered ("opcode %s; ", local_hex_string (ep->opcode));
// OBSOLETE   printf_unfiltered ("pc %s:", local_hex_string (ep->code_seg));
// OBSOLETE   printf_unfiltered ("%s; ", local_hex_string (ep->eip));
// OBSOLETE   printf_unfiltered ("operand %s", local_hex_string (ep->operand_seg));
// OBSOLETE   printf_unfiltered (":%s\n", local_hex_string (ep->operand));
// OBSOLETE 
// OBSOLETE   top = ((ep->status >> 11) & 7);
// OBSOLETE 
// OBSOLETE   printf_unfiltered ("regno  tag  msb              lsb  value\n");
// OBSOLETE   for (fpreg = 7; fpreg >= 0; fpreg--)
// OBSOLETE     {
// OBSOLETE       double val;
// OBSOLETE 
// OBSOLETE       printf_unfiltered ("%s %d: ", fpreg == top ? "=>" : "  ", fpreg);
// OBSOLETE 
// OBSOLETE       switch ((ep->tag >> ((7 - fpreg) * 2)) & 3)
// OBSOLETE 	{
// OBSOLETE 	case 0:
// OBSOLETE 	  printf_unfiltered ("valid ");
// OBSOLETE 	  break;
// OBSOLETE 	case 1:
// OBSOLETE 	  printf_unfiltered ("zero  ");
// OBSOLETE 	  break;
// OBSOLETE 	case 2:
// OBSOLETE 	  printf_unfiltered ("trap  ");
// OBSOLETE 	  break;
// OBSOLETE 	case 3:
// OBSOLETE 	  printf_unfiltered ("empty ");
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       for (i = 9; i >= 0; i--)
// OBSOLETE 	printf_unfiltered ("%02x", ep->regs[fpreg][i]);
// OBSOLETE 
// OBSOLETE       i387_to_double ((char *) ep->regs[fpreg], (char *) &val);
// OBSOLETE       printf_unfiltered ("  %#g\n", val);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct env387 core_env387;
// OBSOLETE 
// OBSOLETE void
// OBSOLETE i386_float_info (void)
// OBSOLETE {
// OBSOLETE   struct env387 fps;
// OBSOLETE   int fpsaved = 0;
// OBSOLETE   /* We need to reverse the order of the registers.  Apparently AIX stores
// OBSOLETE      the highest-numbered ones first.  */
// OBSOLETE   struct env387 fps_fixed;
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   if (! ptid_equal (inferior_ptid, null_ptid))
// OBSOLETE     {
// OBSOLETE       char buf[10];
// OBSOLETE       unsigned short status;
// OBSOLETE 
// OBSOLETE       ptrace (PT_READ_FPR, PIDGET (inferior_ptid), buf,
// OBSOLETE               offsetof (struct env387, status));
// OBSOLETE       memcpy (&status, buf, sizeof (status));
// OBSOLETE       fpsaved = status;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       if ((fpsaved = core_env387.status) != 0)
// OBSOLETE 	memcpy (&fps, &core_env387, sizeof (fps));
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (fpsaved == 0)
// OBSOLETE     {
// OBSOLETE       printf_unfiltered ("no floating point status saved\n");
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (! ptid_equal (inferior_ptid, null_ptid))
// OBSOLETE     {
// OBSOLETE       int offset;
// OBSOLETE       for (offset = 0; offset < sizeof (fps); offset += 10)
// OBSOLETE 	{
// OBSOLETE 	  char buf[10];
// OBSOLETE 	  ptrace (PT_READ_FPR, PIDGET (inferior_ptid), buf, offset);
// OBSOLETE 	  memcpy ((char *) &fps.control + offset, buf,
// OBSOLETE 		  MIN (10, sizeof (fps) - offset));
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   fps_fixed = fps;
// OBSOLETE   for (i = 0; i < 8; ++i)
// OBSOLETE     memcpy (fps_fixed.regs[i], fps.regs[7 - i], 10);
// OBSOLETE   print_387_status (0, &fps_fixed);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Fetch one register.  */
// OBSOLETE static void
// OBSOLETE fetch_register (int regno)
// OBSOLETE {
// OBSOLETE   char buf[MAX_REGISTER_RAW_SIZE];
// OBSOLETE   if (regno < FP0_REGNUM)
// OBSOLETE     *(int *) buf = ptrace (PT_READ_GPR, PIDGET (inferior_ptid),
// OBSOLETE 			   PT_REG (regmap[regno]), 0, 0);
// OBSOLETE   else
// OBSOLETE     ptrace (PT_READ_FPR, PIDGET (inferior_ptid), buf,
// OBSOLETE 	    (regno - FP0_REGNUM) * 10 + offsetof (struct env387, regs));
// OBSOLETE   supply_register (regno, buf);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fetch_inferior_registers (int regno)
// OBSOLETE {
// OBSOLETE   if (regno < 0)
// OBSOLETE     for (regno = 0; regno < NUM_REGS; regno++)
// OBSOLETE       fetch_register (regno);
// OBSOLETE   else
// OBSOLETE     fetch_register (regno);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* store one register */
// OBSOLETE static void
// OBSOLETE store_register (int regno)
// OBSOLETE {
// OBSOLETE   char buf[80];
// OBSOLETE   errno = 0;
// OBSOLETE   if (regno < FP0_REGNUM)
// OBSOLETE     ptrace (PT_WRITE_GPR, PIDGET (inferior_ptid), PT_REG (regmap[regno]),
// OBSOLETE 	    *(int *) &registers[REGISTER_BYTE (regno)], 0);
// OBSOLETE   else
// OBSOLETE     ptrace (PT_WRITE_FPR, PIDGET (inferior_ptid),
// OBSOLETE             &registers[REGISTER_BYTE (regno)],
// OBSOLETE 	    (regno - FP0_REGNUM) * 10 + offsetof (struct env387, regs));
// OBSOLETE 
// OBSOLETE   if (errno != 0)
// OBSOLETE     {
// OBSOLETE       sprintf (buf, "writing register number %d", regno);
// OBSOLETE       perror_with_name (buf);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Store our register values back into the inferior.
// OBSOLETE    If REGNO is -1, do this for all registers.
// OBSOLETE    Otherwise, REGNO specifies which register (so we can save time).  */
// OBSOLETE void
// OBSOLETE store_inferior_registers (int regno)
// OBSOLETE {
// OBSOLETE   if (regno < 0)
// OBSOLETE     for (regno = 0; regno < NUM_REGS; regno++)
// OBSOLETE       store_register (regno);
// OBSOLETE   else
// OBSOLETE     store_register (regno);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #ifndef CD_AX			/* defined in sys/i386/coredump.h */
// OBSOLETE #define CD_AX	0
// OBSOLETE #define CD_BX	1
// OBSOLETE #define CD_CX	2
// OBSOLETE #define CD_DX	3
// OBSOLETE #define CD_SI	4
// OBSOLETE #define CD_DI	5
// OBSOLETE #define CD_BP	6
// OBSOLETE #define CD_SP	7
// OBSOLETE #define CD_FL	8
// OBSOLETE #define CD_IP	9
// OBSOLETE #define CD_CS	10
// OBSOLETE #define CD_DS	11
// OBSOLETE #define CD_ES	12
// OBSOLETE #define CD_FS	13
// OBSOLETE #define CD_GS	14
// OBSOLETE #define CD_SS	15
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * The order here in core_regmap[] has to be the same as in 
// OBSOLETE  * regmap[] above.
// OBSOLETE  */
// OBSOLETE static int core_regmap[] =
// OBSOLETE {
// OBSOLETE   CD_AX, CD_CX, CD_DX, CD_BX,
// OBSOLETE   CD_SP, CD_BP, CD_SI, CD_DI,
// OBSOLETE   CD_IP, CD_FL, CD_CS, CD_SS,
// OBSOLETE   CD_DS, CD_ES, CD_FS, CD_GS,
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Provide registers to GDB from a core file.
// OBSOLETE 
// OBSOLETE    CORE_REG_SECT points to an array of bytes, which were obtained from
// OBSOLETE    a core file which BFD thinks might contain register contents. 
// OBSOLETE    CORE_REG_SIZE is its size.
// OBSOLETE 
// OBSOLETE    WHICH says which register set corelow suspects this is:
// OBSOLETE      0 --- the general-purpose register set
// OBSOLETE      2 --- the floating-point register set
// OBSOLETE 
// OBSOLETE    REG_ADDR isn't used.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE fetch_core_registers (char *core_reg_sect, unsigned core_reg_size,
// OBSOLETE 		      int which, CORE_ADDR reg_addr)
// OBSOLETE {
// OBSOLETE 
// OBSOLETE   if (which == 0)
// OBSOLETE     {
// OBSOLETE       /* Integer registers */
// OBSOLETE 
// OBSOLETE #define cd_regs(n) ((int *)core_reg_sect)[n]
// OBSOLETE #define regs(n) *((int *) &registers[REGISTER_BYTE (n)])
// OBSOLETE 
// OBSOLETE       int i;
// OBSOLETE       for (i = 0; i < FP0_REGNUM; i++)
// OBSOLETE 	regs (i) = cd_regs (core_regmap[i]);
// OBSOLETE     }
// OBSOLETE   else if (which == 2)
// OBSOLETE     {
// OBSOLETE       /* Floating point registers */
// OBSOLETE 
// OBSOLETE       if (core_reg_size >= sizeof (core_env387))
// OBSOLETE 	memcpy (&core_env387, core_reg_sect, core_reg_size);
// OBSOLETE       else
// OBSOLETE 	fprintf_unfiltered (gdb_stderr, "Couldn't read float regs from core file\n");
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Register that we are able to handle i386aix core file formats.
// OBSOLETE    FIXME: is this really bfd_target_unknown_flavour? */
// OBSOLETE 
// OBSOLETE static struct core_fns i386aix_core_fns =
// OBSOLETE {
// OBSOLETE   bfd_target_unknown_flavour,		/* core_flavour */
// OBSOLETE   default_check_format,			/* check_format */
// OBSOLETE   default_core_sniffer,			/* core_sniffer */
// OBSOLETE   fetch_core_registers,			/* core_read_registers */
// OBSOLETE   NULL					/* next */
// OBSOLETE };
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_core_i386aix (void)
// OBSOLETE {
// OBSOLETE   add_core_fns (&i386aix_core_fns);
// OBSOLETE }
