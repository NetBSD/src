/* Print National Semiconductor 32000 instructions.
   Copyright 1986, 88, 91, 92, 94, 95, 1998 Free Software Foundation, Inc.

This file is part of opcodes library.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#include "bfd.h"
#include "sysdep.h"
#include "dis-asm.h"
#if !defined(const) && !defined(__STDC__)
#define const
#endif
#include "opcode/vax.h"

static disassemble_info *dis_info;

/*
 * Hacks to get it to compile <= READ THESE AS FIXES NEEDED
 */
#define CORE_ADDR unsigned long
#define INVALID_FLOAT(val, size) invalid_float((char *)val, size)
#define	ARG_LEN	128
struct varg
{
  char arg_str[ARG_LEN];
  int arg_flags;
  int arg_address;
};

#if 0
static int invalid_float PARAMS ((char *, int));
#endif

static int read_memory_integer(addr, nr)
     unsigned char *addr;
     int nr;
{
  long val;
  switch (nr)
    {
      case 1: val = *(char *) addr; break;
      case 2: val = (((char *) addr)[1] <<  8) | addr[0]; break;
      case 4: val = (((char *) addr)[3] << 24)
		| (addr[2] << 16) | (addr[1] << 8) | (addr[0] << 0); break;
    }
  return val;
}

/* VAX instructions are never longer than this.  */
#define MAXARGS	6
#define MAXLEN	(2+MAXARGS*6)

#include <setjmp.h>

struct private
{
  /* Points to first byte not fetched.  */
  bfd_byte *max_fetched;
  bfd_byte the_buffer[MAXLEN];
  bfd_vma insn_start;
  jmp_buf bailout;
};


/* Make sure that bytes from INFO->PRIVATE_DATA->BUFFER (inclusive)
   to ADDR (exclusive) are valid.  Returns 1 for success, longjmps
   on error.  */
#define FETCH_DATA(info, addr) \
  ((addr) <= ((struct private *)(info->private_data))->max_fetched \
   ? 1 : fetch_data ((info), (addr)))

static int
fetch_data (info, addr)
     struct disassemble_info *info;
     bfd_byte *addr;
{
  int status;
  struct private *priv = (struct private *)info->private_data;
  bfd_vma start = priv->insn_start + (priv->max_fetched - priv->the_buffer);

  status = (*info->read_memory_func) (start,
				      priv->max_fetched,
				      addr - priv->max_fetched,
				      info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, start, info);
      longjmp (priv->bailout, 1);
    }
  else
    priv->max_fetched = addr;
  return 1;
}

/* Number of elements in the opcode table.  */
#define NOPCODES (sizeof vax_opcodes / sizeof vax_opcodes[0])

static const char * const vax_regs[16] = 
{
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "ap", "fp", "sp", "pc"
};
static const char * const vax_literal_floats[64] = 
{
   "0.5",  "0.5625", "0.625", "0.6875", "0.75",  "0.8125", "0.875",  "0.9375",
   "1.0",  "1.125",  "1.25",  "1.375",  "1.5",   "1.625",  "1.75",   "1.875",
   "2.0",  "2.25",   "2.5",   "2.75",   "3.0",   "3.25",   "3.5",    "3.75",
   "4.0",  "4.5",    "5.0",   "5.5",    "6.0",   "6.5",    "7.0",    "7.5",
   "8.0",  "9.0",   "10.0",  "11.0",   "12.0",  "13.0",   "14.0",   "15.0",
  "16.0", "18.0",   "20.0",  "22.0",   "24.0",  "26.0",   "28.0",   "30.0",
  "32.0", "36.0",   "40.0",  "44.0",   "48.0",  "52.0",   "56.0",   "60.0",
  "64.0", "72.0",   "80.0",  "88.0",   "96.0", "104.0",  "112.0",  "120.0",

};

static const int vax_disp_masks[5] = { 0, 0xff, 0xffff, 0xffffff, 0xffffffff };

#define	ARG_BYTE	0x0001
#define	ARG_WORD	0x0002
#define	ARG_LONGWORD	0x0004
#define	ARG_QUADWORD	0x0008
#define	ARG_OCTAWORD	0x0010
#define	ARG_F_FLOAT	0x0104
#define	ARG_D_FLOAT	0x0208
#define	ARG_G_FLOAT	0x0308
#define	ARG_H_FLOAT	0x0410
#define	ARG_DEFERRED	0x1000
#define	ARG_ADDRESS	0x2000

#define	ARG_FLOAT(f)	(((f) >> 8) & 0x04)
#define	ARG_SIZE(f)	((f) & 0xFF)

/* Print the VAX instruction at address MEMADDR in debugged memory,
   on STREAM.  Returns length of the instruction, in bytes.  */

int
print_insn_vax (memaddr, info)
     bfd_vma memaddr;
     disassemble_info *info;
{
  register const char *ap;
  unsigned short first_word;
  int aoffset = 1;	/* bytes into instruction */
  struct varg args[MAXARGS+1], *arg;
  int argcnt = 0;
  struct private priv;
  bfd_byte *buffer = priv.the_buffer;
  const struct vot *vot;

  dis_info = info;

  info->private_data = (PTR) &priv;
  priv.max_fetched = priv.the_buffer;
  priv.insn_start = memaddr;
  if (setjmp (priv.bailout) != 0)
    /* Error return.  */
    return -1;

#ifdef notyet
  if ((*dis_info->symbol_at_address_func)(memaddr, info))
    {
      FETCH_DATA(info, buffer + 2);
      (*dis_info->fprintf_func)(dis_info->stream,
				"\t.word\t0x%02x%02x",
				buffer[1], buffer[0]);
      return 2;
    }
#endif
  /* Look for 8bit opcodes first. Otherwise, fetching two bytes could take
   * us over the end of accessible data unnecessarilly
   */
  FETCH_DATA(info, buffer + 1);
  for (vot = votstrs; vot->vot_name[0] != '\0'; vot++)
    if ((buffer[0] == vot->vot_detail.vot_code))
      break;
  if (vot->vot_name == NULL) {
    /* Maybe it is 16 bits big */
    FETCH_DATA(info, buffer + 2);
    first_word = read_memory_integer(buffer, 2);

    for (vot = votstrs; vot->vot_name[0] != '\0'; vot++)
      if (first_word == vot->vot_detail.vot_code)
	break;

    /* Handle undefined instructions.  */
    if (vot->vot_name[0] == '\0')
      {
	(*dis_info->fprintf_func)(dis_info->stream, "%02x", buffer[0]);
	return 1;
      }
    aoffset = 2;
  }

  for (ap = vot->vot_detail.args, arg = args; *ap; ap += 2, arg++, argcnt++)
    {
      char *cp = arg->arg_str;
      arg->arg_flags = 0;
      cp[0] = '\0';
      switch (ap[1])
	{
	  case 'o':	/* octaword */	arg->arg_flags = ARG_OCTAWORD; break;
	  case 'q':	/* quadword */	arg->arg_flags = ARG_QUADWORD; break;
	  case 'l':	/* longword */	arg->arg_flags = ARG_LONGWORD; break;
	  case 'w':	/* word */	arg->arg_flags = ARG_WORD; break;
	  case 'b':	/* byte */	arg->arg_flags = ARG_BYTE; break;
	  case 'f':	/* f-float */	arg->arg_flags = ARG_F_FLOAT; break;
	  case 'd':	/* d-float */	arg->arg_flags = ARG_D_FLOAT; break;
	  case 'g':	/* g-float */	arg->arg_flags = ARG_G_FLOAT; break;
	  case 'h':	/* h-float */	arg->arg_flags = ARG_H_FLOAT; break;
	}
      switch (ap[0])
	{
	  case 'b':	/* branch displacement */
	    {
	      int displacement;
	      FETCH_DATA(info, buffer + aoffset + ARG_SIZE(arg->arg_flags));
	      displacement = read_memory_integer(buffer+aoffset, ARG_SIZE(arg->arg_flags));
	      aoffset += ARG_SIZE(arg->arg_flags);
	      arg->arg_flags |= ARG_ADDRESS;
	      arg->arg_address = priv.insn_start + aoffset + displacement;
	      break;
	    }
		
	  case 'v':	/* bit base */
	  case 'a':	/* address of */
	  case 'm':	/* modify */
	  case 'r':	/* read */
	  case 'w':	/* write */
	    {
	      int index_reg_no = -1;
	      int reg_no, mode;
	      FETCH_DATA(info, buffer + aoffset + 1);
	      reg_no = buffer[aoffset] & 0x0f;
	      mode = buffer[aoffset] >> 4;
	      aoffset++;
	     retry:
	      switch (mode)
		{
		  case 4:				/* index mode */
		    index_reg_no = reg_no;
		    FETCH_DATA(info, buffer + aoffset + 1);
		    reg_no = buffer[aoffset] & 0x0f;
		    mode = buffer[aoffset] >> 4;
		    aoffset++;
		    goto retry;
		  case 0: case 1: case 2: case 3:	/* short literal */
		    if (!ARG_FLOAT(arg->arg_flags))
		      {
			cp += sprintf(cp, "$%d", buffer[aoffset-1]);
		      }
		    else
		      {
			*cp++ = '$';
			strcpy(cp, vax_literal_floats[buffer[aoffset-1]]);
		      }
		    break;
		  case 5:				/* register mode */
		    cp += sprintf(cp, "%s", vax_regs[reg_no]);
		    break;
		  case 6:				/* register deferred */
		    cp += sprintf(cp, "(%s)", vax_regs[reg_no]);
		    break;
		  case 7:				/* auto decrement */
		    cp += sprintf(cp, "-(%s)", vax_regs[reg_no]);
		    break;
		  case 8:				/* auto increment */
		    if (reg_no == 15)
		      {
			FETCH_DATA(info, buffer + aoffset + ARG_SIZE(arg->arg_flags));
			if (!ARG_FLOAT(arg->arg_flags))
			  {
			    int i;
			    cp += sprintf(cp, "$0x");
			    for (i = ARG_SIZE(arg->arg_flags) - 1; i >= 0; i--)
				cp += sprintf(cp, "%02x", buffer[aoffset + i]);
			  }
			else
			  {
			    int i;
			    cp += sprintf(cp, "$0x");
			    for (i = ARG_SIZE(arg->arg_flags) - 1; i >= 0; i--)
				cp += sprintf(cp, "%02x", buffer[aoffset + i]);
			}
			aoffset += ARG_SIZE(arg->arg_flags);
		    } else {
			cp += sprintf(cp, "(%s)+", vax_regs[reg_no]);
		    }
		    break;
		  case 9:				/* auto incr deferred */
		    {
		      if (reg_no == 15)
		        {
			  FETCH_DATA(info, buffer + aoffset + 4);
			  arg->arg_flags |= ARG_ADDRESS;
			  arg->arg_address = read_memory_integer(buffer + aoffset, 4);
			  strcpy(cp, "*");
			  aoffset += 4;
			}
		      else
			{
			  cp += sprintf(cp, "*(%s)+", vax_regs[reg_no]);
			}
		      break;
		    }
		  case 10: case 11:			/* byte relative */
		  case 12: case 13:			/* word relative */
		  case 14: case 15:			/* longword relative */
		    {
		      int n = 1 << ((mode - 10) >> 1);
		      int displacement;
		      FETCH_DATA(info, buffer + aoffset + n);
		      displacement = read_memory_integer(buffer + aoffset, n);
		      if (mode & 1)
			{
			  *cp++ = '*';
			  arg->arg_flags |= ARG_DEFERRED;
			}
		      aoffset += n;
		      if (reg_no == 15)
			{
		          arg->arg_flags |= ARG_ADDRESS;
			  arg->arg_address = priv.insn_start + aoffset + displacement;
			  *cp++ = '\0';
			}
		      else
			{
			  cp += sprintf(cp, "%d(%s)", displacement, vax_regs[reg_no]);
			}
		      break;
		    }
	        }
	      if (index_reg_no != -1)
		sprintf(cp, "[%s]", vax_regs[index_reg_no]);
	    }
	}
    }

  (*dis_info->fprintf_func)(dis_info->stream, "\t%s", vot->vot_name);

  for (arg = args; argcnt-- > 0; arg++)
    {
      if (arg == args)
	{
	  (*dis_info->fprintf_func)(dis_info->stream, "\t");
	}
      else
	{
	  (*dis_info->fprintf_func)(dis_info->stream, ", ");
	}
      (*dis_info->fprintf_func)(dis_info->stream, arg->arg_str);
      if (arg->arg_flags & ARG_ADDRESS)
	{
	  (*dis_info->print_address_func) (arg->arg_address, dis_info);
	}
    }
  return aoffset;
}

#if 0
#if 0 /* a version that should work on vax f's&d's on any machine */
static int
invalid_float (p, len)
     register char *p;
     register int len;
{
  register int val;

  if ( len == 4 )
    val = (bit_extract(p, 23, 8)/*exponent*/ == 0xff
	   || (bit_extract(p, 23, 8)/*exponent*/ == 0 &&
	       bit_extract(p, 0, 23)/*mantisa*/ != 0));
  else if ( len == 8 )
    val = (bit_extract(p, 52, 11)/*exponent*/ == 0x7ff
	   || (bit_extract(p, 52, 11)/*exponent*/ == 0
	       && (bit_extract(p, 0, 32)/*low mantisa*/ != 0
		   || bit_extract(p, 32, 20)/*high mantisa*/ != 0)));
  else
    val = 1;
  return (val);
}
#else

/* assumes the bytes have been swapped to local order */
typedef union { double d;
		float f;
		struct { unsigned m:23, e:8, :1;} sf;
		struct { unsigned lm; unsigned m:20, e:11, :1;} sd;
	      } float_type_u;

static int
invalid_float (p, len)
     register float_type_u *p;
     register int len;
{
  register int val;
  if ( len == sizeof (float) )
    val = (p->sf.e == 0xff
	   || (p->sf.e == 0 && p->sf.m != 0));
  else if ( len == sizeof (double) )
    val = (p->sd.e == 0x7ff
	   || (p->sd.e == 0 && (p->sd.m != 0 || p->sd.lm != 0)));
  else
    val = 1;
  return (val);
}
#endif
#endif
