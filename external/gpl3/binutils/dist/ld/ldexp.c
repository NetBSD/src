/* This module handles expression trees.
   Copyright 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012
   Free Software Foundation, Inc.
   Written by Steve Chamberlain of Cygnus Support <sac@cygnus.com>.

   This file is part of the GNU Binutils.

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


/* This module is in charge of working out the contents of expressions.

   It has to keep track of the relative/absness of a symbol etc. This
   is done by keeping all values in a struct (an etree_value_type)
   which contains a value, a section to which it is relative and a
   valid bit.  */

#include "sysdep.h"
#include "bfd.h"
#include "bfdlink.h"

#include "ld.h"
#include "ldmain.h"
#include "ldmisc.h"
#include "ldexp.h"
#include "ldlex.h"
#include <ldgram.h>
#include "ldlang.h"
#include "libiberty.h"
#include "safe-ctype.h"

static void exp_fold_tree_1 (etree_type *);
static bfd_vma align_n (bfd_vma, bfd_vma);

segment_type *segments;

struct ldexp_control expld;

/* Print the string representation of the given token.  Surround it
   with spaces if INFIX_P is TRUE.  */

static void
exp_print_token (token_code_type code, int infix_p)
{
  static const struct
  {
    token_code_type code;
    const char * name;
  }
  table[] =
  {
    { INT, "int" },
    { NAME, "NAME" },
    { PLUSEQ, "+=" },
    { MINUSEQ, "-=" },
    { MULTEQ, "*=" },
    { DIVEQ, "/=" },
    { LSHIFTEQ, "<<=" },
    { RSHIFTEQ, ">>=" },
    { ANDEQ, "&=" },
    { OREQ, "|=" },
    { OROR, "||" },
    { ANDAND, "&&" },
    { EQ, "==" },
    { NE, "!=" },
    { LE, "<=" },
    { GE, ">=" },
    { LSHIFT, "<<" },
    { RSHIFT, ">>" },
    { ALIGN_K, "ALIGN" },
    { BLOCK, "BLOCK" },
    { QUAD, "QUAD" },
    { SQUAD, "SQUAD" },
    { LONG, "LONG" },
    { SHORT, "SHORT" },
    { BYTE, "BYTE" },
    { SECTIONS, "SECTIONS" },
    { SIZEOF_HEADERS, "SIZEOF_HEADERS" },
    { MEMORY, "MEMORY" },
    { DEFINED, "DEFINED" },
    { TARGET_K, "TARGET" },
    { SEARCH_DIR, "SEARCH_DIR" },
    { MAP, "MAP" },
    { ENTRY, "ENTRY" },
    { NEXT, "NEXT" },
    { ALIGNOF, "ALIGNOF" },
    { SIZEOF, "SIZEOF" },
    { ADDR, "ADDR" },
    { LOADADDR, "LOADADDR" },
    { CONSTANT, "CONSTANT" },
    { ABSOLUTE, "ABSOLUTE" },
    { MAX_K, "MAX" },
    { MIN_K, "MIN" },
    { ASSERT_K, "ASSERT" },
    { REL, "relocatable" },
    { DATA_SEGMENT_ALIGN, "DATA_SEGMENT_ALIGN" },
    { DATA_SEGMENT_RELRO_END, "DATA_SEGMENT_RELRO_END" },
    { DATA_SEGMENT_END, "DATA_SEGMENT_END" },
    { ORIGIN, "ORIGIN" },
    { LENGTH, "LENGTH" },
    { SEGMENT_START, "SEGMENT_START" }
  };
  unsigned int idx;

  for (idx = 0; idx < ARRAY_SIZE (table); idx++)
    if (table[idx].code == code)
      break;

  if (infix_p)
    fputc (' ', config.map_file);

  if (idx < ARRAY_SIZE (table))
    fputs (table[idx].name, config.map_file);
  else if (code < 127)
    fputc (code, config.map_file);
  else
    fprintf (config.map_file, "<code %d>", code);

  if (infix_p)
    fputc (' ', config.map_file);
}

static void
make_abs (void)
{
  if (expld.result.section != NULL)
    expld.result.value += expld.result.section->vma;
  expld.result.section = bfd_abs_section_ptr;
}

static void
new_abs (bfd_vma value)
{
  expld.result.valid_p = TRUE;
  expld.result.section = bfd_abs_section_ptr;
  expld.result.value = value;
  expld.result.str = NULL;
}

etree_type *
exp_intop (bfd_vma value)
{
  etree_type *new_e = (etree_type *) stat_alloc (sizeof (new_e->value));
  new_e->type.node_code = INT;
  new_e->type.filename = ldlex_filename ();
  new_e->type.lineno = lineno;
  new_e->value.value = value;
  new_e->value.str = NULL;
  new_e->type.node_class = etree_value;
  return new_e;
}

etree_type *
exp_bigintop (bfd_vma value, char *str)
{
  etree_type *new_e = (etree_type *) stat_alloc (sizeof (new_e->value));
  new_e->type.node_code = INT;
  new_e->type.filename = ldlex_filename ();
  new_e->type.lineno = lineno;
  new_e->value.value = value;
  new_e->value.str = str;
  new_e->type.node_class = etree_value;
  return new_e;
}

/* Build an expression representing an unnamed relocatable value.  */

etree_type *
exp_relop (asection *section, bfd_vma value)
{
  etree_type *new_e = (etree_type *) stat_alloc (sizeof (new_e->rel));
  new_e->type.node_code = REL;
  new_e->type.filename = ldlex_filename ();
  new_e->type.lineno = lineno;
  new_e->type.node_class = etree_rel;
  new_e->rel.section = section;
  new_e->rel.value = value;
  return new_e;
}

static void
new_number (bfd_vma value)
{
  expld.result.valid_p = TRUE;
  expld.result.value = value;
  expld.result.str = NULL;
  expld.result.section = NULL;
}

static void
new_rel (bfd_vma value, asection *section)
{
  expld.result.valid_p = TRUE;
  expld.result.value = value;
  expld.result.str = NULL;
  expld.result.section = section;
}

static void
new_rel_from_abs (bfd_vma value)
{
  asection *s = expld.section;

  if (s == bfd_abs_section_ptr && expld.phase == lang_final_phase_enum)
    s = section_for_dot ();
  expld.result.valid_p = TRUE;
  expld.result.value = value - s->vma;
  expld.result.str = NULL;
  expld.result.section = s;
}

static void
fold_unary (etree_type *tree)
{
  exp_fold_tree_1 (tree->unary.child);
  if (expld.result.valid_p)
    {
      switch (tree->type.node_code)
	{
	case ALIGN_K:
	  if (expld.phase != lang_first_phase_enum)
	    new_rel_from_abs (align_n (expld.dot, expld.result.value));
	  else
	    expld.result.valid_p = FALSE;
	  break;

	case ABSOLUTE:
	  make_abs ();
	  break;

	case '~':
	  expld.result.value = ~expld.result.value;
	  break;

	case '!':
	  expld.result.value = !expld.result.value;
	  break;

	case '-':
	  expld.result.value = -expld.result.value;
	  break;

	case NEXT:
	  /* Return next place aligned to value.  */
	  if (expld.phase != lang_first_phase_enum)
	    {
	      make_abs ();
	      expld.result.value = align_n (expld.dot, expld.result.value);
	    }
	  else
	    expld.result.valid_p = FALSE;
	  break;

	case DATA_SEGMENT_END:
	  if (expld.phase == lang_first_phase_enum
	      || expld.section != bfd_abs_section_ptr)
	    {
	      expld.result.valid_p = FALSE;
	    }
	  else if (expld.dataseg.phase == exp_dataseg_align_seen
		   || expld.dataseg.phase == exp_dataseg_relro_seen)
	    {
	      expld.dataseg.phase = exp_dataseg_end_seen;
	      expld.dataseg.end = expld.result.value;
	    }
	  else if (expld.dataseg.phase == exp_dataseg_done
		   || expld.dataseg.phase == exp_dataseg_adjust
		   || expld.dataseg.phase == exp_dataseg_relro_adjust)
	    {
	      /* OK.  */
	    }
	  else
	    expld.result.valid_p = FALSE;
	  break;

	default:
	  FAIL ();
	  break;
	}
    }
}

static void
fold_binary (etree_type *tree)
{
  etree_value_type lhs;
  exp_fold_tree_1 (tree->binary.lhs);

  /* The SEGMENT_START operator is special because its first
     operand is a string, not the name of a symbol.  Note that the
     operands have been swapped, so binary.lhs is second (default)
     operand, binary.rhs is first operand.  */
  if (expld.result.valid_p && tree->type.node_code == SEGMENT_START)
    {
      const char *segment_name;
      segment_type *seg;

      /* Check to see if the user has overridden the default
	 value.  */
      segment_name = tree->binary.rhs->name.name;
      for (seg = segments; seg; seg = seg->next) 
	if (strcmp (seg->name, segment_name) == 0)
	  {
	    if (!seg->used
		&& config.magic_demand_paged
		&& (seg->value % config.maxpagesize) != 0)
	      einfo (_("%P: warning: address of `%s' isn't multiple of maximum page size\n"),
		     segment_name);
	    seg->used = TRUE;
	    new_rel_from_abs (seg->value);
	    break;
	  }
      return;
    }

  lhs = expld.result;
  exp_fold_tree_1 (tree->binary.rhs);
  expld.result.valid_p &= lhs.valid_p;

  if (expld.result.valid_p)
    {
      if (lhs.section != expld.result.section)
	{
	  /* If the values are from different sections, and neither is
	     just a number, make both the source arguments absolute.  */
	  if (expld.result.section != NULL
	      && lhs.section != NULL)
	    {
	      make_abs ();
	      lhs.value += lhs.section->vma;
	      lhs.section = bfd_abs_section_ptr;
	    }

	  /* If the rhs is just a number, keep the lhs section.  */
	  else if (expld.result.section == NULL)
	    {
	      expld.result.section = lhs.section;
	      /* Make this NULL so that we know one of the operands
		 was just a number, for later tests.  */
	      lhs.section = NULL;
	    }
	}
      /* At this point we know that both operands have the same
	 section, or at least one of them is a plain number.  */

      switch (tree->type.node_code)
	{
	  /* Arithmetic operators, bitwise AND, bitwise OR and XOR
	     keep the section of one of their operands only when the
	     other operand is a plain number.  Losing the section when
	     operating on two symbols, ie. a result of a plain number,
	     is required for subtraction and XOR.  It's justifiable
	     for the other operations on the grounds that adding,
	     multiplying etc. two section relative values does not
	     really make sense unless they are just treated as
	     numbers.
	     The same argument could be made for many expressions
	     involving one symbol and a number.  For example,
	     "1 << x" and "100 / x" probably should not be given the
	     section of x.  The trouble is that if we fuss about such
	     things the rules become complex and it is onerous to
	     document ld expression evaluation.  */
#define BOP(x, y) \
	case x:							\
	  expld.result.value = lhs.value y expld.result.value;	\
	  if (expld.result.section == lhs.section)		\
	    expld.result.section = NULL;			\
	  break;

	  /* Comparison operators, logical AND, and logical OR always
	     return a plain number.  */
#define BOPN(x, y) \
	case x:							\
	  expld.result.value = lhs.value y expld.result.value;	\
	  expld.result.section = NULL;				\
	  break;

	  BOP ('+', +);
	  BOP ('*', *);
	  BOP ('-', -);
	  BOP (LSHIFT, <<);
	  BOP (RSHIFT, >>);
	  BOP ('&', &);
	  BOP ('^', ^);
	  BOP ('|', |);
	  BOPN (EQ, ==);
	  BOPN (NE, !=);
	  BOPN ('<', <);
	  BOPN ('>', >);
	  BOPN (LE, <=);
	  BOPN (GE, >=);
	  BOPN (ANDAND, &&);
	  BOPN (OROR, ||);

	case '%':
	  if (expld.result.value != 0)
	    expld.result.value = ((bfd_signed_vma) lhs.value
				  % (bfd_signed_vma) expld.result.value);
	  else if (expld.phase != lang_mark_phase_enum)
	    einfo (_("%F%S %% by zero\n"), tree->binary.rhs);
	  if (expld.result.section == lhs.section)
	    expld.result.section = NULL;
	  break;

	case '/':
	  if (expld.result.value != 0)
	    expld.result.value = ((bfd_signed_vma) lhs.value
				  / (bfd_signed_vma) expld.result.value);
	  else if (expld.phase != lang_mark_phase_enum)
	    einfo (_("%F%S / by zero\n"), tree->binary.rhs);
	  if (expld.result.section == lhs.section)
	    expld.result.section = NULL;
	  break;

	case MAX_K:
	  if (lhs.value > expld.result.value)
	    expld.result.value = lhs.value;
	  break;

	case MIN_K:
	  if (lhs.value < expld.result.value)
	    expld.result.value = lhs.value;
	  break;

	case ALIGN_K:
	  expld.result.value = align_n (lhs.value, expld.result.value);
	  break;

	case DATA_SEGMENT_ALIGN:
	  expld.dataseg.relro = exp_dataseg_relro_start;
	  if (expld.phase == lang_first_phase_enum
	      || expld.section != bfd_abs_section_ptr)
	    expld.result.valid_p = FALSE;
	  else
	    {
	      bfd_vma maxpage = lhs.value;
	      bfd_vma commonpage = expld.result.value;

	      expld.result.value = align_n (expld.dot, maxpage);
	      if (expld.dataseg.phase == exp_dataseg_relro_adjust)
		expld.result.value = expld.dataseg.base;
	      else if (expld.dataseg.phase == exp_dataseg_adjust)
		{
		  if (commonpage < maxpage)
		    expld.result.value += ((expld.dot + commonpage - 1)
					   & (maxpage - commonpage));
		}
	      else
		{
		  expld.result.value += expld.dot & (maxpage - 1);
		  if (expld.dataseg.phase == exp_dataseg_done)
		    {
		      /* OK.  */
		    }
		  else if (expld.dataseg.phase == exp_dataseg_none)
		    {
		      expld.dataseg.phase = exp_dataseg_align_seen;
		      expld.dataseg.min_base = expld.dot;
		      expld.dataseg.base = expld.result.value;
		      expld.dataseg.pagesize = commonpage;
		      expld.dataseg.maxpagesize = maxpage;
		      expld.dataseg.relro_end = 0;
		    }
		  else
		    expld.result.valid_p = FALSE;
		}
	    }
	  break;

	case DATA_SEGMENT_RELRO_END:
	  expld.dataseg.relro = exp_dataseg_relro_end;
	  if (expld.phase == lang_first_phase_enum
	      || expld.section != bfd_abs_section_ptr)
	    expld.result.valid_p = FALSE;
	  else if (expld.dataseg.phase == exp_dataseg_align_seen
		   || expld.dataseg.phase == exp_dataseg_adjust
		   || expld.dataseg.phase == exp_dataseg_relro_adjust
		   || expld.dataseg.phase == exp_dataseg_done)
	    {
	      if (expld.dataseg.phase == exp_dataseg_align_seen
		  || expld.dataseg.phase == exp_dataseg_relro_adjust)
		expld.dataseg.relro_end = lhs.value + expld.result.value;

	      if (expld.dataseg.phase == exp_dataseg_relro_adjust
		  && (expld.dataseg.relro_end
		      & (expld.dataseg.pagesize - 1)))
		{
		  expld.dataseg.relro_end += expld.dataseg.pagesize - 1;
		  expld.dataseg.relro_end &= ~(expld.dataseg.pagesize - 1);
		  expld.result.value = (expld.dataseg.relro_end
					- expld.result.value);
		}
	      else
		expld.result.value = lhs.value;

	      if (expld.dataseg.phase == exp_dataseg_align_seen)
		expld.dataseg.phase = exp_dataseg_relro_seen;
	    }
	  else
	    expld.result.valid_p = FALSE;
	  break;

	default:
	  FAIL ();
	}
    }
}

static void
fold_trinary (etree_type *tree)
{
  exp_fold_tree_1 (tree->trinary.cond);
  if (expld.result.valid_p)
    exp_fold_tree_1 (expld.result.value
		     ? tree->trinary.lhs
		     : tree->trinary.rhs);
}

static void
fold_name (etree_type *tree)
{
  memset (&expld.result, 0, sizeof (expld.result));

  switch (tree->type.node_code)
    {
    case SIZEOF_HEADERS:
      if (expld.phase != lang_first_phase_enum)
	{
	  bfd_vma hdr_size = 0;
	  /* Don't find the real header size if only marking sections;
	     The bfd function may cache incorrect data.  */
	  if (expld.phase != lang_mark_phase_enum)
	    hdr_size = bfd_sizeof_headers (link_info.output_bfd, &link_info);
	  new_number (hdr_size);
	}
      break;

    case DEFINED:
      if (expld.phase == lang_first_phase_enum)
	lang_track_definedness (tree->name.name);
      else
	{
	  struct bfd_link_hash_entry *h;
	  int def_iteration
	    = lang_symbol_definition_iteration (tree->name.name);

	  h = bfd_wrapped_link_hash_lookup (link_info.output_bfd,
					    &link_info,
					    tree->name.name,
					    FALSE, FALSE, TRUE);
	  new_number (h != NULL
		      && (h->type == bfd_link_hash_defined
			  || h->type == bfd_link_hash_defweak
			  || h->type == bfd_link_hash_common)
		      && (def_iteration == lang_statement_iteration
			  || def_iteration == -1));
	}
      break;

    case NAME:
      if (expld.assign_name != NULL
	  && strcmp (expld.assign_name, tree->name.name) == 0)
	expld.assign_name = NULL;
      if (expld.phase == lang_first_phase_enum)
	;
      else if (tree->name.name[0] == '.' && tree->name.name[1] == 0)
	new_rel_from_abs (expld.dot);
      else
	{
	  struct bfd_link_hash_entry *h;

	  h = bfd_wrapped_link_hash_lookup (link_info.output_bfd,
					    &link_info,
					    tree->name.name,
					    TRUE, FALSE, TRUE);
	  if (!h)
	    einfo (_("%P%F: bfd_link_hash_lookup failed: %E\n"));
	  else if (h->type == bfd_link_hash_defined
		   || h->type == bfd_link_hash_defweak)
	    {
	      asection *output_section;

	      output_section = h->u.def.section->output_section;
	      if (output_section == NULL)
		{
		  if (expld.phase == lang_mark_phase_enum)
		    new_rel (h->u.def.value, h->u.def.section);
		  else
		    einfo (_("%X%S: unresolvable symbol `%s'"
			     " referenced in expression\n"),
			   tree, tree->name.name);
		}
	      else if (output_section == bfd_abs_section_ptr
		       && (expld.section != bfd_abs_section_ptr
			   || config.sane_expr))
		new_number (h->u.def.value + h->u.def.section->output_offset);
	      else
		new_rel (h->u.def.value + h->u.def.section->output_offset,
			 output_section);
	    }
	  else if (expld.phase == lang_final_phase_enum
		   || (expld.phase != lang_mark_phase_enum
		       && expld.assigning_to_dot))
	    einfo (_("%F%S: undefined symbol `%s'"
		     " referenced in expression\n"),
		   tree, tree->name.name);
	  else if (h->type == bfd_link_hash_new)
	    {
	      h->type = bfd_link_hash_undefined;
	      h->u.undef.abfd = NULL;
	      if (h->u.undef.next == NULL && h != link_info.hash->undefs_tail)
		bfd_link_add_undef (link_info.hash, h);
	    }
	}
      break;

    case ADDR:
      if (expld.phase != lang_first_phase_enum)
	{
	  lang_output_section_statement_type *os;

	  os = lang_output_section_find (tree->name.name);
	  if (os == NULL)
	    {
	      if (expld.phase == lang_final_phase_enum)
		einfo (_("%F%S: undefined section `%s'"
			 " referenced in expression\n"),
		       tree, tree->name.name);
	    }
	  else if (os->processed_vma)
	    new_rel (0, os->bfd_section);
	}
      break;

    case LOADADDR:
      if (expld.phase != lang_first_phase_enum)
	{
	  lang_output_section_statement_type *os;

	  os = lang_output_section_find (tree->name.name);
	  if (os == NULL)
	    {
	      if (expld.phase == lang_final_phase_enum)
		einfo (_("%F%S: undefined section `%s'"
			 " referenced in expression\n"),
		       tree, tree->name.name);
	    }
	  else if (os->processed_lma)
	    {
	      if (os->load_base == NULL)
		new_abs (os->bfd_section->lma);
	      else
		{
		  exp_fold_tree_1 (os->load_base);
		  if (expld.result.valid_p)
		    make_abs ();
		}
	    }
	}
      break;

    case SIZEOF:
    case ALIGNOF:
      if (expld.phase != lang_first_phase_enum)
	{
	  lang_output_section_statement_type *os;

	  os = lang_output_section_find (tree->name.name);
	  if (os == NULL)
	    {
	      if (expld.phase == lang_final_phase_enum)
		einfo (_("%F%S: undefined section `%s'"
			 " referenced in expression\n"),
		       tree, tree->name.name);
	      new_number (0);
	    }
	  else if (os->bfd_section != NULL)
	    {
	      bfd_vma val;

	      if (tree->type.node_code == SIZEOF)
		val = (os->bfd_section->size
		       / bfd_octets_per_byte (link_info.output_bfd));
	      else
		val = (bfd_vma)1 << os->bfd_section->alignment_power;
	      
	      new_number (val);
	    }
	  else
	    new_number (0);
	}
      break;

    case LENGTH:
      {
        lang_memory_region_type *mem;
        
        mem = lang_memory_region_lookup (tree->name.name, FALSE);  
        if (mem != NULL) 
          new_number (mem->length);
        else          
          einfo (_("%F%S: undefined MEMORY region `%s'"
		   " referenced in expression\n"),
		 tree, tree->name.name);
      }
      break;

    case ORIGIN:
      if (expld.phase != lang_first_phase_enum)
	{
	  lang_memory_region_type *mem;
        
	  mem = lang_memory_region_lookup (tree->name.name, FALSE);  
	  if (mem != NULL) 
	    new_rel_from_abs (mem->origin);
	  else          
	    einfo (_("%F%S: undefined MEMORY region `%s'"
		     " referenced in expression\n"),
		   tree, tree->name.name);
	}
      break;

    case CONSTANT:
      if (strcmp (tree->name.name, "MAXPAGESIZE") == 0)
	new_number (config.maxpagesize);
      else if (strcmp (tree->name.name, "COMMONPAGESIZE") == 0)
	new_number (config.commonpagesize);
      else
	einfo (_("%F%S: unknown constant `%s' referenced in expression\n"),
	       tree, tree->name.name);
      break;

    default:
      FAIL ();
      break;
    }
}

static void
exp_fold_tree_1 (etree_type *tree)
{
  if (tree == NULL)
    {
      memset (&expld.result, 0, sizeof (expld.result));
      return;
    }

  switch (tree->type.node_class)
    {
    case etree_value:
      if (expld.section == bfd_abs_section_ptr
	  && !config.sane_expr)
	new_abs (tree->value.value);
      else
	new_number (tree->value.value);
      expld.result.str = tree->value.str;
      break;

    case etree_rel:
      if (expld.phase != lang_first_phase_enum)
	{
	  asection *output_section = tree->rel.section->output_section;
	  new_rel (tree->rel.value + tree->rel.section->output_offset,
		   output_section);
	}
      else
	memset (&expld.result, 0, sizeof (expld.result));
      break;

    case etree_assert:
      exp_fold_tree_1 (tree->assert_s.child);
      if (expld.phase == lang_final_phase_enum && !expld.result.value)
	einfo ("%X%P: %s\n", tree->assert_s.message);
      break;

    case etree_unary:
      fold_unary (tree);
      break;

    case etree_binary:
      fold_binary (tree);
      break;

    case etree_trinary:
      fold_trinary (tree);
      break;

    case etree_assign:
    case etree_provide:
    case etree_provided:
      if (tree->assign.dst[0] == '.' && tree->assign.dst[1] == 0)
	{
	  if (tree->type.node_class != etree_assign)
	    einfo (_("%F%S can not PROVIDE assignment to"
		     " location counter\n"), tree);
	  if (expld.phase != lang_first_phase_enum)
	    {
	      /* Notify the folder that this is an assignment to dot.  */
	      expld.assigning_to_dot = TRUE;
	      exp_fold_tree_1 (tree->assign.src);
	      expld.assigning_to_dot = FALSE;

	      if (!expld.result.valid_p)
		{
		  if (expld.phase != lang_mark_phase_enum)
		    einfo (_("%F%S invalid assignment to"
			     " location counter\n"), tree);
		}
	      else if (expld.dotp == NULL)
		einfo (_("%F%S assignment to location counter"
			 " invalid outside of SECTIONS\n"), tree);

	      /* After allocation, assignment to dot should not be
		 done inside an output section since allocation adds a
		 padding statement that effectively duplicates the
		 assignment.  */
	      else if (expld.phase <= lang_allocating_phase_enum
		       || expld.section == bfd_abs_section_ptr)
		{
		  bfd_vma nextdot;

		  nextdot = expld.result.value;
		  if (expld.result.section != NULL)
		    nextdot += expld.result.section->vma;
		  else
		    nextdot += expld.section->vma;
		  if (nextdot < expld.dot
		      && expld.section != bfd_abs_section_ptr)
		    einfo (_("%F%S cannot move location counter backwards"
			     " (from %V to %V)\n"),
			   tree, expld.dot, nextdot);
		  else
		    {
		      expld.dot = nextdot;
		      *expld.dotp = nextdot;
		    }
		}
	    }
	  else
	    memset (&expld.result, 0, sizeof (expld.result));
	}
      else
	{
	  struct bfd_link_hash_entry *h = NULL;

	  if (tree->type.node_class == etree_provide)
	    {
	      h = bfd_link_hash_lookup (link_info.hash, tree->assign.dst,
					FALSE, FALSE, TRUE);
	      if (h == NULL
		  || (h->type != bfd_link_hash_new
		      && h->type != bfd_link_hash_undefined
		      && h->type != bfd_link_hash_common))
		{
		  /* Do nothing.  The symbol was never referenced, or was
		     defined by some object.  */
		  break;
		}
	    }

	  expld.assign_name = tree->assign.dst;
	  exp_fold_tree_1 (tree->assign.src);
	  /* expld.assign_name remaining equal to tree->assign.dst
	     below indicates the evaluation of tree->assign.src did
	     not use the value of tree->assign.dst.  We don't allow
	     self assignment until the final phase for two reasons:
	     1) Expressions are evaluated multiple times.  With
	     relaxation, the number of times may vary.
	     2) Section relative symbol values cannot be correctly
	     converted to absolute values, as is required by many
	     expressions, until final section sizing is complete.  */
	  if ((expld.result.valid_p
	       && (expld.phase == lang_final_phase_enum
		   || expld.assign_name != NULL))
	      || (expld.phase <= lang_mark_phase_enum
		  && tree->type.node_class == etree_assign
		  && tree->assign.defsym))
	    {
	      if (h == NULL)
		{
		  h = bfd_link_hash_lookup (link_info.hash, tree->assign.dst,
					    TRUE, FALSE, TRUE);
		  if (h == NULL)
		    einfo (_("%P%F:%s: hash creation failed\n"),
			   tree->assign.dst);
		}

	      /* FIXME: Should we worry if the symbol is already
		 defined?  */
	      lang_update_definedness (tree->assign.dst, h);
	      h->type = bfd_link_hash_defined;
	      h->u.def.value = expld.result.value;
	      if (expld.result.section == NULL)
		expld.result.section = expld.section;
	      h->u.def.section = expld.result.section;
	      if (tree->type.node_class == etree_provide)
		tree->type.node_class = etree_provided;

	      /* Copy the symbol type if this is a simple assignment of
	         one symbol to another.  This could be more general
		 (e.g. a ?: operator with NAMEs in each branch).  */
	      if (tree->assign.src->type.node_class == etree_name)
		{
		  struct bfd_link_hash_entry *hsrc;

		  hsrc = bfd_link_hash_lookup (link_info.hash,
					       tree->assign.src->name.name,
					       FALSE, FALSE, TRUE);
		  if (hsrc)
		    bfd_copy_link_hash_symbol_type (link_info.output_bfd, h,
						    hsrc);
		}
	    }
	  else if (expld.phase == lang_final_phase_enum)
	    {
	      h = bfd_link_hash_lookup (link_info.hash, tree->assign.dst,
					FALSE, FALSE, TRUE);
	      if (h != NULL
		  && h->type == bfd_link_hash_new)
		h->type = bfd_link_hash_undefined;
	    }
	  expld.assign_name = NULL;
	}
      break;

    case etree_name:
      fold_name (tree);
      break;

    default:
      FAIL ();
      memset (&expld.result, 0, sizeof (expld.result));
      break;
    }
}

void
exp_fold_tree (etree_type *tree, asection *current_section, bfd_vma *dotp)
{
  expld.dot = *dotp;
  expld.dotp = dotp;
  expld.section = current_section;
  exp_fold_tree_1 (tree);
}

void
exp_fold_tree_no_dot (etree_type *tree)
{
  expld.dot = 0;
  expld.dotp = NULL;
  expld.section = bfd_abs_section_ptr;
  exp_fold_tree_1 (tree);
}

etree_type *
exp_binop (int code, etree_type *lhs, etree_type *rhs)
{
  etree_type value, *new_e;

  value.type.node_code = code;
  value.type.filename = lhs->type.filename;
  value.type.lineno = lhs->type.lineno;
  value.binary.lhs = lhs;
  value.binary.rhs = rhs;
  value.type.node_class = etree_binary;
  exp_fold_tree_no_dot (&value);
  if (expld.result.valid_p)
    return exp_intop (expld.result.value);

  new_e = (etree_type *) stat_alloc (sizeof (new_e->binary));
  memcpy (new_e, &value, sizeof (new_e->binary));
  return new_e;
}

etree_type *
exp_trinop (int code, etree_type *cond, etree_type *lhs, etree_type *rhs)
{
  etree_type value, *new_e;

  value.type.node_code = code;
  value.type.filename = cond->type.filename;
  value.type.lineno = cond->type.lineno;
  value.trinary.lhs = lhs;
  value.trinary.cond = cond;
  value.trinary.rhs = rhs;
  value.type.node_class = etree_trinary;
  exp_fold_tree_no_dot (&value);
  if (expld.result.valid_p)
    return exp_intop (expld.result.value);

  new_e = (etree_type *) stat_alloc (sizeof (new_e->trinary));
  memcpy (new_e, &value, sizeof (new_e->trinary));
  return new_e;
}

etree_type *
exp_unop (int code, etree_type *child)
{
  etree_type value, *new_e;

  value.unary.type.node_code = code;
  value.unary.type.filename = child->type.filename;
  value.unary.type.lineno = child->type.lineno;
  value.unary.child = child;
  value.unary.type.node_class = etree_unary;
  exp_fold_tree_no_dot (&value);
  if (expld.result.valid_p)
    return exp_intop (expld.result.value);

  new_e = (etree_type *) stat_alloc (sizeof (new_e->unary));
  memcpy (new_e, &value, sizeof (new_e->unary));
  return new_e;
}

etree_type *
exp_nameop (int code, const char *name)
{
  etree_type value, *new_e;

  value.name.type.node_code = code;
  value.name.type.filename = ldlex_filename ();
  value.name.type.lineno = lineno;
  value.name.name = name;
  value.name.type.node_class = etree_name;

  exp_fold_tree_no_dot (&value);
  if (expld.result.valid_p)
    return exp_intop (expld.result.value);

  new_e = (etree_type *) stat_alloc (sizeof (new_e->name));
  memcpy (new_e, &value, sizeof (new_e->name));
  return new_e;

}

static etree_type *
exp_assop (const char *dst,
	   etree_type *src,
	   enum node_tree_enum class,
	   bfd_boolean defsym,
	   bfd_boolean hidden)
{
  etree_type *n;

  n = (etree_type *) stat_alloc (sizeof (n->assign));
  n->assign.type.node_code = '=';
  n->assign.type.filename = src->type.filename;
  n->assign.type.lineno = src->type.lineno;
  n->assign.type.node_class = class;
  n->assign.src = src;
  n->assign.dst = dst;
  n->assign.defsym = defsym;
  n->assign.hidden = hidden;
  return n;
}

/* Handle linker script assignments and HIDDEN.  */

etree_type *
exp_assign (const char *dst, etree_type *src, bfd_boolean hidden)
{
  return exp_assop (dst, src, etree_assign, FALSE, hidden);
}

/* Handle --defsym command-line option.  */

etree_type *
exp_defsym (const char *dst, etree_type *src)
{
  return exp_assop (dst, src, etree_assign, TRUE, FALSE);
}

/* Handle PROVIDE.  */

etree_type *
exp_provide (const char *dst, etree_type *src, bfd_boolean hidden)
{
  return exp_assop (dst, src, etree_provide, FALSE, hidden);
}

/* Handle ASSERT.  */

etree_type *
exp_assert (etree_type *exp, const char *message)
{
  etree_type *n;

  n = (etree_type *) stat_alloc (sizeof (n->assert_s));
  n->assert_s.type.node_code = '!';
  n->assert_s.type.filename = exp->type.filename;
  n->assert_s.type.lineno = exp->type.lineno;
  n->assert_s.type.node_class = etree_assert;
  n->assert_s.child = exp;
  n->assert_s.message = message;
  return n;
}

void
exp_print_tree (etree_type *tree)
{
  bfd_boolean function_like;

  if (config.map_file == NULL)
    config.map_file = stderr;

  if (tree == NULL)
    {
      minfo ("NULL TREE\n");
      return;
    }

  switch (tree->type.node_class)
    {
    case etree_value:
      minfo ("0x%v", tree->value.value);
      return;
    case etree_rel:
      if (tree->rel.section->owner != NULL)
	minfo ("%B:", tree->rel.section->owner);
      minfo ("%s+0x%v", tree->rel.section->name, tree->rel.value);
      return;
    case etree_assign:
      fputs (tree->assign.dst, config.map_file);
      exp_print_token (tree->type.node_code, TRUE);
      exp_print_tree (tree->assign.src);
      break;
    case etree_provide:
    case etree_provided:
      fprintf (config.map_file, "PROVIDE (%s, ", tree->assign.dst);
      exp_print_tree (tree->assign.src);
      fputc (')', config.map_file);
      break;
    case etree_binary:
      function_like = FALSE;
      switch (tree->type.node_code)
	{
	case MAX_K:
	case MIN_K:
	case ALIGN_K:
	case DATA_SEGMENT_ALIGN:
	case DATA_SEGMENT_RELRO_END:
	  function_like = TRUE;
	  break;
	case SEGMENT_START:
	  /* Special handling because arguments are in reverse order and
	     the segment name is quoted.  */
	  exp_print_token (tree->type.node_code, FALSE);
	  fputs (" (\"", config.map_file);
	  exp_print_tree (tree->binary.rhs);
	  fputs ("\", ", config.map_file);
	  exp_print_tree (tree->binary.lhs);
	  fputc (')', config.map_file);
	  return;
	}
      if (function_like)
	{
	  exp_print_token (tree->type.node_code, FALSE);
	  fputc (' ', config.map_file);
	}
      fputc ('(', config.map_file);
      exp_print_tree (tree->binary.lhs);
      if (function_like)
	fprintf (config.map_file, ", ");
      else
	exp_print_token (tree->type.node_code, TRUE);
      exp_print_tree (tree->binary.rhs);
      fputc (')', config.map_file);
      break;
    case etree_trinary:
      exp_print_tree (tree->trinary.cond);
      fputc ('?', config.map_file);
      exp_print_tree (tree->trinary.lhs);
      fputc (':', config.map_file);
      exp_print_tree (tree->trinary.rhs);
      break;
    case etree_unary:
      exp_print_token (tree->unary.type.node_code, FALSE);
      if (tree->unary.child)
	{
	  fprintf (config.map_file, " (");
	  exp_print_tree (tree->unary.child);
	  fputc (')', config.map_file);
	}
      break;

    case etree_assert:
      fprintf (config.map_file, "ASSERT (");
      exp_print_tree (tree->assert_s.child);
      fprintf (config.map_file, ", %s)", tree->assert_s.message);
      break;

    case etree_name:
      if (tree->type.node_code == NAME)
	fputs (tree->name.name, config.map_file);
      else
	{
	  exp_print_token (tree->type.node_code, FALSE);
	  if (tree->name.name)
	    fprintf (config.map_file, " (%s)", tree->name.name);
	}
      break;
    default:
      FAIL ();
      break;
    }
}

bfd_vma
exp_get_vma (etree_type *tree, bfd_vma def, char *name)
{
  if (tree != NULL)
    {
      exp_fold_tree_no_dot (tree);
      if (expld.result.valid_p)
	return expld.result.value;
      else if (name != NULL && expld.phase != lang_mark_phase_enum)
	einfo (_("%F%S: nonconstant expression for %s\n"),
	       tree, name);
    }
  return def;
}

int
exp_get_value_int (etree_type *tree, int def, char *name)
{
  return exp_get_vma (tree, def, name);
}

fill_type *
exp_get_fill (etree_type *tree, fill_type *def, char *name)
{
  fill_type *fill;
  size_t len;
  unsigned int val;

  if (tree == NULL)
    return def;

  exp_fold_tree_no_dot (tree);
  if (!expld.result.valid_p)
    {
      if (name != NULL && expld.phase != lang_mark_phase_enum)
	einfo (_("%F%S: nonconstant expression for %s\n"),
	       tree, name);
      return def;
    }

  if (expld.result.str != NULL && (len = strlen (expld.result.str)) != 0)
    {
      unsigned char *dst;
      unsigned char *s;
      fill = (fill_type *) xmalloc ((len + 1) / 2 + sizeof (*fill) - 1);
      fill->size = (len + 1) / 2;
      dst = fill->data;
      s = (unsigned char *) expld.result.str;
      val = 0;
      do
	{
	  unsigned int digit;

	  digit = *s++ - '0';
	  if (digit > 9)
	    digit = (digit - 'A' + '0' + 10) & 0xf;
	  val <<= 4;
	  val += digit;
	  --len;
	  if ((len & 1) == 0)
	    {
	      *dst++ = val;
	      val = 0;
	    }
	}
      while (len != 0);
    }
  else
    {
      fill = (fill_type *) xmalloc (4 + sizeof (*fill) - 1);
      val = expld.result.value;
      fill->data[0] = (val >> 24) & 0xff;
      fill->data[1] = (val >> 16) & 0xff;
      fill->data[2] = (val >>  8) & 0xff;
      fill->data[3] = (val >>  0) & 0xff;
      fill->size = 4;
    }
  return fill;
}

bfd_vma
exp_get_abs_int (etree_type *tree, int def, char *name)
{
  if (tree != NULL)
    {
      exp_fold_tree_no_dot (tree);

      if (expld.result.valid_p)
	{
	  if (expld.result.section != NULL)
	    expld.result.value += expld.result.section->vma;
	  return expld.result.value;
	}
      else if (name != NULL && expld.phase != lang_mark_phase_enum)
	{
	  einfo (_("%F%S: nonconstant expression for %s\n"),
		 tree, name);
	}
    }
  return def;
}

static bfd_vma
align_n (bfd_vma value, bfd_vma align)
{
  if (align <= 1)
    return value;

  value = (value + align - 1) / align;
  return value * align;
}
