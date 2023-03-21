/* Print values for GNU debugger GDB.

   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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
#include "frame.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "language.h"
#include "c-lang.h"
#include "expression.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "target.h"
#include "breakpoint.h"
#include "demangle.h"
#include "gdb-demangle.h"
#include "valprint.h"
#include "annotate.h"
#include "symfile.h"		/* for overlay functions */
#include "objfiles.h"		/* ditto */
#include "completer.h"		/* for completion functions */
#include "ui-out.h"
#include "block.h"
#include "disasm.h"
#include "target-float.h"
#include "observable.h"
#include "solist.h"
#include "parser-defs.h"
#include "charset.h"
#include "arch-utils.h"
#include "cli/cli-utils.h"
#include "cli/cli-option.h"
#include "cli/cli-script.h"
#include "cli/cli-style.h"
#include "gdbsupport/format.h"
#include "source.h"
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/gdb_optional.h"

/* Last specified output format.  */

static char last_format = 0;

/* Last specified examination size.  'b', 'h', 'w' or `q'.  */

static char last_size = 'w';

/* Last specified count for the 'x' command.  */

static int last_count;

/* Default address to examine next, and associated architecture.  */

static struct gdbarch *next_gdbarch;
static CORE_ADDR next_address;

/* Number of delay instructions following current disassembled insn.  */

static int branch_delay_insns;

/* Last address examined.  */

static CORE_ADDR last_examine_address;

/* Contents of last address examined.
   This is not valid past the end of the `x' command!  */

static value_ref_ptr last_examine_value;

/* Largest offset between a symbolic value and an address, that will be
   printed as `0x1234 <symbol+offset>'.  */

static unsigned int max_symbolic_offset = UINT_MAX;
static void
show_max_symbolic_offset (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("The largest offset that will be "
		      "printed in <symbol+1234> form is %s.\n"),
		    value);
}

/* Append the source filename and linenumber of the symbol when
   printing a symbolic value as `<symbol at filename:linenum>' if set.  */
static bool print_symbol_filename = false;
static void
show_print_symbol_filename (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Printing of source filename and "
			    "line number with <symbol> is %s.\n"),
		    value);
}

/* Number of auto-display expression currently being displayed.
   So that we can disable it if we get a signal within it.
   -1 when not doing one.  */

static int current_display_number;

/* Last allocated display number.  */

static int display_number;

struct display
  {
    display (const char *exp_string_, expression_up &&exp_,
	     const struct format_data &format_, struct program_space *pspace_,
	     const struct block *block_)
      : exp_string (exp_string_),
	exp (std::move (exp_)),
	number (++display_number),
	format (format_),
	pspace (pspace_),
	block (block_),
	enabled_p (true)
    {
    }

    /* The expression as the user typed it.  */
    std::string exp_string;

    /* Expression to be evaluated and displayed.  */
    expression_up exp;

    /* Item number of this auto-display item.  */
    int number;

    /* Display format specified.  */
    struct format_data format;

    /* Program space associated with `block'.  */
    struct program_space *pspace;

    /* Innermost block required by this expression when evaluated.  */
    const struct block *block;

    /* Status of this display (enabled or disabled).  */
    bool enabled_p;
  };

/* Expressions whose values should be displayed automatically each
   time the program stops.  */

static std::vector<std::unique_ptr<struct display>> all_displays;

/* Prototypes for local functions.  */

static void do_one_display (struct display *);


/* Decode a format specification.  *STRING_PTR should point to it.
   OFORMAT and OSIZE are used as defaults for the format and size
   if none are given in the format specification.
   If OSIZE is zero, then the size field of the returned value
   should be set only if a size is explicitly specified by the
   user.
   The structure returned describes all the data
   found in the specification.  In addition, *STRING_PTR is advanced
   past the specification and past all whitespace following it.  */

static struct format_data
decode_format (const char **string_ptr, int oformat, int osize)
{
  struct format_data val;
  const char *p = *string_ptr;

  val.format = '?';
  val.size = '?';
  val.count = 1;
  val.raw = 0;

  if (*p == '-')
    {
      val.count = -1;
      p++;
    }
  if (*p >= '0' && *p <= '9')
    val.count *= atoi (p);
  while (*p >= '0' && *p <= '9')
    p++;

  /* Now process size or format letters that follow.  */

  while (1)
    {
      if (*p == 'b' || *p == 'h' || *p == 'w' || *p == 'g')
	val.size = *p++;
      else if (*p == 'r')
	{
	  val.raw = 1;
	  p++;
	}
      else if (*p >= 'a' && *p <= 'z')
	val.format = *p++;
      else
	break;
    }

  *string_ptr = skip_spaces (p);

  /* Set defaults for format and size if not specified.  */
  if (val.format == '?')
    {
      if (val.size == '?')
	{
	  /* Neither has been specified.  */
	  val.format = oformat;
	  val.size = osize;
	}
      else
	/* If a size is specified, any format makes a reasonable
	   default except 'i'.  */
	val.format = oformat == 'i' ? 'x' : oformat;
    }
  else if (val.size == '?')
    switch (val.format)
      {
      case 'a':
	/* Pick the appropriate size for an address.  This is deferred
	   until do_examine when we know the actual architecture to use.
	   A special size value of 'a' is used to indicate this case.  */
	val.size = osize ? 'a' : osize;
	break;
      case 'f':
	/* Floating point has to be word or giantword.  */
	if (osize == 'w' || osize == 'g')
	  val.size = osize;
	else
	  /* Default it to giantword if the last used size is not
	     appropriate.  */
	  val.size = osize ? 'g' : osize;
	break;
      case 'c':
	/* Characters default to one byte.  */
	val.size = osize ? 'b' : osize;
	break;
      case 's':
	/* Display strings with byte size chars unless explicitly
	   specified.  */
	val.size = '\0';
	break;

      default:
	/* The default is the size most recently specified.  */
	val.size = osize;
      }

  return val;
}

/* Print value VAL on stream according to OPTIONS.
   Do not end with a newline.
   SIZE is the letter for the size of datum being printed.
   This is used to pad hex numbers so they line up.  SIZE is 0
   for print / output and set for examine.  */

static void
print_formatted (struct value *val, int size,
		 const struct value_print_options *options,
		 struct ui_file *stream)
{
  struct type *type = check_typedef (value_type (val));
  int len = TYPE_LENGTH (type);

  if (VALUE_LVAL (val) == lval_memory)
    next_address = value_address (val) + len;

  if (size)
    {
      switch (options->format)
	{
	case 's':
	  {
	    struct type *elttype = value_type (val);

	    next_address = (value_address (val)
			    + val_print_string (elttype, NULL,
						value_address (val), -1,
						stream, options) * len);
	  }
	  return;

	case 'i':
	  /* We often wrap here if there are long symbolic names.  */
	  wrap_here ("    ");
	  next_address = (value_address (val)
			  + gdb_print_insn (get_type_arch (type),
					    value_address (val), stream,
					    &branch_delay_insns));
	  return;
	}
    }

  if (options->format == 0 || options->format == 's'
      || type->code () == TYPE_CODE_REF
      || type->code () == TYPE_CODE_ARRAY
      || type->code () == TYPE_CODE_STRING
      || type->code () == TYPE_CODE_STRUCT
      || type->code () == TYPE_CODE_UNION
      || type->code () == TYPE_CODE_NAMESPACE)
    value_print (val, stream, options);
  else
    /* User specified format, so don't look to the type to tell us
       what to do.  */
    value_print_scalar_formatted (val, options, size, stream);
}

/* Return builtin floating point type of same length as TYPE.
   If no such type is found, return TYPE itself.  */
static struct type *
float_type_from_length (struct type *type)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  const struct builtin_type *builtin = builtin_type (gdbarch);

  if (TYPE_LENGTH (type) == TYPE_LENGTH (builtin->builtin_float))
    type = builtin->builtin_float;
  else if (TYPE_LENGTH (type) == TYPE_LENGTH (builtin->builtin_double))
    type = builtin->builtin_double;
  else if (TYPE_LENGTH (type) == TYPE_LENGTH (builtin->builtin_long_double))
    type = builtin->builtin_long_double;

  return type;
}

/* Print a scalar of data of type TYPE, pointed to in GDB by VALADDR,
   according to OPTIONS and SIZE on STREAM.  Formats s and i are not
   supported at this level.  */

void
print_scalar_formatted (const gdb_byte *valaddr, struct type *type,
			const struct value_print_options *options,
			int size, struct ui_file *stream)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  unsigned int len = TYPE_LENGTH (type);
  enum bfd_endian byte_order = type_byte_order (type);

  /* String printing should go through val_print_scalar_formatted.  */
  gdb_assert (options->format != 's');

  /* If the value is a pointer, and pointers and addresses are not the
     same, then at this point, the value's length (in target bytes) is
     gdbarch_addr_bit/TARGET_CHAR_BIT, not TYPE_LENGTH (type).  */
  if (type->code () == TYPE_CODE_PTR)
    len = gdbarch_addr_bit (gdbarch) / TARGET_CHAR_BIT;

  /* If we are printing it as unsigned, truncate it in case it is actually
     a negative signed value (e.g. "print/u (short)-1" should print 65535
     (if shorts are 16 bits) instead of 4294967295).  */
  if (options->format != 'c'
      && (options->format != 'd' || TYPE_UNSIGNED (type)))
    {
      if (len < TYPE_LENGTH (type) && byte_order == BFD_ENDIAN_BIG)
	valaddr += TYPE_LENGTH (type) - len;
    }

  if (size != 0 && (options->format == 'x' || options->format == 't'))
    {
      /* Truncate to fit.  */
      unsigned newlen;
      switch (size)
	{
	case 'b':
	  newlen = 1;
	  break;
	case 'h':
	  newlen = 2;
	  break;
	case 'w':
	  newlen = 4;
	  break;
	case 'g':
	  newlen = 8;
	  break;
	default:
	  error (_("Undefined output size \"%c\"."), size);
	}
      if (newlen < len && byte_order == BFD_ENDIAN_BIG)
	valaddr += len - newlen;
      len = newlen;
    }

  /* Historically gdb has printed floats by first casting them to a
     long, and then printing the long.  PR cli/16242 suggests changing
     this to using C-style hex float format.

     Biased range types must also be unbiased here; the unbiasing is
     done by unpack_long.  */
  gdb::byte_vector converted_bytes;
  /* Some cases below will unpack the value again.  In the biased
     range case, we want to avoid this, so we store the unpacked value
     here for possible use later.  */
  gdb::optional<LONGEST> val_long;
  if ((type->code () == TYPE_CODE_FLT
       && (options->format == 'o'
	   || options->format == 'x'
	   || options->format == 't'
	   || options->format == 'z'
	   || options->format == 'd'
	   || options->format == 'u'))
      || (type->code () == TYPE_CODE_RANGE && type->bounds ()->bias != 0))
    {
      val_long.emplace (unpack_long (type, valaddr));
      converted_bytes.resize (TYPE_LENGTH (type));
      store_signed_integer (converted_bytes.data (), TYPE_LENGTH (type),
			    byte_order, *val_long);
      valaddr = converted_bytes.data ();
    }

  /* Printing a non-float type as 'f' will interpret the data as if it were
     of a floating-point type of the same length, if that exists.  Otherwise,
     the data is printed as integer.  */
  char format = options->format;
  if (format == 'f' && type->code () != TYPE_CODE_FLT)
    {
      type = float_type_from_length (type);
      if (type->code () != TYPE_CODE_FLT)
        format = 0;
    }

  switch (format)
    {
    case 'o':
      print_octal_chars (stream, valaddr, len, byte_order);
      break;
    case 'd':
      print_decimal_chars (stream, valaddr, len, true, byte_order);
      break;
    case 'u':
      print_decimal_chars (stream, valaddr, len, false, byte_order);
      break;
    case 0:
      if (type->code () != TYPE_CODE_FLT)
	{
	  print_decimal_chars (stream, valaddr, len, !TYPE_UNSIGNED (type),
			       byte_order);
	  break;
	}
      /* FALLTHROUGH */
    case 'f':
      print_floating (valaddr, type, stream);
      break;

    case 't':
      print_binary_chars (stream, valaddr, len, byte_order, size > 0);
      break;
    case 'x':
      print_hex_chars (stream, valaddr, len, byte_order, size > 0);
      break;
    case 'z':
      print_hex_chars (stream, valaddr, len, byte_order, true);
      break;
    case 'c':
      {
	struct value_print_options opts = *options;

	if (!val_long.has_value ())
	  val_long.emplace (unpack_long (type, valaddr));

	opts.format = 0;
	if (TYPE_UNSIGNED (type))
	  type = builtin_type (gdbarch)->builtin_true_unsigned_char;
 	else
	  type = builtin_type (gdbarch)->builtin_true_char;

	value_print (value_from_longest (type, *val_long), stream, &opts);
      }
      break;

    case 'a':
      {
	if (!val_long.has_value ())
	  val_long.emplace (unpack_long (type, valaddr));
	print_address (gdbarch, *val_long, stream);
      }
      break;

    default:
      error (_("Undefined output format \"%c\"."), format);
    }
}

/* Specify default address for `x' command.
   The `info lines' command uses this.  */

void
set_next_address (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  struct type *ptr_type = builtin_type (gdbarch)->builtin_data_ptr;

  next_gdbarch = gdbarch;
  next_address = addr;

  /* Make address available to the user as $_.  */
  set_internalvar (lookup_internalvar ("_"),
		   value_from_pointer (ptr_type, addr));
}

/* Optionally print address ADDR symbolically as <SYMBOL+OFFSET> on STREAM,
   after LEADIN.  Print nothing if no symbolic name is found nearby.
   Optionally also print source file and line number, if available.
   DO_DEMANGLE controls whether to print a symbol in its native "raw" form,
   or to interpret it as a possible C++ name and convert it back to source
   form.  However note that DO_DEMANGLE can be overridden by the specific
   settings of the demangle and asm_demangle variables.  Returns
   non-zero if anything was printed; zero otherwise.  */

int
print_address_symbolic (struct gdbarch *gdbarch, CORE_ADDR addr,
			struct ui_file *stream,
			int do_demangle, const char *leadin)
{
  std::string name, filename;
  int unmapped = 0;
  int offset = 0;
  int line = 0;

  if (build_address_symbolic (gdbarch, addr, do_demangle, false, &name,
                              &offset, &filename, &line, &unmapped))
    return 0;

  fputs_filtered (leadin, stream);
  if (unmapped)
    fputs_filtered ("<*", stream);
  else
    fputs_filtered ("<", stream);
  fputs_styled (name.c_str (), function_name_style.style (), stream);
  if (offset != 0)
    fprintf_filtered (stream, "%+d", offset);

  /* Append source filename and line number if desired.  Give specific
     line # of this addr, if we have it; else line # of the nearest symbol.  */
  if (print_symbol_filename && !filename.empty ())
    {
      fputs_filtered (line == -1 ? " in " : " at ", stream);
      fputs_styled (filename.c_str (), file_name_style.style (), stream);
      if (line != -1)
	fprintf_filtered (stream, ":%d", line);
    }
  if (unmapped)
    fputs_filtered ("*>", stream);
  else
    fputs_filtered (">", stream);

  return 1;
}

/* See valprint.h.  */

int
build_address_symbolic (struct gdbarch *gdbarch,
			CORE_ADDR addr,  /* IN */
			bool do_demangle, /* IN */
			bool prefer_sym_over_minsym, /* IN */
			std::string *name, /* OUT */
			int *offset,     /* OUT */
			std::string *filename, /* OUT */
			int *line,       /* OUT */
			int *unmapped)   /* OUT */
{
  struct bound_minimal_symbol msymbol;
  struct symbol *symbol;
  CORE_ADDR name_location = 0;
  struct obj_section *section = NULL;
  const char *name_temp = "";
  
  /* Let's say it is mapped (not unmapped).  */
  *unmapped = 0;

  /* Determine if the address is in an overlay, and whether it is
     mapped.  */
  if (overlay_debugging)
    {
      section = find_pc_overlay (addr);
      if (pc_in_unmapped_range (addr, section))
	{
	  *unmapped = 1;
	  addr = overlay_mapped_address (addr, section);
	}
    }

  /* Try to find the address in both the symbol table and the minsyms. 
     In most cases, we'll prefer to use the symbol instead of the
     minsym.  However, there are cases (see below) where we'll choose
     to use the minsym instead.  */

  /* This is defective in the sense that it only finds text symbols.  So
     really this is kind of pointless--we should make sure that the
     minimal symbols have everything we need (by changing that we could
     save some memory, but for many debug format--ELF/DWARF or
     anything/stabs--it would be inconvenient to eliminate those minimal
     symbols anyway).  */
  msymbol = lookup_minimal_symbol_by_pc_section (addr, section);
  symbol = find_pc_sect_function (addr, section);

  if (symbol)
    {
      /* If this is a function (i.e. a code address), strip out any
	 non-address bits.  For instance, display a pointer to the
	 first instruction of a Thumb function as <function>; the
	 second instruction will be <function+2>, even though the
	 pointer is <function+3>.  This matches the ISA behavior.  */
      addr = gdbarch_addr_bits_remove (gdbarch, addr);

      name_location = BLOCK_ENTRY_PC (SYMBOL_BLOCK_VALUE (symbol));
      if (do_demangle || asm_demangle)
	name_temp = symbol->print_name ();
      else
	name_temp = symbol->linkage_name ();
    }

  if (msymbol.minsym != NULL
      && MSYMBOL_HAS_SIZE (msymbol.minsym)
      && MSYMBOL_SIZE (msymbol.minsym) == 0
      && MSYMBOL_TYPE (msymbol.minsym) != mst_text
      && MSYMBOL_TYPE (msymbol.minsym) != mst_text_gnu_ifunc
      && MSYMBOL_TYPE (msymbol.minsym) != mst_file_text)
    msymbol.minsym = NULL;

  if (msymbol.minsym != NULL)
    {
      /* Use the minsym if no symbol is found.
      
	 Additionally, use the minsym instead of a (found) symbol if
	 the following conditions all hold:
	   1) The prefer_sym_over_minsym flag is false.
	   2) The minsym address is identical to that of the address under
	      consideration.
	   3) The symbol address is not identical to that of the address
	      under consideration.  */
      if (symbol == NULL ||
           (!prefer_sym_over_minsym
	    && BMSYMBOL_VALUE_ADDRESS (msymbol) == addr
	    && name_location != addr))
	{
	  /* If this is a function (i.e. a code address), strip out any
	     non-address bits.  For instance, display a pointer to the
	     first instruction of a Thumb function as <function>; the
	     second instruction will be <function+2>, even though the
	     pointer is <function+3>.  This matches the ISA behavior.  */
	  if (MSYMBOL_TYPE (msymbol.minsym) == mst_text
	      || MSYMBOL_TYPE (msymbol.minsym) == mst_text_gnu_ifunc
	      || MSYMBOL_TYPE (msymbol.minsym) == mst_file_text
	      || MSYMBOL_TYPE (msymbol.minsym) == mst_solib_trampoline)
	    addr = gdbarch_addr_bits_remove (gdbarch, addr);

	  symbol = 0;
	  name_location = BMSYMBOL_VALUE_ADDRESS (msymbol);
	  if (do_demangle || asm_demangle)
	    name_temp = msymbol.minsym->print_name ();
	  else
	    name_temp = msymbol.minsym->linkage_name ();
	}
    }
  if (symbol == NULL && msymbol.minsym == NULL)
    return 1;

  /* If the nearest symbol is too far away, don't print anything symbolic.  */

  /* For when CORE_ADDR is larger than unsigned int, we do math in
     CORE_ADDR.  But when we detect unsigned wraparound in the
     CORE_ADDR math, we ignore this test and print the offset,
     because addr+max_symbolic_offset has wrapped through the end
     of the address space back to the beginning, giving bogus comparison.  */
  if (addr > name_location + max_symbolic_offset
      && name_location + max_symbolic_offset > name_location)
    return 1;

  *offset = (LONGEST) addr - name_location;

  *name = name_temp;

  if (print_symbol_filename)
    {
      struct symtab_and_line sal;

      sal = find_pc_sect_line (addr, section, 0);

      if (sal.symtab)
	{
	  *filename = symtab_to_filename_for_display (sal.symtab);
	  *line = sal.line;
	}
    }
  return 0;
}


/* Print address ADDR symbolically on STREAM.
   First print it as a number.  Then perhaps print
   <SYMBOL + OFFSET> after the number.  */

void
print_address (struct gdbarch *gdbarch,
	       CORE_ADDR addr, struct ui_file *stream)
{
  fputs_styled (paddress (gdbarch, addr), address_style.style (), stream);
  print_address_symbolic (gdbarch, addr, stream, asm_demangle, " ");
}

/* Return a prefix for instruction address:
   "=> " for current instruction, else "   ".  */

const char *
pc_prefix (CORE_ADDR addr)
{
  if (has_stack_frames ())
    {
      struct frame_info *frame;
      CORE_ADDR pc;

      frame = get_selected_frame (NULL);
      if (get_frame_pc_if_available (frame, &pc) && pc == addr)
	return "=> ";
    }
  return "   ";
}

/* Print address ADDR symbolically on STREAM.  Parameter DEMANGLE
   controls whether to print the symbolic name "raw" or demangled.
   Return non-zero if anything was printed; zero otherwise.  */

int
print_address_demangle (const struct value_print_options *opts,
			struct gdbarch *gdbarch, CORE_ADDR addr,
			struct ui_file *stream, int do_demangle)
{
  if (opts->addressprint)
    {
      fputs_styled (paddress (gdbarch, addr), address_style.style (), stream);
      print_address_symbolic (gdbarch, addr, stream, do_demangle, " ");
    }
  else
    {
      return print_address_symbolic (gdbarch, addr, stream, do_demangle, "");
    }
  return 1;
}


/* Find the address of the instruction that is INST_COUNT instructions before
   the instruction at ADDR.
   Since some architectures have variable-length instructions, we can't just
   simply subtract INST_COUNT * INSN_LEN from ADDR.  Instead, we use line
   number information to locate the nearest known instruction boundary,
   and disassemble forward from there.  If we go out of the symbol range
   during disassembling, we return the lowest address we've got so far and
   set the number of instructions read to INST_READ.  */

static CORE_ADDR
find_instruction_backward (struct gdbarch *gdbarch, CORE_ADDR addr,
                           int inst_count, int *inst_read)
{
  /* The vector PCS is used to store instruction addresses within
     a pc range.  */
  CORE_ADDR loop_start, loop_end, p;
  std::vector<CORE_ADDR> pcs;
  struct symtab_and_line sal;

  *inst_read = 0;
  loop_start = loop_end = addr;

  /* In each iteration of the outer loop, we get a pc range that ends before
     LOOP_START, then we count and store every instruction address of the range
     iterated in the loop.
     If the number of instructions counted reaches INST_COUNT, return the
     stored address that is located INST_COUNT instructions back from ADDR.
     If INST_COUNT is not reached, we subtract the number of counted
     instructions from INST_COUNT, and go to the next iteration.  */
  do
    {
      pcs.clear ();
      sal = find_pc_sect_line (loop_start, NULL, 1);
      if (sal.line <= 0)
        {
          /* We reach here when line info is not available.  In this case,
             we print a message and just exit the loop.  The return value
             is calculated after the loop.  */
          printf_filtered (_("No line number information available "
                             "for address "));
          wrap_here ("  ");
          print_address (gdbarch, loop_start - 1, gdb_stdout);
          printf_filtered ("\n");
          break;
        }

      loop_end = loop_start;
      loop_start = sal.pc;

      /* This loop pushes instruction addresses in the range from
         LOOP_START to LOOP_END.  */
      for (p = loop_start; p < loop_end;)
        {
	  pcs.push_back (p);
          p += gdb_insn_length (gdbarch, p);
        }

      inst_count -= pcs.size ();
      *inst_read += pcs.size ();
    }
  while (inst_count > 0);

  /* After the loop, the vector PCS has instruction addresses of the last
     source line we processed, and INST_COUNT has a negative value.
     We return the address at the index of -INST_COUNT in the vector for
     the reason below.
     Let's assume the following instruction addresses and run 'x/-4i 0x400e'.
       Line X of File
          0x4000
          0x4001
          0x4005
       Line Y of File
          0x4009
          0x400c
       => 0x400e
          0x4011
     find_instruction_backward is called with INST_COUNT = 4 and expected to
     return 0x4001.  When we reach here, INST_COUNT is set to -1 because
     it was subtracted by 2 (from Line Y) and 3 (from Line X).  The value
     4001 is located at the index 1 of the last iterated line (= Line X),
     which is simply calculated by -INST_COUNT.
     The case when the length of PCS is 0 means that we reached an area for
     which line info is not available.  In such case, we return LOOP_START,
     which was the lowest instruction address that had line info.  */
  p = pcs.size () > 0 ? pcs[-inst_count] : loop_start;

  /* INST_READ includes all instruction addresses in a pc range.  Need to
     exclude the beginning part up to the address we're returning.  That
     is, exclude {0x4000} in the example above.  */
  if (inst_count < 0)
    *inst_read += inst_count;

  return p;
}

/* Backward read LEN bytes of target memory from address MEMADDR + LEN,
   placing the results in GDB's memory from MYADDR + LEN.  Returns
   a count of the bytes actually read.  */

static int
read_memory_backward (struct gdbarch *gdbarch,
                      CORE_ADDR memaddr, gdb_byte *myaddr, int len)
{
  int errcode;
  int nread;      /* Number of bytes actually read.  */

  /* First try a complete read.  */
  errcode = target_read_memory (memaddr, myaddr, len);
  if (errcode == 0)
    {
      /* Got it all.  */
      nread = len;
    }
  else
    {
      /* Loop, reading one byte at a time until we get as much as we can.  */
      memaddr += len;
      myaddr += len;
      for (nread = 0; nread < len; ++nread)
        {
          errcode = target_read_memory (--memaddr, --myaddr, 1);
          if (errcode != 0)
            {
              /* The read was unsuccessful, so exit the loop.  */
              printf_filtered (_("Cannot access memory at address %s\n"),
                               paddress (gdbarch, memaddr));
              break;
            }
        }
    }
  return nread;
}

/* Returns true if X (which is LEN bytes wide) is the number zero.  */

static int
integer_is_zero (const gdb_byte *x, int len)
{
  int i = 0;

  while (i < len && x[i] == 0)
    ++i;
  return (i == len);
}

/* Find the start address of a string in which ADDR is included.
   Basically we search for '\0' and return the next address,
   but if OPTIONS->PRINT_MAX is smaller than the length of a string,
   we stop searching and return the address to print characters as many as
   PRINT_MAX from the string.  */

static CORE_ADDR
find_string_backward (struct gdbarch *gdbarch,
                      CORE_ADDR addr, int count, int char_size,
                      const struct value_print_options *options,
                      int *strings_counted)
{
  const int chunk_size = 0x20;
  int read_error = 0;
  int chars_read = 0;
  int chars_to_read = chunk_size;
  int chars_counted = 0;
  int count_original = count;
  CORE_ADDR string_start_addr = addr;

  gdb_assert (char_size == 1 || char_size == 2 || char_size == 4);
  gdb::byte_vector buffer (chars_to_read * char_size);
  while (count > 0 && read_error == 0)
    {
      int i;

      addr -= chars_to_read * char_size;
      chars_read = read_memory_backward (gdbarch, addr, buffer.data (),
                                         chars_to_read * char_size);
      chars_read /= char_size;
      read_error = (chars_read == chars_to_read) ? 0 : 1;
      /* Searching for '\0' from the end of buffer in backward direction.  */
      for (i = 0; i < chars_read && count > 0 ; ++i, ++chars_counted)
        {
          int offset = (chars_to_read - i - 1) * char_size;

          if (integer_is_zero (&buffer[offset], char_size)
              || chars_counted == options->print_max)
            {
              /* Found '\0' or reached print_max.  As OFFSET is the offset to
                 '\0', we add CHAR_SIZE to return the start address of
                 a string.  */
              --count;
              string_start_addr = addr + offset + char_size;
              chars_counted = 0;
            }
        }
    }

  /* Update STRINGS_COUNTED with the actual number of loaded strings.  */
  *strings_counted = count_original - count;

  if (read_error != 0)
    {
      /* In error case, STRING_START_ADDR is pointing to the string that
         was last successfully loaded.  Rewind the partially loaded string.  */
      string_start_addr -= chars_counted * char_size;
    }

  return string_start_addr;
}

/* Examine data at address ADDR in format FMT.
   Fetch it from memory and print on gdb_stdout.  */

static void
do_examine (struct format_data fmt, struct gdbarch *gdbarch, CORE_ADDR addr)
{
  char format = 0;
  char size;
  int count = 1;
  struct type *val_type = NULL;
  int i;
  int maxelts;
  struct value_print_options opts;
  int need_to_update_next_address = 0;
  CORE_ADDR addr_rewound = 0;

  format = fmt.format;
  size = fmt.size;
  count = fmt.count;
  next_gdbarch = gdbarch;
  next_address = addr;

  /* Instruction format implies fetch single bytes
     regardless of the specified size.
     The case of strings is handled in decode_format, only explicit
     size operator are not changed to 'b'.  */
  if (format == 'i')
    size = 'b';

  if (size == 'a')
    {
      /* Pick the appropriate size for an address.  */
      if (gdbarch_ptr_bit (next_gdbarch) == 64)
	size = 'g';
      else if (gdbarch_ptr_bit (next_gdbarch) == 32)
	size = 'w';
      else if (gdbarch_ptr_bit (next_gdbarch) == 16)
	size = 'h';
      else
	/* Bad value for gdbarch_ptr_bit.  */
	internal_error (__FILE__, __LINE__,
			_("failed internal consistency check"));
    }

  if (size == 'b')
    val_type = builtin_type (next_gdbarch)->builtin_int8;
  else if (size == 'h')
    val_type = builtin_type (next_gdbarch)->builtin_int16;
  else if (size == 'w')
    val_type = builtin_type (next_gdbarch)->builtin_int32;
  else if (size == 'g')
    val_type = builtin_type (next_gdbarch)->builtin_int64;

  if (format == 's')
    {
      struct type *char_type = NULL;

      /* Search for "char16_t"  or "char32_t" types or fall back to 8-bit char
	 if type is not found.  */
      if (size == 'h')
	char_type = builtin_type (next_gdbarch)->builtin_char16;
      else if (size == 'w')
	char_type = builtin_type (next_gdbarch)->builtin_char32;
      if (char_type)
        val_type = char_type;
      else
        {
	  if (size != '\0' && size != 'b')
	    warning (_("Unable to display strings with "
		       "size '%c', using 'b' instead."), size);
	  size = 'b';
	  val_type = builtin_type (next_gdbarch)->builtin_int8;
        }
    }

  maxelts = 8;
  if (size == 'w')
    maxelts = 4;
  if (size == 'g')
    maxelts = 2;
  if (format == 's' || format == 'i')
    maxelts = 1;

  get_formatted_print_options (&opts, format);

  if (count < 0)
    {
      /* This is the negative repeat count case.
         We rewind the address based on the given repeat count and format,
         then examine memory from there in forward direction.  */

      count = -count;
      if (format == 'i')
        {
          next_address = find_instruction_backward (gdbarch, addr, count,
                                                    &count);
        }
      else if (format == 's')
        {
          next_address = find_string_backward (gdbarch, addr, count,
                                               TYPE_LENGTH (val_type),
                                               &opts, &count);
        }
      else
        {
          next_address = addr - count * TYPE_LENGTH (val_type);
        }

      /* The following call to print_formatted updates next_address in every
         iteration.  In backward case, we store the start address here
         and update next_address with it before exiting the function.  */
      addr_rewound = (format == 's'
                      ? next_address - TYPE_LENGTH (val_type)
                      : next_address);
      need_to_update_next_address = 1;
    }

  /* Print as many objects as specified in COUNT, at most maxelts per line,
     with the address of the next one at the start of each line.  */

  while (count > 0)
    {
      QUIT;
      if (format == 'i')
	fputs_filtered (pc_prefix (next_address), gdb_stdout);
      print_address (next_gdbarch, next_address, gdb_stdout);
      printf_filtered (":");
      for (i = maxelts;
	   i > 0 && count > 0;
	   i--, count--)
	{
	  printf_filtered ("\t");
	  /* Note that print_formatted sets next_address for the next
	     object.  */
	  last_examine_address = next_address;

	  /* The value to be displayed is not fetched greedily.
	     Instead, to avoid the possibility of a fetched value not
	     being used, its retrieval is delayed until the print code
	     uses it.  When examining an instruction stream, the
	     disassembler will perform its own memory fetch using just
	     the address stored in LAST_EXAMINE_VALUE.  FIXME: Should
	     the disassembler be modified so that LAST_EXAMINE_VALUE
	     is left with the byte sequence from the last complete
	     instruction fetched from memory?  */
	  last_examine_value
	    = release_value (value_at_lazy (val_type, next_address));

	  print_formatted (last_examine_value.get (), size, &opts, gdb_stdout);

	  /* Display any branch delay slots following the final insn.  */
	  if (format == 'i' && count == 1)
	    count += branch_delay_insns;
	}
      printf_filtered ("\n");
    }

  if (need_to_update_next_address)
    next_address = addr_rewound;
}

static void
validate_format (struct format_data fmt, const char *cmdname)
{
  if (fmt.size != 0)
    error (_("Size letters are meaningless in \"%s\" command."), cmdname);
  if (fmt.count != 1)
    error (_("Item count other than 1 is meaningless in \"%s\" command."),
	   cmdname);
  if (fmt.format == 'i')
    error (_("Format letter \"%c\" is meaningless in \"%s\" command."),
	   fmt.format, cmdname);
}

/* Parse print command format string into *OPTS and update *EXPP.
   CMDNAME should name the current command.  */

void
print_command_parse_format (const char **expp, const char *cmdname,
			    value_print_options *opts)
{
  const char *exp = *expp;

  /* opts->raw value might already have been set by 'set print raw-values'
     or by using 'print -raw-values'.
     So, do not set opts->raw to 0, only set it to 1 if /r is given.  */
  if (exp && *exp == '/')
    {
      format_data fmt;

      exp++;
      fmt = decode_format (&exp, last_format, 0);
      validate_format (fmt, cmdname);
      last_format = fmt.format;

      opts->format = fmt.format;
      opts->raw = opts->raw || fmt.raw;
    }
  else
    {
      opts->format = 0;
    }

  *expp = exp;
}

/* See valprint.h.  */

void
print_value (value *val, const value_print_options &opts)
{
  int histindex = record_latest_value (val);

  annotate_value_history_begin (histindex, value_type (val));

  printf_filtered ("$%d = ", histindex);

  annotate_value_history_value ();

  print_formatted (val, 0, &opts, gdb_stdout);
  printf_filtered ("\n");

  annotate_value_history_end ();
}

/* Implementation of the "print" and "call" commands.  */

static void
print_command_1 (const char *args, int voidprint)
{
  struct value *val;
  value_print_options print_opts;

  get_user_print_options (&print_opts);
  /* Override global settings with explicit options, if any.  */
  auto group = make_value_print_options_def_group (&print_opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_REQUIRE_DELIMITER, group);

  print_command_parse_format (&args, "print", &print_opts);

  const char *exp = args;

  if (exp != nullptr && *exp)
    {
      expression_up expr = parse_expression (exp);
      val = evaluate_expression (expr.get ());
    }
  else
    val = access_value_history (0);

  if (voidprint || (val && value_type (val) &&
		    value_type (val)->code () != TYPE_CODE_VOID))
    print_value (val, print_opts);
}

/* See valprint.h.  */

void
print_command_completer (struct cmd_list_element *ignore,
			 completion_tracker &tracker,
			 const char *text, const char * /*word*/)
{
  const auto group = make_value_print_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_REQUIRE_DELIMITER, group))
    return;

  const char *word = advance_to_expression_complete_word_point (tracker, text);
  expression_completer (ignore, tracker, text, word);
}

static void
print_command (const char *exp, int from_tty)
{
  print_command_1 (exp, 1);
}

/* Same as print, except it doesn't print void results.  */
static void
call_command (const char *exp, int from_tty)
{
  print_command_1 (exp, 0);
}

/* Implementation of the "output" command.  */

void
output_command (const char *exp, int from_tty)
{
  char format = 0;
  struct value *val;
  struct format_data fmt;
  struct value_print_options opts;

  fmt.size = 0;
  fmt.raw = 0;

  if (exp && *exp == '/')
    {
      exp++;
      fmt = decode_format (&exp, 0, 0);
      validate_format (fmt, "output");
      format = fmt.format;
    }

  expression_up expr = parse_expression (exp);

  val = evaluate_expression (expr.get ());

  annotate_value_begin (value_type (val));

  get_formatted_print_options (&opts, format);
  opts.raw = fmt.raw;
  print_formatted (val, fmt.size, &opts, gdb_stdout);

  annotate_value_end ();

  wrap_here ("");
  gdb_flush (gdb_stdout);
}

static void
set_command (const char *exp, int from_tty)
{
  expression_up expr = parse_expression (exp);

  if (expr->nelts >= 1)
    switch (expr->elts[0].opcode)
      {
      case UNOP_PREINCREMENT:
      case UNOP_POSTINCREMENT:
      case UNOP_PREDECREMENT:
      case UNOP_POSTDECREMENT:
      case BINOP_ASSIGN:
      case BINOP_ASSIGN_MODIFY:
      case BINOP_COMMA:
	break;
      default:
	warning
	  (_("Expression is not an assignment (and might have no effect)"));
      }

  evaluate_expression (expr.get ());
}

static void
info_symbol_command (const char *arg, int from_tty)
{
  struct minimal_symbol *msymbol;
  struct obj_section *osect;
  CORE_ADDR addr, sect_addr;
  int matches = 0;
  unsigned int offset;

  if (!arg)
    error_no_arg (_("address"));

  addr = parse_and_eval_address (arg);
  for (objfile *objfile : current_program_space->objfiles ())
    ALL_OBJFILE_OSECTIONS (objfile, osect)
      {
	/* Only process each object file once, even if there's a separate
	   debug file.  */
	if (objfile->separate_debug_objfile_backlink)
	  continue;

	sect_addr = overlay_mapped_address (addr, osect);

	if (obj_section_addr (osect) <= sect_addr
	    && sect_addr < obj_section_endaddr (osect)
	    && (msymbol
		= lookup_minimal_symbol_by_pc_section (sect_addr,
						       osect).minsym))
	  {
	    const char *obj_name, *mapped, *sec_name, *msym_name;
	    const char *loc_string;

	    matches = 1;
	    offset = sect_addr - MSYMBOL_VALUE_ADDRESS (objfile, msymbol);
	    mapped = section_is_mapped (osect) ? _("mapped") : _("unmapped");
	    sec_name = osect->the_bfd_section->name;
	    msym_name = msymbol->print_name ();

	    /* Don't print the offset if it is zero.
	       We assume there's no need to handle i18n of "sym + offset".  */
	    std::string string_holder;
	    if (offset)
	      {
		string_holder = string_printf ("%s + %u", msym_name, offset);
		loc_string = string_holder.c_str ();
	      }
	    else
	      loc_string = msym_name;

	    gdb_assert (osect->objfile && objfile_name (osect->objfile));
	    obj_name = objfile_name (osect->objfile);

	    if (current_program_space->multi_objfile_p ())
	      if (pc_in_unmapped_range (addr, osect))
		if (section_is_overlay (osect))
		  printf_filtered (_("%s in load address range of "
				     "%s overlay section %s of %s\n"),
				   loc_string, mapped, sec_name, obj_name);
		else
		  printf_filtered (_("%s in load address range of "
				     "section %s of %s\n"),
				   loc_string, sec_name, obj_name);
	      else
		if (section_is_overlay (osect))
		  printf_filtered (_("%s in %s overlay section %s of %s\n"),
				   loc_string, mapped, sec_name, obj_name);
		else
		  printf_filtered (_("%s in section %s of %s\n"),
				   loc_string, sec_name, obj_name);
	    else
	      if (pc_in_unmapped_range (addr, osect))
		if (section_is_overlay (osect))
		  printf_filtered (_("%s in load address range of %s overlay "
				     "section %s\n"),
				   loc_string, mapped, sec_name);
		else
		  printf_filtered
		    (_("%s in load address range of section %s\n"),
		     loc_string, sec_name);
	      else
		if (section_is_overlay (osect))
		  printf_filtered (_("%s in %s overlay section %s\n"),
				   loc_string, mapped, sec_name);
		else
		  printf_filtered (_("%s in section %s\n"),
				   loc_string, sec_name);
	  }
      }
  if (matches == 0)
    printf_filtered (_("No symbol matches %s.\n"), arg);
}

static void
info_address_command (const char *exp, int from_tty)
{
  struct gdbarch *gdbarch;
  int regno;
  struct symbol *sym;
  struct bound_minimal_symbol msymbol;
  long val;
  struct obj_section *section;
  CORE_ADDR load_addr, context_pc = 0;
  struct field_of_this_result is_a_field_of_this;

  if (exp == 0)
    error (_("Argument required."));

  sym = lookup_symbol (exp, get_selected_block (&context_pc), VAR_DOMAIN,
		       &is_a_field_of_this).symbol;
  if (sym == NULL)
    {
      if (is_a_field_of_this.type != NULL)
	{
	  printf_filtered ("Symbol \"");
	  fprintf_symbol_filtered (gdb_stdout, exp,
				   current_language->la_language, DMGL_ANSI);
	  printf_filtered ("\" is a field of the local class variable ");
	  if (current_language->la_language == language_objc)
	    printf_filtered ("`self'\n");	/* ObjC equivalent of "this" */
	  else
	    printf_filtered ("`this'\n");
	  return;
	}

      msymbol = lookup_bound_minimal_symbol (exp);

      if (msymbol.minsym != NULL)
	{
	  struct objfile *objfile = msymbol.objfile;

	  gdbarch = objfile->arch ();
	  load_addr = BMSYMBOL_VALUE_ADDRESS (msymbol);

	  printf_filtered ("Symbol \"");
	  fprintf_symbol_filtered (gdb_stdout, exp,
				   current_language->la_language, DMGL_ANSI);
	  printf_filtered ("\" is at ");
	  fputs_styled (paddress (gdbarch, load_addr), address_style.style (),
			gdb_stdout);
	  printf_filtered (" in a file compiled without debugging");
	  section = MSYMBOL_OBJ_SECTION (objfile, msymbol.minsym);
	  if (section_is_overlay (section))
	    {
	      load_addr = overlay_unmapped_address (load_addr, section);
	      printf_filtered (",\n -- loaded at ");
	      fputs_styled (paddress (gdbarch, load_addr),
			    address_style.style (),
			    gdb_stdout);
	      printf_filtered (" in overlay section %s",
			       section->the_bfd_section->name);
	    }
	  printf_filtered (".\n");
	}
      else
	error (_("No symbol \"%s\" in current context."), exp);
      return;
    }

  printf_filtered ("Symbol \"");
  fprintf_symbol_filtered (gdb_stdout, sym->print_name (),
			   current_language->la_language, DMGL_ANSI);
  printf_filtered ("\" is ");
  val = SYMBOL_VALUE (sym);
  if (SYMBOL_OBJFILE_OWNED (sym))
    section = SYMBOL_OBJ_SECTION (symbol_objfile (sym), sym);
  else
    section = NULL;
  gdbarch = symbol_arch (sym);

  if (SYMBOL_COMPUTED_OPS (sym) != NULL)
    {
      SYMBOL_COMPUTED_OPS (sym)->describe_location (sym, context_pc,
						    gdb_stdout);
      printf_filtered (".\n");
      return;
    }

  switch (SYMBOL_CLASS (sym))
    {
    case LOC_CONST:
    case LOC_CONST_BYTES:
      printf_filtered ("constant");
      break;

    case LOC_LABEL:
      printf_filtered ("a label at address ");
      load_addr = SYMBOL_VALUE_ADDRESS (sym);
      fputs_styled (paddress (gdbarch, load_addr), address_style.style (),
		    gdb_stdout);
      if (section_is_overlay (section))
	{
	  load_addr = overlay_unmapped_address (load_addr, section);
	  printf_filtered (",\n -- loaded at ");
	  fputs_styled (paddress (gdbarch, load_addr), address_style.style (),
			gdb_stdout);
	  printf_filtered (" in overlay section %s",
			   section->the_bfd_section->name);
	}
      break;

    case LOC_COMPUTED:
      gdb_assert_not_reached (_("LOC_COMPUTED variable missing a method"));

    case LOC_REGISTER:
      /* GDBARCH is the architecture associated with the objfile the symbol
	 is defined in; the target architecture may be different, and may
	 provide additional registers.  However, we do not know the target
	 architecture at this point.  We assume the objfile architecture
	 will contain all the standard registers that occur in debug info
	 in that objfile.  */
      regno = SYMBOL_REGISTER_OPS (sym)->register_number (sym, gdbarch);

      if (SYMBOL_IS_ARGUMENT (sym))
	printf_filtered (_("an argument in register %s"),
			 gdbarch_register_name (gdbarch, regno));
      else
	printf_filtered (_("a variable in register %s"),
			 gdbarch_register_name (gdbarch, regno));
      break;

    case LOC_STATIC:
      printf_filtered (_("static storage at address "));
      load_addr = SYMBOL_VALUE_ADDRESS (sym);
      fputs_styled (paddress (gdbarch, load_addr), address_style.style (),
		    gdb_stdout);
      if (section_is_overlay (section))
	{
	  load_addr = overlay_unmapped_address (load_addr, section);
	  printf_filtered (_(",\n -- loaded at "));
	  fputs_styled (paddress (gdbarch, load_addr), address_style.style (),
			gdb_stdout);
	  printf_filtered (_(" in overlay section %s"),
			   section->the_bfd_section->name);
	}
      break;

    case LOC_REGPARM_ADDR:
      /* Note comment at LOC_REGISTER.  */
      regno = SYMBOL_REGISTER_OPS (sym)->register_number (sym, gdbarch);
      printf_filtered (_("address of an argument in register %s"),
		       gdbarch_register_name (gdbarch, regno));
      break;

    case LOC_ARG:
      printf_filtered (_("an argument at offset %ld"), val);
      break;

    case LOC_LOCAL:
      printf_filtered (_("a local variable at frame offset %ld"), val);
      break;

    case LOC_REF_ARG:
      printf_filtered (_("a reference argument at offset %ld"), val);
      break;

    case LOC_TYPEDEF:
      printf_filtered (_("a typedef"));
      break;

    case LOC_BLOCK:
      printf_filtered (_("a function at address "));
      load_addr = BLOCK_ENTRY_PC (SYMBOL_BLOCK_VALUE (sym));
      fputs_styled (paddress (gdbarch, load_addr), address_style.style (),
		    gdb_stdout);
      if (section_is_overlay (section))
	{
	  load_addr = overlay_unmapped_address (load_addr, section);
	  printf_filtered (_(",\n -- loaded at "));
	  fputs_styled (paddress (gdbarch, load_addr), address_style.style (),
			gdb_stdout);
	  printf_filtered (_(" in overlay section %s"),
			   section->the_bfd_section->name);
	}
      break;

    case LOC_UNRESOLVED:
      {
	struct bound_minimal_symbol msym;

	msym = lookup_bound_minimal_symbol (sym->linkage_name ());
	if (msym.minsym == NULL)
	  printf_filtered ("unresolved");
	else
	  {
	    section = MSYMBOL_OBJ_SECTION (msym.objfile, msym.minsym);

	    if (section
		&& (section->the_bfd_section->flags & SEC_THREAD_LOCAL) != 0)
	      {
		load_addr = MSYMBOL_VALUE_RAW_ADDRESS (msym.minsym);
		printf_filtered (_("a thread-local variable at offset %s "
				   "in the thread-local storage for `%s'"),
				 paddress (gdbarch, load_addr),
				 objfile_name (section->objfile));
	      }
	    else
	      {
		load_addr = BMSYMBOL_VALUE_ADDRESS (msym);
		printf_filtered (_("static storage at address "));
		fputs_styled (paddress (gdbarch, load_addr),
			      address_style.style (), gdb_stdout);
		if (section_is_overlay (section))
		  {
		    load_addr = overlay_unmapped_address (load_addr, section);
		    printf_filtered (_(",\n -- loaded at "));
		    fputs_styled (paddress (gdbarch, load_addr),
				  address_style.style (),
				  gdb_stdout);
		    printf_filtered (_(" in overlay section %s"),
				     section->the_bfd_section->name);
		  }
	      }
	  }
      }
      break;

    case LOC_OPTIMIZED_OUT:
      printf_filtered (_("optimized out"));
      break;

    default:
      printf_filtered (_("of unknown (botched) type"));
      break;
    }
  printf_filtered (".\n");
}


static void
x_command (const char *exp, int from_tty)
{
  struct format_data fmt;
  struct value *val;

  fmt.format = last_format ? last_format : 'x';
  fmt.size = last_size;
  fmt.count = 1;
  fmt.raw = 0;

  /* If there is no expression and no format, use the most recent
     count.  */
  if (exp == nullptr && last_count > 0)
    fmt.count = last_count;

  if (exp && *exp == '/')
    {
      const char *tmp = exp + 1;

      fmt = decode_format (&tmp, last_format, last_size);
      exp = (char *) tmp;
    }

  last_count = fmt.count;

  /* If we have an expression, evaluate it and use it as the address.  */

  if (exp != 0 && *exp != 0)
    {
      expression_up expr = parse_expression (exp);
      /* Cause expression not to be there any more if this command is
         repeated with Newline.  But don't clobber a user-defined
         command's definition.  */
      if (from_tty)
	set_repeat_arguments ("");
      val = evaluate_expression (expr.get ());
      if (TYPE_IS_REFERENCE (value_type (val)))
	val = coerce_ref (val);
      /* In rvalue contexts, such as this, functions are coerced into
         pointers to functions.  This makes "x/i main" work.  */
      if (value_type (val)->code () == TYPE_CODE_FUNC
	   && VALUE_LVAL (val) == lval_memory)
	next_address = value_address (val);
      else
	next_address = value_as_address (val);

      next_gdbarch = expr->gdbarch;
    }

  if (!next_gdbarch)
    error_no_arg (_("starting display address"));

  do_examine (fmt, next_gdbarch, next_address);

  /* If the examine succeeds, we remember its size and format for next
     time.  Set last_size to 'b' for strings.  */
  if (fmt.format == 's')
    last_size = 'b';
  else
    last_size = fmt.size;
  last_format = fmt.format;

  /* Set a couple of internal variables if appropriate.  */
  if (last_examine_value != nullptr)
    {
      /* Make last address examined available to the user as $_.  Use
         the correct pointer type.  */
      struct type *pointer_type
	= lookup_pointer_type (value_type (last_examine_value.get ()));
      set_internalvar (lookup_internalvar ("_"),
		       value_from_pointer (pointer_type,
					   last_examine_address));

      /* Make contents of last address examined available to the user
	 as $__.  If the last value has not been fetched from memory
	 then don't fetch it now; instead mark it by voiding the $__
	 variable.  */
      if (value_lazy (last_examine_value.get ()))
	clear_internalvar (lookup_internalvar ("__"));
      else
	set_internalvar (lookup_internalvar ("__"), last_examine_value.get ());
    }
}


/* Add an expression to the auto-display chain.
   Specify the expression.  */

static void
display_command (const char *arg, int from_tty)
{
  struct format_data fmt;
  struct display *newobj;
  const char *exp = arg;

  if (exp == 0)
    {
      do_displays ();
      return;
    }

  if (*exp == '/')
    {
      exp++;
      fmt = decode_format (&exp, 0, 0);
      if (fmt.size && fmt.format == 0)
	fmt.format = 'x';
      if (fmt.format == 'i' || fmt.format == 's')
	fmt.size = 'b';
    }
  else
    {
      fmt.format = 0;
      fmt.size = 0;
      fmt.count = 0;
      fmt.raw = 0;
    }

  innermost_block_tracker tracker;
  expression_up expr = parse_expression (exp, &tracker);

  newobj = new display (exp, std::move (expr), fmt,
			current_program_space, tracker.block ());
  all_displays.emplace_back (newobj);

  if (from_tty)
    do_one_display (newobj);

  dont_repeat ();
}

/* Clear out the display_chain.  Done when new symtabs are loaded,
   since this invalidates the types stored in many expressions.  */

void
clear_displays ()
{
  all_displays.clear ();
}

/* Delete the auto-display DISPLAY.  */

static void
delete_display (struct display *display)
{
  gdb_assert (display != NULL);

  auto iter = std::find_if (all_displays.begin (),
			    all_displays.end (),
			    [=] (const std::unique_ptr<struct display> &item)
			    {
			      return item.get () == display;
			    });
  gdb_assert (iter != all_displays.end ());
  all_displays.erase (iter);
}

/* Call FUNCTION on each of the displays whose numbers are given in
   ARGS.  DATA is passed unmodified to FUNCTION.  */

static void
map_display_numbers (const char *args,
		     gdb::function_view<void (struct display *)> function)
{
  int num;

  if (args == NULL)
    error_no_arg (_("one or more display numbers"));

  number_or_range_parser parser (args);

  while (!parser.finished ())
    {
      const char *p = parser.cur_tok ();

      num = parser.get_number ();
      if (num == 0)
	warning (_("bad display number at or near '%s'"), p);
      else
	{
	  auto iter = std::find_if (all_displays.begin (),
				    all_displays.end (),
				    [=] (const std::unique_ptr<display> &item)
				    {
				      return item->number == num;
				    });
	  if (iter == all_displays.end ())
	    printf_unfiltered (_("No display number %d.\n"), num);
	  else
	    function (iter->get ());
	}
    }
}

/* "undisplay" command.  */

static void
undisplay_command (const char *args, int from_tty)
{
  if (args == NULL)
    {
      if (query (_("Delete all auto-display expressions? ")))
	clear_displays ();
      dont_repeat ();
      return;
    }

  map_display_numbers (args, delete_display);
  dont_repeat ();
}

/* Display a single auto-display.  
   Do nothing if the display cannot be printed in the current context,
   or if the display is disabled.  */

static void
do_one_display (struct display *d)
{
  int within_current_scope;

  if (!d->enabled_p)
    return;

  /* The expression carries the architecture that was used at parse time.
     This is a problem if the expression depends on architecture features
     (e.g. register numbers), and the current architecture is now different.
     For example, a display statement like "display/i $pc" is expected to
     display the PC register of the current architecture, not the arch at
     the time the display command was given.  Therefore, we re-parse the
     expression if the current architecture has changed.  */
  if (d->exp != NULL && d->exp->gdbarch != get_current_arch ())
    {
      d->exp.reset ();
      d->block = NULL;
    }

  if (d->exp == NULL)
    {

      try
	{
	  innermost_block_tracker tracker;
	  d->exp = parse_expression (d->exp_string.c_str (), &tracker);
	  d->block = tracker.block ();
	}
      catch (const gdb_exception &ex)
	{
	  /* Can't re-parse the expression.  Disable this display item.  */
	  d->enabled_p = false;
	  warning (_("Unable to display \"%s\": %s"),
		   d->exp_string.c_str (), ex.what ());
	  return;
	}
    }

  if (d->block)
    {
      if (d->pspace == current_program_space)
	within_current_scope = contained_in (get_selected_block (0), d->block,
					     true);
      else
	within_current_scope = 0;
    }
  else
    within_current_scope = 1;
  if (!within_current_scope)
    return;

  scoped_restore save_display_number
    = make_scoped_restore (&current_display_number, d->number);

  annotate_display_begin ();
  printf_filtered ("%d", d->number);
  annotate_display_number_end ();
  printf_filtered (": ");
  if (d->format.size)
    {

      annotate_display_format ();

      printf_filtered ("x/");
      if (d->format.count != 1)
	printf_filtered ("%d", d->format.count);
      printf_filtered ("%c", d->format.format);
      if (d->format.format != 'i' && d->format.format != 's')
	printf_filtered ("%c", d->format.size);
      printf_filtered (" ");

      annotate_display_expression ();

      puts_filtered (d->exp_string.c_str ());
      annotate_display_expression_end ();

      if (d->format.count != 1 || d->format.format == 'i')
	printf_filtered ("\n");
      else
	printf_filtered ("  ");

      annotate_display_value ();

      try
        {
	  struct value *val;
	  CORE_ADDR addr;

	  val = evaluate_expression (d->exp.get ());
	  addr = value_as_address (val);
	  if (d->format.format == 'i')
	    addr = gdbarch_addr_bits_remove (d->exp->gdbarch, addr);
	  do_examine (d->format, d->exp->gdbarch, addr);
	}
      catch (const gdb_exception_error &ex)
	{
	  fprintf_filtered (gdb_stdout, _("%p[<error: %s>%p]\n"),
			    metadata_style.style ().ptr (), ex.what (),
			    nullptr);
	}
    }
  else
    {
      struct value_print_options opts;

      annotate_display_format ();

      if (d->format.format)
	printf_filtered ("/%c ", d->format.format);

      annotate_display_expression ();

      puts_filtered (d->exp_string.c_str ());
      annotate_display_expression_end ();

      printf_filtered (" = ");

      annotate_display_expression ();

      get_formatted_print_options (&opts, d->format.format);
      opts.raw = d->format.raw;

      try
        {
	  struct value *val;

	  val = evaluate_expression (d->exp.get ());
	  print_formatted (val, d->format.size, &opts, gdb_stdout);
	}
      catch (const gdb_exception_error &ex)
	{
	  fprintf_styled (gdb_stdout, metadata_style.style (),
			  _("<error: %s>"), ex.what ());
	}

      printf_filtered ("\n");
    }

  annotate_display_end ();

  gdb_flush (gdb_stdout);
}

/* Display all of the values on the auto-display chain which can be
   evaluated in the current scope.  */

void
do_displays (void)
{
  for (auto &d : all_displays)
    do_one_display (d.get ());
}

/* Delete the auto-display which we were in the process of displaying.
   This is done when there is an error or a signal.  */

void
disable_display (int num)
{
  for (auto &d : all_displays)
    if (d->number == num)
      {
	d->enabled_p = false;
	return;
      }
  printf_unfiltered (_("No display number %d.\n"), num);
}

void
disable_current_display (void)
{
  if (current_display_number >= 0)
    {
      disable_display (current_display_number);
      fprintf_unfiltered (gdb_stderr,
			  _("Disabling display %d to "
			    "avoid infinite recursion.\n"),
			  current_display_number);
    }
  current_display_number = -1;
}

static void
info_display_command (const char *ignore, int from_tty)
{
  if (all_displays.empty ())
    printf_unfiltered (_("There are no auto-display expressions now.\n"));
  else
    printf_filtered (_("Auto-display expressions now in effect:\n\
Num Enb Expression\n"));

  for (auto &d : all_displays)
    {
      printf_filtered ("%d:   %c  ", d->number, "ny"[(int) d->enabled_p]);
      if (d->format.size)
	printf_filtered ("/%d%c%c ", d->format.count, d->format.size,
			 d->format.format);
      else if (d->format.format)
	printf_filtered ("/%c ", d->format.format);
      puts_filtered (d->exp_string.c_str ());
      if (d->block && !contained_in (get_selected_block (0), d->block, true))
	printf_filtered (_(" (cannot be evaluated in the current context)"));
      printf_filtered ("\n");
    }
}

/* Implementation of both the "disable display" and "enable display"
   commands.  ENABLE decides what to do.  */

static void
enable_disable_display_command (const char *args, int from_tty, bool enable)
{
  if (args == NULL)
    {
      for (auto &d : all_displays)
	d->enabled_p = enable;
      return;
    }

  map_display_numbers (args,
		       [=] (struct display *d)
		       {
			 d->enabled_p = enable;
		       });
}

/* The "enable display" command.  */

static void
enable_display_command (const char *args, int from_tty)
{
  enable_disable_display_command (args, from_tty, true);
}

/* The "disable display" command.  */

static void
disable_display_command (const char *args, int from_tty)
{
  enable_disable_display_command (args, from_tty, false);
}

/* display_chain items point to blocks and expressions.  Some expressions in
   turn may point to symbols.
   Both symbols and blocks are obstack_alloc'd on objfile_stack, and are
   obstack_free'd when a shared library is unloaded.
   Clear pointers that are about to become dangling.
   Both .exp and .block fields will be restored next time we need to display
   an item by re-parsing .exp_string field in the new execution context.  */

static void
clear_dangling_display_expressions (struct objfile *objfile)
{
  struct program_space *pspace;

  /* With no symbol file we cannot have a block or expression from it.  */
  if (objfile == NULL)
    return;
  pspace = objfile->pspace;
  if (objfile->separate_debug_objfile_backlink)
    {
      objfile = objfile->separate_debug_objfile_backlink;
      gdb_assert (objfile->pspace == pspace);
    }

  for (auto &d : all_displays)
    {
      if (d->pspace != pspace)
	continue;

      struct objfile *bl_objf = nullptr;
      if (d->block != nullptr)
	{
	  bl_objf = block_objfile (d->block);
	  if (bl_objf->separate_debug_objfile_backlink != nullptr)
	    bl_objf = bl_objf->separate_debug_objfile_backlink;
	}

      if (bl_objf == objfile
	  || (d->exp != NULL && exp_uses_objfile (d->exp.get (), objfile)))
	{
	  d->exp.reset ();
	  d->block = NULL;
	}
    }
}


/* Print the value in stack frame FRAME of a variable specified by a
   struct symbol.  NAME is the name to print; if NULL then VAR's print
   name will be used.  STREAM is the ui_file on which to print the
   value.  INDENT specifies the number of indent levels to print
   before printing the variable name.

   This function invalidates FRAME.  */

void
print_variable_and_value (const char *name, struct symbol *var,
			  struct frame_info *frame,
			  struct ui_file *stream, int indent)
{

  if (!name)
    name = var->print_name ();

  fprintf_filtered (stream, "%s%ps = ", n_spaces (2 * indent),
		    styled_string (variable_name_style.style (), name));

  try
    {
      struct value *val;
      struct value_print_options opts;

      /* READ_VAR_VALUE needs a block in order to deal with non-local
	 references (i.e. to handle nested functions).  In this context, we
	 print variables that are local to this frame, so we can avoid passing
	 a block to it.  */
      val = read_var_value (var, NULL, frame);
      get_user_print_options (&opts);
      opts.deref_ref = 1;
      common_val_print (val, stream, indent, &opts, current_language);

      /* common_val_print invalidates FRAME when a pretty printer calls inferior
	 function.  */
      frame = NULL;
    }
  catch (const gdb_exception_error &except)
    {
      fprintf_styled (stream, metadata_style.style (),
		      "<error reading variable %s (%s)>", name,
		      except.what ());
    }

  fprintf_filtered (stream, "\n");
}

/* Subroutine of ui_printf to simplify it.
   Print VALUE to STREAM using FORMAT.
   VALUE is a C-style string either on the target or
   in a GDB internal variable.  */

static void
printf_c_string (struct ui_file *stream, const char *format,
		 struct value *value)
{
  const gdb_byte *str;

  if (value_type (value)->code () != TYPE_CODE_PTR
      && VALUE_LVAL (value) == lval_internalvar
      && c_is_string_type_p (value_type (value)))
    {
      size_t len = TYPE_LENGTH (value_type (value));

      /* Copy the internal var value to TEM_STR and append a terminating null
	 character.  This protects against corrupted C-style strings that lack
	 the terminating null char.  It also allows Ada-style strings (not
	 null terminated) to be printed without problems.  */
      gdb_byte *tem_str = (gdb_byte *) alloca (len + 1);

      memcpy (tem_str, value_contents (value), len);
      tem_str [len] = 0;
      str = tem_str;
    }
  else
    {
      CORE_ADDR tem = value_as_address (value);;

      if (tem == 0)
	{
	  DIAGNOSTIC_PUSH
	  DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
	  fprintf_filtered (stream, format, "(null)");
	  DIAGNOSTIC_POP
	  return;
	}

      /* This is a %s argument.  Find the length of the string.  */
      size_t len;

      for (len = 0;; len++)
	{
	  gdb_byte c;

	  QUIT;
	  read_memory (tem + len, &c, 1);
	  if (c == 0)
	    break;
	}

      /* Copy the string contents into a string inside GDB.  */
      gdb_byte *tem_str = (gdb_byte *) alloca (len + 1);

      if (len != 0)
	read_memory (tem, tem_str, len);
      tem_str[len] = 0;
      str = tem_str;
    }

  DIAGNOSTIC_PUSH
  DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
  fprintf_filtered (stream, format, (char *) str);
  DIAGNOSTIC_POP
}

/* Subroutine of ui_printf to simplify it.
   Print VALUE to STREAM using FORMAT.
   VALUE is a wide C-style string on the target or
   in a GDB internal variable.  */

static void
printf_wide_c_string (struct ui_file *stream, const char *format,
		      struct value *value)
{
  const gdb_byte *str;
  size_t len;
  struct gdbarch *gdbarch = get_type_arch (value_type (value));
  struct type *wctype = lookup_typename (current_language,
					 "wchar_t", NULL, 0);
  int wcwidth = TYPE_LENGTH (wctype);

  if (VALUE_LVAL (value) == lval_internalvar
      && c_is_string_type_p (value_type (value)))
    {
      str = value_contents (value);
      len = TYPE_LENGTH (value_type (value));
    }
  else
    {
      CORE_ADDR tem = value_as_address (value);

      if (tem == 0)
	{
	  DIAGNOSTIC_PUSH
	  DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
	  fprintf_filtered (stream, format, "(null)");
	  DIAGNOSTIC_POP
	  return;
	}

      /* This is a %s argument.  Find the length of the string.  */
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      gdb_byte *buf = (gdb_byte *) alloca (wcwidth);

      for (len = 0;; len += wcwidth)
	{
	  QUIT;
	  read_memory (tem + len, buf, wcwidth);
	  if (extract_unsigned_integer (buf, wcwidth, byte_order) == 0)
	    break;
	}

      /* Copy the string contents into a string inside GDB.  */
      gdb_byte *tem_str = (gdb_byte *) alloca (len + wcwidth);

      if (len != 0)
	read_memory (tem, tem_str, len);
      memset (&tem_str[len], 0, wcwidth);
      str = tem_str;
    }

  auto_obstack output;

  convert_between_encodings (target_wide_charset (gdbarch),
			     host_charset (),
			     str, len, wcwidth,
			     &output, translit_char);
  obstack_grow_str0 (&output, "");

  DIAGNOSTIC_PUSH
  DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
  fprintf_filtered (stream, format, obstack_base (&output));
  DIAGNOSTIC_POP
}

/* Subroutine of ui_printf to simplify it.
   Print VALUE, a floating point value, to STREAM using FORMAT.  */

static void
printf_floating (struct ui_file *stream, const char *format,
		 struct value *value, enum argclass argclass)
{
  /* Parameter data.  */
  struct type *param_type = value_type (value);
  struct gdbarch *gdbarch = get_type_arch (param_type);

  /* Determine target type corresponding to the format string.  */
  struct type *fmt_type;
  switch (argclass)
    {
      case double_arg:
	fmt_type = builtin_type (gdbarch)->builtin_double;
	break;
      case long_double_arg:
	fmt_type = builtin_type (gdbarch)->builtin_long_double;
	break;
      case dec32float_arg:
	fmt_type = builtin_type (gdbarch)->builtin_decfloat;
	break;
      case dec64float_arg:
	fmt_type = builtin_type (gdbarch)->builtin_decdouble;
	break;
      case dec128float_arg:
	fmt_type = builtin_type (gdbarch)->builtin_declong;
	break;
      default:
	gdb_assert_not_reached ("unexpected argument class");
    }

  /* To match the traditional GDB behavior, the conversion is
     done differently depending on the type of the parameter:

     - if the parameter has floating-point type, it's value
       is converted to the target type;

     - otherwise, if the parameter has a type that is of the
       same size as a built-in floating-point type, the value
       bytes are interpreted as if they were of that type, and
       then converted to the target type (this is not done for
       decimal floating-point argument classes);

     - otherwise, if the source value has an integer value,
       it's value is converted to the target type;

     - otherwise, an error is raised.

     In either case, the result of the conversion is a byte buffer
     formatted in the target format for the target type.  */

  if (fmt_type->code () == TYPE_CODE_FLT)
    {
      param_type = float_type_from_length (param_type);
      if (param_type != value_type (value))
	value = value_from_contents (param_type, value_contents (value));
    }

  value = value_cast (fmt_type, value);

  /* Convert the value to a string and print it.  */
  std::string str
    = target_float_to_string (value_contents (value), fmt_type, format);
  fputs_filtered (str.c_str (), stream);
}

/* Subroutine of ui_printf to simplify it.
   Print VALUE, a target pointer, to STREAM using FORMAT.  */

static void
printf_pointer (struct ui_file *stream, const char *format,
		struct value *value)
{
  /* We avoid the host's %p because pointers are too
     likely to be the wrong size.  The only interesting
     modifier for %p is a width; extract that, and then
     handle %p as glibc would: %#x or a literal "(nil)".  */

  const char *p;
  char *fmt, *fmt_p;
#ifdef PRINTF_HAS_LONG_LONG
  long long val = value_as_long (value);
#else
  long val = value_as_long (value);
#endif

  fmt = (char *) alloca (strlen (format) + 5);

  /* Copy up to the leading %.  */
  p = format;
  fmt_p = fmt;
  while (*p)
    {
      int is_percent = (*p == '%');

      *fmt_p++ = *p++;
      if (is_percent)
	{
	  if (*p == '%')
	    *fmt_p++ = *p++;
	  else
	    break;
	}
    }

  if (val != 0)
    *fmt_p++ = '#';

  /* Copy any width or flags.  Only the "-" flag is valid for pointers
     -- see the format_pieces constructor.  */
  while (*p == '-' || (*p >= '0' && *p < '9'))
    *fmt_p++ = *p++;

  gdb_assert (*p == 'p' && *(p + 1) == '\0');
  if (val != 0)
    {
#ifdef PRINTF_HAS_LONG_LONG
      *fmt_p++ = 'l';
#endif
      *fmt_p++ = 'l';
      *fmt_p++ = 'x';
      *fmt_p++ = '\0';
      DIAGNOSTIC_PUSH
      DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
      fprintf_filtered (stream, fmt, val);
      DIAGNOSTIC_POP
    }
  else
    {
      *fmt_p++ = 's';
      *fmt_p++ = '\0';
      DIAGNOSTIC_PUSH
      DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
      fprintf_filtered (stream, fmt, "(nil)");
      DIAGNOSTIC_POP
    }
}

/* printf "printf format string" ARG to STREAM.  */

static void
ui_printf (const char *arg, struct ui_file *stream)
{
  const char *s = arg;
  std::vector<struct value *> val_args;

  if (s == 0)
    error_no_arg (_("format-control string and values to print"));

  s = skip_spaces (s);

  /* A format string should follow, enveloped in double quotes.  */
  if (*s++ != '"')
    error (_("Bad format string, missing '\"'."));

  format_pieces fpieces (&s);

  if (*s++ != '"')
    error (_("Bad format string, non-terminated '\"'."));
  
  s = skip_spaces (s);

  if (*s != ',' && *s != 0)
    error (_("Invalid argument syntax"));

  if (*s == ',')
    s++;
  s = skip_spaces (s);

  {
    int nargs_wanted;
    int i;
    const char *current_substring;

    nargs_wanted = 0;
    for (auto &&piece : fpieces)
      if (piece.argclass != literal_piece)
	++nargs_wanted;

    /* Now, parse all arguments and evaluate them.
       Store the VALUEs in VAL_ARGS.  */

    while (*s != '\0')
      {
	const char *s1;

	s1 = s;
	val_args.push_back (parse_to_comma_and_eval (&s1));

	s = s1;
	if (*s == ',')
	  s++;
      }

    if (val_args.size () != nargs_wanted)
      error (_("Wrong number of arguments for specified format-string"));

    /* Now actually print them.  */
    i = 0;
    for (auto &&piece : fpieces)
      {
	current_substring = piece.string;
	switch (piece.argclass)
	  {
	  case string_arg:
	    printf_c_string (stream, current_substring, val_args[i]);
	    break;
	  case wide_string_arg:
	    printf_wide_c_string (stream, current_substring, val_args[i]);
	    break;
	  case wide_char_arg:
	    {
	      struct gdbarch *gdbarch
		= get_type_arch (value_type (val_args[i]));
	      struct type *wctype = lookup_typename (current_language,
						     "wchar_t", NULL, 0);
	      struct type *valtype;
	      const gdb_byte *bytes;

	      valtype = value_type (val_args[i]);
	      if (TYPE_LENGTH (valtype) != TYPE_LENGTH (wctype)
		  || valtype->code () != TYPE_CODE_INT)
		error (_("expected wchar_t argument for %%lc"));

	      bytes = value_contents (val_args[i]);

	      auto_obstack output;

	      convert_between_encodings (target_wide_charset (gdbarch),
					 host_charset (),
					 bytes, TYPE_LENGTH (valtype),
					 TYPE_LENGTH (valtype),
					 &output, translit_char);
	      obstack_grow_str0 (&output, "");

	      DIAGNOSTIC_PUSH
	      DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
	      fprintf_filtered (stream, current_substring,
                                obstack_base (&output));
	      DIAGNOSTIC_POP
	    }
	    break;
	  case long_long_arg:
#ifdef PRINTF_HAS_LONG_LONG
	    {
	      long long val = value_as_long (val_args[i]);

	      DIAGNOSTIC_PUSH
	      DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
              fprintf_filtered (stream, current_substring, val);
	      DIAGNOSTIC_POP
	      break;
	    }
#else
	    error (_("long long not supported in printf"));
#endif
	  case int_arg:
	    {
	      int val = value_as_long (val_args[i]);

	      DIAGNOSTIC_PUSH
	      DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
              fprintf_filtered (stream, current_substring, val);
	      DIAGNOSTIC_POP
	      break;
	    }
	  case long_arg:
	    {
	      long val = value_as_long (val_args[i]);

	      DIAGNOSTIC_PUSH
	      DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
              fprintf_filtered (stream, current_substring, val);
	      DIAGNOSTIC_POP
	      break;
	    }
	  case size_t_arg:
	    {
	      size_t val = value_as_long (val_args[i]);

	      DIAGNOSTIC_PUSH
	      DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
              fprintf_filtered (stream, current_substring, val);
	      DIAGNOSTIC_POP
	      break;
	    }
	  /* Handles floating-point values.  */
	  case double_arg:
	  case long_double_arg:
	  case dec32float_arg:
	  case dec64float_arg:
	  case dec128float_arg:
	    printf_floating (stream, current_substring, val_args[i],
			     piece.argclass);
	    break;
	  case ptr_arg:
	    printf_pointer (stream, current_substring, val_args[i]);
	    break;
	  case literal_piece:
	    /* Print a portion of the format string that has no
	       directives.  Note that this will not include any
	       ordinary %-specs, but it might include "%%".  That is
	       why we use printf_filtered and not puts_filtered here.
	       Also, we pass a dummy argument because some platforms
	       have modified GCC to include -Wformat-security by
	       default, which will warn here if there is no
	       argument.  */
	    DIAGNOSTIC_PUSH
	    DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
	    fprintf_filtered (stream, current_substring, 0);
	    DIAGNOSTIC_POP
	    break;
	  default:
	    internal_error (__FILE__, __LINE__,
			    _("failed internal consistency check"));
	  }
	/* Maybe advance to the next argument.  */
	if (piece.argclass != literal_piece)
	  ++i;
      }
  }
}

/* Implement the "printf" command.  */

static void
printf_command (const char *arg, int from_tty)
{
  ui_printf (arg, gdb_stdout);
  reset_terminal_style (gdb_stdout);
  wrap_here ("");
  gdb_stdout->flush ();
}

/* Implement the "eval" command.  */

static void
eval_command (const char *arg, int from_tty)
{
  string_file stb;

  ui_printf (arg, &stb);

  std::string expanded = insert_user_defined_cmd_args (stb.c_str ());

  execute_command (expanded.c_str (), from_tty);
}

void _initialize_printcmd ();
void
_initialize_printcmd ()
{
  struct cmd_list_element *c;

  current_display_number = -1;

  gdb::observers::free_objfile.attach (clear_dangling_display_expressions);

  add_info ("address", info_address_command,
	    _("Describe where symbol SYM is stored.\n\
Usage: info address SYM"));

  add_info ("symbol", info_symbol_command, _("\
Describe what symbol is at location ADDR.\n\
Usage: info symbol ADDR\n\
Only for symbols with fixed locations (global or static scope)."));

  add_com ("x", class_vars, x_command, _("\
Examine memory: x/FMT ADDRESS.\n\
ADDRESS is an expression for the memory address to examine.\n\
FMT is a repeat count followed by a format letter and a size letter.\n\
Format letters are o(octal), x(hex), d(decimal), u(unsigned decimal),\n\
  t(binary), f(float), a(address), i(instruction), c(char), s(string)\n\
  and z(hex, zero padded on the left).\n\
Size letters are b(byte), h(halfword), w(word), g(giant, 8 bytes).\n\
The specified number of objects of the specified size are printed\n\
according to the format.  If a negative number is specified, memory is\n\
examined backward from the address.\n\n\
Defaults for format and size letters are those previously used.\n\
Default count is 1.  Default address is following last thing printed\n\
with this command or \"print\"."));

  add_info ("display", info_display_command, _("\
Expressions to display when program stops, with code numbers.\n\
Usage: info display"));

  add_cmd ("undisplay", class_vars, undisplay_command, _("\
Cancel some expressions to be displayed when program stops.\n\
Usage: undisplay [NUM]...\n\
Arguments are the code numbers of the expressions to stop displaying.\n\
No argument means cancel all automatic-display expressions.\n\
\"delete display\" has the same effect as this command.\n\
Do \"info display\" to see current list of code numbers."),
	   &cmdlist);

  add_com ("display", class_vars, display_command, _("\
Print value of expression EXP each time the program stops.\n\
Usage: display[/FMT] EXP\n\
/FMT may be used before EXP as in the \"print\" command.\n\
/FMT \"i\" or \"s\" or including a size-letter is allowed,\n\
as in the \"x\" command, and then EXP is used to get the address to examine\n\
and examining is done as in the \"x\" command.\n\n\
With no argument, display all currently requested auto-display expressions.\n\
Use \"undisplay\" to cancel display requests previously made."));

  add_cmd ("display", class_vars, enable_display_command, _("\
Enable some expressions to be displayed when program stops.\n\
Usage: enable display [NUM]...\n\
Arguments are the code numbers of the expressions to resume displaying.\n\
No argument means enable all automatic-display expressions.\n\
Do \"info display\" to see current list of code numbers."), &enablelist);

  add_cmd ("display", class_vars, disable_display_command, _("\
Disable some expressions to be displayed when program stops.\n\
Usage: disable display [NUM]...\n\
Arguments are the code numbers of the expressions to stop displaying.\n\
No argument means disable all automatic-display expressions.\n\
Do \"info display\" to see current list of code numbers."), &disablelist);

  add_cmd ("display", class_vars, undisplay_command, _("\
Cancel some expressions to be displayed when program stops.\n\
Usage: delete display [NUM]...\n\
Arguments are the code numbers of the expressions to stop displaying.\n\
No argument means cancel all automatic-display expressions.\n\
Do \"info display\" to see current list of code numbers."), &deletelist);

  add_com ("printf", class_vars, printf_command, _("\
Formatted printing, like the C \"printf\" function.\n\
Usage: printf \"format string\", ARG1, ARG2, ARG3, ..., ARGN\n\
This supports most C printf format specifications, like %s, %d, etc."));

  add_com ("output", class_vars, output_command, _("\
Like \"print\" but don't put in value history and don't print newline.\n\
Usage: output EXP\n\
This is useful in user-defined commands."));

  add_prefix_cmd ("set", class_vars, set_command, _("\
Evaluate expression EXP and assign result to variable VAR.\n\
Usage: set VAR = EXP\n\
This uses assignment syntax appropriate for the current language\n\
(VAR = EXP or VAR := EXP for example).\n\
VAR may be a debugger \"convenience\" variable (names starting\n\
with $), a register (a few standard names starting with $), or an actual\n\
variable in the program being debugged.  EXP is any valid expression.\n\
Use \"set variable\" for variables with names identical to set subcommands.\n\
\n\
With a subcommand, this command modifies parts of the gdb environment.\n\
You can see these environment settings with the \"show\" command."),
		  &setlist, "set ", 1, &cmdlist);
  if (dbx_commands)
    add_com ("assign", class_vars, set_command, _("\
Evaluate expression EXP and assign result to variable VAR.\n\
Usage: assign VAR = EXP\n\
This uses assignment syntax appropriate for the current language\n\
(VAR = EXP or VAR := EXP for example).\n\
VAR may be a debugger \"convenience\" variable (names starting\n\
with $), a register (a few standard names starting with $), or an actual\n\
variable in the program being debugged.  EXP is any valid expression.\n\
Use \"set variable\" for variables with names identical to set subcommands.\n\
\nWith a subcommand, this command modifies parts of the gdb environment.\n\
You can see these environment settings with the \"show\" command."));

  /* "call" is the same as "set", but handy for dbx users to call fns.  */
  c = add_com ("call", class_vars, call_command, _("\
Call a function in the program.\n\
Usage: call EXP\n\
The argument is the function name and arguments, in the notation of the\n\
current working language.  The result is printed and saved in the value\n\
history, if it is not void."));
  set_cmd_completer_handle_brkchars (c, print_command_completer);

  add_cmd ("variable", class_vars, set_command, _("\
Evaluate expression EXP and assign result to variable VAR.\n\
Usage: set variable VAR = EXP\n\
This uses assignment syntax appropriate for the current language\n\
(VAR = EXP or VAR := EXP for example).\n\
VAR may be a debugger \"convenience\" variable (names starting\n\
with $), a register (a few standard names starting with $), or an actual\n\
variable in the program being debugged.  EXP is any valid expression.\n\
This may usually be abbreviated to simply \"set\"."),
	   &setlist);
  add_alias_cmd ("var", "variable", class_vars, 0, &setlist);

  const auto print_opts = make_value_print_options_def_group (nullptr);

  static const std::string print_help = gdb::option::build_help (_("\
Print value of expression EXP.\n\
Usage: print [[OPTION]... --] [/FMT] [EXP]\n\
\n\
Options:\n\
%OPTIONS%\n\
\n\
Note: because this command accepts arbitrary expressions, if you\n\
specify any command option, you must use a double dash (\"--\")\n\
to mark the end of option processing.  E.g.: \"print -o -- myobj\".\n\
\n\
Variables accessible are those of the lexical environment of the selected\n\
stack frame, plus all those whose scope is global or an entire file.\n\
\n\
$NUM gets previous value number NUM.  $ and $$ are the last two values.\n\
$$NUM refers to NUM'th value back from the last one.\n\
Names starting with $ refer to registers (with the values they would have\n\
if the program were to return to the stack frame now selected, restoring\n\
all registers saved by frames farther in) or else to debugger\n\
\"convenience\" variables (any such name not a known register).\n\
Use assignment expressions to give values to convenience variables.\n\
\n\
{TYPE}ADREXP refers to a datum of data type TYPE, located at address ADREXP.\n\
@ is a binary operator for treating consecutive data objects\n\
anywhere in memory as an array.  FOO@NUM gives an array whose first\n\
element is FOO, whose second element is stored in the space following\n\
where FOO is stored, etc.  FOO must be an expression whose value\n\
resides in memory.\n\
\n\
EXP may be preceded with /FMT, where FMT is a format letter\n\
but no count or size letter (see \"x\" command)."),
					      print_opts);

  c = add_com ("print", class_vars, print_command, print_help.c_str ());
  set_cmd_completer_handle_brkchars (c, print_command_completer);
  add_com_alias ("p", "print", class_vars, 1);
  add_com_alias ("inspect", "print", class_vars, 1);

  add_setshow_uinteger_cmd ("max-symbolic-offset", no_class,
			    &max_symbolic_offset, _("\
Set the largest offset that will be printed in <SYMBOL+1234> form."), _("\
Show the largest offset that will be printed in <SYMBOL+1234> form."), _("\
Tell GDB to only display the symbolic form of an address if the\n\
offset between the closest earlier symbol and the address is less than\n\
the specified maximum offset.  The default is \"unlimited\", which tells GDB\n\
to always print the symbolic form of an address if any symbol precedes\n\
it.  Zero is equivalent to \"unlimited\"."),
			    NULL,
			    show_max_symbolic_offset,
			    &setprintlist, &showprintlist);
  add_setshow_boolean_cmd ("symbol-filename", no_class,
			   &print_symbol_filename, _("\
Set printing of source filename and line number with <SYMBOL>."), _("\
Show printing of source filename and line number with <SYMBOL>."), NULL,
			   NULL,
			   show_print_symbol_filename,
			   &setprintlist, &showprintlist);

  add_com ("eval", no_class, eval_command, _("\
Construct a GDB command and then evaluate it.\n\
Usage: eval \"format string\", ARG1, ARG2, ARG3, ..., ARGN\n\
Convert the arguments to a string as \"printf\" would, but then\n\
treat this string as a command line, and evaluate it."));
}
