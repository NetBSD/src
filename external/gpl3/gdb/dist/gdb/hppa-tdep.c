/* Target-dependent code for the HP PA-RISC architecture.

   Copyright (C) 1986-2023 Free Software Foundation, Inc.

   Contributed by the Center for Software Science at the
   University of Utah (pa-gdb-bugs@cs.utah.edu).

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "bfd.h"
#include "inferior.h"
#include "regcache.h"
#include "completer.h"
#include "osabi.h"
#include "arch-utils.h"
/* For argument passing to the inferior.  */
#include "symtab.h"
#include "dis-asm.h"
#include "trad-frame.h"
#include "frame-unwind.h"
#include "frame-base.h"

#include "gdbcore.h"
#include "gdbcmd.h"
#include "gdbtypes.h"
#include "objfiles.h"
#include "hppa-tdep.h"
#include <algorithm>

static bool hppa_debug = false;

/* Some local constants.  */
static const int hppa32_num_regs = 128;
static const int hppa64_num_regs = 96;

/* We use the objfile->obj_private pointer for two things:
 * 1.  An unwind table;
 *
 * 2.  A pointer to any associated shared library object.
 *
 * #defines are used to help refer to these objects.
 */

/* Info about the unwind table associated with an object file.
 * This is hung off of the "objfile->obj_private" pointer, and
 * is allocated in the objfile's psymbol obstack.  This allows
 * us to have unique unwind info for each executable and shared
 * library that we are debugging.
 */
struct hppa_unwind_info
  {
    struct unwind_table_entry *table;	/* Pointer to unwind info */
    struct unwind_table_entry *cache;	/* Pointer to last entry we found */
    int last;				/* Index of last entry */
  };

struct hppa_objfile_private
  {
    struct hppa_unwind_info *unwind_info = nullptr;	/* a pointer */
    struct so_list *so_info = nullptr;			/* a pointer  */
    CORE_ADDR dp = 0;

    int dummy_call_sequence_reg = 0;
    CORE_ADDR dummy_call_sequence_addr = 0;
  };

/* hppa-specific object data -- unwind and solib info.
   TODO/maybe: think about splitting this into two parts; the unwind data is 
   common to all hppa targets, but is only used in this file; we can register 
   that separately and make this static. The solib data is probably hpux-
   specific, so we can create a separate extern objfile_data that is registered
   by hppa-hpux-tdep.c and shared with pa64solib.c and somsolib.c.  */
static const registry<objfile>::key<hppa_objfile_private>
  hppa_objfile_priv_data;

/* Get at various relevant fields of an instruction word.  */
#define MASK_5 0x1f
#define MASK_11 0x7ff
#define MASK_14 0x3fff
#define MASK_21 0x1fffff

/* Sizes (in bytes) of the native unwind entries.  */
#define UNWIND_ENTRY_SIZE 16
#define STUB_UNWIND_ENTRY_SIZE 8

/* Routines to extract various sized constants out of hppa 
   instructions.  */

/* This assumes that no garbage lies outside of the lower bits of 
   value.  */

static int
hppa_sign_extend (unsigned val, unsigned bits)
{
  return (int) (val >> (bits - 1) ? (-(1 << bits)) | val : val);
}

/* For many immediate values the sign bit is the low bit!  */

static int
hppa_low_hppa_sign_extend (unsigned val, unsigned bits)
{
  return (int) ((val & 0x1 ? (-(1 << (bits - 1))) : 0) | val >> 1);
}

/* Extract the bits at positions between FROM and TO, using HP's numbering
   (MSB = 0).  */

int
hppa_get_field (unsigned word, int from, int to)
{
  return ((word) >> (31 - (to)) & ((1 << ((to) - (from) + 1)) - 1));
}

/* Extract the immediate field from a ld{bhw}s instruction.  */

int
hppa_extract_5_load (unsigned word)
{
  return hppa_low_hppa_sign_extend (word >> 16 & MASK_5, 5);
}

/* Extract the immediate field from a break instruction.  */

unsigned
hppa_extract_5r_store (unsigned word)
{
  return (word & MASK_5);
}

/* Extract the immediate field from a {sr}sm instruction.  */

unsigned
hppa_extract_5R_store (unsigned word)
{
  return (word >> 16 & MASK_5);
}

/* Extract a 14 bit immediate field.  */

int
hppa_extract_14 (unsigned word)
{
  return hppa_low_hppa_sign_extend (word & MASK_14, 14);
}

/* Extract a 21 bit constant.  */

int
hppa_extract_21 (unsigned word)
{
  int val;

  word &= MASK_21;
  word <<= 11;
  val = hppa_get_field (word, 20, 20);
  val <<= 11;
  val |= hppa_get_field (word, 9, 19);
  val <<= 2;
  val |= hppa_get_field (word, 5, 6);
  val <<= 5;
  val |= hppa_get_field (word, 0, 4);
  val <<= 2;
  val |= hppa_get_field (word, 7, 8);
  return hppa_sign_extend (val, 21) << 11;
}

/* extract a 17 bit constant from branch instructions, returning the
   19 bit signed value.  */

int
hppa_extract_17 (unsigned word)
{
  return hppa_sign_extend (hppa_get_field (word, 19, 28) |
		      hppa_get_field (word, 29, 29) << 10 |
		      hppa_get_field (word, 11, 15) << 11 |
		      (word & 0x1) << 16, 17) << 2;
}

CORE_ADDR 
hppa_symbol_address(const char *sym)
{
  struct bound_minimal_symbol minsym;

  minsym = lookup_minimal_symbol (sym, NULL, NULL);
  if (minsym.minsym)
    return minsym.value_address ();
  else
    return (CORE_ADDR)-1;
}



/* Compare the start address for two unwind entries returning 1 if 
   the first address is larger than the second, -1 if the second is
   larger than the first, and zero if they are equal.  */

static int
compare_unwind_entries (const void *arg1, const void *arg2)
{
  const struct unwind_table_entry *a = (const struct unwind_table_entry *) arg1;
  const struct unwind_table_entry *b = (const struct unwind_table_entry *) arg2;

  if (a->region_start > b->region_start)
    return 1;
  else if (a->region_start < b->region_start)
    return -1;
  else
    return 0;
}

static void
record_text_segment_lowaddr (bfd *abfd, asection *section, void *data)
{
  if ((section->flags & (SEC_ALLOC | SEC_LOAD | SEC_READONLY))
       == (SEC_ALLOC | SEC_LOAD | SEC_READONLY))
    {
      bfd_vma value = section->vma - section->filepos;
      CORE_ADDR *low_text_segment_address = (CORE_ADDR *)data;

      if (value < *low_text_segment_address)
	  *low_text_segment_address = value;
    }
}

static void
internalize_unwinds (struct objfile *objfile, struct unwind_table_entry *table,
		     asection *section, unsigned int entries,
		     size_t size, CORE_ADDR text_offset)
{
  /* We will read the unwind entries into temporary memory, then
     fill in the actual unwind table.  */

  if (size > 0)
    {
      struct gdbarch *gdbarch = objfile->arch ();
      hppa_gdbarch_tdep *tdep = gdbarch_tdep<hppa_gdbarch_tdep> (gdbarch);
      unsigned long tmp;
      unsigned i;
      char *buf = (char *) alloca (size);
      CORE_ADDR low_text_segment_address;

      /* For ELF targets, then unwinds are supposed to
	 be segment relative offsets instead of absolute addresses.

	 Note that when loading a shared library (text_offset != 0) the
	 unwinds are already relative to the text_offset that will be
	 passed in.  */
      if (tdep->is_elf && text_offset == 0)
	{
	  low_text_segment_address = -1;

	  bfd_map_over_sections (objfile->obfd.get (),
				 record_text_segment_lowaddr, 
				 &low_text_segment_address);

	  text_offset = low_text_segment_address;
	}
      else if (tdep->solib_get_text_base)
	{
	  text_offset = tdep->solib_get_text_base (objfile);
	}

      bfd_get_section_contents (objfile->obfd.get (), section, buf, 0, size);

      /* Now internalize the information being careful to handle host/target
	 endian issues.  */
      for (i = 0; i < entries; i++)
	{
	  table[i].region_start = bfd_get_32 (objfile->obfd,
					      (bfd_byte *) buf);
	  table[i].region_start += text_offset;
	  buf += 4;
	  table[i].region_end = bfd_get_32 (objfile->obfd, (bfd_byte *) buf);
	  table[i].region_end += text_offset;
	  buf += 4;
	  tmp = bfd_get_32 (objfile->obfd, (bfd_byte *) buf);
	  buf += 4;
	  table[i].Cannot_unwind = (tmp >> 31) & 0x1;
	  table[i].Millicode = (tmp >> 30) & 0x1;
	  table[i].Millicode_save_sr0 = (tmp >> 29) & 0x1;
	  table[i].Region_description = (tmp >> 27) & 0x3;
	  table[i].reserved = (tmp >> 26) & 0x1;
	  table[i].Entry_SR = (tmp >> 25) & 0x1;
	  table[i].Entry_FR = (tmp >> 21) & 0xf;
	  table[i].Entry_GR = (tmp >> 16) & 0x1f;
	  table[i].Args_stored = (tmp >> 15) & 0x1;
	  table[i].Variable_Frame = (tmp >> 14) & 0x1;
	  table[i].Separate_Package_Body = (tmp >> 13) & 0x1;
	  table[i].Frame_Extension_Millicode = (tmp >> 12) & 0x1;
	  table[i].Stack_Overflow_Check = (tmp >> 11) & 0x1;
	  table[i].Two_Instruction_SP_Increment = (tmp >> 10) & 0x1;
	  table[i].sr4export = (tmp >> 9) & 0x1;
	  table[i].cxx_info = (tmp >> 8) & 0x1;
	  table[i].cxx_try_catch = (tmp >> 7) & 0x1;
	  table[i].sched_entry_seq = (tmp >> 6) & 0x1;
	  table[i].reserved1 = (tmp >> 5) & 0x1;
	  table[i].Save_SP = (tmp >> 4) & 0x1;
	  table[i].Save_RP = (tmp >> 3) & 0x1;
	  table[i].Save_MRP_in_frame = (tmp >> 2) & 0x1;
	  table[i].save_r19 = (tmp >> 1) & 0x1;
	  table[i].Cleanup_defined = tmp & 0x1;
	  tmp = bfd_get_32 (objfile->obfd, (bfd_byte *) buf);
	  buf += 4;
	  table[i].MPE_XL_interrupt_marker = (tmp >> 31) & 0x1;
	  table[i].HP_UX_interrupt_marker = (tmp >> 30) & 0x1;
	  table[i].Large_frame = (tmp >> 29) & 0x1;
	  table[i].alloca_frame = (tmp >> 28) & 0x1;
	  table[i].reserved2 = (tmp >> 27) & 0x1;
	  table[i].Total_frame_size = tmp & 0x7ffffff;

	  /* Stub unwinds are handled elsewhere.  */
	  table[i].stub_unwind.stub_type = 0;
	  table[i].stub_unwind.padding = 0;
	}
    }
}

/* Read in the backtrace information stored in the `$UNWIND_START$' section of
   the object file.  This info is used mainly by find_unwind_entry() to find
   out the stack frame size and frame pointer used by procedures.  We put
   everything on the psymbol obstack in the objfile so that it automatically
   gets freed when the objfile is destroyed.  */

static void
read_unwind_info (struct objfile *objfile)
{
  asection *unwind_sec, *stub_unwind_sec;
  size_t unwind_size, stub_unwind_size, total_size;
  unsigned index, unwind_entries;
  unsigned stub_entries, total_entries;
  CORE_ADDR text_offset;
  struct hppa_unwind_info *ui;
  struct hppa_objfile_private *obj_private;

  text_offset = objfile->text_section_offset ();
  ui = (struct hppa_unwind_info *) obstack_alloc (&objfile->objfile_obstack,
					   sizeof (struct hppa_unwind_info));

  ui->table = NULL;
  ui->cache = NULL;
  ui->last = -1;

  /* For reasons unknown the HP PA64 tools generate multiple unwinder
     sections in a single executable.  So we just iterate over every
     section in the BFD looking for unwinder sections instead of trying
     to do a lookup with bfd_get_section_by_name.

     First determine the total size of the unwind tables so that we
     can allocate memory in a nice big hunk.  */
  total_entries = 0;
  for (unwind_sec = objfile->obfd->sections;
       unwind_sec;
       unwind_sec = unwind_sec->next)
    {
      if (strcmp (unwind_sec->name, "$UNWIND_START$") == 0
	  || strcmp (unwind_sec->name, ".PARISC.unwind") == 0)
	{
	  unwind_size = bfd_section_size (unwind_sec);
	  unwind_entries = unwind_size / UNWIND_ENTRY_SIZE;

	  total_entries += unwind_entries;
	}
    }

  /* Now compute the size of the stub unwinds.  Note the ELF tools do not
     use stub unwinds at the current time.  */
  stub_unwind_sec = bfd_get_section_by_name (objfile->obfd.get (),
					     "$UNWIND_END$");

  if (stub_unwind_sec)
    {
      stub_unwind_size = bfd_section_size (stub_unwind_sec);
      stub_entries = stub_unwind_size / STUB_UNWIND_ENTRY_SIZE;
    }
  else
    {
      stub_unwind_size = 0;
      stub_entries = 0;
    }

  /* Compute total number of unwind entries and their total size.  */
  total_entries += stub_entries;
  total_size = total_entries * sizeof (struct unwind_table_entry);

  /* Allocate memory for the unwind table.  */
  ui->table = (struct unwind_table_entry *)
    obstack_alloc (&objfile->objfile_obstack, total_size);
  ui->last = total_entries - 1;

  /* Now read in each unwind section and internalize the standard unwind
     entries.  */
  index = 0;
  for (unwind_sec = objfile->obfd->sections;
       unwind_sec;
       unwind_sec = unwind_sec->next)
    {
      if (strcmp (unwind_sec->name, "$UNWIND_START$") == 0
	  || strcmp (unwind_sec->name, ".PARISC.unwind") == 0)
	{
	  unwind_size = bfd_section_size (unwind_sec);
	  unwind_entries = unwind_size / UNWIND_ENTRY_SIZE;

	  internalize_unwinds (objfile, &ui->table[index], unwind_sec,
			       unwind_entries, unwind_size, text_offset);
	  index += unwind_entries;
	}
    }

  /* Now read in and internalize the stub unwind entries.  */
  if (stub_unwind_size > 0)
    {
      unsigned int i;
      char *buf = (char *) alloca (stub_unwind_size);

      /* Read in the stub unwind entries.  */
      bfd_get_section_contents (objfile->obfd.get (), stub_unwind_sec, buf,
				0, stub_unwind_size);

      /* Now convert them into regular unwind entries.  */
      for (i = 0; i < stub_entries; i++, index++)
	{
	  /* Clear out the next unwind entry.  */
	  memset (&ui->table[index], 0, sizeof (struct unwind_table_entry));

	  /* Convert offset & size into region_start and region_end.
	     Stuff away the stub type into "reserved" fields.  */
	  ui->table[index].region_start = bfd_get_32 (objfile->obfd,
						      (bfd_byte *) buf);
	  ui->table[index].region_start += text_offset;
	  buf += 4;
	  ui->table[index].stub_unwind.stub_type = bfd_get_8 (objfile->obfd,
							  (bfd_byte *) buf);
	  buf += 2;
	  ui->table[index].region_end
	    = ui->table[index].region_start + 4 *
	    (bfd_get_16 (objfile->obfd, (bfd_byte *) buf) - 1);
	  buf += 2;
	}

    }

  /* Unwind table needs to be kept sorted.  */
  qsort (ui->table, total_entries, sizeof (struct unwind_table_entry),
	 compare_unwind_entries);

  /* Keep a pointer to the unwind information.  */
  obj_private = hppa_objfile_priv_data.get (objfile);
  if (obj_private == NULL)
    obj_private = hppa_objfile_priv_data.emplace (objfile);

  obj_private->unwind_info = ui;
}

/* Lookup the unwind (stack backtrace) info for the given PC.  We search all
   of the objfiles seeking the unwind table entry for this PC.  Each objfile
   contains a sorted list of struct unwind_table_entry.  Since we do a binary
   search of the unwind tables, we depend upon them to be sorted.  */

struct unwind_table_entry *
find_unwind_entry (CORE_ADDR pc)
{
  int first, middle, last;

  if (hppa_debug)
    gdb_printf (gdb_stdlog, "{ find_unwind_entry %s -> ",
		hex_string (pc));

  /* A function at address 0?  Not in HP-UX!  */
  if (pc == (CORE_ADDR) 0)
    {
      if (hppa_debug)
	gdb_printf (gdb_stdlog, "NULL }\n");
      return NULL;
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      struct hppa_unwind_info *ui;
      ui = NULL;
      struct hppa_objfile_private *priv = hppa_objfile_priv_data.get (objfile);
      if (priv)
	ui = priv->unwind_info;

      if (!ui)
	{
	  read_unwind_info (objfile);
	  priv = hppa_objfile_priv_data.get (objfile);
	  if (priv == NULL)
	    error (_("Internal error reading unwind information."));
	  ui = priv->unwind_info;
	}

      /* First, check the cache.  */

      if (ui->cache
	  && pc >= ui->cache->region_start
	  && pc <= ui->cache->region_end)
	{
	  if (hppa_debug)
	    gdb_printf (gdb_stdlog, "%s (cached) }\n",
			hex_string ((uintptr_t) ui->cache));
	  return ui->cache;
	}

      /* Not in the cache, do a binary search.  */

      first = 0;
      last = ui->last;

      while (first <= last)
	{
	  middle = (first + last) / 2;
	  if (pc >= ui->table[middle].region_start
	      && pc <= ui->table[middle].region_end)
	    {
	      ui->cache = &ui->table[middle];
	      if (hppa_debug)
		gdb_printf (gdb_stdlog, "%s }\n",
			    hex_string ((uintptr_t) ui->cache));
	      return &ui->table[middle];
	    }

	  if (pc < ui->table[middle].region_start)
	    last = middle - 1;
	  else
	    first = middle + 1;
	}
    }

  if (hppa_debug)
    gdb_printf (gdb_stdlog, "NULL (not found) }\n");

  return NULL;
}

/* Implement the stack_frame_destroyed_p gdbarch method.

   The epilogue is defined here as the area either on the `bv' instruction 
   itself or an instruction which destroys the function's stack frame.
   
   We do not assume that the epilogue is at the end of a function as we can
   also have return sequences in the middle of a function.  */

static int
hppa_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned long status;
  unsigned int inst;
  gdb_byte buf[4];

  status = target_read_memory (pc, buf, 4);
  if (status != 0)
    return 0;

  inst = extract_unsigned_integer (buf, 4, byte_order);

  /* The most common way to perform a stack adjustment ldo X(sp),sp 
     We are destroying a stack frame if the offset is negative.  */
  if ((inst & 0xffffc000) == 0x37de0000
      && hppa_extract_14 (inst) < 0)
    return 1;

  /* ldw,mb D(sp),X or ldd,mb D(sp),X */
  if (((inst & 0x0fc010e0) == 0x0fc010e0 
       || (inst & 0x0fc010e0) == 0x0fc010e0)
      && hppa_extract_14 (inst) < 0)
    return 1;

  /* bv %r0(%rp) or bv,n %r0(%rp) */
  if (inst == 0xe840c000 || inst == 0xe840c002)
    return 1;

  return 0;
}

constexpr gdb_byte hppa_break_insn[] = {0x00, 0x01, 0x00, 0x04};

typedef BP_MANIPULATION (hppa_break_insn) hppa_breakpoint;

/* Return the name of a register.  */

static const char *
hppa32_register_name (struct gdbarch *gdbarch, int i)
{
  static const char *names[] = {
    "flags",  "r1",      "rp",     "r3",
    "r4",     "r5",      "r6",     "r7",
    "r8",     "r9",      "r10",    "r11",
    "r12",    "r13",     "r14",    "r15",
    "r16",    "r17",     "r18",    "r19",
    "r20",    "r21",     "r22",    "r23",
    "r24",    "r25",     "r26",    "dp",
    "ret0",   "ret1",    "sp",     "r31",
    "sar",    "pcoqh",   "pcsqh",  "pcoqt",
    "pcsqt",  "eiem",    "iir",    "isr",
    "ior",    "ipsw",    "goto",   "sr4",
    "sr0",    "sr1",     "sr2",    "sr3",
    "sr5",    "sr6",     "sr7",    "cr0",
    "cr8",    "cr9",     "ccr",    "cr12",
    "cr13",   "cr24",    "cr25",   "cr26",
    "mpsfu_high","mpsfu_low","mpsfu_ovflo","pad",
    "fpsr",    "fpe1",   "fpe2",   "fpe3",
    "fpe4",   "fpe5",    "fpe6",   "fpe7",
    "fr4",     "fr4R",   "fr5",    "fr5R",
    "fr6",    "fr6R",    "fr7",    "fr7R",
    "fr8",     "fr8R",   "fr9",    "fr9R",
    "fr10",   "fr10R",   "fr11",   "fr11R",
    "fr12",    "fr12R",  "fr13",   "fr13R",
    "fr14",   "fr14R",   "fr15",   "fr15R",
    "fr16",    "fr16R",  "fr17",   "fr17R",
    "fr18",   "fr18R",   "fr19",   "fr19R",
    "fr20",    "fr20R",  "fr21",   "fr21R",
    "fr22",   "fr22R",   "fr23",   "fr23R",
    "fr24",    "fr24R",  "fr25",   "fr25R",
    "fr26",   "fr26R",   "fr27",   "fr27R",
    "fr28",    "fr28R",  "fr29",   "fr29R",
    "fr30",   "fr30R",   "fr31",   "fr31R"
  };
  gdb_static_assert (ARRAY_SIZE (names) == hppa32_num_regs);
  return names[i];
}

static const char *
hppa64_register_name (struct gdbarch *gdbarch, int i)
{
  static const char *names[] = {
    "flags",  "r1",      "rp",     "r3",
    "r4",     "r5",      "r6",     "r7",
    "r8",     "r9",      "r10",    "r11",
    "r12",    "r13",     "r14",    "r15",
    "r16",    "r17",     "r18",    "r19",
    "r20",    "r21",     "r22",    "r23",
    "r24",    "r25",     "r26",    "dp",
    "ret0",   "ret1",    "sp",     "r31",
    "sar",    "pcoqh",   "pcsqh",  "pcoqt",
    "pcsqt",  "eiem",    "iir",    "isr",
    "ior",    "ipsw",    "goto",   "sr4",
    "sr0",    "sr1",     "sr2",    "sr3",
    "sr5",    "sr6",     "sr7",    "cr0",
    "cr8",    "cr9",     "ccr",    "cr12",
    "cr13",   "cr24",    "cr25",   "cr26",
    "mpsfu_high","mpsfu_low","mpsfu_ovflo","pad",
    "fpsr",    "fpe1",   "fpe2",   "fpe3",
    "fr4",    "fr5",     "fr6",    "fr7",
    "fr8",     "fr9",    "fr10",   "fr11",
    "fr12",   "fr13",    "fr14",   "fr15",
    "fr16",    "fr17",   "fr18",   "fr19",
    "fr20",   "fr21",    "fr22",   "fr23",
    "fr24",    "fr25",   "fr26",   "fr27",
    "fr28",  "fr29",    "fr30",   "fr31"
  };
  gdb_static_assert (ARRAY_SIZE (names) == hppa64_num_regs);
  return names[i];
}

/* Map dwarf DBX register numbers to GDB register numbers.  */
static int
hppa64_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  /* The general registers and the sar are the same in both sets.  */
  if (reg >= 0 && reg <= 32)
    return reg;

  /* fr4-fr31 are mapped from 72 in steps of 2.  */
  if (reg >= 72 && reg < 72 + 28 * 2 && !(reg & 1))
    return HPPA64_FP4_REGNUM + (reg - 72) / 2;

  return -1;
}

/* This function pushes a stack frame with arguments as part of the
   inferior function calling mechanism.

   This is the version of the function for the 32-bit PA machines, in
   which later arguments appear at lower addresses.  (The stack always
   grows towards higher addresses.)

   We simply allocate the appropriate amount of stack space and put
   arguments into their proper slots.  */
   
static CORE_ADDR
hppa32_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			struct regcache *regcache, CORE_ADDR bp_addr,
			int nargs, struct value **args, CORE_ADDR sp,
			function_call_return_method return_method,
			CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Stack base address at which any pass-by-reference parameters are
     stored.  */
  CORE_ADDR struct_end = 0;
  /* Stack base address at which the first parameter is stored.  */
  CORE_ADDR param_end = 0;

  /* Two passes.  First pass computes the location of everything,
     second pass writes the bytes out.  */
  int write_pass;

  /* Global pointer (r19) of the function we are trying to call.  */
  CORE_ADDR gp;

  hppa_gdbarch_tdep *tdep = gdbarch_tdep<hppa_gdbarch_tdep> (gdbarch);

  for (write_pass = 0; write_pass < 2; write_pass++)
    {
      CORE_ADDR struct_ptr = 0;
      /* The first parameter goes into sp-36, each stack slot is 4-bytes.
	 struct_ptr is adjusted for each argument below, so the first
	 argument will end up at sp-36.  */
      CORE_ADDR param_ptr = 32;
      int i;
      int small_struct = 0;

      for (i = 0; i < nargs; i++)
	{
	  struct value *arg = args[i];
	  struct type *type = check_typedef (value_type (arg));
	  /* The corresponding parameter that is pushed onto the
	     stack, and [possibly] passed in a register.  */
	  gdb_byte param_val[8];
	  int param_len;
	  memset (param_val, 0, sizeof param_val);
	  if (type->length () > 8)
	    {
	      /* Large parameter, pass by reference.  Store the value
		 in "struct" area and then pass its address.  */
	      param_len = 4;
	      struct_ptr += align_up (type->length (), 8);
	      if (write_pass)
		write_memory (struct_end - struct_ptr,
			      value_contents (arg).data (), type->length ());
	      store_unsigned_integer (param_val, 4, byte_order,
				      struct_end - struct_ptr);
	    }
	  else if (type->code () == TYPE_CODE_INT
		   || type->code () == TYPE_CODE_ENUM)
	    {
	      /* Integer value store, right aligned.  "unpack_long"
		 takes care of any sign-extension problems.  */
	      param_len = align_up (type->length (), 4);
	      store_unsigned_integer
		(param_val, param_len, byte_order,
		 unpack_long (type, value_contents (arg).data ()));
	    }
	  else if (type->code () == TYPE_CODE_FLT)
	    {
	      /* Floating point value store, right aligned.  */
	      param_len = align_up (type->length (), 4);
	      memcpy (param_val, value_contents (arg).data (), param_len);
	    }
	  else
	    {
	      param_len = align_up (type->length (), 4);

	      /* Small struct value are stored right-aligned.  */
	      memcpy (param_val + param_len - type->length (),
		      value_contents (arg).data (), type->length ());

	      /* Structures of size 5, 6 and 7 bytes are special in that
		 the higher-ordered word is stored in the lower-ordered
		 argument, and even though it is a 8-byte quantity the
		 registers need not be 8-byte aligned.  */
	      if (param_len > 4 && param_len < 8)
		small_struct = 1;
	    }

	  param_ptr += param_len;
	  if (param_len == 8 && !small_struct)
	    param_ptr = align_up (param_ptr, 8);

	  /* First 4 non-FP arguments are passed in gr26-gr23.
	     First 4 32-bit FP arguments are passed in fr4L-fr7L.
	     First 2 64-bit FP arguments are passed in fr5 and fr7.

	     The rest go on the stack, starting at sp-36, towards lower
	     addresses.  8-byte arguments must be aligned to a 8-byte
	     stack boundary.  */
	  if (write_pass)
	    {
	      write_memory (param_end - param_ptr, param_val, param_len);

	      /* There are some cases when we don't know the type
		 expected by the callee (e.g. for variadic functions), so 
		 pass the parameters in both general and fp regs.  */
	      if (param_ptr <= 48)
		{
		  int grreg = 26 - (param_ptr - 36) / 4;
		  int fpLreg = 72 + (param_ptr - 36) / 4 * 2;
		  int fpreg = 74 + (param_ptr - 32) / 8 * 4;

		  regcache->cooked_write (grreg, param_val);
		  regcache->cooked_write (fpLreg, param_val);

		  if (param_len > 4)
		    {
		      regcache->cooked_write (grreg + 1, param_val + 4);

		      regcache->cooked_write (fpreg, param_val);
		      regcache->cooked_write (fpreg + 1, param_val + 4);
		    }
		}
	    }
	}

      /* Update the various stack pointers.  */
      if (!write_pass)
	{
	  struct_end = sp + align_up (struct_ptr, 64);
	  /* PARAM_PTR already accounts for all the arguments passed
	     by the user.  However, the ABI mandates minimum stack
	     space allocations for outgoing arguments.  The ABI also
	     mandates minimum stack alignments which we must
	     preserve.  */
	  param_end = struct_end + align_up (param_ptr, 64);
	}
    }

  /* If a structure has to be returned, set up register 28 to hold its
     address.  */
  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, 28, struct_addr);

  gp = tdep->find_global_pointer (gdbarch, function);

  if (gp != 0)
    regcache_cooked_write_unsigned (regcache, 19, gp);

  /* Set the return address.  */
  if (!gdbarch_push_dummy_code_p (gdbarch))
    regcache_cooked_write_unsigned (regcache, HPPA_RP_REGNUM, bp_addr);

  /* Update the Stack Pointer.  */
  regcache_cooked_write_unsigned (regcache, HPPA_SP_REGNUM, param_end);

  return param_end;
}

/* The 64-bit PA-RISC calling conventions are documented in "64-Bit
   Runtime Architecture for PA-RISC 2.0", which is distributed as part
   as of the HP-UX Software Transition Kit (STK).  This implementation
   is based on version 3.3, dated October 6, 1997.  */

/* Check whether TYPE is an "Integral or Pointer Scalar Type".  */

static int
hppa64_integral_or_pointer_p (const struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_RANGE:
      {
	int len = type->length ();
	return (len == 1 || len == 2 || len == 4 || len == 8);
      }
    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
      return (type->length () == 8);
    default:
      break;
    }

  return 0;
}

/* Check whether TYPE is a "Floating Scalar Type".  */

static int
hppa64_floating_p (const struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_FLT:
      {
	int len = type->length ();
	return (len == 4 || len == 8 || len == 16);
      }
    default:
      break;
    }

  return 0;
}

/* If CODE points to a function entry address, try to look up the corresponding
   function descriptor and return its address instead.  If CODE is not a
   function entry address, then just return it unchanged.  */
static CORE_ADDR
hppa64_convert_code_addr_to_fptr (struct gdbarch *gdbarch, CORE_ADDR code)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct obj_section *sec, *opd;

  sec = find_pc_section (code);

  if (!sec)
    return code;

  /* If CODE is in a data section, assume it's already a fptr.  */
  if (!(sec->the_bfd_section->flags & SEC_CODE))
    return code;

  ALL_OBJFILE_OSECTIONS (sec->objfile, opd)
    {
      if (strcmp (opd->the_bfd_section->name, ".opd") == 0)
	break;
    }

  if (opd < sec->objfile->sections_end)
    {
      for (CORE_ADDR addr = opd->addr (); addr < opd->endaddr (); addr += 2 * 8)
	{
	  ULONGEST opdaddr;
	  gdb_byte tmp[8];

	  if (target_read_memory (addr, tmp, sizeof (tmp)))
	      break;
	  opdaddr = extract_unsigned_integer (tmp, sizeof (tmp), byte_order);

	  if (opdaddr == code)
	    return addr - 16;
	}
    }

  return code;
}

static CORE_ADDR
hppa64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			struct regcache *regcache, CORE_ADDR bp_addr,
			int nargs, struct value **args, CORE_ADDR sp,
			function_call_return_method return_method,
			CORE_ADDR struct_addr)
{
  hppa_gdbarch_tdep *tdep = gdbarch_tdep<hppa_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int i, offset = 0;
  CORE_ADDR gp;

  /* "The outgoing parameter area [...] must be aligned at a 16-byte
     boundary."  */
  sp = align_up (sp, 16);

  for (i = 0; i < nargs; i++)
    {
      struct value *arg = args[i];
      struct type *type = value_type (arg);
      int len = type->length ();
      const bfd_byte *valbuf;
      bfd_byte fptrbuf[8];
      int regnum;

      /* "Each parameter begins on a 64-bit (8-byte) boundary."  */
      offset = align_up (offset, 8);

      if (hppa64_integral_or_pointer_p (type))
	{
	  /* "Integral scalar parameters smaller than 64 bits are
	     padded on the left (i.e., the value is in the
	     least-significant bits of the 64-bit storage unit, and
	     the high-order bits are undefined)."  Therefore we can
	     safely sign-extend them.  */
	  if (len < 8)
	    {
	      arg = value_cast (builtin_type (gdbarch)->builtin_int64, arg);
	      len = 8;
	    }
	}
      else if (hppa64_floating_p (type))
	{
	  if (len > 8)
	    {
	      /* "Quad-precision (128-bit) floating-point scalar
		 parameters are aligned on a 16-byte boundary."  */
	      offset = align_up (offset, 16);

	      /* "Double-extended- and quad-precision floating-point
		 parameters within the first 64 bytes of the parameter
		 list are always passed in general registers."  */
	    }
	  else
	    {
	      if (len == 4)
		{
		  /* "Single-precision (32-bit) floating-point scalar
		     parameters are padded on the left with 32 bits of
		     garbage (i.e., the floating-point value is in the
		     least-significant 32 bits of a 64-bit storage
		     unit)."  */
		  offset += 4;
		}

	      /* "Single- and double-precision floating-point
		 parameters in this area are passed according to the
		 available formal parameter information in a function
		 prototype.  [...]  If no prototype is in scope,
		 floating-point parameters must be passed both in the
		 corresponding general registers and in the
		 corresponding floating-point registers."  */
	      regnum = HPPA64_FP4_REGNUM + offset / 8;

	      if (regnum < HPPA64_FP4_REGNUM + 8)
		{
		  /* "Single-precision floating-point parameters, when
		     passed in floating-point registers, are passed in
		     the right halves of the floating point registers;
		     the left halves are unused."  */
		  regcache->cooked_write_part (regnum, offset % 8, len,
					       value_contents (arg).data ());
		}
	    }
	}
      else
	{
	  if (len > 8)
	    {
	      /* "Aggregates larger than 8 bytes are aligned on a
		 16-byte boundary, possibly leaving an unused argument
		 slot, which is filled with garbage.  If necessary,
		 they are padded on the right (with garbage), to a
		 multiple of 8 bytes."  */
	      offset = align_up (offset, 16);
	    }
	}

      /* If we are passing a function pointer, make sure we pass a function
	 descriptor instead of the function entry address.  */
      if (type->code () == TYPE_CODE_PTR
	  && type->target_type ()->code () == TYPE_CODE_FUNC)
	{
	  ULONGEST codeptr, fptr;

	  codeptr = unpack_long (type, value_contents (arg).data ());
	  fptr = hppa64_convert_code_addr_to_fptr (gdbarch, codeptr);
	  store_unsigned_integer (fptrbuf, type->length (), byte_order,
				  fptr);
	  valbuf = fptrbuf;
	}
      else
	{
	  valbuf = value_contents (arg).data ();
	}

      /* Always store the argument in memory.  */
      write_memory (sp + offset, valbuf, len);

      regnum = HPPA_ARG0_REGNUM - offset / 8;
      while (regnum > HPPA_ARG0_REGNUM - 8 && len > 0)
	{
	  regcache->cooked_write_part (regnum, offset % 8, std::min (len, 8),
				       valbuf);
	  offset += std::min (len, 8);
	  valbuf += std::min (len, 8);
	  len -= std::min (len, 8);
	  regnum--;
	}

      offset += len;
    }

  /* Set up GR29 (%ret1) to hold the argument pointer (ap).  */
  regcache_cooked_write_unsigned (regcache, HPPA_RET1_REGNUM, sp + 64);

  /* Allocate the outgoing parameter area.  Make sure the outgoing
     parameter area is multiple of 16 bytes in length.  */
  sp += std::max (align_up (offset, 16), (ULONGEST) 64);

  /* Allocate 32-bytes of scratch space.  The documentation doesn't
     mention this, but it seems to be needed.  */
  sp += 32;

  /* Allocate the frame marker area.  */
  sp += 16;

  /* If a structure has to be returned, set up GR 28 (%ret0) to hold
     its address.  */
  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, HPPA_RET0_REGNUM, struct_addr);

  /* Set up GR27 (%dp) to hold the global pointer (gp).  */
  gp = tdep->find_global_pointer (gdbarch, function);
  if (gp != 0)
    regcache_cooked_write_unsigned (regcache, HPPA_DP_REGNUM, gp);

  /* Set up GR2 (%rp) to hold the return pointer (rp).  */
  if (!gdbarch_push_dummy_code_p (gdbarch))
    regcache_cooked_write_unsigned (regcache, HPPA_RP_REGNUM, bp_addr);

  /* Set up GR30 to hold the stack pointer (sp).  */
  regcache_cooked_write_unsigned (regcache, HPPA_SP_REGNUM, sp);

  return sp;
}


/* Handle 32/64-bit struct return conventions.  */

static enum return_value_convention
hppa32_return_value (struct gdbarch *gdbarch, struct value *function,
		     struct type *type, struct regcache *regcache,
		     gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (type->length () <= 2 * 4)
    {
      /* The value always lives in the right hand end of the register
	 (or register pair)?  */
      int b;
      int reg = type->code () == TYPE_CODE_FLT ? HPPA_FP4_REGNUM : 28;
      int part = type->length () % 4;
      /* The left hand register contains only part of the value,
	 transfer that first so that the rest can be xfered as entire
	 4-byte registers.  */
      if (part > 0)
	{
	  if (readbuf != NULL)
	    regcache->cooked_read_part (reg, 4 - part, part, readbuf);
	  if (writebuf != NULL)
	    regcache->cooked_write_part (reg, 4 - part, part, writebuf);
	  reg++;
	}
      /* Now transfer the remaining register values.  */
      for (b = part; b < type->length (); b += 4)
	{
	  if (readbuf != NULL)
	    regcache->cooked_read (reg, readbuf + b);
	  if (writebuf != NULL)
	    regcache->cooked_write (reg, writebuf + b);
	  reg++;
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else
    return RETURN_VALUE_STRUCT_CONVENTION;
}

static enum return_value_convention
hppa64_return_value (struct gdbarch *gdbarch, struct value *function,
		     struct type *type, struct regcache *regcache,
		     gdb_byte *readbuf, const gdb_byte *writebuf)
{
  int len = type->length ();
  int regnum, offset;

  if (len > 16)
    {
      /* All return values larger than 128 bits must be aggregate
	 return values.  */
      gdb_assert (!hppa64_integral_or_pointer_p (type));
      gdb_assert (!hppa64_floating_p (type));

      /* "Aggregate return values larger than 128 bits are returned in
	 a buffer allocated by the caller.  The address of the buffer
	 must be passed in GR 28."  */
      return RETURN_VALUE_STRUCT_CONVENTION;
    }

  if (hppa64_integral_or_pointer_p (type))
    {
      /* "Integral return values are returned in GR 28.  Values
	 smaller than 64 bits are padded on the left (with garbage)."  */
      regnum = HPPA_RET0_REGNUM;
      offset = 8 - len;
    }
  else if (hppa64_floating_p (type))
    {
      if (len > 8)
	{
	  /* "Double-extended- and quad-precision floating-point
	     values are returned in GRs 28 and 29.  The sign,
	     exponent, and most-significant bits of the mantissa are
	     returned in GR 28; the least-significant bits of the
	     mantissa are passed in GR 29.  For double-extended
	     precision values, GR 29 is padded on the right with 48
	     bits of garbage."  */
	  regnum = HPPA_RET0_REGNUM;
	  offset = 0;
	}
      else
	{
	  /* "Single-precision and double-precision floating-point
	     return values are returned in FR 4R (single precision) or
	     FR 4 (double-precision)."  */
	  regnum = HPPA64_FP4_REGNUM;
	  offset = 8 - len;
	}
    }
  else
    {
      /* "Aggregate return values up to 64 bits in size are returned
	 in GR 28.  Aggregates smaller than 64 bits are left aligned
	 in the register; the pad bits on the right are undefined."

	 "Aggregate return values between 65 and 128 bits are returned
	 in GRs 28 and 29.  The first 64 bits are placed in GR 28, and
	 the remaining bits are placed, left aligned, in GR 29.  The
	 pad bits on the right of GR 29 (if any) are undefined."  */
      regnum = HPPA_RET0_REGNUM;
      offset = 0;
    }

  if (readbuf)
    {
      while (len > 0)
	{
	  regcache->cooked_read_part (regnum, offset, std::min (len, 8),
				      readbuf);
	  readbuf += std::min (len, 8);
	  len -= std::min (len, 8);
	  regnum++;
	}
    }

  if (writebuf)
    {
      while (len > 0)
	{
	  regcache->cooked_write_part (regnum, offset, std::min (len, 8),
				       writebuf);
	  writebuf += std::min (len, 8);
	  len -= std::min (len, 8);
	  regnum++;
	}
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}


static CORE_ADDR
hppa32_convert_from_func_ptr_addr (struct gdbarch *gdbarch, CORE_ADDR addr,
				   struct target_ops *targ)
{
  if (addr & 2)
    {
      struct type *func_ptr_type = builtin_type (gdbarch)->builtin_func_ptr;
      CORE_ADDR plabel = addr & ~3;
      return read_memory_typed_address (plabel, func_ptr_type);
    }

  return addr;
}

static CORE_ADDR
hppa32_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  /* HP frames are 64-byte (or cache line) aligned (yes that's _byte_
     and not _bit_)!  */
  return align_up (addr, 64);
}

/* Force all frames to 16-byte alignment.  Better safe than sorry.  */

static CORE_ADDR
hppa64_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  /* Just always 16-byte align.  */
  return align_up (addr, 16);
}

static CORE_ADDR
hppa_read_pc (readable_regcache *regcache)
{
  ULONGEST ipsw;
  ULONGEST pc;

  regcache->cooked_read (HPPA_IPSW_REGNUM, &ipsw);
  regcache->cooked_read (HPPA_PCOQ_HEAD_REGNUM, &pc);

  /* If the current instruction is nullified, then we are effectively
     still executing the previous instruction.  Pretend we are still
     there.  This is needed when single stepping; if the nullified
     instruction is on a different line, we don't want GDB to think
     we've stepped onto that line.  */
  if (ipsw & 0x00200000)
    pc -= 4;

  return pc & ~0x3;
}

void
hppa_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  regcache_cooked_write_unsigned (regcache, HPPA_PCOQ_HEAD_REGNUM, pc);
  regcache_cooked_write_unsigned (regcache, HPPA_PCOQ_TAIL_REGNUM, pc + 4);
}

/* For the given instruction (INST), return any adjustment it makes
   to the stack pointer or zero for no adjustment.

   This only handles instructions commonly found in prologues.  */

static int
prologue_inst_adjust_sp (unsigned long inst)
{
  /* This must persist across calls.  */
  static int save_high21;

  /* The most common way to perform a stack adjustment ldo X(sp),sp */
  if ((inst & 0xffffc000) == 0x37de0000)
    return hppa_extract_14 (inst);

  /* stwm X,D(sp) */
  if ((inst & 0xffe00000) == 0x6fc00000)
    return hppa_extract_14 (inst);

  /* std,ma X,D(sp) */
  if ((inst & 0xffe00008) == 0x73c00008)
    return (inst & 0x1 ? -(1 << 13) : 0) | (((inst >> 4) & 0x3ff) << 3);

  /* addil high21,%r30; ldo low11,(%r1),%r30)
     save high bits in save_high21 for later use.  */
  if ((inst & 0xffe00000) == 0x2bc00000)
    {
      save_high21 = hppa_extract_21 (inst);
      return 0;
    }

  if ((inst & 0xffff0000) == 0x343e0000)
    return save_high21 + hppa_extract_14 (inst);

  /* fstws as used by the HP compilers.  */
  if ((inst & 0xffffffe0) == 0x2fd01220)
    return hppa_extract_5_load (inst);

  /* No adjustment.  */
  return 0;
}

/* Return nonzero if INST is a branch of some kind, else return zero.  */

static int
is_branch (unsigned long inst)
{
  switch (inst >> 26)
    {
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2f:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x38:
    case 0x39:
    case 0x3a:
    case 0x3b:
      return 1;

    default:
      return 0;
    }
}

/* Return the register number for a GR which is saved by INST or
   zero if INST does not save a GR.

   Referenced from:

     parisc 1.1:
     https://parisc.wiki.kernel.org/images-parisc/6/68/Pa11_acd.pdf

     parisc 2.0:
     https://parisc.wiki.kernel.org/images-parisc/7/73/Parisc2.0.pdf

     According to Table 6-5 of Chapter 6 (Memory Reference Instructions)
     on page 106 in parisc 2.0, all instructions for storing values from
     the general registers are:

       Store:          stb, sth, stw, std (according to Chapter 7, they
		       are only in both "inst >> 26" and "inst >> 6".
       Store Absolute: stwa, stda (according to Chapter 7, they are only
		       in "inst >> 6".
       Store Bytes:    stby, stdby (according to Chapter 7, they are
		       only in "inst >> 6").

   For (inst >> 26), according to Chapter 7:

     The effective memory reference address is formed by the addition
     of an immediate displacement to a base value.

    - stb: 0x18, store a byte from a general register.

    - sth: 0x19, store a halfword from a general register.

    - stw: 0x1a, store a word from a general register.

    - stwm: 0x1b, store a word from a general register and perform base
      register modification (2.0 will still treat it as stw).

    - std: 0x1c, store a doubleword from a general register (2.0 only).

    - stw: 0x1f, store a word from a general register (2.0 only).

   For (inst >> 6) when ((inst >> 26) == 0x03), according to Chapter 7:

     The effective memory reference address is formed by the addition
     of an index value to a base value specified in the instruction.

    - stb: 0x08, store a byte from a general register (1.1 calls stbs).

    - sth: 0x09, store a halfword from a general register (1.1 calls
      sths).

    - stw: 0x0a, store a word from a general register (1.1 calls stws).

    - std: 0x0b: store a doubleword from a general register (2.0 only)

     Implement fast byte moves (stores) to unaligned word or doubleword
     destination.

    - stby: 0x0c, for unaligned word (1.1 calls stbys).

    - stdby: 0x0d for unaligned doubleword (2.0 only).

     Store a word or doubleword using an absolute memory address formed
     using short or long displacement or indexed

    - stwa: 0x0e, store a word from a general register to an absolute
      address (1.0 calls stwas).

    - stda: 0x0f, store a doubleword from a general register to an
      absolute address (2.0 only).  */

static int
inst_saves_gr (unsigned long inst)
{
  switch ((inst >> 26) & 0x0f)
    {
      case 0x03:
	switch ((inst >> 6) & 0x0f)
	  {
	    case 0x08:
	    case 0x09:
	    case 0x0a:
	    case 0x0b:
	    case 0x0c:
	    case 0x0d:
	    case 0x0e:
	    case 0x0f:
	      return hppa_extract_5R_store (inst);
	    default:
	      return 0;
	  }
      case 0x18:
      case 0x19:
      case 0x1a:
      case 0x1b:
      case 0x1c:
      /* no 0x1d or 0x1e -- according to parisc 2.0 document */
      case 0x1f:
	return hppa_extract_5R_store (inst);
      default:
	return 0;
    }
}

/* Return the register number for a FR which is saved by INST or
   zero it INST does not save a FR.

   Note we only care about full 64bit register stores (that's the only
   kind of stores the prologue will use).

   FIXME: What about argument stores with the HP compiler in ANSI mode? */

static int
inst_saves_fr (unsigned long inst)
{
  /* Is this an FSTD?  */
  if ((inst & 0xfc00dfc0) == 0x2c001200)
    return hppa_extract_5r_store (inst);
  if ((inst & 0xfc000002) == 0x70000002)
    return hppa_extract_5R_store (inst);
  /* Is this an FSTW?  */
  if ((inst & 0xfc00df80) == 0x24001200)
    return hppa_extract_5r_store (inst);
  if ((inst & 0xfc000002) == 0x7c000000)
    return hppa_extract_5R_store (inst);
  return 0;
}

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.

   Use information in the unwind table to determine what exactly should
   be in the prologue.  */


static CORE_ADDR
skip_prologue_hard_way (struct gdbarch *gdbarch, CORE_ADDR pc,
			int stop_before_branch)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  CORE_ADDR orig_pc = pc;
  unsigned long inst, stack_remaining, save_gr, save_fr, save_rp, save_sp;
  unsigned long args_stored, status, i, restart_gr, restart_fr;
  struct unwind_table_entry *u;
  int final_iteration;

  restart_gr = 0;
  restart_fr = 0;

restart:
  u = find_unwind_entry (pc);
  if (!u)
    return pc;

  /* If we are not at the beginning of a function, then return now.  */
  if ((pc & ~0x3) != u->region_start)
    return pc;

  /* This is how much of a frame adjustment we need to account for.  */
  stack_remaining = u->Total_frame_size << 3;

  /* Magic register saves we want to know about.  */
  save_rp = u->Save_RP;
  save_sp = u->Save_SP;

  /* An indication that args may be stored into the stack.  Unfortunately
     the HPUX compilers tend to set this in cases where no args were
     stored too!.  */
  args_stored = 1;

  /* Turn the Entry_GR field into a bitmask.  */
  save_gr = 0;
  for (i = 3; i < u->Entry_GR + 3; i++)
    {
      /* Frame pointer gets saved into a special location.  */
      if (u->Save_SP && i == HPPA_FP_REGNUM)
	continue;

      save_gr |= (1 << i);
    }
  save_gr &= ~restart_gr;

  /* Turn the Entry_FR field into a bitmask too.  */
  save_fr = 0;
  for (i = 12; i < u->Entry_FR + 12; i++)
    save_fr |= (1 << i);
  save_fr &= ~restart_fr;

  final_iteration = 0;

  /* Loop until we find everything of interest or hit a branch.

     For unoptimized GCC code and for any HP CC code this will never ever
     examine any user instructions.

     For optimized GCC code we're faced with problems.  GCC will schedule
     its prologue and make prologue instructions available for delay slot
     filling.  The end result is user code gets mixed in with the prologue
     and a prologue instruction may be in the delay slot of the first branch
     or call.

     Some unexpected things are expected with debugging optimized code, so
     we allow this routine to walk past user instructions in optimized
     GCC code.  */
  while (save_gr || save_fr || save_rp || save_sp || stack_remaining > 0
	 || args_stored)
    {
      unsigned int reg_num;
      unsigned long old_stack_remaining, old_save_gr, old_save_fr;
      unsigned long old_save_rp, old_save_sp, next_inst;

      /* Save copies of all the triggers so we can compare them later
	 (only for HPC).  */
      old_save_gr = save_gr;
      old_save_fr = save_fr;
      old_save_rp = save_rp;
      old_save_sp = save_sp;
      old_stack_remaining = stack_remaining;

      status = target_read_memory (pc, buf, 4);
      inst = extract_unsigned_integer (buf, 4, byte_order);

      /* Yow! */
      if (status != 0)
	return pc;

      /* Note the interesting effects of this instruction.  */
      stack_remaining -= prologue_inst_adjust_sp (inst);

      /* There are limited ways to store the return pointer into the
	 stack.  */
      if (inst == 0x6bc23fd9 || inst == 0x0fc212c1 || inst == 0x73c23fe1)
	save_rp = 0;

      /* These are the only ways we save SP into the stack.  At this time
	 the HP compilers never bother to save SP into the stack.  */
      if ((inst & 0xffffc000) == 0x6fc10000
	  || (inst & 0xffffc00c) == 0x73c10008)
	save_sp = 0;

      /* Are we loading some register with an offset from the argument
	 pointer?  */
      if ((inst & 0xffe00000) == 0x37a00000
	  || (inst & 0xffffffe0) == 0x081d0240)
	{
	  pc += 4;
	  continue;
	}

      /* Account for general and floating-point register saves.  */
      reg_num = inst_saves_gr (inst);
      save_gr &= ~(1 << reg_num);

      /* Ugh.  Also account for argument stores into the stack.
	 Unfortunately args_stored only tells us that some arguments
	 where stored into the stack.  Not how many or what kind!

	 This is a kludge as on the HP compiler sets this bit and it
	 never does prologue scheduling.  So once we see one, skip past
	 all of them.   We have similar code for the fp arg stores below.

	 FIXME.  Can still die if we have a mix of GR and FR argument
	 stores!  */
      if (reg_num >= (gdbarch_ptr_bit (gdbarch) == 64 ? 19 : 23)
	  && reg_num <= 26)
	{
	  while (reg_num >= (gdbarch_ptr_bit (gdbarch) == 64 ? 19 : 23)
		 && reg_num <= 26)
	    {
	      pc += 4;
	      status = target_read_memory (pc, buf, 4);
	      inst = extract_unsigned_integer (buf, 4, byte_order);
	      if (status != 0)
		return pc;
	      reg_num = inst_saves_gr (inst);
	    }
	  args_stored = 0;
	  continue;
	}

      reg_num = inst_saves_fr (inst);
      save_fr &= ~(1 << reg_num);

      status = target_read_memory (pc + 4, buf, 4);
      next_inst = extract_unsigned_integer (buf, 4, byte_order);

      /* Yow! */
      if (status != 0)
	return pc;

      /* We've got to be read to handle the ldo before the fp register
	 save.  */
      if ((inst & 0xfc000000) == 0x34000000
	  && inst_saves_fr (next_inst) >= 4
	  && inst_saves_fr (next_inst)
	       <= (gdbarch_ptr_bit (gdbarch) == 64 ? 11 : 7))
	{
	  /* So we drop into the code below in a reasonable state.  */
	  reg_num = inst_saves_fr (next_inst);
	  pc -= 4;
	}

      /* Ugh.  Also account for argument stores into the stack.
	 This is a kludge as on the HP compiler sets this bit and it
	 never does prologue scheduling.  So once we see one, skip past
	 all of them.  */
      if (reg_num >= 4
	  && reg_num <= (gdbarch_ptr_bit (gdbarch) == 64 ? 11 : 7))
	{
	  while (reg_num >= 4
		 && reg_num
		      <= (gdbarch_ptr_bit (gdbarch) == 64 ? 11 : 7))
	    {
	      pc += 8;
	      status = target_read_memory (pc, buf, 4);
	      inst = extract_unsigned_integer (buf, 4, byte_order);
	      if (status != 0)
		return pc;
	      if ((inst & 0xfc000000) != 0x34000000)
		break;
	      status = target_read_memory (pc + 4, buf, 4);
	      next_inst = extract_unsigned_integer (buf, 4, byte_order);
	      if (status != 0)
		return pc;
	      reg_num = inst_saves_fr (next_inst);
	    }
	  args_stored = 0;
	  continue;
	}

      /* Quit if we hit any kind of branch.  This can happen if a prologue
	 instruction is in the delay slot of the first call/branch.  */
      if (is_branch (inst) && stop_before_branch)
	break;

      /* What a crock.  The HP compilers set args_stored even if no
	 arguments were stored into the stack (boo hiss).  This could
	 cause this code to then skip a bunch of user insns (up to the
	 first branch).

	 To combat this we try to identify when args_stored was bogusly
	 set and clear it.   We only do this when args_stored is nonzero,
	 all other resources are accounted for, and nothing changed on
	 this pass.  */
      if (args_stored
       && !(save_gr || save_fr || save_rp || save_sp || stack_remaining > 0)
	  && old_save_gr == save_gr && old_save_fr == save_fr
	  && old_save_rp == save_rp && old_save_sp == save_sp
	  && old_stack_remaining == stack_remaining)
	break;

      /* Bump the PC.  */
      pc += 4;

      /* !stop_before_branch, so also look at the insn in the delay slot 
	 of the branch.  */
      if (final_iteration)
	break;
      if (is_branch (inst))
	final_iteration = 1;
    }

  /* We've got a tentative location for the end of the prologue.  However
     because of limitations in the unwind descriptor mechanism we may
     have went too far into user code looking for the save of a register
     that does not exist.  So, if there registers we expected to be saved
     but never were, mask them out and restart.

     This should only happen in optimized code, and should be very rare.  */
  if (save_gr || (save_fr && !(restart_fr || restart_gr)))
    {
      pc = orig_pc;
      restart_gr = save_gr;
      restart_fr = save_fr;
      goto restart;
    }

  return pc;
}


/* Return the address of the PC after the last prologue instruction if
   we can determine it from the debug symbols.  Else return zero.  */

static CORE_ADDR
after_prologue (CORE_ADDR pc)
{
  struct symtab_and_line sal;
  CORE_ADDR func_addr, func_end;

  /* If we can not find the symbol in the partial symbol table, then
     there is no hope we can determine the function's start address
     with this code.  */
  if (!find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    return 0;

  /* Get the line associated with FUNC_ADDR.  */
  sal = find_pc_line (func_addr, 0);

  /* There are only two cases to consider.  First, the end of the source line
     is within the function bounds.  In that case we return the end of the
     source line.  Second is the end of the source line extends beyond the
     bounds of the current function.  We need to use the slow code to
     examine instructions in that case.

     Anything else is simply a bug elsewhere.  Fixing it here is absolutely
     the wrong thing to do.  In fact, it should be entirely possible for this
     function to always return zero since the slow instruction scanning code
     is supposed to *always* work.  If it does not, then it is a bug.  */
  if (sal.end < func_end)
    return sal.end;
  else
    return 0;
}

/* To skip prologues, I use this predicate.  Returns either PC itself
   if the code at PC does not look like a function prologue; otherwise
   returns an address that (if we're lucky) follows the prologue.
   
   hppa_skip_prologue is called by gdb to place a breakpoint in a function.
   It doesn't necessarily skips all the insns in the prologue.  In fact
   we might not want to skip all the insns because a prologue insn may
   appear in the delay slot of the first branch, and we don't want to
   skip over the branch in that case.  */

static CORE_ADDR
hppa_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR post_prologue_pc;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */

  post_prologue_pc = after_prologue (pc);

  /* If after_prologue returned a useful address, then use it.  Else
     fall back on the instruction skipping code.

     Some folks have claimed this causes problems because the breakpoint
     may be the first instruction of the prologue.  If that happens, then
     the instruction skipping code has a bug that needs to be fixed.  */
  if (post_prologue_pc != 0)
    return std::max (pc, post_prologue_pc);
  else
    return (skip_prologue_hard_way (gdbarch, pc, 1));
}

/* Return an unwind entry that falls within the frame's code block.  */

static struct unwind_table_entry *
hppa_find_unwind_entry_in_block (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_address_in_block (this_frame);

  /* FIXME drow/20070101: Calling gdbarch_addr_bits_remove on the
     result of get_frame_address_in_block implies a problem.
     The bits should have been removed earlier, before the return
     value of gdbarch_unwind_pc.  That might be happening already;
     if it isn't, it should be fixed.  Then this call can be
     removed.  */
  pc = gdbarch_addr_bits_remove (get_frame_arch (this_frame), pc);
  return find_unwind_entry (pc);
}

struct hppa_frame_cache
{
  CORE_ADDR base;
  trad_frame_saved_reg *saved_regs;
};

static struct hppa_frame_cache *
hppa_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int word_size = gdbarch_ptr_bit (gdbarch) / 8;
  struct hppa_frame_cache *cache;
  long saved_gr_mask;
  long saved_fr_mask;
  long frame_size;
  struct unwind_table_entry *u;
  CORE_ADDR prologue_end;
  int fp_in_r1 = 0;
  int i;

  if (hppa_debug)
    gdb_printf (gdb_stdlog, "{ hppa_frame_cache (frame=%d) -> ",
		frame_relative_level(this_frame));

  if ((*this_cache) != NULL)
    {
      if (hppa_debug)
	gdb_printf (gdb_stdlog, "base=%s (cached) }",
		    paddress (gdbarch, ((struct hppa_frame_cache *)*this_cache)->base));
      return (struct hppa_frame_cache *) (*this_cache);
    }
  cache = FRAME_OBSTACK_ZALLOC (struct hppa_frame_cache);
  (*this_cache) = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Yow! */
  u = hppa_find_unwind_entry_in_block (this_frame);
  if (!u)
    {
      if (hppa_debug)
	gdb_printf (gdb_stdlog, "base=NULL (no unwind entry) }");
      return (struct hppa_frame_cache *) (*this_cache);
    }

  /* Turn the Entry_GR field into a bitmask.  */
  saved_gr_mask = 0;
  for (i = 3; i < u->Entry_GR + 3; i++)
    {
      /* Frame pointer gets saved into a special location.  */
      if (u->Save_SP && i == HPPA_FP_REGNUM)
	continue;
	
      saved_gr_mask |= (1 << i);
    }

  /* Turn the Entry_FR field into a bitmask too.  */
  saved_fr_mask = 0;
  for (i = 12; i < u->Entry_FR + 12; i++)
    saved_fr_mask |= (1 << i);

  /* Loop until we find everything of interest or hit a branch.

     For unoptimized GCC code and for any HP CC code this will never ever
     examine any user instructions.

     For optimized GCC code we're faced with problems.  GCC will schedule
     its prologue and make prologue instructions available for delay slot
     filling.  The end result is user code gets mixed in with the prologue
     and a prologue instruction may be in the delay slot of the first branch
     or call.

     Some unexpected things are expected with debugging optimized code, so
     we allow this routine to walk past user instructions in optimized
     GCC code.  */
  {
    int final_iteration = 0;
    CORE_ADDR pc, start_pc, end_pc;
    int looking_for_sp = u->Save_SP;
    int looking_for_rp = u->Save_RP;
    int fp_loc = -1;

    /* We have to use skip_prologue_hard_way instead of just 
       skip_prologue_using_sal, in case we stepped into a function without
       symbol information.  hppa_skip_prologue also bounds the returned
       pc by the passed in pc, so it will not return a pc in the next
       function.
       
       We used to call hppa_skip_prologue to find the end of the prologue,
       but if some non-prologue instructions get scheduled into the prologue,
       and the program is compiled with debug information, the "easy" way
       in hppa_skip_prologue will return a prologue end that is too early
       for us to notice any potential frame adjustments.  */

    /* We used to use get_frame_func to locate the beginning of the
       function to pass to skip_prologue.  However, when objects are
       compiled without debug symbols, get_frame_func can return the wrong
       function (or 0).  We can do better than that by using unwind records.
       This only works if the Region_description of the unwind record
       indicates that it includes the entry point of the function.
       HP compilers sometimes generate unwind records for regions that
       do not include the entry or exit point of a function.  GNU tools
       do not do this.  */

    if ((u->Region_description & 0x2) == 0)
      start_pc = u->region_start;
    else
      start_pc = get_frame_func (this_frame);

    prologue_end = skip_prologue_hard_way (gdbarch, start_pc, 0);
    end_pc = get_frame_pc (this_frame);

    if (prologue_end != 0 && end_pc > prologue_end)
      end_pc = prologue_end;

    frame_size = 0;

    for (pc = start_pc;
	 ((saved_gr_mask || saved_fr_mask
	   || looking_for_sp || looking_for_rp
	   || frame_size < (u->Total_frame_size << 3))
	  && pc < end_pc);
	 pc += 4)
      {
	int reg;
	gdb_byte buf4[4];
	long inst;

	if (!safe_frame_unwind_memory (this_frame, pc, buf4))
	  {
	    error (_("Cannot read instruction at %s."),
		   paddress (gdbarch, pc));
	    return (struct hppa_frame_cache *) (*this_cache);
	  }

	inst = extract_unsigned_integer (buf4, sizeof buf4, byte_order);

	/* Note the interesting effects of this instruction.  */
	frame_size += prologue_inst_adjust_sp (inst);
	
	/* There are limited ways to store the return pointer into the
	   stack.  */
	if (inst == 0x6bc23fd9) /* stw rp,-0x14(sr0,sp) */
	  {
	    looking_for_rp = 0;
	    cache->saved_regs[HPPA_RP_REGNUM].set_addr (-20);
	  }
	else if (inst == 0x6bc23fd1) /* stw rp,-0x18(sr0,sp) */
	  {
	    looking_for_rp = 0;
	    cache->saved_regs[HPPA_RP_REGNUM].set_addr (-24);
	  }
	else if (inst == 0x0fc212c1 
		 || inst == 0x73c23fe1) /* std rp,-0x10(sr0,sp) */
	  {
	    looking_for_rp = 0;
	    cache->saved_regs[HPPA_RP_REGNUM].set_addr (-16);
	  }
	
	/* Check to see if we saved SP into the stack.  This also
	   happens to indicate the location of the saved frame
	   pointer.  */
	if ((inst & 0xffffc000) == 0x6fc10000  /* stw,ma r1,N(sr0,sp) */
	    || (inst & 0xffffc00c) == 0x73c10008) /* std,ma r1,N(sr0,sp) */
	  {
	    looking_for_sp = 0;
	    cache->saved_regs[HPPA_FP_REGNUM].set_addr (0);
	  }
	else if (inst == 0x08030241) /* copy %r3, %r1 */
	  {
	    fp_in_r1 = 1;
	  }
	
	/* Account for general and floating-point register saves.  */
	reg = inst_saves_gr (inst);
	if (reg >= 3 && reg <= 18
	    && (!u->Save_SP || reg != HPPA_FP_REGNUM))
	  {
	    saved_gr_mask &= ~(1 << reg);
	    if ((inst >> 26) == 0x1b && hppa_extract_14 (inst) >= 0)
	      /* stwm with a positive displacement is a _post_
		 _modify_.  */
	      cache->saved_regs[reg].set_addr (0);
	    else if ((inst & 0xfc00000c) == 0x70000008)
	      /* A std has explicit post_modify forms.  */
	      cache->saved_regs[reg].set_addr (0);
	    else
	      {
		CORE_ADDR offset;
		
		if ((inst >> 26) == 0x1c)
		  offset = (inst & 0x1 ? -(1 << 13) : 0)
		    | (((inst >> 4) & 0x3ff) << 3);
		else if ((inst >> 26) == 0x03)
		  offset = hppa_low_hppa_sign_extend (inst & 0x1f, 5);
		else
		  offset = hppa_extract_14 (inst);
		
		/* Handle code with and without frame pointers.  */
		if (u->Save_SP)
		  cache->saved_regs[reg].set_addr (offset);
		else
		  cache->saved_regs[reg].set_addr ((u->Total_frame_size << 3)
						   + offset);
	      }
	  }

	/* GCC handles callee saved FP regs a little differently.  
	   
	   It emits an instruction to put the value of the start of
	   the FP store area into %r1.  It then uses fstds,ma with a
	   basereg of %r1 for the stores.

	   HP CC emits them at the current stack pointer modifying the
	   stack pointer as it stores each register.  */
	
	/* ldo X(%r3),%r1 or ldo X(%r30),%r1.  */
	if ((inst & 0xffffc000) == 0x34610000
	    || (inst & 0xffffc000) == 0x37c10000)
	  fp_loc = hppa_extract_14 (inst);
	
	reg = inst_saves_fr (inst);
	if (reg >= 12 && reg <= 21)
	  {
	    /* Note +4 braindamage below is necessary because the FP
	       status registers are internally 8 registers rather than
	       the expected 4 registers.  */
	    saved_fr_mask &= ~(1 << reg);
	    if (fp_loc == -1)
	      {
		/* 1st HP CC FP register store.  After this
		   instruction we've set enough state that the GCC and
		   HPCC code are both handled in the same manner.  */
		cache->saved_regs[reg + HPPA_FP4_REGNUM + 4].set_addr (0);
		fp_loc = 8;
	      }
	    else
	      {
		cache->saved_regs[reg + HPPA_FP0_REGNUM + 4].set_addr (fp_loc);
		fp_loc += 8;
	      }
	  }
	
	/* Quit if we hit any kind of branch the previous iteration.  */
	if (final_iteration)
	  break;
	/* We want to look precisely one instruction beyond the branch
	   if we have not found everything yet.  */
	if (is_branch (inst))
	  final_iteration = 1;
      }
  }

  {
    /* The frame base always represents the value of %sp at entry to
       the current function (and is thus equivalent to the "saved"
       stack pointer.  */
    CORE_ADDR this_sp = get_frame_register_unsigned (this_frame,
						     HPPA_SP_REGNUM);
    CORE_ADDR fp;

    if (hppa_debug)
      gdb_printf (gdb_stdlog, " (this_sp=%s, pc=%s, "
		  "prologue_end=%s) ",
		  paddress (gdbarch, this_sp),
		  paddress (gdbarch, get_frame_pc (this_frame)),
		  paddress (gdbarch, prologue_end));

     /* Check to see if a frame pointer is available, and use it for
	frame unwinding if it is.
 
	There are some situations where we need to rely on the frame
	pointer to do stack unwinding.  For example, if a function calls
	alloca (), the stack pointer can get adjusted inside the body of
	the function.  In this case, the ABI requires that the compiler
	maintain a frame pointer for the function.
 
	The unwind record has a flag (alloca_frame) that indicates that
	a function has a variable frame; unfortunately, gcc/binutils 
	does not set this flag.  Instead, whenever a frame pointer is used
	and saved on the stack, the Save_SP flag is set.  We use this to
	decide whether to use the frame pointer for unwinding.
	
	TODO: For the HP compiler, maybe we should use the alloca_frame flag 
	instead of Save_SP.  */
 
     fp = get_frame_register_unsigned (this_frame, HPPA_FP_REGNUM);

     if (u->alloca_frame)
       fp -= u->Total_frame_size << 3;
 
     if (get_frame_pc (this_frame) >= prologue_end
	 && (u->Save_SP || u->alloca_frame) && fp != 0)
      {
	cache->base = fp;
 
	if (hppa_debug)
	  gdb_printf (gdb_stdlog, " (base=%s) [frame pointer]",
		      paddress (gdbarch, cache->base));
      }
     else if (u->Save_SP 
	      && cache->saved_regs[HPPA_SP_REGNUM].is_addr ())
      {
	    /* Both we're expecting the SP to be saved and the SP has been
	       saved.  The entry SP value is saved at this frame's SP
	       address.  */
	    cache->base = read_memory_integer (this_sp, word_size, byte_order);

	    if (hppa_debug)
	      gdb_printf (gdb_stdlog, " (base=%s) [saved]",
			  paddress (gdbarch, cache->base));
      }
     else
      {
	/* The prologue has been slowly allocating stack space.  Adjust
	   the SP back.  */
	cache->base = this_sp - frame_size;
	if (hppa_debug)
	  gdb_printf (gdb_stdlog, " (base=%s) [unwind adjust]",
		      paddress (gdbarch, cache->base));

      }
    cache->saved_regs[HPPA_SP_REGNUM].set_value (cache->base);
  }

  /* The PC is found in the "return register", "Millicode" uses "r31"
     as the return register while normal code uses "rp".  */
  if (u->Millicode)
    {
      if (cache->saved_regs[31].is_addr ())
	{
	  cache->saved_regs[HPPA_PCOQ_HEAD_REGNUM] = cache->saved_regs[31];
	  if (hppa_debug)
	    gdb_printf (gdb_stdlog, " (pc=r31) [stack] } ");
	}
      else
	{
	  ULONGEST r31 = get_frame_register_unsigned (this_frame, 31);
	  cache->saved_regs[HPPA_PCOQ_HEAD_REGNUM].set_value (r31);
	  if (hppa_debug)
	    gdb_printf (gdb_stdlog, " (pc=r31) [frame] } ");
	}
    }
  else
    {
      if (cache->saved_regs[HPPA_RP_REGNUM].is_addr ())
	{
	  cache->saved_regs[HPPA_PCOQ_HEAD_REGNUM] = 
	    cache->saved_regs[HPPA_RP_REGNUM];
	  if (hppa_debug)
	    gdb_printf (gdb_stdlog, " (pc=rp) [stack] } ");
	}
      else
	{
	  ULONGEST rp = get_frame_register_unsigned (this_frame,
						     HPPA_RP_REGNUM);
	  cache->saved_regs[HPPA_PCOQ_HEAD_REGNUM].set_value (rp);
	  if (hppa_debug)
	    gdb_printf (gdb_stdlog, " (pc=rp) [frame] } ");
	}
    }

  /* If Save_SP is set, then we expect the frame pointer to be saved in the
     frame.  However, there is a one-insn window where we haven't saved it
     yet, but we've already clobbered it.  Detect this case and fix it up.

     The prologue sequence for frame-pointer functions is:
	0: stw %rp, -20(%sp)
	4: copy %r3, %r1
	8: copy %sp, %r3
	c: stw,ma %r1, XX(%sp)

     So if we are at offset c, the r3 value that we want is not yet saved
     on the stack, but it's been overwritten.  The prologue analyzer will
     set fp_in_r1 when it sees the copy insn so we know to get the value 
     from r1 instead.  */
  if (u->Save_SP && !cache->saved_regs[HPPA_FP_REGNUM].is_addr ()
      && fp_in_r1)
    {
      ULONGEST r1 = get_frame_register_unsigned (this_frame, 1);
      cache->saved_regs[HPPA_FP_REGNUM].set_value (r1);
    }

  {
    /* Convert all the offsets into addresses.  */
    int reg;
    for (reg = 0; reg < gdbarch_num_regs (gdbarch); reg++)
      {
	if (cache->saved_regs[reg].is_addr ())
	  cache->saved_regs[reg].set_addr (cache->saved_regs[reg].addr ()
					   + cache->base);
      }
  }

  {
    hppa_gdbarch_tdep *tdep = gdbarch_tdep<hppa_gdbarch_tdep> (gdbarch);

    if (tdep->unwind_adjust_stub)
      tdep->unwind_adjust_stub (this_frame, cache->base, cache->saved_regs);
  }

  if (hppa_debug)
    gdb_printf (gdb_stdlog, "base=%s }",
		paddress (gdbarch, ((struct hppa_frame_cache *)*this_cache)->base));
  return (struct hppa_frame_cache *) (*this_cache);
}

static void
hppa_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		    struct frame_id *this_id)
{
  struct hppa_frame_cache *info;
  struct unwind_table_entry *u;

  info = hppa_frame_cache (this_frame, this_cache);
  u = hppa_find_unwind_entry_in_block (this_frame);

  (*this_id) = frame_id_build (info->base, u->region_start);
}

static struct value *
hppa_frame_prev_register (frame_info_ptr this_frame,
			  void **this_cache, int regnum)
{
  struct hppa_frame_cache *info = hppa_frame_cache (this_frame, this_cache);

  return hppa_frame_prev_register_helper (this_frame,
					  info->saved_regs, regnum);
}

static int
hppa_frame_unwind_sniffer (const struct frame_unwind *self,
			   frame_info_ptr this_frame, void **this_cache)
{
  if (hppa_find_unwind_entry_in_block (this_frame))
    return 1;

  return 0;
}

static const struct frame_unwind hppa_frame_unwind =
{
  "hppa unwind table",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  hppa_frame_this_id,
  hppa_frame_prev_register,
  NULL,
  hppa_frame_unwind_sniffer
};

/* This is a generic fallback frame unwinder that kicks in if we fail all
   the other ones.  Normally we would expect the stub and regular unwinder
   to work, but in some cases we might hit a function that just doesn't
   have any unwind information available.  In this case we try to do
   unwinding solely based on code reading.  This is obviously going to be
   slow, so only use this as a last resort.  Currently this will only
   identify the stack and pc for the frame.  */

static struct hppa_frame_cache *
hppa_fallback_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct hppa_frame_cache *cache;
  unsigned int frame_size = 0;
  int found_rp = 0;
  CORE_ADDR start_pc;

  if (hppa_debug)
    gdb_printf (gdb_stdlog,
		"{ hppa_fallback_frame_cache (frame=%d) -> ",
		frame_relative_level (this_frame));

  cache = FRAME_OBSTACK_ZALLOC (struct hppa_frame_cache);
  (*this_cache) = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  start_pc = get_frame_func (this_frame);
  if (start_pc)
    {
      CORE_ADDR cur_pc = get_frame_pc (this_frame);
      CORE_ADDR pc;

      for (pc = start_pc; pc < cur_pc; pc += 4)
	{
	  unsigned int insn;

	  insn = read_memory_unsigned_integer (pc, 4, byte_order);
	  frame_size += prologue_inst_adjust_sp (insn);

	  /* There are limited ways to store the return pointer into the
	     stack.  */
	  if (insn == 0x6bc23fd9) /* stw rp,-0x14(sr0,sp) */
	    {
	      cache->saved_regs[HPPA_RP_REGNUM].set_addr (-20);
	      found_rp = 1;
	    }
	  else if (insn == 0x0fc212c1
		   || insn == 0x73c23fe1) /* std rp,-0x10(sr0,sp) */
	    {
	      cache->saved_regs[HPPA_RP_REGNUM].set_addr (-16);
	      found_rp = 1;
	    }
	}
    }

  if (hppa_debug)
    gdb_printf (gdb_stdlog, " frame_size=%d, found_rp=%d }\n",
		frame_size, found_rp);

  cache->base = get_frame_register_unsigned (this_frame, HPPA_SP_REGNUM);
  cache->base -= frame_size;
  cache->saved_regs[HPPA_SP_REGNUM].set_value (cache->base);

  if (cache->saved_regs[HPPA_RP_REGNUM].is_addr ())
    {
      cache->saved_regs[HPPA_RP_REGNUM].set_addr (cache->saved_regs[HPPA_RP_REGNUM].addr ()
						  + cache->base);
      cache->saved_regs[HPPA_PCOQ_HEAD_REGNUM] = 
	cache->saved_regs[HPPA_RP_REGNUM];
    }
  else
    {
      ULONGEST rp;
      rp = get_frame_register_unsigned (this_frame, HPPA_RP_REGNUM);
      cache->saved_regs[HPPA_PCOQ_HEAD_REGNUM].set_value (rp);
    }

  return cache;
}

static void
hppa_fallback_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			     struct frame_id *this_id)
{
  struct hppa_frame_cache *info = 
    hppa_fallback_frame_cache (this_frame, this_cache);

  (*this_id) = frame_id_build (info->base, get_frame_func (this_frame));
}

static struct value *
hppa_fallback_frame_prev_register (frame_info_ptr this_frame,
				   void **this_cache, int regnum)
{
  struct hppa_frame_cache *info
    = hppa_fallback_frame_cache (this_frame, this_cache);

  return hppa_frame_prev_register_helper (this_frame,
					  info->saved_regs, regnum);
}

static const struct frame_unwind hppa_fallback_frame_unwind =
{
  "hppa prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  hppa_fallback_frame_this_id,
  hppa_fallback_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Stub frames, used for all kinds of call stubs.  */
struct hppa_stub_unwind_cache
{
  CORE_ADDR base;
  trad_frame_saved_reg *saved_regs;
};

static struct hppa_stub_unwind_cache *
hppa_stub_frame_unwind_cache (frame_info_ptr this_frame,
			      void **this_cache)
{
  struct hppa_stub_unwind_cache *info;

  if (*this_cache)
    return (struct hppa_stub_unwind_cache *) *this_cache;

  info = FRAME_OBSTACK_ZALLOC (struct hppa_stub_unwind_cache);
  *this_cache = info;
  info->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  info->base = get_frame_register_unsigned (this_frame, HPPA_SP_REGNUM);

  /* By default we assume that stubs do not change the rp.  */
  info->saved_regs[HPPA_PCOQ_HEAD_REGNUM].set_realreg (HPPA_RP_REGNUM);

  return info;
}

static void
hppa_stub_frame_this_id (frame_info_ptr this_frame,
			 void **this_prologue_cache,
			 struct frame_id *this_id)
{
  struct hppa_stub_unwind_cache *info
    = hppa_stub_frame_unwind_cache (this_frame, this_prologue_cache);

  if (info)
    *this_id = frame_id_build (info->base, get_frame_func (this_frame));
}

static struct value *
hppa_stub_frame_prev_register (frame_info_ptr this_frame,
			       void **this_prologue_cache, int regnum)
{
  struct hppa_stub_unwind_cache *info
    = hppa_stub_frame_unwind_cache (this_frame, this_prologue_cache);

  if (info == NULL)
    error (_("Requesting registers from null frame."));

  return hppa_frame_prev_register_helper (this_frame,
					  info->saved_regs, regnum);
}

static int
hppa_stub_unwind_sniffer (const struct frame_unwind *self,
			  frame_info_ptr this_frame,
			  void **this_cache)
{
  CORE_ADDR pc = get_frame_address_in_block (this_frame);
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  hppa_gdbarch_tdep *tdep = gdbarch_tdep<hppa_gdbarch_tdep> (gdbarch);

  if (pc == 0
      || (tdep->in_solib_call_trampoline != NULL
	  && tdep->in_solib_call_trampoline (gdbarch, pc))
      || gdbarch_in_solib_return_trampoline (gdbarch, pc, NULL))
    return 1;
  return 0;
}

static const struct frame_unwind hppa_stub_frame_unwind = {
  "hppa stub",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  hppa_stub_frame_this_id,
  hppa_stub_frame_prev_register,
  NULL,
  hppa_stub_unwind_sniffer
};

CORE_ADDR
hppa_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  ULONGEST ipsw;
  CORE_ADDR pc;

  ipsw = frame_unwind_register_unsigned (next_frame, HPPA_IPSW_REGNUM);
  pc = frame_unwind_register_unsigned (next_frame, HPPA_PCOQ_HEAD_REGNUM);

  /* If the current instruction is nullified, then we are effectively
     still executing the previous instruction.  Pretend we are still
     there.  This is needed when single stepping; if the nullified
     instruction is on a different line, we don't want GDB to think
     we've stepped onto that line.  */
  if (ipsw & 0x00200000)
    pc -= 4;

  return pc & ~0x3;
}

static void
unwind_command (const char *exp, int from_tty)
{
  CORE_ADDR address;
  struct unwind_table_entry *u;

  /* If we have an expression, evaluate it and use it as the address.  */

  if (exp != 0 && *exp != 0)
    address = parse_and_eval_address (exp);
  else
    return;

  u = find_unwind_entry (address);

  if (!u)
    {
      gdb_printf ("Can't find unwind table entry for %s\n", exp);
      return;
    }

  gdb_printf ("unwind_table_entry (%s):\n", host_address_to_string (u));

  gdb_printf ("\tregion_start = %s\n", hex_string (u->region_start));

  gdb_printf ("\tregion_end = %s\n", hex_string (u->region_end));

#define pif(FLD) if (u->FLD) gdb_printf (" "#FLD);

  gdb_printf ("\n\tflags =");
  pif (Cannot_unwind);
  pif (Millicode);
  pif (Millicode_save_sr0);
  pif (Entry_SR);
  pif (Args_stored);
  pif (Variable_Frame);
  pif (Separate_Package_Body);
  pif (Frame_Extension_Millicode);
  pif (Stack_Overflow_Check);
  pif (Two_Instruction_SP_Increment);
  pif (sr4export);
  pif (cxx_info);
  pif (cxx_try_catch);
  pif (sched_entry_seq);
  pif (Save_SP);
  pif (Save_RP);
  pif (Save_MRP_in_frame);
  pif (save_r19);
  pif (Cleanup_defined);
  pif (MPE_XL_interrupt_marker);
  pif (HP_UX_interrupt_marker);
  pif (Large_frame);
  pif (alloca_frame);

  gdb_putc ('\n');

#define pin(FLD) gdb_printf ("\t"#FLD" = 0x%x\n", u->FLD);

  pin (Region_description);
  pin (Entry_FR);
  pin (Entry_GR);
  pin (Total_frame_size);

  if (u->stub_unwind.stub_type)
    {
      gdb_printf ("\tstub type = ");
      switch (u->stub_unwind.stub_type)
	{
	  case LONG_BRANCH:
	    gdb_printf ("long branch\n");
	    break;
	  case PARAMETER_RELOCATION:
	    gdb_printf ("parameter relocation\n");
	    break;
	  case EXPORT:
	    gdb_printf ("export\n");
	    break;
	  case IMPORT:
	    gdb_printf ("import\n");
	    break;
	  case IMPORT_SHLIB:
	    gdb_printf ("import shlib\n");
	    break;
	  default:
	    gdb_printf ("unknown (%d)\n", u->stub_unwind.stub_type);
	}
    }
}

/* Return the GDB type object for the "standard" data type of data in
   register REGNUM.  */

static struct type *
hppa32_register_type (struct gdbarch *gdbarch, int regnum)
{
   if (regnum < HPPA_FP4_REGNUM)
     return builtin_type (gdbarch)->builtin_uint32;
   else
     return builtin_type (gdbarch)->builtin_float;
}

static struct type *
hppa64_register_type (struct gdbarch *gdbarch, int regnum)
{
   if (regnum < HPPA64_FP4_REGNUM)
     return builtin_type (gdbarch)->builtin_uint64;
   else
     return builtin_type (gdbarch)->builtin_double;
}

/* Return non-zero if REGNUM is not a register available to the user
   through ptrace/ttrace.  */

static int
hppa32_cannot_store_register (struct gdbarch *gdbarch, int regnum)
{
  return (regnum == 0
	  || regnum == HPPA_PCSQ_HEAD_REGNUM
	  || (regnum >= HPPA_PCSQ_TAIL_REGNUM && regnum < HPPA_IPSW_REGNUM)
	  || (regnum > HPPA_IPSW_REGNUM && regnum < HPPA_FP4_REGNUM));
}

static int
hppa32_cannot_fetch_register (struct gdbarch *gdbarch, int regnum)
{
  /* cr26 and cr27 are readable (but not writable) from userspace.  */
  if (regnum == HPPA_CR26_REGNUM || regnum == HPPA_CR27_REGNUM)
    return 0;
  else
    return hppa32_cannot_store_register (gdbarch, regnum);
}

static int
hppa64_cannot_store_register (struct gdbarch *gdbarch, int regnum)
{
  return (regnum == 0
	  || regnum == HPPA_PCSQ_HEAD_REGNUM
	  || (regnum >= HPPA_PCSQ_TAIL_REGNUM && regnum < HPPA_IPSW_REGNUM)
	  || (regnum > HPPA_IPSW_REGNUM && regnum < HPPA64_FP4_REGNUM));
}

static int
hppa64_cannot_fetch_register (struct gdbarch *gdbarch, int regnum)
{
  /* cr26 and cr27 are readable (but not writable) from userspace.  */
  if (regnum == HPPA_CR26_REGNUM || regnum == HPPA_CR27_REGNUM)
    return 0;
  else
    return hppa64_cannot_store_register (gdbarch, regnum);
}

static CORE_ADDR
hppa_addr_bits_remove (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  /* The low two bits of the PC on the PA contain the privilege level.
     Some genius implementing a (non-GCC) compiler apparently decided
     this means that "addresses" in a text section therefore include a
     privilege level, and thus symbol tables should contain these bits.
     This seems like a bonehead thing to do--anyway, it seems to work
     for our purposes to just ignore those bits.  */

  return (addr &= ~0x3);
}

/* Get the ARGIth function argument for the current function.  */

static CORE_ADDR
hppa_fetch_pointer_argument (frame_info_ptr frame, int argi, 
			     struct type *type)
{
  return get_frame_register_unsigned (frame, HPPA_R0_REGNUM + 26 - argi);
}

static enum register_status
hppa_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			   int regnum, gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST tmp;
  enum register_status status;

  status = regcache->raw_read (regnum, &tmp);
  if (status == REG_VALID)
    {
      if (regnum == HPPA_PCOQ_HEAD_REGNUM || regnum == HPPA_PCOQ_TAIL_REGNUM)
	tmp &= ~0x3;
      store_unsigned_integer (buf, sizeof tmp, byte_order, tmp);
    }
  return status;
}

static CORE_ADDR
hppa_find_global_pointer (struct gdbarch *gdbarch, struct value *function)
{
  return 0;
}

struct value *
hppa_frame_prev_register_helper (frame_info_ptr this_frame,
				 trad_frame_saved_reg saved_regs[],
				 int regnum)
{
  struct gdbarch *arch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (arch);

  if (regnum == HPPA_PCOQ_TAIL_REGNUM)
    {
      int size = register_size (arch, HPPA_PCOQ_HEAD_REGNUM);
      CORE_ADDR pc;
      struct value *pcoq_val =
	trad_frame_get_prev_register (this_frame, saved_regs,
				      HPPA_PCOQ_HEAD_REGNUM);

      pc = extract_unsigned_integer (value_contents_all (pcoq_val).data (),
				     size, byte_order);
      return frame_unwind_got_constant (this_frame, regnum, pc + 4);
    }

  return trad_frame_get_prev_register (this_frame, saved_regs, regnum);
}


/* An instruction to match.  */
struct insn_pattern
{
  unsigned int data;            /* See if it matches this....  */
  unsigned int mask;            /* ... with this mask.  */
};

/* See bfd/elf32-hppa.c */
static struct insn_pattern hppa_long_branch_stub[] = {
  /* ldil LR'xxx,%r1 */
  { 0x20200000, 0xffe00000 },
  /* be,n RR'xxx(%sr4,%r1) */
  { 0xe0202002, 0xffe02002 }, 
  { 0, 0 }
};

static struct insn_pattern hppa_long_branch_pic_stub[] = {
  /* b,l .+8, %r1 */
  { 0xe8200000, 0xffe00000 },
  /* addil LR'xxx - ($PIC_pcrel$0 - 4), %r1 */
  { 0x28200000, 0xffe00000 },
  /* be,n RR'xxxx - ($PIC_pcrel$0 - 8)(%sr4, %r1) */
  { 0xe0202002, 0xffe02002 }, 
  { 0, 0 }
};

static struct insn_pattern hppa_import_stub[] = {
  /* addil LR'xxx, %dp */
  { 0x2b600000, 0xffe00000 },
  /* ldw RR'xxx(%r1), %r21 */
  { 0x48350000, 0xffffb000 },
  /* bv %r0(%r21) */
  { 0xeaa0c000, 0xffffffff },
  /* ldw RR'xxx+4(%r1), %r19 */
  { 0x48330000, 0xffffb000 },
  { 0, 0 }
};

static struct insn_pattern hppa_import_pic_stub[] = {
  /* addil LR'xxx,%r19 */
  { 0x2a600000, 0xffe00000 },
  /* ldw RR'xxx(%r1),%r21 */
  { 0x48350000, 0xffffb000 },
  /* bv %r0(%r21) */
  { 0xeaa0c000, 0xffffffff },
  /* ldw RR'xxx+4(%r1),%r19 */
  { 0x48330000, 0xffffb000 },
  { 0, 0 },
};

static struct insn_pattern hppa_plt_stub[] = {
  /* b,l 1b, %r20 - 1b is 3 insns before here */
  { 0xea9f1fdd, 0xffffffff },
  /* depi 0,31,2,%r20 */
  { 0xd6801c1e, 0xffffffff },
  { 0, 0 }
};

/* Maximum number of instructions on the patterns above.  */
#define HPPA_MAX_INSN_PATTERN_LEN	4

/* Return non-zero if the instructions at PC match the series
   described in PATTERN, or zero otherwise.  PATTERN is an array of
   'struct insn_pattern' objects, terminated by an entry whose mask is
   zero.

   When the match is successful, fill INSN[i] with what PATTERN[i]
   matched.  */

static int
hppa_match_insns (struct gdbarch *gdbarch, CORE_ADDR pc,
		  struct insn_pattern *pattern, unsigned int *insn)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR npc = pc;
  int i;

  for (i = 0; pattern[i].mask; i++)
    {
      gdb_byte buf[HPPA_INSN_SIZE];

      target_read_memory (npc, buf, HPPA_INSN_SIZE);
      insn[i] = extract_unsigned_integer (buf, HPPA_INSN_SIZE, byte_order);
      if ((insn[i] & pattern[i].mask) == pattern[i].data)
	npc += 4;
      else
	return 0;
    }

  return 1;
}

/* This relaxed version of the instruction matcher allows us to match
   from somewhere inside the pattern, by looking backwards in the
   instruction scheme.  */

static int
hppa_match_insns_relaxed (struct gdbarch *gdbarch, CORE_ADDR pc,
			  struct insn_pattern *pattern, unsigned int *insn)
{
  int offset, len = 0;

  while (pattern[len].mask)
    len++;

  for (offset = 0; offset < len; offset++)
    if (hppa_match_insns (gdbarch, pc - offset * HPPA_INSN_SIZE,
			  pattern, insn))
      return 1;

  return 0;
}

static int
hppa_in_dyncall (CORE_ADDR pc)
{
  struct unwind_table_entry *u;

  u = find_unwind_entry (hppa_symbol_address ("$$dyncall"));
  if (!u)
    return 0;

  return (pc >= u->region_start && pc <= u->region_end);
}

int
hppa_in_solib_call_trampoline (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  unsigned int insn[HPPA_MAX_INSN_PATTERN_LEN];
  struct unwind_table_entry *u;

  if (in_plt_section (pc) || hppa_in_dyncall (pc))
    return 1;

  /* The GNU toolchain produces linker stubs without unwind
     information.  Since the pattern matching for linker stubs can be
     quite slow, so bail out if we do have an unwind entry.  */

  u = find_unwind_entry (pc);
  if (u != NULL)
    return 0;

  return
    (hppa_match_insns_relaxed (gdbarch, pc, hppa_import_stub, insn)
     || hppa_match_insns_relaxed (gdbarch, pc, hppa_import_pic_stub, insn)
     || hppa_match_insns_relaxed (gdbarch, pc, hppa_long_branch_stub, insn)
     || hppa_match_insns_relaxed (gdbarch, pc,
				  hppa_long_branch_pic_stub, insn));
}

/* This code skips several kind of "trampolines" used on PA-RISC
   systems: $$dyncall, import stubs and PLT stubs.  */

CORE_ADDR
hppa_skip_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct type *func_ptr_type = builtin_type (gdbarch)->builtin_func_ptr;

  unsigned int insn[HPPA_MAX_INSN_PATTERN_LEN];
  int dp_rel;

  /* $$dyncall handles both PLABELs and direct addresses.  */
  if (hppa_in_dyncall (pc))
    {
      pc = get_frame_register_unsigned (frame, HPPA_R0_REGNUM + 22);

      /* PLABELs have bit 30 set; if it's a PLABEL, then dereference it.  */
      if (pc & 0x2)
	pc = read_memory_typed_address (pc & ~0x3, func_ptr_type);

      return pc;
    }

  dp_rel = hppa_match_insns (gdbarch, pc, hppa_import_stub, insn);
  if (dp_rel || hppa_match_insns (gdbarch, pc, hppa_import_pic_stub, insn))
    {
      /* Extract the target address from the addil/ldw sequence.  */
      pc = hppa_extract_21 (insn[0]) + hppa_extract_14 (insn[1]);

      if (dp_rel)
	pc += get_frame_register_unsigned (frame, HPPA_DP_REGNUM);
      else
	pc += get_frame_register_unsigned (frame, HPPA_R0_REGNUM + 19);

      /* fallthrough */
    }

  if (in_plt_section (pc))
    {
      pc = read_memory_typed_address (pc, func_ptr_type);

      /* If the PLT slot has not yet been resolved, the target will be
	 the PLT stub.  */
      if (in_plt_section (pc))
	{
	  /* Sanity check: are we pointing to the PLT stub?  */
	  if (!hppa_match_insns (gdbarch, pc, hppa_plt_stub, insn))
	    {
	      warning (_("Cannot resolve PLT stub at %s."),
		       paddress (gdbarch, pc));
	      return 0;
	    }

	  /* This should point to the fixup routine.  */
	  pc = read_memory_typed_address (pc + 8, func_ptr_type);
	}
    }

  return pc;
}


/* Here is a table of C type sizes on hppa with various compiles
   and options.  I measured this on PA 9000/800 with HP-UX 11.11
   and these compilers:

     /usr/ccs/bin/cc    HP92453-01 A.11.01.21
     /opt/ansic/bin/cc  HP92453-01 B.11.11.28706.GP
     /opt/aCC/bin/aCC   B3910B A.03.45
     gcc                gcc 3.3.2 native hppa2.0w-hp-hpux11.11

     cc            : 1 2 4 4 8 : 4 8 -- : 4 4
     ansic +DA1.1  : 1 2 4 4 8 : 4 8 16 : 4 4
     ansic +DA2.0  : 1 2 4 4 8 : 4 8 16 : 4 4
     ansic +DA2.0W : 1 2 4 8 8 : 4 8 16 : 8 8
     acc   +DA1.1  : 1 2 4 4 8 : 4 8 16 : 4 4
     acc   +DA2.0  : 1 2 4 4 8 : 4 8 16 : 4 4
     acc   +DA2.0W : 1 2 4 8 8 : 4 8 16 : 8 8
     gcc           : 1 2 4 4 8 : 4 8 16 : 4 4

   Each line is:

     compiler and options
     char, short, int, long, long long
     float, double, long double
     char *, void (*)()

   So all these compilers use either ILP32 or LP64 model.
   TODO: gcc has more options so it needs more investigation.

   For floating point types, see:

     http://docs.hp.com/hpux/pdf/B3906-90006.pdf
     HP-UX floating-point guide, hpux 11.00

   -- chastain 2003-12-18  */

static struct gdbarch *
hppa_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;

  /* find a candidate among the list of pre-declared architectures.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return (arches->gdbarch);

  /* If none found, then allocate and initialize one.  */
  hppa_gdbarch_tdep *tdep = new hppa_gdbarch_tdep;
  gdbarch = gdbarch_alloc (&info, tdep);

  /* Determine from the bfd_arch_info structure if we are dealing with
     a 32 or 64 bits architecture.  If the bfd_arch_info is not available,
     then default to a 32bit machine.  */
  if (info.bfd_arch_info != NULL)
    tdep->bytes_per_address =
      info.bfd_arch_info->bits_per_address / info.bfd_arch_info->bits_per_byte;
  else
    tdep->bytes_per_address = 4;

  tdep->find_global_pointer = hppa_find_global_pointer;

  /* Some parts of the gdbarch vector depend on whether we are running
     on a 32 bits or 64 bits target.  */
  switch (tdep->bytes_per_address)
    {
      case 4:
	set_gdbarch_num_regs (gdbarch, hppa32_num_regs);
	set_gdbarch_register_name (gdbarch, hppa32_register_name);
	set_gdbarch_register_type (gdbarch, hppa32_register_type);
	set_gdbarch_cannot_store_register (gdbarch,
					   hppa32_cannot_store_register);
	set_gdbarch_cannot_fetch_register (gdbarch,
					   hppa32_cannot_fetch_register);
	break;
      case 8:
	set_gdbarch_num_regs (gdbarch, hppa64_num_regs);
	set_gdbarch_register_name (gdbarch, hppa64_register_name);
	set_gdbarch_register_type (gdbarch, hppa64_register_type);
	set_gdbarch_dwarf2_reg_to_regnum (gdbarch, hppa64_dwarf_reg_to_regnum);
	set_gdbarch_cannot_store_register (gdbarch,
					   hppa64_cannot_store_register);
	set_gdbarch_cannot_fetch_register (gdbarch,
					   hppa64_cannot_fetch_register);
	break;
      default:
	internal_error (_("Unsupported address size: %d"),
			tdep->bytes_per_address);
    }

  set_gdbarch_long_bit (gdbarch, tdep->bytes_per_address * TARGET_CHAR_BIT);
  set_gdbarch_ptr_bit (gdbarch, tdep->bytes_per_address * TARGET_CHAR_BIT);

  /* The following gdbarch vector elements are the same in both ILP32
     and LP64, but might show differences some day.  */
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 128);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_quad);

  /* The following gdbarch vector elements do not depend on the address
     size, or in any other gdbarch element previously set.  */
  set_gdbarch_skip_prologue (gdbarch, hppa_skip_prologue);
  set_gdbarch_stack_frame_destroyed_p (gdbarch,
				       hppa_stack_frame_destroyed_p);
  set_gdbarch_inner_than (gdbarch, core_addr_greaterthan);
  set_gdbarch_sp_regnum (gdbarch, HPPA_SP_REGNUM);
  set_gdbarch_fp0_regnum (gdbarch, HPPA_FP0_REGNUM);
  set_gdbarch_addr_bits_remove (gdbarch, hppa_addr_bits_remove);
  set_gdbarch_believe_pcc_promotion (gdbarch, 1);
  set_gdbarch_read_pc (gdbarch, hppa_read_pc);
  set_gdbarch_write_pc (gdbarch, hppa_write_pc);

  /* Helper for function argument information.  */
  set_gdbarch_fetch_pointer_argument (gdbarch, hppa_fetch_pointer_argument);

  /* When a hardware watchpoint triggers, we'll move the inferior past
     it by removing all eventpoints; stepping past the instruction
     that caused the trigger; reinserting eventpoints; and checking
     whether any watched location changed.  */
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  /* Inferior function call methods.  */
  switch (tdep->bytes_per_address)
    {
    case 4:
      set_gdbarch_push_dummy_call (gdbarch, hppa32_push_dummy_call);
      set_gdbarch_frame_align (gdbarch, hppa32_frame_align);
      set_gdbarch_convert_from_func_ptr_addr
	(gdbarch, hppa32_convert_from_func_ptr_addr);
      break;
    case 8:
      set_gdbarch_push_dummy_call (gdbarch, hppa64_push_dummy_call);
      set_gdbarch_frame_align (gdbarch, hppa64_frame_align);
      break;
    default:
      internal_error (_("bad switch"));
    }
      
  /* Struct return methods.  */
  switch (tdep->bytes_per_address)
    {
    case 4:
      set_gdbarch_return_value (gdbarch, hppa32_return_value);
      break;
    case 8:
      set_gdbarch_return_value (gdbarch, hppa64_return_value);
      break;
    default:
      internal_error (_("bad switch"));
    }
      
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, hppa_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, hppa_breakpoint::bp_from_kind);
  set_gdbarch_pseudo_register_read (gdbarch, hppa_pseudo_register_read);

  /* Frame unwind methods.  */
  set_gdbarch_unwind_pc (gdbarch, hppa_unwind_pc);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Hook in the default unwinders.  */
  frame_unwind_append_unwinder (gdbarch, &hppa_stub_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &hppa_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &hppa_fallback_frame_unwind);

  return gdbarch;
}

static void
hppa_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  hppa_gdbarch_tdep *tdep = gdbarch_tdep<hppa_gdbarch_tdep> (gdbarch);

  gdb_printf (file, "bytes_per_address = %d\n", 
	      tdep->bytes_per_address);
  gdb_printf (file, "elf = %s\n", tdep->is_elf ? "yes" : "no");
}

void _initialize_hppa_tdep ();
void
_initialize_hppa_tdep ()
{
  gdbarch_register (bfd_arch_hppa, hppa_gdbarch_init, hppa_dump_tdep);

  add_cmd ("unwind", class_maintenance, unwind_command,
	   _("Print unwind table entry at given address."),
	   &maintenanceprintlist);

  /* Debug this files internals.  */
  add_setshow_boolean_cmd ("hppa", class_maintenance, &hppa_debug, _("\
Set whether hppa target specific debugging information should be displayed."),
			   _("\
Show whether hppa target specific debugging information is displayed."), _("\
This flag controls whether hppa target specific debugging information is\n\
displayed.  This information is particularly useful for debugging frame\n\
unwinding problems."),
			   NULL,
			   NULL, /* FIXME: i18n: hppa debug flag is %s.  */
			   &setdebuglist, &showdebuglist);
}
