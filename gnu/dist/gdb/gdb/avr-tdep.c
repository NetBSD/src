/* Target-dependent code for Atmel AVR, for GDB.
   Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.

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

/* Contributed by Theodore A. Roth, troth@verinet.com */

/* Portions of this file were taken from the original gdb-4.18 patch developed
   by Denis Chertykov, denisc@overta.ru */

#include "defs.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "inferior.h"
#include "symfile.h"
#include "arch-utils.h"
#include "regcache.h"
#include "gdb_string.h"

/* AVR Background:

   (AVR micros are pure Harvard Architecture processors.)

   The AVR family of microcontrollers have three distinctly different memory
   spaces: flash, sram and eeprom. The flash is 16 bits wide and is used for
   the most part to store program instructions. The sram is 8 bits wide and is
   used for the stack and the heap. Some devices lack sram and some can have
   an additional external sram added on as a peripheral.

   The eeprom is 8 bits wide and is used to store data when the device is
   powered down. Eeprom is not directly accessible, it can only be accessed
   via io-registers using a special algorithm. Accessing eeprom via gdb's
   remote serial protocol ('m' or 'M' packets) looks difficult to do and is
   not included at this time.

   [The eeprom could be read manually via ``x/b <eaddr + AVR_EMEM_START>'' or
   written using ``set {unsigned char}<eaddr + AVR_EMEM_START>''.  For this to
   work, the remote target must be able to handle eeprom accesses and perform
   the address translation.]

   All three memory spaces have physical addresses beginning at 0x0. In
   addition, the flash is addressed by gcc/binutils/gdb with respect to 8 bit
   bytes instead of the 16 bit wide words used by the real device for the
   Program Counter.

   In order for remote targets to work correctly, extra bits must be added to
   addresses before they are send to the target or received from the target
   via the remote serial protocol. The extra bits are the MSBs and are used to
   decode which memory space the address is referring to. */

#undef XMALLOC
#define XMALLOC(TYPE) ((TYPE*) xmalloc (sizeof (TYPE)))

#undef EXTRACT_INSN
#define EXTRACT_INSN(addr) extract_unsigned_integer(addr,2)

/* Constants: prefixed with AVR_ to avoid name space clashes */

enum
{
  AVR_REG_W = 24,
  AVR_REG_X = 26,
  AVR_REG_Y = 28,
  AVR_FP_REGNUM = 28,
  AVR_REG_Z = 30,

  AVR_SREG_REGNUM = 32,
  AVR_SP_REGNUM = 33,
  AVR_PC_REGNUM = 34,

  AVR_NUM_REGS = 32 + 1 /*SREG*/ + 1 /*SP*/ + 1 /*PC*/,
  AVR_NUM_REG_BYTES = 32 + 1 /*SREG*/ + 2 /*SP*/ + 4 /*PC*/,

  AVR_PC_REG_INDEX = 35,	/* index into array of registers */

  AVR_MAX_PROLOGUE_SIZE = 56,	/* bytes */

  /* Count of pushed registers. From r2 to r17 (inclusively), r28, r29 */
  AVR_MAX_PUSHES = 18,

  /* Number of the last pushed register. r17 for current avr-gcc */
  AVR_LAST_PUSHED_REGNUM = 17,

  /* FIXME: TRoth/2002-01-??: Can we shift all these memory masks left 8
     bits? Do these have to match the bfd vma values?. It sure would make
     things easier in the future if they didn't need to match.

     Note: I chose these values so as to be consistent with bfd vma
     addresses.

     TRoth/2002-04-08: There is already a conflict with very large programs
     in the mega128. The mega128 has 128K instruction bytes (64K words),
     thus the Most Significant Bit is 0x10000 which gets masked off my
     AVR_MEM_MASK.

     The problem manifests itself when trying to set a breakpoint in a
     function which resides in the upper half of the instruction space and
     thus requires a 17-bit address.

     For now, I've just removed the EEPROM mask and changed AVR_MEM_MASK
     from 0x00ff0000 to 0x00f00000. Eeprom is not accessible from gdb yet,
     but could be for some remote targets by just adding the correct offset
     to the address and letting the remote target handle the low-level
     details of actually accessing the eeprom. */

  AVR_IMEM_START = 0x00000000,	/* INSN memory */
  AVR_SMEM_START = 0x00800000,	/* SRAM memory */
#if 1
  /* No eeprom mask defined */
  AVR_MEM_MASK = 0x00f00000,	/* mask to determine memory space */
#else
  AVR_EMEM_START = 0x00810000,	/* EEPROM memory */
  AVR_MEM_MASK = 0x00ff0000,	/* mask to determine memory space */
#endif
};

/* Any function with a frame looks like this
   .......    <-SP POINTS HERE
   LOCALS1    <-FP POINTS HERE
   LOCALS0
   SAVED FP
   SAVED R3
   SAVED R2
   RET PC
   FIRST ARG
   SECOND ARG */

struct frame_extra_info
{
  CORE_ADDR return_pc;
  CORE_ADDR args_pointer;
  int locals_size;
  int framereg;
  int framesize;
  int is_main;
};

struct gdbarch_tdep
{
  /* FIXME: TRoth: is there anything to put here? */
  int foo;
};

/* Lookup the name of a register given it's number. */

static const char *
avr_register_name (int regnum)
{
  static char *register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
    "SREG", "SP", "PC"
  };
  if (regnum < 0)
    return NULL;
  if (regnum >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[regnum];
}

/* Index within `registers' of the first byte of the space for
   register REGNUM.  */

static int
avr_register_byte (int regnum)
{
  if (regnum < AVR_PC_REGNUM)
    return regnum;
  else
    return AVR_PC_REG_INDEX;
}

/* Number of bytes of storage in the actual machine representation for
   register REGNUM.  */

static int
avr_register_raw_size (int regnum)
{
  switch (regnum)
    {
    case AVR_PC_REGNUM:
      return 4;
    case AVR_SP_REGNUM:
    case AVR_FP_REGNUM:
      return 2;
    default:
      return 1;
    }
}

/* Number of bytes of storage in the program's representation
   for register N.  */

static int
avr_register_virtual_size (int regnum)
{
  return TYPE_LENGTH (REGISTER_VIRTUAL_TYPE (regnum));
}

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

static struct type *
avr_register_virtual_type (int regnum)
{
  switch (regnum)
    {
    case AVR_PC_REGNUM:
      return builtin_type_unsigned_long;
    case AVR_SP_REGNUM:
      return builtin_type_unsigned_short;
    default:
      return builtin_type_unsigned_char;
    }
}

/* Instruction address checks and convertions. */

static CORE_ADDR
avr_make_iaddr (CORE_ADDR x)
{
  return ((x) | AVR_IMEM_START);
}

static int
avr_iaddr_p (CORE_ADDR x)
{
  return (((x) & AVR_MEM_MASK) == AVR_IMEM_START);
}

/* FIXME: TRoth: Really need to use a larger mask for instructions. Some
   devices are already up to 128KBytes of flash space.

   TRoth/2002-04-8: See comment above where AVR_IMEM_START is defined. */

static CORE_ADDR
avr_convert_iaddr_to_raw (CORE_ADDR x)
{
  return ((x) & 0xffffffff);
}

/* SRAM address checks and convertions. */

static CORE_ADDR
avr_make_saddr (CORE_ADDR x)
{
  return ((x) | AVR_SMEM_START);
}

static int
avr_saddr_p (CORE_ADDR x)
{
  return (((x) & AVR_MEM_MASK) == AVR_SMEM_START);
}

static CORE_ADDR
avr_convert_saddr_to_raw (CORE_ADDR x)
{
  return ((x) & 0xffffffff);
}

/* EEPROM address checks and convertions. I don't know if these will ever
   actually be used, but I've added them just the same. TRoth */

/* TRoth/2002-04-08: Commented out for now to allow fix for problem with large
   programs in the mega128. */

/*  static CORE_ADDR */
/*  avr_make_eaddr (CORE_ADDR x) */
/*  { */
/*    return ((x) | AVR_EMEM_START); */
/*  } */

/*  static int */
/*  avr_eaddr_p (CORE_ADDR x) */
/*  { */
/*    return (((x) & AVR_MEM_MASK) == AVR_EMEM_START); */
/*  } */

/*  static CORE_ADDR */
/*  avr_convert_eaddr_to_raw (CORE_ADDR x) */
/*  { */
/*    return ((x) & 0xffffffff); */
/*  } */

/* Convert from address to pointer and vice-versa. */

static void
avr_address_to_pointer (struct type *type, void *buf, CORE_ADDR addr)
{
  /* Is it a code address?  */
  if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_FUNC
      || TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_METHOD)
    {
      store_unsigned_integer (buf, TYPE_LENGTH (type),
			      avr_convert_iaddr_to_raw (addr));
    }
  else
    {
      /* Strip off any upper segment bits.  */
      store_unsigned_integer (buf, TYPE_LENGTH (type),
			      avr_convert_saddr_to_raw (addr));
    }
}

static CORE_ADDR
avr_pointer_to_address (struct type *type, void *buf)
{
  CORE_ADDR addr = extract_address (buf, TYPE_LENGTH (type));

  if (TYPE_CODE_SPACE (TYPE_TARGET_TYPE (type)))
    {
      fprintf_unfiltered (gdb_stderr, "CODE_SPACE ---->> ptr->addr: 0x%lx\n",
			  addr);
      fprintf_unfiltered (gdb_stderr,
			  "+++ If you see this, please send me an email <troth@verinet.com>\n");
    }

  /* Is it a code address?  */
  if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_FUNC
      || TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_METHOD
      || TYPE_CODE_SPACE (TYPE_TARGET_TYPE (type)))
    return avr_make_iaddr (addr);
  else
    return avr_make_saddr (addr);
}

static CORE_ADDR
avr_read_pc (ptid_t ptid)
{
  ptid_t save_ptid;
  CORE_ADDR pc;
  CORE_ADDR retval;

  save_ptid = inferior_ptid;
  inferior_ptid = ptid;
  pc = (int) read_register (AVR_PC_REGNUM);
  inferior_ptid = save_ptid;
  retval = avr_make_iaddr (pc);
  return retval;
}

static void
avr_write_pc (CORE_ADDR val, ptid_t ptid)
{
  ptid_t save_ptid;

  save_ptid = inferior_ptid;
  inferior_ptid = ptid;
  write_register (AVR_PC_REGNUM, avr_convert_iaddr_to_raw (val));
  inferior_ptid = save_ptid;
}

static CORE_ADDR
avr_read_sp (void)
{
  return (avr_make_saddr (read_register (AVR_SP_REGNUM)));
}

static void
avr_write_sp (CORE_ADDR val)
{
  write_register (AVR_SP_REGNUM, avr_convert_saddr_to_raw (val));
}

static CORE_ADDR
avr_read_fp (void)
{
  return (avr_make_saddr (read_register (AVR_FP_REGNUM)));
}

/* Translate a GDB virtual ADDR/LEN into a format the remote target
   understands.  Returns number of bytes that can be transfered
   starting at TARG_ADDR.  Return ZERO if no bytes can be transfered
   (segmentation fault).

   TRoth/2002-04-08: Could this be used to check for dereferencing an invalid
   pointer? */

static void
avr_remote_translate_xfer_address (CORE_ADDR memaddr, int nr_bytes,
				   CORE_ADDR *targ_addr, int *targ_len)
{
  long out_addr;
  long out_len;

  /* FIXME: TRoth: Do nothing for now. Will need to examine memaddr at this
     point and see if the high bit are set with the masks that we want. */

  *targ_addr = memaddr;
  *targ_len = nr_bytes;
}

/* Function pointers obtained from the target are half of what gdb expects so
   multiply by 2. */

static CORE_ADDR
avr_convert_from_func_ptr_addr (CORE_ADDR addr)
{
  return addr * 2;
}

/* avr_scan_prologue is also used as the frame_init_saved_regs().

   Put here the code to store, into fi->saved_regs, the addresses of
   the saved registers of frame described by FRAME_INFO.  This
   includes special registers such as pc and fp saved in special ways
   in the stack frame.  sp is even more special: the address we return
   for it IS the sp for the next frame. */

/* Function: avr_scan_prologue (helper function for avr_init_extra_frame_info)
   This function decodes a AVR function prologue to determine:
     1) the size of the stack frame
     2) which registers are saved on it
     3) the offsets of saved regs
   This information is stored in the "extra_info" field of the frame_info.

   A typical AVR function prologue might look like this:
        push rXX
        push r28
        push r29
        in r28,__SP_L__
        in r29,__SP_H__
        sbiw r28,<LOCALS_SIZE>
        in __tmp_reg__,__SREG__
        cli
        out __SP_L__,r28
        out __SREG__,__tmp_reg__
        out __SP_H__,r29

  A `-mcall-prologues' prologue look like this:
        ldi r26,<LOCALS_SIZE>
        ldi r27,<LOCALS_SIZE>/265
        ldi r30,pm_lo8(.L_foo_body)
        ldi r31,pm_hi8(.L_foo_body)
        rjmp __prologue_saves__+RRR
  .L_foo_body:  */

static void
avr_scan_prologue (struct frame_info *fi)
{
  CORE_ADDR prologue_start;
  CORE_ADDR prologue_end;
  int i;
  unsigned short insn;
  int regno;
  int scan_stage = 0;
  char *name;
  struct minimal_symbol *msymbol;
  int prologue_len;
  unsigned char prologue[AVR_MAX_PROLOGUE_SIZE];
  int vpc = 0;

  fi->extra_info->framereg = AVR_SP_REGNUM;

  if (find_pc_partial_function
      (fi->pc, &name, &prologue_start, &prologue_end))
    {
      struct symtab_and_line sal = find_pc_line (prologue_start, 0);

      if (sal.line == 0)	/* no line info, use current PC */
	prologue_end = fi->pc;
      else if (sal.end < prologue_end)	/* next line begins after fn end */
	prologue_end = sal.end;	/* (probably means no prologue)  */
    }
  else
    /* We're in the boondocks: allow for */
    /* 19 pushes, an add, and "mv fp,sp" */
    prologue_end = prologue_start + AVR_MAX_PROLOGUE_SIZE;

  prologue_end = min (prologue_end, fi->pc);

  /* Search the prologue looking for instructions that set up the
     frame pointer, adjust the stack pointer, and save registers.  */

  fi->extra_info->framesize = 0;
  prologue_len = prologue_end - prologue_start;
  read_memory (prologue_start, prologue, prologue_len);

  /* Scanning main()'s prologue
     ldi r28,lo8(<RAM_ADDR> - <LOCALS_SIZE>)
     ldi r29,hi8(<RAM_ADDR> - <LOCALS_SIZE>)
     out __SP_H__,r29
     out __SP_L__,r28 */

  if (name && strcmp ("main", name) == 0 && prologue_len == 8)
    {
      CORE_ADDR locals;
      unsigned char img[] = {
	0xde, 0xbf,		/* out __SP_H__,r29 */
	0xcd, 0xbf		/* out __SP_L__,r28 */
      };

      fi->extra_info->framereg = AVR_FP_REGNUM;
      insn = EXTRACT_INSN (&prologue[vpc]);
      /* ldi r28,lo8(<RAM_ADDR> - <LOCALS_SIZE>) */
      if ((insn & 0xf0f0) == 0xe0c0)
	{
	  locals = (insn & 0xf) | ((insn & 0x0f00) >> 4);
	  insn = EXTRACT_INSN (&prologue[vpc + 2]);
	  /* ldi r29,hi8(<RAM_ADDR> - <LOCALS_SIZE>) */
	  if ((insn & 0xf0f0) == 0xe0d0)
	    {
	      locals |= ((insn & 0xf) | ((insn & 0x0f00) >> 4)) << 8;
	      if (memcmp (prologue + vpc + 4, img, sizeof (img)) == 0)
		{
		  fi->frame = locals;

		  fi->extra_info->is_main = 1;
		  return;
		}
	    }
	}
    }

  /* Scanning `-mcall-prologues' prologue
     FIXME: mega prologue have a 12 bytes long */

  while (prologue_len <= 12)	/* I'm use while to avoit many goto's */
    {
      int loc_size;
      int body_addr;
      unsigned num_pushes;

      insn = EXTRACT_INSN (&prologue[vpc]);
      /* ldi r26,<LOCALS_SIZE> */
      if ((insn & 0xf0f0) != 0xe0a0)
	break;
      loc_size = (insn & 0xf) | ((insn & 0x0f00) >> 4);

      insn = EXTRACT_INSN (&prologue[vpc + 2]);
      /* ldi r27,<LOCALS_SIZE> / 256 */
      if ((insn & 0xf0f0) != 0xe0b0)
	break;
      loc_size |= ((insn & 0xf) | ((insn & 0x0f00) >> 4)) << 8;

      insn = EXTRACT_INSN (&prologue[vpc + 4]);
      /* ldi r30,pm_lo8(.L_foo_body) */
      if ((insn & 0xf0f0) != 0xe0e0)
	break;
      body_addr = (insn & 0xf) | ((insn & 0x0f00) >> 4);

      insn = EXTRACT_INSN (&prologue[vpc + 6]);
      /* ldi r31,pm_hi8(.L_foo_body) */
      if ((insn & 0xf0f0) != 0xe0f0)
	break;
      body_addr |= ((insn & 0xf) | ((insn & 0x0f00) >> 4)) << 8;

      if (body_addr != (prologue_start + 10) / 2)
	break;

      msymbol = lookup_minimal_symbol ("__prologue_saves__", NULL, NULL);
      if (!msymbol)
	break;

      /* FIXME: prologue for mega have a JMP instead of RJMP */
      insn = EXTRACT_INSN (&prologue[vpc + 8]);
      /* rjmp __prologue_saves__+RRR */
      if ((insn & 0xf000) != 0xc000)
	break;

      /* Extract PC relative offset from RJMP */
      i = (insn & 0xfff) | (insn & 0x800 ? (-1 ^ 0xfff) : 0);
      /* Convert offset to byte addressable mode */
      i *= 2;
      /* Destination address */
      i += vpc + prologue_start + 10;
      /* Resovle offset (in words) from __prologue_saves__ symbol.
         Which is a pushes count in `-mcall-prologues' mode */
      num_pushes = AVR_MAX_PUSHES - (i - SYMBOL_VALUE_ADDRESS (msymbol)) / 2;

      if (num_pushes > AVR_MAX_PUSHES)
	num_pushes = 0;

      if (num_pushes)
	{
	  int from;
	  fi->saved_regs[AVR_FP_REGNUM + 1] = num_pushes;
	  if (num_pushes >= 2)
	    fi->saved_regs[AVR_FP_REGNUM] = num_pushes - 1;
	  i = 0;
	  for (from = AVR_LAST_PUSHED_REGNUM + 1 - (num_pushes - 2);
	       from <= AVR_LAST_PUSHED_REGNUM; ++from)
	    fi->saved_regs[from] = ++i;
	}
      fi->extra_info->locals_size = loc_size;
      fi->extra_info->framesize = loc_size + num_pushes;
      fi->extra_info->framereg = AVR_FP_REGNUM;
      return;
    }

  /* Scan interrupt or signal function */

  if (prologue_len >= 12)
    {
      unsigned char img[] = {
	0x78, 0x94,		/* sei */
	0x1f, 0x92,		/* push r1 */
	0x0f, 0x92,		/* push r0 */
	0x0f, 0xb6,		/* in r0,0x3f SREG */
	0x0f, 0x92,		/* push r0 */
	0x11, 0x24		/* clr r1 */
      };
      if (memcmp (prologue, img, sizeof (img)) == 0)
	{
	  vpc += sizeof (img);
	  fi->saved_regs[0] = 2;
	  fi->saved_regs[1] = 1;
	  fi->extra_info->framesize += 3;
	}
      else if (memcmp (img + 1, prologue, sizeof (img) - 1) == 0)
	{
	  vpc += sizeof (img) - 1;
	  fi->saved_regs[0] = 2;
	  fi->saved_regs[1] = 1;
	  fi->extra_info->framesize += 3;
	}
    }

  /* First stage of the prologue scanning.
     Scan pushes */

  for (; vpc <= prologue_len; vpc += 2)
    {
      insn = EXTRACT_INSN (&prologue[vpc]);
      if ((insn & 0xfe0f) == 0x920f)	/* push rXX */
	{
	  /* Bits 4-9 contain a mask for registers R0-R32. */
	  regno = (insn & 0x1f0) >> 4;
	  ++fi->extra_info->framesize;
	  fi->saved_regs[regno] = fi->extra_info->framesize;
	  scan_stage = 1;
	}
      else
	break;
    }

  /* Second stage of the prologue scanning.
     Scan:
     in r28,__SP_L__
     in r29,__SP_H__ */

  if (scan_stage == 1 && vpc + 4 <= prologue_len)
    {
      unsigned char img[] = {
	0xcd, 0xb7,		/* in r28,__SP_L__ */
	0xde, 0xb7		/* in r29,__SP_H__ */
      };
      unsigned short insn1;

      if (memcmp (prologue + vpc, img, sizeof (img)) == 0)
	{
	  vpc += 4;
	  fi->extra_info->framereg = AVR_FP_REGNUM;
	  scan_stage = 2;
	}
    }

  /* Third stage of the prologue scanning. (Really two stages)
     Scan for:
     sbiw r28,XX or subi r28,lo8(XX)
     sbci r29,hi8(XX)
     in __tmp_reg__,__SREG__
     cli
     out __SP_L__,r28
     out __SREG__,__tmp_reg__
     out __SP_H__,r29 */

  if (scan_stage == 2 && vpc + 12 <= prologue_len)
    {
      int locals_size = 0;
      unsigned char img[] = {
	0x0f, 0xb6,		/* in r0,0x3f */
	0xf8, 0x94,		/* cli */
	0xcd, 0xbf,		/* out 0x3d,r28 ; SPL */
	0x0f, 0xbe,		/* out 0x3f,r0  ; SREG */
	0xde, 0xbf		/* out 0x3e,r29 ; SPH */
      };
      unsigned char img_sig[] = {
	0xcd, 0xbf,		/* out 0x3d,r28 ; SPL */
	0xde, 0xbf		/* out 0x3e,r29 ; SPH */
      };
      unsigned char img_int[] = {
	0xf8, 0x94,		/* cli */
	0xcd, 0xbf,		/* out 0x3d,r28 ; SPL */
	0x78, 0x94,		/* sei */
	0xde, 0xbf		/* out 0x3e,r29 ; SPH */
      };

      insn = EXTRACT_INSN (&prologue[vpc]);
      vpc += 2;
      if ((insn & 0xff30) == 0x9720)	/* sbiw r28,XXX */
	locals_size = (insn & 0xf) | ((insn & 0xc0) >> 2);
      else if ((insn & 0xf0f0) == 0x50c0)	/* subi r28,lo8(XX) */
	{
	  locals_size = (insn & 0xf) | ((insn & 0xf00) >> 4);
	  insn = EXTRACT_INSN (&prologue[vpc]);
	  vpc += 2;
	  locals_size += ((insn & 0xf) | ((insn & 0xf00) >> 4) << 8);
	}
      else
	return;
      fi->extra_info->locals_size = locals_size;
      fi->extra_info->framesize += locals_size;
    }
}

/* This function actually figures out the frame address for a given pc and
   sp.  This is tricky  because we sometimes don't use an explicit
   frame pointer, and the previous stack pointer isn't necessarily recorded
   on the stack.  The only reliable way to get this info is to
   examine the prologue.  */

static void
avr_init_extra_frame_info (int fromleaf, struct frame_info *fi)
{
  int reg;

  if (fi->next)
    fi->pc = FRAME_SAVED_PC (fi->next);

  fi->extra_info = (struct frame_extra_info *)
    frame_obstack_alloc (sizeof (struct frame_extra_info));
  frame_saved_regs_zalloc (fi);

  fi->extra_info->return_pc = 0;
  fi->extra_info->args_pointer = 0;
  fi->extra_info->locals_size = 0;
  fi->extra_info->framereg = 0;
  fi->extra_info->framesize = 0;
  fi->extra_info->is_main = 0;

  avr_scan_prologue (fi);

  if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
    {
      /* We need to setup fi->frame here because run_stack_dummy gets it wrong
         by assuming it's always FP.  */
      fi->frame = generic_read_register_dummy (fi->pc, fi->frame,
                                               AVR_PC_REGNUM);
    }
  else if (!fi->next)		/* this is the innermost frame? */
    fi->frame = read_register (fi->extra_info->framereg);
  else if (fi->extra_info->is_main != 1)	/* not the innermost frame, not `main' */
    /* If we have an next frame,  the callee saved it. */
    {
      struct frame_info *next_fi = fi->next;
      if (fi->extra_info->framereg == AVR_SP_REGNUM)
	fi->frame =
	  next_fi->frame + 2 /* ret addr */  + next_fi->extra_info->framesize;
      /* FIXME: I don't analyse va_args functions  */
      else
	{
	  CORE_ADDR fp = 0;
	  CORE_ADDR fp1 = 0;
	  unsigned int fp_low, fp_high;

	  /* Scan all frames */
	  for (; next_fi; next_fi = next_fi->next)
	    {
	      /* look for saved AVR_FP_REGNUM */
	      if (next_fi->saved_regs[AVR_FP_REGNUM] && !fp)
		fp = next_fi->saved_regs[AVR_FP_REGNUM];
	      /* look for saved AVR_FP_REGNUM + 1 */
	      if (next_fi->saved_regs[AVR_FP_REGNUM + 1] && !fp1)
		fp1 = next_fi->saved_regs[AVR_FP_REGNUM + 1];
	    }
	  fp_low = (fp ? read_memory_unsigned_integer (avr_make_saddr (fp), 1)
		    : read_register (AVR_FP_REGNUM)) & 0xff;
	  fp_high =
	    (fp1 ? read_memory_unsigned_integer (avr_make_saddr (fp1), 1) :
	     read_register (AVR_FP_REGNUM + 1)) & 0xff;
	  fi->frame = fp_low | (fp_high << 8);
	}
    }

  /* TRoth: Do we want to do this if we are in main? I don't think we should
     since return_pc makes no sense when we are in main. */

  if ((fi->pc) && (fi->extra_info->is_main == 0))	/* We are not in CALL_DUMMY */
    {
      CORE_ADDR addr;
      int i;

      addr = fi->frame + fi->extra_info->framesize + 1;

      /* Return address in stack in different endianness */

      fi->extra_info->return_pc =
	read_memory_unsigned_integer (avr_make_saddr (addr), 1) << 8;
      fi->extra_info->return_pc |=
	read_memory_unsigned_integer (avr_make_saddr (addr + 1), 1);

      /* This return address in words,
         must be converted to the bytes address */
      fi->extra_info->return_pc *= 2;

      /* Resolve a pushed registers addresses */
      for (i = 0; i < NUM_REGS; i++)
	{
	  if (fi->saved_regs[i])
	    fi->saved_regs[i] = addr - fi->saved_regs[i];
	}
    }
}

/* Restore the machine to the state it had before the current frame was
   created.  Usually used either by the "RETURN" command, or by
   call_function_by_hand after the dummy_frame is finished. */

static void
avr_pop_frame (void)
{
  unsigned regnum;
  CORE_ADDR saddr;
  struct frame_info *frame = get_current_frame ();

  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    {
      generic_pop_dummy_frame ();
    }
  else
    {
      /* TRoth: Why only loop over 8 registers? */

      for (regnum = 0; regnum < 8; regnum++)
	{
	  /* Don't forget AVR_SP_REGNUM in a frame_saved_regs struct is the
	     actual value we want, not the address of the value we want.  */
	  if (frame->saved_regs[regnum] && regnum != AVR_SP_REGNUM)
	    {
	      saddr = avr_make_saddr (frame->saved_regs[regnum]);
	      write_register (regnum,
			      read_memory_unsigned_integer (saddr, 1));
	    }
	  else if (frame->saved_regs[regnum] && regnum == AVR_SP_REGNUM)
	    write_register (regnum, frame->frame + 2);
	}

      /* Don't forget the update the PC too!  */
      write_pc (frame->extra_info->return_pc);
    }
  flush_cached_frames ();
}

/* Return the saved PC from this frame. */

static CORE_ADDR
avr_frame_saved_pc (struct frame_info *frame)
{
  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    return generic_read_register_dummy (frame->pc, frame->frame,
					AVR_PC_REGNUM);
  else
    return frame->extra_info->return_pc;
}

static CORE_ADDR
avr_saved_pc_after_call (struct frame_info *frame)
{
  unsigned char m1, m2;
  unsigned int sp = read_register (AVR_SP_REGNUM);
  m1 = read_memory_unsigned_integer (avr_make_saddr (sp + 1), 1);
  m2 = read_memory_unsigned_integer (avr_make_saddr (sp + 2), 1);
  return (m2 | (m1 << 8)) * 2;
}

/* Figure out where in REGBUF the called function has left its return value.
   Copy that into VALBUF. */

static void
avr_extract_return_value (struct type *type, char *regbuf, char *valbuf)
{
  int wordsize, len;

  wordsize = 2;

  len = TYPE_LENGTH (type);

  switch (len)
    {
    case 1:			/* (char) */
    case 2:			/* (short), (int) */
      memcpy (valbuf, regbuf + REGISTER_BYTE (24), 2);
      break;
    case 4:			/* (long), (float) */
      memcpy (valbuf, regbuf + REGISTER_BYTE (22), 4);
      break;
    case 8:			/* (double) (doesn't seem to happen, which is good,
				   because this almost certainly isn't right.  */
      error ("I don't know how a double is returned.");
      break;
    }
}

/* Returns the return address for a dummy. */

static CORE_ADDR
avr_call_dummy_address (void)
{
  return entry_point_address ();
}

/* Place the appropriate value in the appropriate registers.
   Primarily used by the RETURN command.  */

static void
avr_store_return_value (struct type *type, char *valbuf)
{
  int wordsize, len, regval;

  wordsize = 2;

  len = TYPE_LENGTH (type);
  switch (len)
    {
    case 1:			/* char */
    case 2:			/* short, int */
      regval = extract_address (valbuf, len);
      write_register (0, regval);
      break;
    case 4:			/* long, float */
      regval = extract_address (valbuf, len);
      write_register (0, regval >> 16);
      write_register (1, regval & 0xffff);
      break;
    case 8:			/* presumeably double, but doesn't seem to happen */
      error ("I don't know how to return a double.");
      break;
    }
}

/* Setup the return address for a dummy frame, as called by
   call_function_by_hand.  Only necessary when you are using an empty
   CALL_DUMMY. */

static CORE_ADDR
avr_push_return_address (CORE_ADDR pc, CORE_ADDR sp)
{
  unsigned char buf[2];
  int wordsize = 2;
#if 0
  struct minimal_symbol *msymbol;
  CORE_ADDR mon_brk;
#endif

  buf[0] = 0;
  buf[1] = 0;
  sp -= wordsize;
  write_memory (sp + 1, buf, 2);

#if 0
  /* FIXME: TRoth/2002-02-18: This should probably be removed since it's a
     left-over from Denis' original patch which used avr-mon for the target
     instead of the generic remote target. */
  if ((strcmp (target_shortname, "avr-mon") == 0)
      && (msymbol = lookup_minimal_symbol ("gdb_break", NULL, NULL)))
    {
      mon_brk = SYMBOL_VALUE_ADDRESS (msymbol);
      store_unsigned_integer (buf, wordsize, mon_brk / 2);
      sp -= wordsize;
      write_memory (sp + 1, buf + 1, 1);
      write_memory (sp + 2, buf, 1);
    }
#endif
  return sp;
}

static CORE_ADDR
avr_skip_prologue (CORE_ADDR pc)
{
  CORE_ADDR func_addr, func_end;
  struct symtab_and_line sal;

  /* See what the symbol table says */

  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      sal = find_pc_line (func_addr, 0);

      /* troth/2002-08-05: For some very simple functions, gcc doesn't
         generate a prologue and the sal.end ends up being the 2-byte ``ret''
         instruction at the end of the function, but func_end ends up being
         the address of the first instruction of the _next_ function. By
         adjusting func_end by 2 bytes, we can catch these functions and not
         return sal.end if it is the ``ret'' instruction. */

      if (sal.line != 0 && sal.end < (func_end-2))
	return sal.end;
    }

/* Either we didn't find the start of this function (nothing we can do),
   or there's no line info, or the line after the prologue is after
   the end of the function (there probably isn't a prologue). */

  return pc;
}

static CORE_ADDR
avr_frame_address (struct frame_info *fi)
{
  return avr_make_saddr (fi->frame);
}

/* Given a GDB frame, determine the address of the calling function's frame.
   This will be used to create a new GDB frame struct, and then
   INIT_EXTRA_FRAME_INFO and INIT_FRAME_PC will be called for the new frame.

   For us, the frame address is its stack pointer value, so we look up
   the function prologue to determine the caller's sp value, and return it.  */

static CORE_ADDR
avr_frame_chain (struct frame_info *frame)
{
  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    {
      /* initialize the return_pc now */
      frame->extra_info->return_pc = generic_read_register_dummy (frame->pc,
								  frame->
								  frame,
								  AVR_PC_REGNUM);
      return frame->frame;
    }
  return (frame->extra_info->is_main ? 0
	  : frame->frame + frame->extra_info->framesize + 2 /* ret addr */ );
}

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. 

   We store structs through a pointer passed in the first Argument
   register. */

static void
avr_store_struct_return (CORE_ADDR addr, CORE_ADDR sp)
{
  write_register (0, addr);
}

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one). */

static CORE_ADDR
avr_extract_struct_value_address (char *regbuf)
{
  return (extract_address ((regbuf) + REGISTER_BYTE (0),
			   REGISTER_RAW_SIZE (0)) | AVR_SMEM_START);
}

/* Setup the function arguments for calling a function in the inferior.

   On the AVR architecture, there are 18 registers (R25 to R8) which are
   dedicated for passing function arguments.  Up to the first 18 arguments
   (depending on size) may go into these registers.  The rest go on the stack.

   Arguments that are larger than WORDSIZE bytes will be split between two or
   more registers as available, but will NOT be split between a register and
   the stack.

   An exceptional case exists for struct arguments (and possibly other
   aggregates such as arrays) -- if the size is larger than WORDSIZE bytes but
   not a multiple of WORDSIZE bytes.  In this case the argument is never split
   between the registers and the stack, but instead is copied in its entirety
   onto the stack, AND also copied into as many registers as there is room
   for.  In other words, space in registers permitting, two copies of the same
   argument are passed in.  As far as I can tell, only the one on the stack is
   used, although that may be a function of the level of compiler
   optimization.  I suspect this is a compiler bug.  Arguments of these odd
   sizes are left-justified within the word (as opposed to arguments smaller
   than WORDSIZE bytes, which are right-justified).
 
   If the function is to return an aggregate type such as a struct, the caller
   must allocate space into which the callee will copy the return value.  In
   this case, a pointer to the return value location is passed into the callee
   in register R0, which displaces one of the other arguments passed in via
   registers R0 to R2. */

static CORE_ADDR
avr_push_arguments (int nargs, struct value **args, CORE_ADDR sp,
		    int struct_return, CORE_ADDR struct_addr)
{
  int stack_alloc, stack_offset;
  int wordsize;
  int argreg;
  int argnum;
  struct type *type;
  CORE_ADDR regval;
  char *val;
  char valbuf[4];
  int len;

  wordsize = 1;
#if 0
  /* Now make sure there's space on the stack */
  for (argnum = 0, stack_alloc = 0; argnum < nargs; argnum++)
    stack_alloc += TYPE_LENGTH (VALUE_TYPE (args[argnum]));
  sp -= stack_alloc;		/* make room on stack for args */
  /* we may over-allocate a little here, but that won't hurt anything */
#endif
  argreg = 25;
  if (struct_return)		/* "struct return" pointer takes up one argreg */
    {
      write_register (--argreg, struct_addr);
    }

  /* Now load as many as possible of the first arguments into registers, and
     push the rest onto the stack.  There are 3N bytes in three registers
     available.  Loop thru args from first to last.  */

  for (argnum = 0, stack_offset = 0; argnum < nargs; argnum++)
    {
      type = VALUE_TYPE (args[argnum]);
      len = TYPE_LENGTH (type);
      val = (char *) VALUE_CONTENTS (args[argnum]);

      /* NOTE WELL!!!!!  This is not an "else if" clause!!!  That's because
         some *&^%$ things get passed on the stack AND in the registers!  */
      while (len > 0)
	{			/* there's room in registers */
	  len -= wordsize;
	  regval = extract_address (val + len, wordsize);
	  write_register (argreg--, regval);
	}
    }
  return sp;
}

/* Initialize the gdbarch structure for the AVR's. */

static struct gdbarch *
avr_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  /* FIXME: TRoth/2002-02-18: I have no idea if avr_call_dummy_words[] should
     be bigger or not. Initial testing seems to show that `call my_func()`
     works and backtrace from a breakpoint within the call looks correct.
     Admittedly, I haven't tested with more than a very simple program. */
  static LONGEST avr_call_dummy_words[] = { 0 };

  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;

  /* Find a candidate among the list of pre-declared architectures. */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* None found, create a new architecture from the information provided. */
  tdep = XMALLOC (struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);

  /* If we ever need to differentiate the device types, do it here. */
  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_avr1:
    case bfd_mach_avr2:
    case bfd_mach_avr3:
    case bfd_mach_avr4:
    case bfd_mach_avr5:
      break;
    }

  set_gdbarch_short_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_int_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_long_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_long_bit (gdbarch, 8 * TARGET_CHAR_BIT);
  set_gdbarch_ptr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_addr_bit (gdbarch, 32);
  set_gdbarch_bfd_vma_bit (gdbarch, 32);	/* FIXME: TRoth/2002-02-18: Is this needed? */

  set_gdbarch_float_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_double_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_bit (gdbarch, 4 * TARGET_CHAR_BIT);

  set_gdbarch_float_format (gdbarch, &floatformat_ieee_single_little);
  set_gdbarch_double_format (gdbarch, &floatformat_ieee_single_little);
  set_gdbarch_long_double_format (gdbarch, &floatformat_ieee_single_little);

  set_gdbarch_read_pc (gdbarch, avr_read_pc);
  set_gdbarch_write_pc (gdbarch, avr_write_pc);
  set_gdbarch_read_fp (gdbarch, avr_read_fp);
  set_gdbarch_read_sp (gdbarch, avr_read_sp);
  set_gdbarch_write_sp (gdbarch, avr_write_sp);

  set_gdbarch_num_regs (gdbarch, AVR_NUM_REGS);

  set_gdbarch_sp_regnum (gdbarch, AVR_SP_REGNUM);
  set_gdbarch_fp_regnum (gdbarch, AVR_FP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, AVR_PC_REGNUM);

  set_gdbarch_register_name (gdbarch, avr_register_name);
  set_gdbarch_register_size (gdbarch, 1);
  set_gdbarch_register_bytes (gdbarch, AVR_NUM_REG_BYTES);
  set_gdbarch_register_byte (gdbarch, avr_register_byte);
  set_gdbarch_register_raw_size (gdbarch, avr_register_raw_size);
  set_gdbarch_max_register_raw_size (gdbarch, 4);
  set_gdbarch_register_virtual_size (gdbarch, avr_register_virtual_size);
  set_gdbarch_max_register_virtual_size (gdbarch, 4);
  set_gdbarch_register_virtual_type (gdbarch, avr_register_virtual_type);

  set_gdbarch_get_saved_register (gdbarch, generic_unwind_get_saved_register);

  set_gdbarch_print_insn (gdbarch, print_insn_avr);

  set_gdbarch_use_generic_dummy_frames (gdbarch, 1);
  set_gdbarch_call_dummy_location (gdbarch, AT_ENTRY_POINT);
  set_gdbarch_call_dummy_address (gdbarch, avr_call_dummy_address);
  set_gdbarch_call_dummy_start_offset (gdbarch, 0);
  set_gdbarch_call_dummy_breakpoint_offset_p (gdbarch, 1);
  set_gdbarch_call_dummy_breakpoint_offset (gdbarch, 0);
  set_gdbarch_call_dummy_length (gdbarch, 0);
  set_gdbarch_pc_in_call_dummy (gdbarch, generic_pc_in_call_dummy);
  set_gdbarch_call_dummy_p (gdbarch, 1);
  set_gdbarch_call_dummy_words (gdbarch, avr_call_dummy_words);
  set_gdbarch_call_dummy_stack_adjust_p (gdbarch, 0);
  set_gdbarch_fix_call_dummy (gdbarch, generic_fix_call_dummy);

/*    set_gdbarch_believe_pcc_promotion (gdbarch, 1); // TRoth: should this be set? */

  set_gdbarch_address_to_pointer (gdbarch, avr_address_to_pointer);
  set_gdbarch_pointer_to_address (gdbarch, avr_pointer_to_address);
  set_gdbarch_deprecated_extract_return_value (gdbarch, avr_extract_return_value);
  set_gdbarch_push_arguments (gdbarch, avr_push_arguments);
  set_gdbarch_push_dummy_frame (gdbarch, generic_push_dummy_frame);
  set_gdbarch_push_return_address (gdbarch, avr_push_return_address);
  set_gdbarch_pop_frame (gdbarch, avr_pop_frame);

  set_gdbarch_deprecated_store_return_value (gdbarch, avr_store_return_value);

  set_gdbarch_use_struct_convention (gdbarch, generic_use_struct_convention);
  set_gdbarch_store_struct_return (gdbarch, avr_store_struct_return);
  set_gdbarch_deprecated_extract_struct_value_address
    (gdbarch, avr_extract_struct_value_address);

  set_gdbarch_frame_init_saved_regs (gdbarch, avr_scan_prologue);
  set_gdbarch_init_extra_frame_info (gdbarch, avr_init_extra_frame_info);
  set_gdbarch_skip_prologue (gdbarch, avr_skip_prologue);
/*    set_gdbarch_prologue_frameless_p (gdbarch, avr_prologue_frameless_p); */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_decr_pc_after_break (gdbarch, 0);

  set_gdbarch_function_start_offset (gdbarch, 0);
  set_gdbarch_remote_translate_xfer_address (gdbarch,
					     avr_remote_translate_xfer_address);
  set_gdbarch_frame_args_skip (gdbarch, 0);
  set_gdbarch_frameless_function_invocation (gdbarch, frameless_look_for_prologue);	/* ??? */
  set_gdbarch_frame_chain (gdbarch, avr_frame_chain);
  set_gdbarch_frame_chain_valid (gdbarch, generic_func_frame_chain_valid);
  set_gdbarch_frame_saved_pc (gdbarch, avr_frame_saved_pc);
  set_gdbarch_frame_args_address (gdbarch, avr_frame_address);
  set_gdbarch_frame_locals_address (gdbarch, avr_frame_address);
  set_gdbarch_saved_pc_after_call (gdbarch, avr_saved_pc_after_call);
  set_gdbarch_frame_num_args (gdbarch, frame_num_args_unknown);

  set_gdbarch_convert_from_func_ptr_addr (gdbarch,
					  avr_convert_from_func_ptr_addr);

  return gdbarch;
}

/* Send a query request to the avr remote target asking for values of the io
   registers. If args parameter is not NULL, then the user has requested info
   on a specific io register [This still needs implemented and is ignored for
   now]. The query string should be one of these forms:

   "Ravr.io_reg" -> reply is "NN" number of io registers

   "Ravr.io_reg:addr,len" where addr is first register and len is number of
   registers to be read. The reply should be "<NAME>,VV;" for each io register
   where, <NAME> is a string, and VV is the hex value of the register.

   All io registers are 8-bit. */

static void
avr_io_reg_read_command (char *args, int from_tty)
{
  int bufsiz = 0;
  char buf[400];
  char query[400];
  char *p;
  unsigned int nreg = 0;
  unsigned int val;
  int i, j, k, step;

/*    fprintf_unfiltered (gdb_stderr, "DEBUG: avr_io_reg_read_command (\"%s\", %d)\n", */
/*             args, from_tty); */

  if (!current_target.to_query)
    {
      fprintf_unfiltered (gdb_stderr,
			  "ERR: info io_registers NOT supported by current target\n");
      return;
    }

  /* Just get the maximum buffer size. */
  target_query ((int) 'R', 0, 0, &bufsiz);
  if (bufsiz > sizeof (buf))
    bufsiz = sizeof (buf);

  /* Find out how many io registers the target has. */
  strcpy (query, "avr.io_reg");
  target_query ((int) 'R', query, buf, &bufsiz);

  if (strncmp (buf, "", bufsiz) == 0)
    {
      fprintf_unfiltered (gdb_stderr,
			  "info io_registers NOT supported by target\n");
      return;
    }

  if (sscanf (buf, "%x", &nreg) != 1)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Error fetching number of io registers\n");
      return;
    }

  reinitialize_more_filter ();

  printf_unfiltered ("Target has %u io registers:\n\n", nreg);

  /* only fetch up to 8 registers at a time to keep the buffer small */
  step = 8;

  for (i = 0; i < nreg; i += step)
    {
      j = step - (nreg % step);	/* how many registers this round? */

      snprintf (query, sizeof (query) - 1, "avr.io_reg:%x,%x", i, j);
      target_query ((int) 'R', query, buf, &bufsiz);

      p = buf;
      for (k = i; k < (i + j); k++)
	{
	  if (sscanf (p, "%[^,],%x;", query, &val) == 2)
	    {
	      printf_filtered ("[%02x] %-15s : %02x\n", k, query, val);
	      while ((*p != ';') && (*p != '\0'))
		p++;
	      p++;		/* skip over ';' */
	      if (*p == '\0')
		break;
	    }
	}
    }
}

void
_initialize_avr_tdep (void)
{
  register_gdbarch_init (bfd_arch_avr, avr_gdbarch_init);

  /* Add a new command to allow the user to query the avr remote target for
     the values of the io space registers in a saner way than just using
     `x/NNNb ADDR`. */

  /* FIXME: TRoth/2002-02-18: This should probably be changed to 'info avr
     io_registers' to signify it is not available on other platforms. */

  add_cmd ("io_registers", class_info, avr_io_reg_read_command,
	   "query remote avr target for io space register values", &infolist);
}
