/* BFD back-end for Intel 386 PE IMAGE COFF files.
   Copyright 2006, 2007, 2009 Free Software Foundation, Inc.

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
   MA 02110-1301, USA.
   
   Written by Kai Tietz, OneVision Software GmbH&CoKg.  */

#include "sysdep.h"
#include "bfd.h"

#define TARGET_SYM 		x86_64pei_vec
#define TARGET_NAME 		"pei-x86-64"
#define COFF_IMAGE_WITH_PE
#define COFF_WITH_PE
#define COFF_WITH_pex64
#define PCRELOFFSET 		TRUE
#if defined (USE_MINGW64_LEADING_UNDERSCORES)
#define TARGET_UNDERSCORE 	'_'
#else
#define TARGET_UNDERSCORE 	0
#endif
/* Long section names not allowed in executable images, only object files.  */
#define COFF_LONG_SECTION_NAMES 0
#define COFF_SUPPORT_GNU_LINKONCE
#define COFF_LONG_FILENAMES
#define PDATA_ROW_SIZE	(3 * 4)

#define COFF_SECTION_ALIGNMENT_ENTRIES \
{ COFF_SECTION_NAME_EXACT_MATCH (".bss"), \
  COFF_ALIGNMENT_FIELD_EMPTY, COFF_ALIGNMENT_FIELD_EMPTY, 4 }, \
{ COFF_SECTION_NAME_PARTIAL_MATCH (".data"), \
  COFF_ALIGNMENT_FIELD_EMPTY, COFF_ALIGNMENT_FIELD_EMPTY, 4 }, \
{ COFF_SECTION_NAME_PARTIAL_MATCH (".rdata"), \
  COFF_ALIGNMENT_FIELD_EMPTY, COFF_ALIGNMENT_FIELD_EMPTY, 4 }, \
{ COFF_SECTION_NAME_PARTIAL_MATCH (".text"), \
  COFF_ALIGNMENT_FIELD_EMPTY, COFF_ALIGNMENT_FIELD_EMPTY, 4 }, \
{ COFF_SECTION_NAME_PARTIAL_MATCH (".idata"), \
  COFF_ALIGNMENT_FIELD_EMPTY, COFF_ALIGNMENT_FIELD_EMPTY, 2 }, \
{ COFF_SECTION_NAME_EXACT_MATCH (".pdata"), \
  COFF_ALIGNMENT_FIELD_EMPTY, COFF_ALIGNMENT_FIELD_EMPTY, 2 }, \
{ COFF_SECTION_NAME_PARTIAL_MATCH (".debug"), \
  COFF_ALIGNMENT_FIELD_EMPTY, COFF_ALIGNMENT_FIELD_EMPTY, 0 }, \
{ COFF_SECTION_NAME_PARTIAL_MATCH (".gnu.linkonce.wi."), \
  COFF_ALIGNMENT_FIELD_EMPTY, COFF_ALIGNMENT_FIELD_EMPTY, 0 }

/* Note we have to make sure not to include headers twice.
   Not all headers are wrapped in #ifdef guards, so we define
   PEI_HEADERS to prevent double including in coff-x86_64.c  */
#define PEI_HEADERS
#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "coff/x86_64.h"
#include "coff/internal.h"
#include "coff/pe.h"
#include "libcoff.h"
#include "libpei.h"
#include "libiberty.h"

#undef AOUTSZ
#define AOUTSZ		PEPAOUTSZ
#define PEAOUTHDR	PEPAOUTHDR

static const char *pex_regs[16] = {
  "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

static void
pex64_get_runtime_function (bfd *abfd, struct pex64_runtime_function *rf,
			    const void *data)
{
  const struct external_pex64_runtime_function *ex_rf =
    (const struct external_pex64_runtime_function *) data;
  rf->rva_BeginAddress = bfd_get_32 (abfd, ex_rf->rva_BeginAddress);
  rf->rva_EndAddress = bfd_get_32 (abfd, ex_rf->rva_EndAddress);
  rf->rva_UnwindData =	bfd_get_32 (abfd, ex_rf->rva_UnwindData);
  rf->isChained = PEX64_IS_RUNTIME_FUNCTION_CHAINED (rf);
  rf->rva_UnwindData = PEX64_GET_UNWINDDATA_UNIFIED_RVA (rf);
}

static void
pex64_get_unwind_info (bfd *abfd, struct pex64_unwind_info *ui, void *data)
{
  struct external_pex64_unwind_info *ex_ui =
    (struct external_pex64_unwind_info *) data;
  bfd_byte *ex_dta = (bfd_byte *) data;

  memset (ui, 0, sizeof (struct pex64_unwind_info));
  ui->Version = PEX64_UWI_VERSION (ex_ui->Version_Flags);
  ui->Flags = PEX64_UWI_FLAGS (ex_ui->Version_Flags);
  ui->SizeOfPrologue = (bfd_vma) ex_ui->SizeOfPrologue;
  ui->CountOfCodes = (bfd_vma) ex_ui->CountOfCodes;
  ui->FrameRegister = PEX64_UWI_FRAMEREG (ex_ui->FrameRegisterOffset);
  ui->FrameOffset = PEX64_UWI_FRAMEOFF (ex_ui->FrameRegisterOffset);
  ui->sizeofUnwindCodes = PEX64_UWI_SIZEOF_UWCODE_ARRAY (ui->CountOfCodes);
  ui->SizeOfBlock = ui->sizeofUnwindCodes + 4;
  ui->rawUnwindCodes = &ex_dta[4];
  ex_dta += ui->SizeOfBlock;
  switch (ui->Flags)
    {
    case UNW_FLAG_CHAININFO:
      ui->rva_FunctionEntry = bfd_get_32 (abfd, ex_dta);
      ui->SizeOfBlock += 4;
      return;
    default:
      return;
    }
}

static void
pex64_xdata_print_uwd_codes (FILE *file, struct pex64_unwind_info *ui,
			     bfd_vma pc_addr)
{
  bfd_vma i;
  bfd_vma tmp = 0;
  const bfd_byte *insns[256];
  bfd_vma insns_count = 0;
  const bfd_byte *dta = ui->rawUnwindCodes;

  if (ui->CountOfCodes == 0 || !dta)
    return;

  /* Sort array ascending. Note: it is stored in reversed order.  */
  for (i = 0; i < ui->CountOfCodes; i++)
    {
      const bfd_byte *t;

      t = insns[insns_count++] = &dta[i * 2];
      switch (PEX64_UNWCODE_CODE (t[1]))
	{
	case UWOP_PUSH_NONVOL:
	case UWOP_ALLOC_SMALL:
	case UWOP_SET_FPREG:
	case UWOP_PUSH_MACHFRAME:
	  break;
	case UWOP_ALLOC_LARGE:
	  if (PEX64_UNWCODE_INFO (t[1]) == 0)
	    {
	      i += 1;
	      break;
	    }
	  else if (PEX64_UNWCODE_INFO (t[1]) == 1)
	    {
	      i += 2;
	      break;
	    }
	  /* fall through.  */
	default:
	  fprintf (file, "\t contains unknown code (%u).\n",
		   (unsigned int) PEX64_UNWCODE_CODE (t[1]));
	  return;
	case UWOP_SAVE_NONVOL:
	case UWOP_SAVE_XMM:
	case UWOP_SAVE_XMM128:
	  i++;
	  break;
	case UWOP_SAVE_NONVOL_FAR:
	case UWOP_SAVE_XMM_FAR:
	case UWOP_SAVE_XMM128_FAR:
	  i += 2;
	  break;
	}
    }
  fprintf (file, "\t At pc 0x");
  fprintf_vma (file, pc_addr);
  fprintf (file, " there are the following saves (in logical order).\n");
  for (i = insns_count; i > 0;)
    {
      --i;
      dta = insns[i];
      fprintf (file, "\t  insn ends at pc+0x%02x: ", (unsigned int) dta[0]);
      switch (PEX64_UNWCODE_CODE (dta[1]))
	{
	case UWOP_PUSH_NONVOL:
	  fprintf (file, "push %s.\n", pex_regs[PEX64_UNWCODE_INFO (dta[1])]);
	  break;
	case UWOP_ALLOC_LARGE:
	  if (PEX64_UNWCODE_INFO (dta[1]) == 0)
	    {
	      tmp = (bfd_vma) (*((unsigned short *) &dta[2]));
	      tmp *= 8;
	    }
	  else
	    tmp = (bfd_vma) (*((unsigned int *)&dta[2]));
	  fprintf (file, "save stack region of size 0x");
	  fprintf_vma (file, tmp);
	  fprintf (file,".\n");
	  break;
	case UWOP_ALLOC_SMALL:
	  tmp = (bfd_vma) PEX64_UNWCODE_INFO (dta[1]);
	  tmp += 1;
	  tmp *= 8;
	  fprintf (file, "save stack region of size 0x");
	  fprintf_vma (file, tmp);
	  fprintf (file,".\n");
	  break;
	case UWOP_SET_FPREG:
	  tmp = (bfd_vma) PEX64_UNWCODE_INFO (dta[1]);
	  tmp *= 16;
	  fprintf (file, "FPReg = (FrameReg) + 0x");
	  fprintf_vma (file, tmp);
	  fprintf (file, ".\n");
	  break;
	case UWOP_SAVE_NONVOL:
	  fprintf (file, "mov %s at 0x",
		   pex_regs[PEX64_UNWCODE_INFO (dta[1])]);
	  tmp = (bfd_vma) (*((unsigned short *) &dta[2]));
	  tmp *= 8;
	  fprintf_vma (file, tmp);
	  fprintf (file, ".\n");
	  break;
	case UWOP_SAVE_NONVOL_FAR:
	  fprintf (file, "mov %s at 0x",
		   pex_regs[PEX64_UNWCODE_INFO (dta[1])]);
	  tmp = (bfd_vma) (*((unsigned int *) &dta[2]));
	  fprintf_vma (file, tmp);
	  fprintf (file, ".\n");
	  break;
	case UWOP_SAVE_XMM:
	  tmp = (bfd_vma) (*((unsigned short *) &dta[2]));
	  tmp *= 8;
	  fprintf (file, "mov mm%u at 0x",
		   (unsigned int) PEX64_UNWCODE_INFO (dta[1]));
	  fprintf_vma (file, tmp);
	  fprintf (file, ".\n");
	  break;
	case UWOP_SAVE_XMM_FAR:
	  tmp = (bfd_vma) (*((unsigned int *) &dta[2]));
	  fprintf (file, "mov mm%u at 0x",
		   (unsigned int) PEX64_UNWCODE_INFO (dta[1]));
	  fprintf_vma (file, tmp);
	  fprintf (file, ".\n");
	  break;
	case UWOP_SAVE_XMM128:
	  tmp = (bfd_vma) (*((unsigned short *) &dta[2]));
	  tmp *= 16;
	  fprintf (file, "mov xmm%u at 0x",
		   (unsigned int) PEX64_UNWCODE_INFO ( dta[1]));
	  fprintf_vma (file, tmp);
	  fprintf (file, ".\n");
	  break;
	case UWOP_SAVE_XMM128_FAR:
	  tmp = (bfd_vma) (*((unsigned int *) &dta[2]));
	  fprintf (file, "mov xmm%u at 0x",
		   (unsigned int) PEX64_UNWCODE_INFO (dta[1]));
	  fprintf_vma (file, tmp);
	  fprintf (file, ".\n");
	  break;
	case UWOP_PUSH_MACHFRAME:
	  fprintf (file, "interrupt entry (SS, old RSP, EFLAGS, CS, RIP");
	  if (PEX64_UNWCODE_INFO (dta[1]) == 0)
	    {
	      fprintf (file, ")");
	    }
	  else if (PEX64_UNWCODE_INFO (dta[1]) == 1)
	    {
	      fprintf (file, ",ErrorCode)");
	    }
	  else
	    fprintf (file, ", unknown(%u))",
		     (unsigned int) PEX64_UNWCODE_INFO (dta[1]));
	  fprintf (file,".\n");
	  break;
	default:
	  fprintf (file, "unknown code %u.\n",
		   (unsigned int) PEX64_UNWCODE_INFO (dta[1]));
	  break;
      }
    }
}

static asection *
pex64_get_section_by_rva (bfd *abfd, bfd_vma addr, const char *sec_name)
{
  asection *section = bfd_get_section_by_name (abfd, sec_name);
  bfd_vma vsize;
  bfd_size_type datasize = 0;

  if (section == NULL
      || coff_section_data (abfd, section) == NULL
      || pei_section_data (abfd, section) == NULL)
    return NULL;
  vsize = section->vma - pe_data (abfd)->pe_opthdr.ImageBase;
  datasize = section->size;
  if (!datasize || vsize > addr || (vsize + datasize) < addr)
    return NULL;
  return section;
}

static void
pex64_dump_xdata (FILE *file, bfd *abfd, bfd_vma addr, bfd_vma pc_addr,
		  bfd_vma *endx)
{
  asection *section = pex64_get_section_by_rva (abfd, addr, ".rdata");
  bfd_vma vsize;
  bfd_byte *data = NULL;
  bfd_vma end_addr;

  if (!section)
    section = pex64_get_section_by_rva (abfd, addr, ".data");
  if (!section)
    section = pex64_get_section_by_rva (abfd, addr, ".xdata");
  if (!section)
    {
      section = pex64_get_section_by_rva (abfd, addr, ".pdata");
      if (section)
	{
	  fprintf (file, "\t Shares information with pdata element at 0x");
	  fprintf_vma (file, addr + pe_data (abfd)->pe_opthdr.ImageBase);
	  fprintf (file, ".\n");
	}
    }
  if (!section)
    return;

  vsize = section->vma - pe_data (abfd)->pe_opthdr.ImageBase;
  addr -= vsize;

  if (endx)
    end_addr = endx[0] - vsize;
  else
    end_addr = (section->rawsize != 0 ? section->rawsize : section->size);

  if (bfd_malloc_and_get_section (abfd, section, &data))
    {
      struct pex64_unwind_info ui;

      if (!data)
	return;

      pex64_get_unwind_info (abfd, &ui, &data[addr]);

      if (ui.Version != 1)
	{
	  fprintf (file, "\tVersion %u (unknown).\n", (unsigned int) ui.Version);
	  return;
	}

      fprintf (file, "\tFlags: ");
      switch (ui.Flags)
	{
	case UNW_FLAG_NHANDLER:
	  fprintf (file, "UNW_FLAG_NHANDLER");
	  break;
	case UNW_FLAG_EHANDLER:
	  fprintf (file, "UNW_FLAG_EHANDLER");
	  break;
	case UNW_FLAG_UHANDLER:
	  fprintf (file, "UNW_FLAG_UHANDLER");
	  break;
	case UNW_FLAG_FHANDLER:
	  fprintf (file, "UNW_FLAG_FHANDLER = (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER)");
	  break;
	case UNW_FLAG_CHAININFO:
	  fprintf (file, "UNW_FLAG_CHAININFO");
	  break;
	default:
	  fprintf (file, "unknown flags value 0x%x", (unsigned int) ui.Flags);
	  break;
	}
      fprintf (file, ".\n");
      if (ui.CountOfCodes != 0)
	fprintf (file, "\tEntry has %u codes.", (unsigned int) ui.CountOfCodes);
      fprintf (file, "\tPrologue size: %u, Frame offset = 0x%x.\n",
	       (unsigned int) ui.SizeOfPrologue, (unsigned int) ui.FrameOffset);
      fprintf (file, "\tFrame register is %s.\n",
	ui.FrameRegister == 0 ? "none"
			      : pex_regs[(unsigned int) ui.FrameRegister]);

      pex64_xdata_print_uwd_codes (file, &ui, pc_addr);
      
      /* Now we need end of this xdata block.  */
      addr += ui.SizeOfBlock;
      if (addr < end_addr)
        {
	  unsigned int i;
	  fprintf (file,"\tUser data:\n");
	  for (i = 0; addr < end_addr; addr += 1, i++)
	    {
	      if ((i & 15) == 0)
	        fprintf (file, "\t  %03x:", i);
	      fprintf (file, " %02x", data[addr]);
	      if ((i & 15) == 15)
	        fprintf (file, "\n");
	    }
	  if ((i & 15) != 0)
	    fprintf (file, "\n");
        }
    }
  if (data != NULL)
    free (data);
}

static int
sort_xdata_arr (const void *l, const void *r)
{
  const bfd_vma *lp = (const bfd_vma *) l;
  const bfd_vma *rp = (const bfd_vma *) r;

  if (*lp == *rp)
    return 0;
  return (*lp < *rp ? -1 : 1);
}

static bfd_boolean
pex64_bfd_print_pdata (bfd *abfd, void *vfile)
{
  FILE *file = (FILE *) vfile;
  bfd_byte *data = NULL;
  asection *section = bfd_get_section_by_name (abfd, ".pdata");
  bfd_size_type datasize = 0;
  bfd_size_type i;
  bfd_size_type stop;
  bfd_vma prev_beginaddress = 0;
  int onaline = PDATA_ROW_SIZE;
  int seen_error = 0;
  bfd_vma *xdata_arr;
  int xdata_arr_cnt;

  if (section == NULL
      || coff_section_data (abfd, section) == NULL
      || pei_section_data (abfd, section) == NULL)
    return TRUE;

  stop = pei_section_data (abfd, section)->virt_size;
  if ((stop % onaline) != 0)
    fprintf (file,
	     _("warning: .pdata section size (%ld) is not a multiple of %d\n"),
	     (long) stop, onaline);

  fprintf (file,
	   _("\nThe Function Table (interpreted .pdata section contents)\n"));

  fprintf (file, _("vma:\t\t\tBeginAddress\t EndAddress\t  UnwindData\n"));

  datasize = section->size;
  if (datasize == 0)
    return TRUE;

  if (!bfd_malloc_and_get_section (abfd, section, &data))
    {
      if (data != NULL)
	free (data);
      return FALSE;
    }

  xdata_arr = (bfd_vma *) xmalloc (sizeof (bfd_vma) * ((stop / onaline) + 1));
  xdata_arr_cnt = 0;
  /* Do sanity check of pdata.  */
  for (i = 0; i < stop; i += onaline)
    {
      struct pex64_runtime_function rf;

      if (i + PDATA_ROW_SIZE > stop)
	break;
      pex64_get_runtime_function (abfd, &rf, &data[i]);

      if (rf.rva_BeginAddress == 0 && rf.rva_EndAddress == 0
	  && rf.rva_UnwindData == 0)
	/* We are probably into the padding of the section now.  */
	break;
      fputc (' ', file);
      fprintf_vma (file, i + section->vma);
      fprintf (file, ":\t");
      fprintf_vma (file, rf.rva_BeginAddress);
      fputc (' ', file);
      fprintf_vma (file, rf.rva_EndAddress);
      fputc (' ', file);
      fprintf_vma (file, rf.rva_UnwindData);
      fprintf (file, "\n");
      if (i != 0 && rf.rva_BeginAddress <= prev_beginaddress)
	{
	  seen_error = 1;
	  fprintf (file, "  has %s begin address as predecessor\n",
	    (rf.rva_BeginAddress < prev_beginaddress ? "smaller" : "same"));
        }
      prev_beginaddress = rf.rva_BeginAddress;
      /* Now we check for negative addresses.  */
      if ((prev_beginaddress & 0x80000000) != 0)
	{
	  seen_error = 1;
	  fprintf (file, "  has negative begin address\n");
	}
      if ((rf.rva_EndAddress & 0x80000000) != 0)
	{
	  seen_error = 1;
	  fprintf (file, "  has negative end address\n");
	}
      if ((rf.rva_UnwindData & 0x80000000) != 0)
	{
	  seen_error = 1;
	  fprintf (file, "  has negative unwind address\n");
	}
      if (rf.rva_UnwindData && !rf.isChained)
        xdata_arr[xdata_arr_cnt++] = rf.rva_UnwindData;
    }

  if (seen_error)
    {
      free (data);
      free (xdata_arr);

      return TRUE;
    }

  /* Add end of list marker.  */
  xdata_arr[xdata_arr_cnt++] = ~((bfd_vma) 0);

  /* Sort start RVAs of xdata.  */
  if (xdata_arr_cnt > 1)
    qsort (xdata_arr, (size_t) xdata_arr_cnt, sizeof (bfd_vma),
	   sort_xdata_arr);

  /* Do dump of pdata related xdata.  */

  for (i = 0; i < stop; i += onaline)
    {
      struct pex64_runtime_function rf;

      if (i + PDATA_ROW_SIZE > stop)
	break;
      pex64_get_runtime_function (abfd, &rf, &data[i]);

      if (rf.rva_BeginAddress == 0 && rf.rva_EndAddress == 0
	  && rf.rva_UnwindData == 0)
	/* We are probably into the padding of the section now.  */
	break;
      if (i == 0)
        fprintf (file, "\nDump of .xdata\n");
      fputc (' ', file);
      fprintf_vma (file, rf.rva_UnwindData);
      fprintf (file, ":\n");

      rf.rva_BeginAddress += pe_data (abfd)->pe_opthdr.ImageBase;
      rf.rva_EndAddress += pe_data (abfd)->pe_opthdr.ImageBase;

      if (rf.rva_UnwindData != 0)
	{
	  if (rf.isChained)
	    {
	      fprintf (file, "\t shares information with pdata element at 0x");
	      fprintf_vma (file, rf.rva_UnwindData);
	      fprintf (file, ".\n");
	    }
	  else
	    {
	      bfd_vma *p;

	      /* Search for the current entry in the sorted array.  */
	      p = (bfd_vma *)
	          bsearch (&rf.rva_UnwindData, xdata_arr,
			   (size_t) xdata_arr_cnt, sizeof (bfd_vma),
			   sort_xdata_arr);

	      /* Advance to the next pointer into the xdata section.  We may
		 have shared xdata entries, which will result in a string of
		 identical pointers in the array; advance past all of them.  */
	      while (p[0] <= rf.rva_UnwindData)
		++p;
	      if (p[0] == ~((bfd_vma) 0))
		p = NULL;

	      pex64_dump_xdata (file, abfd, rf.rva_UnwindData,
				rf.rva_BeginAddress, p);
	    }
	}
    }

  free (data);
  free (xdata_arr);

  return TRUE;
}

#define bfd_pe_print_pdata   pex64_bfd_print_pdata

#include "coff-x86_64.c"
