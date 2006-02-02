/* b.out object file format
   Copyright 1989, 1990, 1991, 1992, 1993, 1994, 1996, 2000, 2001, 2002,
   2003, 2005 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2,
   or (at your option) any later version.

   GAS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include "as.h"
#include "obstack.h"

/* In: segT   Out: N_TYPE bits  */
const short seg_N_TYPE[] =
{
  N_ABS,
  N_TEXT,
  N_DATA,
  N_BSS,
  N_UNDF,			/* unknown  */
  N_UNDF,			/* error  */
  N_UNDF,			/* expression  */
  N_UNDF,			/* debug  */
  N_UNDF,			/* ntv  */
  N_UNDF,			/* ptv  */
  N_REGISTER,			/* register  */
};

const segT N_TYPE_seg[N_TYPE + 2] =
{				/* N_TYPE == 0x1E = 32-2  */
  SEG_UNKNOWN,			/* N_UNDF == 0  */
  SEG_GOOF,
  SEG_ABSOLUTE,			/* N_ABS == 2  */
  SEG_GOOF,
  SEG_TEXT,			/* N_TEXT == 4  */
  SEG_GOOF,
  SEG_DATA,			/* N_DATA == 6  */
  SEG_GOOF,
  SEG_BSS,			/* N_BSS == 8  */
  SEG_GOOF,
  SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF,
  SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF,
  SEG_GOOF, SEG_GOOF, SEG_GOOF, SEG_GOOF,
  SEG_REGISTER,			/* dummy N_REGISTER for regs = 30  */
  SEG_GOOF,
};

static void obj_bout_line PARAMS ((int));

const pseudo_typeS obj_pseudo_table[] =
{
  {"line", obj_bout_line, 0},	/* Source code line number.  */

/* coff debugging directives.  Currently ignored silently.  */
  {"def", s_ignore, 0},
  {"dim", s_ignore, 0},
  {"endef", s_ignore, 0},
  {"ln", s_ignore, 0},
  {"scl", s_ignore, 0},
  {"size", s_ignore, 0},
  {"tag", s_ignore, 0},
  {"type", s_ignore, 0},
  {"val", s_ignore, 0},

/* other stuff we don't handle */
  {"ABORT", s_ignore, 0},
  {"ident", s_ignore, 0},

  {NULL, NULL, 0}		/* End sentinel.  */
};

/* Relocation.  */

/* Crawl along a fixS chain. Emit the segment's relocations.  */

void
obj_emit_relocations (where, fixP, segment_address_in_file)
     char **where;
     fixS *fixP;		/* Fixup chain for this segment.  */
     relax_addressT segment_address_in_file;
{
  for (; fixP; fixP = fixP->fx_next)
    {
      if (fixP->fx_done == 0
	  || fixP->fx_r_type != NO_RELOC)
	{
	  symbolS *sym;

	  sym = fixP->fx_addsy;
	  while (sym->sy_value.X_op == O_symbol
		 && (! S_IS_DEFINED (sym) || S_IS_COMMON (sym)))
	    sym = sym->sy_value.X_add_symbol;
	  fixP->fx_addsy = sym;

	  tc_bout_fix_to_chars (*where, fixP, segment_address_in_file);
	  *where += md_reloc_size;
	}			/* if there's a symbol  */
    }				/* for each fixup  */
}

/* Aout file generation & utilities .  */

/* Convert a lvalue to machine dependent data.  */

void
obj_header_append (where, headers)
     char **where;
     object_headers *headers;
{
  /* Always leave in host byte order.  */
  char *p;

  headers->header.a_talign = section_alignment[SEG_TEXT];

  /* Force to at least 2.  */
  if (headers->header.a_talign < 2)
    {
      headers->header.a_talign = 2;
    }

  headers->header.a_dalign = section_alignment[SEG_DATA];
  headers->header.a_balign = section_alignment[SEG_BSS];

  headers->header.a_tload = 0;
  headers->header.a_dload =
    md_section_align (SEG_DATA, H_GET_TEXT_SIZE (headers));

  headers->header.a_relaxable = linkrelax;

  p = *where;
  host_number_to_chars (p, headers->header.a_magic, 4);
  p += 4;
  host_number_to_chars (p, headers->header.a_text, 4);
  p += 4;
  host_number_to_chars (p, headers->header.a_data, 4);
  p += 4;
  host_number_to_chars (p, headers->header.a_bss, 4);
  p += 4;
  host_number_to_chars (p, headers->header.a_syms, 4);
  p += 4;
  host_number_to_chars (p, headers->header.a_entry, 4);
  p += 4;
  host_number_to_chars (p, headers->header.a_trsize, 4);
  p += 4;
  host_number_to_chars (p, headers->header.a_drsize, 4);
  p += 4;
  host_number_to_chars (p, headers->header.a_tload, 4);
  p += 4;
  host_number_to_chars (p, headers->header.a_dload, 4);
  p += 4;
  *p++ = headers->header.a_talign;
  *p++ = headers->header.a_dalign;
  *p++ = headers->header.a_balign;
  *p++ = headers->header.a_relaxable;
  *where = p;
}

void
obj_symbol_to_chars (where, symbolP)
     char **where;
     symbolS *symbolP;
{
  char *p = *where;
  host_number_to_chars (p, S_GET_OFFSET (symbolP), 4);
  p += 4;
  /* Can't use S_GET_TYPE here as it masks.  */
  *p++ = symbolP->sy_symbol.n_type;
  *p++ = symbolP->sy_symbol.n_other;
  host_number_to_chars (p, S_GET_DESC (symbolP), 2);
  p += 2;
  host_number_to_chars (p, S_GET_VALUE (symbolP), 4);
  p += 4;
  *where = p;
}

void
obj_emit_symbols (where, symbol_rootP)
     char **where;
     symbolS *symbol_rootP;
{
  symbolS *symbolP;

  /* Emit all symbols left in the symbol chain.  */
  for (symbolP = symbol_rootP; symbolP; symbolP = symbol_next (symbolP))
    {
      /* Used to save the offset of the name.  It is used to point to
	 the string in memory but must be a file offset.  */
      char *temp;

      temp = S_GET_NAME (symbolP);
      S_SET_OFFSET (symbolP, symbolP->sy_name_offset);

      /* Any symbol still undefined and is not a dbg symbol is made N_EXT.  */
      if (!S_IS_DEBUG (symbolP) && !S_IS_DEFINED (symbolP))
	S_SET_EXTERNAL (symbolP);

      obj_symbol_to_chars (where, symbolP);
      S_SET_NAME (symbolP, temp);
    }
}

void
obj_symbol_new_hook (symbolP)
     symbolS *symbolP;
{
  S_SET_OTHER (symbolP, 0);
  S_SET_DESC (symbolP, 0);
}

static void
obj_bout_line (ignore)
     int ignore ATTRIBUTE_UNUSED;
{
  /* Assume delimiter is part of expression.  */
  /* BSD4.2 as fails with delightful bug, so we are not being
     incompatible here.  */
  new_logical_line ((char *) NULL, (int) (get_absolute_expression ()));
  demand_empty_rest_of_line ();
}

void
obj_read_begin_hook ()
{
}

void
obj_crawl_symbol_chain (headers)
     object_headers *headers;
{
  symbolS **symbolPP;
  symbolS *symbolP;
  int symbol_number = 0;

  tc_crawl_symbol_chain (headers);

  symbolPP = &symbol_rootP;	/* -> last symbol chain link.  */
  while ((symbolP = *symbolPP) != NULL)
    {
      if (flag_readonly_data_in_text && (S_GET_SEGMENT (symbolP) == SEG_DATA))
	{
	  S_SET_SEGMENT (symbolP, SEG_TEXT);
	}			/* if pushing data into text  */

      resolve_symbol_value (symbolP);

      /* Skip symbols which were equated to undefined or common
	 symbols.  */
      if (symbolP->sy_value.X_op == O_symbol
	  && (! S_IS_DEFINED (symbolP) || S_IS_COMMON (symbolP)))
	{
	  *symbolPP = symbol_next (symbolP);
	  continue;
	}

      /* OK, here is how we decide which symbols go out into the
	 brave new symtab.  Symbols that do are:

	 * symbols with no name (stabd's?)
	 * symbols with debug info in their N_TYPE

	 Symbols that don't are:
	 * symbols that are registers
	 * symbols with \1 as their 3rd character (numeric labels)
	 * "local labels" as defined by S_LOCAL_NAME(name)
	 if the -L switch was passed to gas.

	 All other symbols are output.  We complain if a deleted
	 symbol was marked external.  */

      if (1
	  && !S_IS_REGISTER (symbolP)
	  && (!S_GET_NAME (symbolP)
	      || S_IS_DEBUG (symbolP)
#ifdef TC_I960
      /* FIXME-SOON this ifdef seems highly dubious to me.  xoxorich.  */
	      || !S_IS_DEFINED (symbolP)
	      || S_IS_EXTERNAL (symbolP)
#endif /* TC_I960 */
	      || (S_GET_NAME (symbolP)[0] != '\001'
		  && (flag_keep_locals || !S_LOCAL_NAME (symbolP)))))
	{
	  symbolP->sy_number = symbol_number++;

	  /* The + 1 after strlen account for the \0 at the end of
	     each string.  */
	  if (!S_IS_STABD (symbolP))
	    {
	      /* Ordinary case.  */
	      symbolP->sy_name_offset = string_byte_count;
	      string_byte_count += strlen (S_GET_NAME (symbolP)) + 1;
	    }
	  else			/* .Stabd case.  */
	    symbolP->sy_name_offset = 0;
	  symbolPP = &(symbolP->sy_next);
	}
      else
	{
	  if (S_IS_EXTERNAL (symbolP) || !S_IS_DEFINED (symbolP))
	    {
	      as_bad (_("Local symbol %s never defined"),
		      S_GET_NAME (symbolP));
	    }			/* Oops.  */

	  /* Unhook it from the chain.  */
	  *symbolPP = symbol_next (symbolP);
	}			/* if this symbol should be in the output  */
    }				/* for each symbol  */

  H_SET_SYMBOL_TABLE_SIZE (headers, symbol_number);
}

/* Find strings by crawling along symbol table chain.  */

void
obj_emit_strings (where)
     char **where;
{
  symbolS *symbolP;

  md_number_to_chars (*where, string_byte_count, 4);
  *where += 4;

  for (symbolP = symbol_rootP; symbolP; symbolP = symbol_next (symbolP))
    {
      if (S_GET_NAME (symbolP))
	append (where, S_GET_NAME (symbolP),
		(unsigned long) (strlen (S_GET_NAME (symbolP)) + 1));
    }				/* Walk symbol chain.  */
}
