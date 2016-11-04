/* BFD back-end for RISC iX (Acorn, arm) binaries.
   Copyright (C) 1994-2015 Free Software Foundation, Inc.
   Contributed by Richard Earnshaw (rwe@pegasus.esprit.ec.org)

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */


/* RISC iX overloads the MAGIC field to indicate more than just the usual
   [ZNO]MAGIC values.  Also included are squeezing information and
   shared library usage.  */

/* The following come from the man page.  */
#define SHLIBLEN 60

#define MF_IMPURE       00200
#define MF_SQUEEZED     01000
#define MF_USES_SL      02000
#define MF_IS_SL        04000

/* Common combinations.  */

/* Demand load (impure text).  */
#define IMAGIC          (MF_IMPURE | ZMAGIC)

/* OMAGIC with large header.
   May contain a ref to a shared lib required by the object.  */
#define SPOMAGIC        (MF_USES_SL | OMAGIC)

/* A reference to a shared library.
   The text portion of the object contains "overflow text" from
   the shared library to be linked in with an object.  */
#define SLOMAGIC        (MF_IS_SL | OMAGIC)

/* Sqeezed demand paged.
   NOTE: This interpretation of QMAGIC seems to be at variance
   with that used on other architectures.  */
#define QMAGIC          (MF_SQUEEZED | ZMAGIC)

/* Program which uses sl.  */
#define SPZMAGIC        (MF_USES_SL | ZMAGIC)

/* Sqeezed ditto.  */
#define SPQMAGIC        (MF_USES_SL | QMAGIC)

/* Shared lib part of prog.  */
#define SLZMAGIC        (MF_IS_SL | ZMAGIC)

/* Sl which uses another.  */
#define SLPZMAGIC       (MF_USES_SL | SLZMAGIC)

#define N_SHARED_LIB(x) ((x).a_info & MF_USES_SL)

/* Only a pure OMAGIC file has the minimal header.  */
#define N_TXTOFF(x)		\
 ((x).a_info == OMAGIC		\
  ? 32				\
  : (N_MAGIC(x) == ZMAGIC	\
     ? TARGET_PAGE_SIZE		\
     : 999))

#define N_TXTADDR(x)							     \
  (N_MAGIC(x) != ZMAGIC							     \
   ? (bfd_vma) 0 /* object file or NMAGIC */				     \
   /* Programs with shared libs are loaded at the first page after all the   \
      text segments of the shared library programs.  Without looking this    \
      up we can't know exactly what the address will be.  A reasonable guess \
      is that a_entry will be in the first page of the executable.  */	     \
   : (N_SHARED_LIB(x)							     \
      ? ((x).a_entry & ~(bfd_vma) (TARGET_PAGE_SIZE - 1))		     \
      : (bfd_vma) TEXT_START_ADDR))

#define N_SYMOFF(x) \
  (N_TXTOFF (x) + (x).a_text + (x).a_data + (x).a_trsize + (x).a_drsize)

#define N_STROFF(x) (N_SYMOFF (x) + (x).a_syms)

#define TEXT_START_ADDR   32768
#define TARGET_PAGE_SIZE  32768
#define SEGMENT_SIZE      TARGET_PAGE_SIZE
#define DEFAULT_ARCH      bfd_arch_arm

/* Do not "beautify" the CONCAT* macro args.  Traditional C will not
   remove whitespace added here, and thus will fail to concatenate
   the tokens.  */
#define MY(OP) CONCAT2 (arm_aout_riscix_,OP)
#define TARGETNAME "a.out-riscix"
#define N_BADMAG(x) ((((x).a_info & ~007200) != ZMAGIC) \
                  && (((x).a_info & ~006000) != OMAGIC) \
                  && ((x).a_info != NMAGIC))
#define N_MAGIC(x) ((x).a_info & ~07200)

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

#define WRITE_HEADERS(abfd, execp)					    \
  {									    \
    bfd_size_type text_size; /* Dummy vars.  */				    \
    file_ptr text_end;							    \
    									    \
    if (adata (abfd).magic == undecided_magic)				    \
      NAME (aout, adjust_sizes_and_vmas) (abfd, & text_size, & text_end);   \
    									    \
    execp->a_syms = bfd_get_symcount (abfd) * EXTERNAL_NLIST_SIZE;	    \
    execp->a_entry = bfd_get_start_address (abfd);			    \
    									    \
    execp->a_trsize = ((obj_textsec (abfd)->reloc_count) *		    \
		       obj_reloc_entry_size (abfd));			    \
    execp->a_drsize = ((obj_datasec (abfd)->reloc_count) *		    \
		       obj_reloc_entry_size (abfd));			    \
    NAME (aout, swap_exec_header_out) (abfd, execp, & exec_bytes);	    \
    									    \
    if (bfd_seek (abfd, (file_ptr) 0, SEEK_SET) != 0			    \
	|| bfd_bwrite ((void *) & exec_bytes, (bfd_size_type) EXEC_BYTES_SIZE,  \
		      abfd) != EXEC_BYTES_SIZE)				    \
      return FALSE;							    \
    /* Now write out reloc info, followed by syms and strings.  */	    \
									    \
    if (bfd_get_outsymbols (abfd) != NULL			    	    \
	&& bfd_get_symcount (abfd) != 0)				    \
      {									    \
	if (bfd_seek (abfd, (file_ptr) (N_SYMOFF (* execp)), SEEK_SET) != 0)\
	  return FALSE;							    \
									    \
	if (! NAME (aout, write_syms) (abfd))				    \
          return FALSE;							    \
									    \
	if (bfd_seek (abfd, (file_ptr) (N_TRELOFF (* execp)), SEEK_SET) != 0)\
	  return FALSE;							    \
									    \
	if (! riscix_squirt_out_relocs (abfd, obj_textsec (abfd)))	    \
	  return FALSE;							    \
	if (bfd_seek (abfd, (file_ptr) (N_DRELOFF (* execp)), SEEK_SET) != 0)\
	  return FALSE;							    \
									    \
	if (!NAME (aout, squirt_out_relocs) (abfd, obj_datasec (abfd)))	    \
	  return FALSE;							    \
      }									    \
  }

#include "libaout.h"
#include "aout/aout64.h"

static bfd_reloc_status_type
riscix_fix_pcrel_26_done (bfd *abfd ATTRIBUTE_UNUSED,
			  arelent *reloc_entry ATTRIBUTE_UNUSED,
			  asymbol *symbol ATTRIBUTE_UNUSED,
			  void * data ATTRIBUTE_UNUSED,
			  asection *input_section ATTRIBUTE_UNUSED,
			  bfd *output_bfd ATTRIBUTE_UNUSED,
			  char **error_message ATTRIBUTE_UNUSED)
{
  /* This is dead simple at present.  */
  return bfd_reloc_ok;
}

static bfd_reloc_status_type riscix_fix_pcrel_26 (bfd *, arelent *, asymbol *, void *, asection *, bfd *, char **);
static const bfd_target *arm_aout_riscix_callback (bfd *);

static reloc_howto_type riscix_std_reloc_howto[] =
{
  /* Type              rs size bsz  pcrel bitpos ovrf                     sf name     part_inpl readmask  setmask    pcdone */
  HOWTO( 0,              0,  0,   8,  FALSE, 0, complain_overflow_bitfield,0,"8",         TRUE, 0x000000ff,0x000000ff, FALSE),
  HOWTO( 1,              0,  1,   16, FALSE, 0, complain_overflow_bitfield,0,"16",        TRUE, 0x0000ffff,0x0000ffff, FALSE),
  HOWTO( 2,              0,  2,   32, FALSE, 0, complain_overflow_bitfield,0,"32",        TRUE, 0xffffffff,0xffffffff, FALSE),
  HOWTO( 3,              2,  3,   26, TRUE,  0, complain_overflow_signed,  riscix_fix_pcrel_26 , "ARM26",      TRUE, 0x00ffffff,0x00ffffff, FALSE),
  HOWTO( 4,              0,  0,   8,  TRUE,  0, complain_overflow_signed,  0,"DISP8",     TRUE, 0x000000ff,0x000000ff, TRUE),
  HOWTO( 5,              0,  1,   16, TRUE,  0, complain_overflow_signed,  0,"DISP16",    TRUE, 0x0000ffff,0x0000ffff, TRUE),
  HOWTO( 6,              0,  2,   32, TRUE,  0, complain_overflow_signed,  0,"DISP32",    TRUE, 0xffffffff,0xffffffff, TRUE),
  HOWTO( 7,              2,  3,   26, FALSE, 0, complain_overflow_signed,  riscix_fix_pcrel_26_done, "ARM26D",TRUE,0x00ffffff,0x00ffffff, FALSE),
  EMPTY_HOWTO (-1),
  HOWTO( 9,              0, -1,   16, FALSE, 0, complain_overflow_bitfield,0,"NEG16",     TRUE, 0x0000ffff,0x0000ffff, FALSE),
  HOWTO( 10,              0, -2,  32, FALSE, 0, complain_overflow_bitfield,0,"NEG32",     TRUE, 0xffffffff,0xffffffff, FALSE)
};

#define RISCIX_TABLE_SIZE \
  (sizeof (riscix_std_reloc_howto) / sizeof (reloc_howto_type))

static bfd_reloc_status_type
riscix_fix_pcrel_26 (bfd *abfd,
		     arelent *reloc_entry,
		     asymbol *symbol,
		     void * data,
		     asection *input_section,
		     bfd *output_bfd,
		     char **error_message ATTRIBUTE_UNUSED)
{
  bfd_vma relocation;
  bfd_size_type addr = reloc_entry->address;
  long target = bfd_get_32 (abfd, (bfd_byte *) data + addr);
  bfd_reloc_status_type flag = bfd_reloc_ok;

  /* If this is an undefined symbol, return error.  */
  if (bfd_is_und_section (symbol->section)
      && (symbol->flags & BSF_WEAK) == 0)
    return output_bfd ? bfd_reloc_continue : bfd_reloc_undefined;

  /* If the sections are different, and we are doing a partial relocation,
     just ignore it for now.  */
  if (symbol->section->name != input_section->name
      && output_bfd != NULL)
    return bfd_reloc_continue;

  relocation = (target & 0x00ffffff) << 2;
  relocation = (relocation ^ 0x02000000) - 0x02000000; /* Sign extend.  */
  relocation += symbol->value;
  relocation += symbol->section->output_section->vma;
  relocation += symbol->section->output_offset;
  relocation += reloc_entry->addend;
  relocation -= input_section->output_section->vma;
  relocation -= input_section->output_offset;
  relocation -= addr;
  if (relocation & 3)
    return bfd_reloc_overflow;

  /* Check for overflow.  */
  if (relocation & 0x02000000)
    {
      if ((relocation & ~ (bfd_vma) 0x03ffffff) != ~ (bfd_vma) 0x03ffffff)
	flag = bfd_reloc_overflow;
    }
  else if (relocation & ~ (bfd_vma) 0x03ffffff)
    flag = bfd_reloc_overflow;

  target &= ~0x00ffffff;
  target |= (relocation >> 2) & 0x00ffffff;
  bfd_put_32 (abfd, (bfd_vma) target, (bfd_byte *) data + addr);

  /* Now the ARM magic... Change the reloc type so that it is marked as done.
     Strictly this is only necessary if we are doing a partial relocation.  */
  reloc_entry->howto = &riscix_std_reloc_howto[7];

  return flag;
}

static reloc_howto_type *
riscix_reloc_type_lookup (bfd *abfd, bfd_reloc_code_real_type code)
{
#define ASTD(i,j)       case i: return &riscix_std_reloc_howto[j]
  if (code == BFD_RELOC_CTOR)
    switch (bfd_arch_bits_per_address (abfd))
      {
      case 32:
        code = BFD_RELOC_32;
        break;
      default:
	return NULL;
      }

  switch (code)
    {
      ASTD (BFD_RELOC_16, 1);
      ASTD (BFD_RELOC_32, 2);
      ASTD (BFD_RELOC_ARM_PCREL_BRANCH, 3);
      ASTD (BFD_RELOC_8_PCREL, 4);
      ASTD (BFD_RELOC_16_PCREL, 5);
      ASTD (BFD_RELOC_32_PCREL, 6);
    default:
      return NULL;
    }
}

static reloc_howto_type *
riscix_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			  const char *r_name)
{
  unsigned int i;

  for (i = 0;
       i < sizeof (riscix_std_reloc_howto) / sizeof (riscix_std_reloc_howto[0]);
       i++)
    if (riscix_std_reloc_howto[i].name != NULL
	&& strcasecmp (riscix_std_reloc_howto[i].name, r_name) == 0)
      return &riscix_std_reloc_howto[i];

  return NULL;
}

#define MY_bfd_link_hash_table_create  _bfd_generic_link_hash_table_create
#define MY_bfd_link_add_symbols        _bfd_generic_link_add_symbols
#define MY_final_link_callback         should_not_be_used
#define MY_bfd_final_link              _bfd_generic_final_link

#define MY_bfd_reloc_type_lookup       riscix_reloc_type_lookup
#define MY_bfd_reloc_name_lookup       riscix_reloc_name_lookup
#define MY_canonicalize_reloc          arm_aout_riscix_canonicalize_reloc
#define MY_object_p                    arm_aout_riscix_object_p

static void
riscix_swap_std_reloc_out (bfd *abfd,
			   arelent *g,
			   struct reloc_std_external *natptr)
{
  int r_index;
  asymbol *sym = *(g->sym_ptr_ptr);
  int r_extern;
  int r_length;
  int r_pcrel;
  int r_neg = 0;	/* Negative relocs use the BASEREL bit.  */
  asection *output_section = sym->section->output_section;

  PUT_WORD(abfd, g->address, natptr->r_address);

  r_length = g->howto->size ;   /* Size as a power of two.  */
  if (r_length < 0)
    {
      r_length = -r_length;
      r_neg = 1;
    }

  r_pcrel  = (int) g->howto->pc_relative; /* Relative to PC?  */

  /* For RISC iX, in pc-relative relocs the r_pcrel bit means that the
     relocation has been done already (Only for the 26-bit one I think)?  */
  if (r_length == 3)
    r_pcrel = r_pcrel ? 0 : 1;

  /* Name was clobbered by aout_write_syms to be symbol index.  */

  /* If this relocation is relative to a symbol then set the
     r_index to the symbols index, and the r_extern bit.

     Absolute symbols can come in in two ways, either as an offset
     from the abs section, or as a symbol which has an abs value.
     check for that here.  */

  if (bfd_is_com_section (output_section)
      || bfd_is_abs_section (output_section)
      || bfd_is_und_section (output_section))
    {
      if (bfd_abs_section_ptr->symbol == sym)
	{
	  /* Whoops, looked like an abs symbol, but is really an offset
	     from the abs section.  */
	  r_index = 0;
	  r_extern = 0;
	}
      else
	{
	  /* Fill in symbol.  */
	  r_extern = 1;
	  r_index = (*g->sym_ptr_ptr)->udata.i;
	}
    }
  else
    {
      /* Just an ordinary section.  */
      r_extern = 0;
      r_index  = output_section->target_index;
    }

  /* Now the fun stuff.  */
  if (bfd_header_big_endian (abfd))
    {
      natptr->r_index[0] = r_index >> 16;
      natptr->r_index[1] = r_index >> 8;
      natptr->r_index[2] = r_index;
      natptr->r_type[0] =
	(  (r_extern ?   RELOC_STD_BITS_EXTERN_BIG: 0)
	 | (r_pcrel  ?   RELOC_STD_BITS_PCREL_BIG: 0)
	 | (r_neg    ?   RELOC_STD_BITS_BASEREL_BIG: 0)
	 | (r_length <<  RELOC_STD_BITS_LENGTH_SH_BIG));
    }
  else
    {
      natptr->r_index[2] = r_index >> 16;
      natptr->r_index[1] = r_index >> 8;
      natptr->r_index[0] = r_index;
      natptr->r_type[0] =
	(  (r_extern ?   RELOC_STD_BITS_EXTERN_LITTLE: 0)
	 | (r_pcrel  ?   RELOC_STD_BITS_PCREL_LITTLE: 0)
	 | (r_neg    ?   RELOC_STD_BITS_BASEREL_LITTLE: 0)
	 | (r_length <<  RELOC_STD_BITS_LENGTH_SH_LITTLE));
    }
}

static bfd_boolean
riscix_squirt_out_relocs (bfd *abfd, asection *section)
{
  arelent **generic;
  unsigned char *native, *natptr;
  size_t each_size;
  unsigned int count = section->reloc_count;
  bfd_size_type natsize;

  if (count == 0)
    return TRUE;

  each_size = obj_reloc_entry_size (abfd);
  natsize = each_size;
  natsize *= count;
  native = bfd_zalloc (abfd, natsize);
  if (!native)
    return FALSE;

  generic = section->orelocation;

  for (natptr = native;
       count != 0;
       --count, natptr += each_size, ++generic)
    riscix_swap_std_reloc_out (abfd, *generic,
			       (struct reloc_std_external *) natptr);

  if (bfd_bwrite ((void *) native, natsize, abfd) != natsize)
    {
      bfd_release (abfd, native);
      return FALSE;
    }

  bfd_release (abfd, native);
  return TRUE;
}

/* This is just like the standard aoutx.h version but we need to do our
   own mapping of external reloc type values to howto entries.  */

static long
MY (canonicalize_reloc) (bfd *abfd,
			 sec_ptr section,
			 arelent **relptr,
			 asymbol **symbols)
{
  arelent *tblptr = section->relocation;
  unsigned int count, c;
  extern reloc_howto_type NAME (aout, std_howto_table)[];

  /* If we have already read in the relocation table, return the values.  */
  if (section->flags & SEC_CONSTRUCTOR)
    {
      arelent_chain *chain = section->constructor_chain;

      for (count = 0; count < section->reloc_count; count++)
	{
	  *relptr++ = &chain->relent;
	  chain = chain->next;
	}
      *relptr = 0;
      return section->reloc_count;
    }

  if (tblptr && section->reloc_count)
    {
      for (count = 0; count++ < section->reloc_count;)
	*relptr++ = tblptr++;
      *relptr = 0;
      return section->reloc_count;
    }

  if (!NAME (aout, slurp_reloc_table) (abfd, section, symbols))
    return -1;
  tblptr = section->relocation;

  /* Fix up howto entries.  */
  for (count = 0; count++ < section->reloc_count;)
    {
      c = tblptr->howto - NAME(aout,std_howto_table);
      BFD_ASSERT (c < RISCIX_TABLE_SIZE);
      tblptr->howto = &riscix_std_reloc_howto[c];

      *relptr++ = tblptr++;
    }
  *relptr = 0;
  return section->reloc_count;
}

/* This is the same as NAME(aout,some_aout_object_p), but has different
   expansions of the macro definitions.  */

static const bfd_target *
riscix_some_aout_object_p (bfd *abfd,
			   struct internal_exec *execp,
			   const bfd_target *(*callback_to_real_object_p) (bfd *))
{
  struct aout_data_struct *rawptr, *oldrawptr;
  const bfd_target *result;
  bfd_size_type amt = sizeof (struct aout_data_struct);

  rawptr = bfd_zalloc (abfd, amt);

  if (rawptr == NULL)
    return NULL;

  oldrawptr = abfd->tdata.aout_data;
  abfd->tdata.aout_data = rawptr;

  /* Copy the contents of the old tdata struct.
     In particular, we want the subformat, since for hpux it was set in
     hp300hpux.c:swap_exec_header_in and will be used in
     hp300hpux.c:callback.  */
  if (oldrawptr != NULL)
    *abfd->tdata.aout_data = *oldrawptr;

  abfd->tdata.aout_data->a.hdr = &rawptr->e;
  /* Copy in the internal_exec struct.  */
  *(abfd->tdata.aout_data->a.hdr) = *execp;
  execp = abfd->tdata.aout_data->a.hdr;

  /* Set the file flags.  */
  abfd->flags = BFD_NO_FLAGS;
  if (execp->a_drsize || execp->a_trsize)
    abfd->flags |= HAS_RELOC;
  /* Setting of EXEC_P has been deferred to the bottom of this function.  */
  if (execp->a_syms)
    abfd->flags |= HAS_LINENO | HAS_DEBUG | HAS_SYMS | HAS_LOCALS;
  if (N_DYNAMIC(*execp))
    abfd->flags |= DYNAMIC;

 /* Squeezed files aren't supported (yet)!  */
  if ((execp->a_info & MF_SQUEEZED) != 0)
    {
      bfd_set_error (bfd_error_wrong_format);
      return NULL;
    }
  else if ((execp->a_info & MF_IS_SL) != 0)
    {
      /* Nor are shared libraries.  */
      bfd_set_error (bfd_error_wrong_format);
      return NULL;
    }
  else if (N_MAGIC (*execp) == ZMAGIC)
    {
      abfd->flags |= D_PAGED | WP_TEXT;
      adata (abfd).magic = z_magic;
    }
  else if (N_MAGIC (*execp) == NMAGIC)
    {
      abfd->flags |= WP_TEXT;
      adata (abfd).magic = n_magic;
    }
  else if (N_MAGIC (*execp) == OMAGIC)
    adata (abfd).magic = o_magic;
  else
    /* Should have been checked with N_BADMAG before this routine
       was called.  */
    abort ();

  bfd_get_start_address (abfd) = execp->a_entry;

  obj_aout_symbols (abfd) = NULL;
  bfd_get_symcount (abfd) = execp->a_syms / sizeof (struct external_nlist);

  /* The default relocation entry size is that of traditional V7 Unix.  */
  obj_reloc_entry_size (abfd) = RELOC_STD_SIZE;

  /* The default symbol entry size is that of traditional Unix.  */
  obj_symbol_entry_size (abfd) = EXTERNAL_NLIST_SIZE;

  obj_aout_external_syms (abfd) = NULL;
  obj_aout_external_strings (abfd) = NULL;
  obj_aout_sym_hashes (abfd) = NULL;

  if (! NAME (aout, make_sections) (abfd))
    return NULL;

  obj_datasec (abfd)->size = execp->a_data;
  obj_bsssec (abfd)->size = execp->a_bss;

  obj_textsec (abfd)->flags =
    (execp->a_trsize != 0
     ? (SEC_ALLOC | SEC_LOAD | SEC_CODE | SEC_HAS_CONTENTS | SEC_RELOC)
     : (SEC_ALLOC | SEC_LOAD | SEC_CODE | SEC_HAS_CONTENTS));
  obj_datasec (abfd)->flags =
    (execp->a_drsize != 0
     ? (SEC_ALLOC | SEC_LOAD | SEC_DATA | SEC_HAS_CONTENTS | SEC_RELOC)
     : (SEC_ALLOC | SEC_LOAD | SEC_DATA | SEC_HAS_CONTENTS));
  obj_bsssec (abfd)->flags = SEC_ALLOC;

  result = (*callback_to_real_object_p) (abfd);

#if defined(MACH) || defined(STAT_FOR_EXEC)
  /* The original heuristic doesn't work in some important cases. The
     a.out file has no information about the text start address. For
     files (like kernels) linked to non-standard addresses (ld -Ttext
     nnn) the entry point may not be between the default text start
     (obj_textsec(abfd)->vma) and (obj_textsec(abfd)->vma) + text size
     This is not just a mach issue. Many kernels are loaded at non
     standard addresses.  */
  {
    struct stat stat_buf;

    if (abfd->iostream != NULL
	&& (abfd->flags & BFD_IN_MEMORY) == 0
        && (fstat(fileno((FILE *) (abfd->iostream)), &stat_buf) == 0)
        && ((stat_buf.st_mode & 0111) != 0))
      abfd->flags |= EXEC_P;
  }
#else /* ! MACH */
  /* Now that the segment addresses have been worked out, take a better
     guess at whether the file is executable.  If the entry point
     is within the text segment, assume it is.  (This makes files
     executable even if their entry point address is 0, as long as
     their text starts at zero.)

     At some point we should probably break down and stat the file and
     declare it executable if (one of) its 'x' bits are on...  */
  if ((execp->a_entry >= obj_textsec(abfd)->vma) &&
      (execp->a_entry < obj_textsec(abfd)->vma + obj_textsec(abfd)->size))
    abfd->flags |= EXEC_P;
#endif /* MACH */
  if (result == NULL)
    {
      free (rawptr);
      abfd->tdata.aout_data = oldrawptr;
    }
  return result;
}

static const bfd_target *
MY (object_p) (bfd *abfd)
{
  struct external_exec exec_bytes;      /* Raw exec header from file.  */
  struct internal_exec exec;            /* Cleaned-up exec header.  */
  const bfd_target *target;

  if (bfd_bread ((void *) &exec_bytes, (bfd_size_type) EXEC_BYTES_SIZE, abfd)
      != EXEC_BYTES_SIZE)
    {
      if (bfd_get_error () != bfd_error_system_call)
	bfd_set_error (bfd_error_wrong_format);
      return NULL;
    }

  exec.a_info = H_GET_32 (abfd, exec_bytes.e_info);

  if (N_BADMAG (exec))
    return NULL;

#ifdef MACHTYPE_OK
  if (!(MACHTYPE_OK (N_MACHTYPE (exec))))
    return NULL;
#endif

  NAME (aout, swap_exec_header_in) (abfd, & exec_bytes, & exec);

  target = riscix_some_aout_object_p (abfd, & exec, MY (callback));

  return target;
}

#include "aout-target.h"
