/* Print values for GNU debugger GDB.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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
#include "dfp.h"
#include "observer.h"
#include "solist.h"
#include "parser-defs.h"
#include "charset.h"
#include "arch-utils.h"
#include "cli/cli-utils.h"
#include "format.h"
#include "source.h"

#ifdef TUI
#include "tui/tui.h"		/* For tui_active et al.   */
#endif

/* Last specified output format.  */

static char last_format = 0;

/* Last specified examination size.  'b', 'h', 'w' or `q'.  */

static char last_size = 'w';

/* Default address to examine next, and associated architecture.  */

static struct gdbarch *next_gdbarch;
static CORE_ADDR next_address;

/* Number of delay instructions following current disassembled insn.  */

static int branch_delay_insns;

/* Last address examined.  */

static CORE_ADDR last_examine_address;

/* Contents of last address examined.
   This is not valid past the end of the `x' command!  */

static struct value *last_examine_value;

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
static int print_symbol_filename = 0;
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

struct display
  {
    /* Chain link to next auto-display item.  */
    struct display *next;

    /* The expression as the user typed it.  */
    char *exp_string;

    /* Expression to be evaluated and displayed.  */
    struct expression *exp;

    /* Item number of this auto-display item.  */
    int number;

    /* Display format specified.  */
    struct format_data format;

    /* Program space associated with `block'.  */
    struct program_space *pspace;

    /* Innermost block required by this expression when evaluated.  */
    const struct block *block;

    /* Status of this display (enabled or disabled).  */
    int enabled_p;
  };

/* Chain of expressions whose values should be displayed
   automatically each time the program stops.  */

static struct display *display_chain;

static int display_number;

/* Walk the following statement or block through all displays.
   ALL_DISPLAYS_SAFE does so even if the statement deletes the current
   display.  */

#define ALL_DISPLAYS(B)				\
  for (B = display_chain; B; B = B->next)

#define ALL_DISPLAYS_SAFE(B,TMP)		\
  for (B = display_chain;			\
       B ? (TMP = B->next, 1): 0;		\
       B = TMP)

/* Prototypes for exported functions.  */

void _initialize_printcmd (void);

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

  while (*p == ' ' || *p == '\t')
    p++;
  *string_ptr = p;

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
      || TYPE_CODE (type) == TYPE_CODE_REF
      || TYPE_CODE (type) == TYPE_CODE_ARRAY
      || TYPE_CODE (type) == TYPE_CODE_STRING
      || TYPE_CODE (type) == TYPE_CODE_STRUCT
      || TYPE_CODE (type) == TYPE_CODE_UNION
      || TYPE_CODE (type) == TYPE_CODE_NAMESPACE)
    value_print (val, stream, options);
  else
    /* User specified format, so don't look to the type to tell us
       what to do.  */
    val_print_scalar_formatted (type,
				value_contents_for_printing (val),
				value_embedded_offset (val),
				val,
				options, size, stream);
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
  LONGEST val_long = 0;
  unsigned int len = TYPE_LENGTH (type);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* String printing should go through val_print_scalar_formatted.  */
  gdb_assert (options->format != 's');

  if (len > sizeof(LONGEST)
      && (TYPE_CODE (type) == TYPE_CODE_INT
	  || TYPE_CODE (type) == TYPE_CODE_ENUM))
    {
      switch (options->format)
	{
	case 'o':
	  print_octal_chars (stream, valaddr, len, byte_order);
	  return;
	case 'u':
	case 'd':
	  print_decimal_chars (stream, valaddr, len, byte_order);
	  return;
	case 't':
	  print_binary_chars (stream, valaddr, len, byte_order);
	  return;
	case 'x':
	  print_hex_chars (stream, valaddr, len, byte_order);
	  return;
	case 'c':
	  print_char_chars (stream, type, valaddr, len, byte_order);
	  return;
	default:
	  break;
	};
    }

  if (options->format != 'f')
    val_long = unpack_long (type, valaddr);

  /* If the value is a pointer, and pointers and addresses are not the
     same, then at this point, the value's length (in target bytes) is
     gdbarch_addr_bit/TARGET_CHAR_BIT, not TYPE_LENGTH (type).  */
  if (TYPE_CODE (type) == TYPE_CODE_PTR)
    len = gdbarch_addr_bit (gdbarch) / TARGET_CHAR_BIT;

  /* If we are printing it as unsigned, truncate it in case it is actually
     a negative signed value (e.g. "print/u (short)-1" should print 65535
     (if shorts are 16 bits) instead of 4294967295).  */
  if (options->format != 'd' || TYPE_UNSIGNED (type))
    {
      if (len < sizeof (LONGEST))
	val_long &= ((LONGEST) 1 << HOST_CHAR_BIT * len) - 1;
    }

  switch (options->format)
    {
    case 'x':
      if (!size)
	{
	  /* No size specified, like in print.  Print varying # of digits.  */
	  print_longest (stream, 'x', 1, val_long);
	}
      else
	switch (size)
	  {
	  case 'b':
	  case 'h':
	  case 'w':
	  case 'g':
	    print_longest (stream, size, 1, val_long);
	    break;
	  default:
	    error (_("Undefined output size \"%c\"."), size);
	  }
      break;

    case 'd':
      print_longest (stream, 'd', 1, val_long);
      break;

    case 'u':
      print_longest (stream, 'u', 0, val_long);
      break;

    case 'o':
      if (val_long)
	print_longest (stream, 'o', 1, val_long);
      else
	fprintf_filtered (stream, "0");
      break;

    case 'a':
      {
	CORE_ADDR addr = unpack_pointer (type, valaddr);

	print_address (gdbarch, addr, stream);
      }
      break;

    case 'c':
      {
	struct value_print_options opts = *options;

	opts.format = 0;
	if (TYPE_UNSIGNED (type))
	  type = builtin_type (gdbarch)->builtin_true_unsigned_char;
 	else
	  type = builtin_type (gdbarch)->builtin_true_char;

	value_print (value_from_longest (type, val_long), stream, &opts);
      }
      break;

    case 'f':
      type = float_type_from_length (type);
      print_floating (valaddr, type, stream);
      break;

    case 0:
      internal_error (__FILE__, __LINE__,
		      _("failed internal consistency check"));

    case 't':
      /* Binary; 't' stands for "two".  */
      {
	char bits[8 * (sizeof val_long) + 1];
	char buf[8 * (sizeof val_long) + 32];
	char *cp = bits;
	int width;

	if (!size)
	  width = 8 * (sizeof val_long);
	else
	  switch (size)
	    {
	    case 'b':
	      width = 8;
	      break;
	    case 'h':
	      width = 16;
	      break;
	    case 'w':
	      width = 32;
	      break;
	    case 'g':
	      width = 64;
	      break;
	    default:
	      error (_("Undefined output size \"%c\"."), size);
	    }

	bits[width] = '\0';
	while (width-- > 0)
	  {
	    bits[width] = (val_long & 1) ? '1' : '0';
	    val_long >>= 1;
	  }
	if (!size)
	  {
	    while (*cp && *cp == '0')
	      cp++;
	    if (*cp == '\0')
	      cp--;
	  }
	strncpy (buf, cp, sizeof (bits));
	fputs_filtered (buf, stream);
      }
      break;

    case 'z':
      print_hex_chars (stream, valaddr, len, byte_order);
      break;

    default:
      error (_("Undefined output format \"%c\"."), options->format);
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
			int do_demangle, char *leadin)
{
  char *name = NULL;
  char *filename = NULL;
  int unmapped = 0;
  int offset = 0;
  int line = 0;

  /* Throw away both name and filename.  */
  struct cleanup *cleanup_chain = make_cleanup (free_current_contents, &name);
  make_cleanup (free_current_contents, &filename);

  if (build_address_symbolic (gdbarch, addr, do_demangle, &name, &offset,
			      &filename, &line, &unmapped))
    {
      do_cleanups (cleanup_chain);
      return 0;
    }

  fputs_filtered (leadin, stream);
  if (unmapped)
    fputs_filtered ("<*", stream);
  else
    fputs_filtered ("<", stream);
  fputs_filtered (name, stream);
  if (offset != 0)
    fprintf_filtered (stream, "+%u", (unsigned int) offset);

  /* Append source filename and line number if desired.  Give specific
     line # of this addr, if we have it; else line # of the nearest symbol.  */
  if (print_symbol_filename && filename != NULL)
    {
      if (line != -1)
	fprintf_filtered (stream, " at %s:%d", filename, line);
      else
	fprintf_filtered (stream, " in %s", filename);
    }
  if (unmapped)
    fputs_filtered ("*>", stream);
  else
    fputs_filtered (">", stream);

  do_cleanups (cleanup_chain);
  return 1;
}

/* Given an address ADDR return all the elements needed to print the
   address in a symbolic form.  NAME can be mangled or not depending
   on DO_DEMANGLE (and also on the asm_demangle global variable,
   manipulated via ''set print asm-demangle'').  Return 0 in case of
   success, when all the info in the OUT paramters is valid.  Return 1
   otherwise.  */
int
build_address_symbolic (struct gdbarch *gdbarch,
			CORE_ADDR addr,  /* IN */
			int do_demangle, /* IN */
			char **name,     /* OUT */
			int *offset,     /* OUT */
			char **filename, /* OUT */
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

  /* First try to find the address in the symbol table, then
     in the minsyms.  Take the closest one.  */

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

      name_location = BLOCK_START (SYMBOL_BLOCK_VALUE (symbol));
      if (do_demangle || asm_demangle)
	name_temp = SYMBOL_PRINT_NAME (symbol);
      else
	name_temp = SYMBOL_LINKAGE_NAME (symbol);
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
      if (BMSYMBOL_VALUE_ADDRESS (msymbol) > name_location || symbol == NULL)
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

	  /* The msymbol is closer to the address than the symbol;
	     use the msymbol instead.  */
	  symbol = 0;
	  name_location = BMSYMBOL_VALUE_ADDRESS (msymbol);
	  if (do_demangle || asm_demangle)
	    name_temp = MSYMBOL_PRINT_NAME (msymbol.minsym);
	  else
	    name_temp = MSYMBOL_LINKAGE_NAME (msymbol.minsym);
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

  *offset = addr - name_location;

  *name = xstrdup (name_temp);

  if (print_symbol_filename)
    {
      struct symtab_and_line sal;

      sal = find_pc_sect_line (addr, section, 0);

      if (sal.symtab)
	{
	  *filename = xstrdup (symtab_to_filename_for_display (sal.symtab));
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
  fputs_filtered (paddress (gdbarch, addr), stream);
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
      fputs_filtered (paddress (gdbarch, addr), stream);
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
  VEC (CORE_ADDR) *pcs = NULL;
  struct symtab_and_line sal;
  struct cleanup *cleanup = make_cleanup (VEC_cleanup (CORE_ADDR), &pcs);

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
      VEC_truncate (CORE_ADDR, pcs, 0);
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
          VEC_safe_push (CORE_ADDR, pcs, p);
          p += gdb_insn_length (gdbarch, p);
        }

      inst_count -= VEC_length (CORE_ADDR, pcs);
      *inst_read += VEC_length (CORE_ADDR, pcs);
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
  p = VEC_length (CORE_ADDR, pcs) > 0
    ? VEC_index (CORE_ADDR, pcs, -inst_count)
    : loop_start;

  /* INST_READ includes all instruction addresses in a pc range.  Need to
     exclude the beginning part up to the address we're returning.  That
     is, exclude {0x4000} in the example above.  */
  if (inst_count < 0)
    *inst_read += inst_count;

  do_cleanups (cleanup);
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
  gdb_byte *buffer = NULL;
  struct cleanup *cleanup = NULL;
  int read_error = 0;
  int chars_read = 0;
  int chars_to_read = chunk_size;
  int chars_counted = 0;
  int count_original = count;
  CORE_ADDR string_start_addr = addr;

  gdb_assert (char_size == 1 || char_size == 2 || char_size == 4);
  buffer = (gdb_byte *) xmalloc (chars_to_read * char_size);
  cleanup = make_cleanup (xfree, buffer);
  while (count > 0 && read_error == 0)
    {
      int i;

      addr -= chars_to_read * char_size;
      chars_read = read_memory_backward (gdbarch, addr, buffer,
                                         chars_to_read * char_size);
      chars_read /= char_size;
      read_error = (chars_read == chars_to_read) ? 0 : 1;
      /* Searching for '\0' from the end of buffer in backward direction.  */
      for (i = 0; i < chars_read && count > 0 ; ++i, ++chars_counted)
        {
          int offset = (chars_to_read - i - 1) * char_size;

          if (integer_is_zero (buffer + offset, char_size)
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

  do_cleanups (cleanup);
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

	  if (last_examine_value)
	    value_free (last_examine_value);

	  /* The value to be displayed is not fetched greedily.
	     Instead, to avoid the possibility of a fetched value not
	     being used, its retrieval is delayed until the print code
	     uses it.  When examining an instruction stream, the
	     disassembler will perform its own memory fetch using just
	     the address stored in LAST_EXAMINE_VALUE.  FIXME: Should
	     the disassembler be modified so that LAST_EXAMINE_VALUE
	     is left with the byte sequence from the last complete
	     instruction fetched from memory?  */
	  last_examine_value = value_at_lazy (val_type, next_address);

	  if (last_examine_value)
	    release_value (last_examine_value);

	  print_formatted (last_examine_value, size, &opts, gdb_stdout);

	  /* Display any branch delay slots following the final insn.  */
	  if (format == 'i' && count == 1)
	    count += branch_delay_insns;
	}
      printf_filtered ("\n");
      gdb_flush (gdb_stdout);
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

/* Parse print command format string into *FMTP and update *EXPP.
   CMDNAME should name the current command.  */

void
print_command_parse_format (const char **expp, const char *cmdname,
			    struct format_data *fmtp)
{
  const char *exp = *expp;

  if (exp && *exp == '/')
    {
      exp++;
      *fmtp = decode_format (&exp, last_format, 0);
      validate_format (*fmtp, cmdname);
      last_format = fmtp->format;
    }
  else
    {
      fmtp->count = 1;
      fmtp->format = 0;
      fmtp->size = 0;
      fmtp->raw = 0;
    }

  *expp = exp;
}

/* Print VAL to console according to *FMTP, including recording it to
   the history.  */

void
print_value (struct value *val, const struct format_data *fmtp)
{
  struct value_print_options opts;
  int histindex = record_latest_value (val);

  annotate_value_history_begin (histindex, value_type (val));

  printf_filtered ("$%d = ", histindex);

  annotate_value_history_value ();

  get_formatted_print_options (&opts, fmtp->format);
  opts.raw = fmtp->raw;

  print_formatted (val, fmtp->size, &opts, gdb_stdout);
  printf_filtered ("\n");

  annotate_value_history_end ();
}

/* Evaluate string EXP as an expression in the current language and
   print the resulting value.  EXP may contain a format specifier as the
   first argument ("/x myvar" for example, to print myvar in hex).  */

static void
print_command_1 (const char *exp, int voidprint)
{
  struct expression *expr;
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);
  struct value *val;
  struct format_data fmt;

  print_command_parse_format (&exp, "print", &fmt);

  if (exp && *exp)
    {
      expr = parse_expression (exp);
      make_cleanup (free_current_contents, &expr);
      val = evaluate_expression (expr);
    }
  else
    val = access_value_history (0);

  if (voidprint || (val && value_type (val) &&
		    TYPE_CODE (value_type (val)) != TYPE_CODE_VOID))
    print_value (val, &fmt);

  do_cleanups (old_chain);
}

static void
print_command (char *exp, int from_tty)
{
  print_command_1 (exp, 1);
}

/* Same as print, except it doesn't print void results.  */
static void
call_command (char *exp, int from_tty)
{
  print_command_1 (exp, 0);
}

/* Implementation of the "output" command.  */

static void
output_command (char *exp, int from_tty)
{
  output_command_const (exp, from_tty);
}

/* Like output_command, but takes a const string as argument.  */

void
output_command_const (const char *exp, int from_tty)
{
  struct expression *expr;
  struct cleanup *old_chain;
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

  expr = parse_expression (exp);
  old_chain = make_cleanup (free_current_contents, &expr);

  val = evaluate_expression (expr);

  annotate_value_begin (value_type (val));

  get_formatted_print_options (&opts, format);
  opts.raw = fmt.raw;
  print_formatted (val, fmt.size, &opts, gdb_stdout);

  annotate_value_end ();

  wrap_here ("");
  gdb_flush (gdb_stdout);

  do_cleanups (old_chain);
}

static void
set_command (char *exp, int from_tty)
{
  struct expression *expr = parse_expression (exp);
  struct cleanup *old_chain =
    make_cleanup (free_current_contents, &expr);

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

  evaluate_expression (expr);
  do_cleanups (old_chain);
}

static void
sym_info (char *arg, int from_tty)
{
  struct minimal_symbol *msymbol;
  struct objfile *objfile;
  struct obj_section *osect;
  CORE_ADDR addr, sect_addr;
  int matches = 0;
  unsigned int offset;

  if (!arg)
    error_no_arg (_("address"));

  addr = parse_and_eval_address (arg);
  ALL_OBJSECTIONS (objfile, osect)
  {
    /* Only process each object file once, even if there's a separate
       debug file.  */
    if (objfile->separate_debug_objfile_backlink)
      continue;

    sect_addr = overlay_mapped_address (addr, osect);

    if (obj_section_addr (osect) <= sect_addr
	&& sect_addr < obj_section_endaddr (osect)
	&& (msymbol
	    = lookup_minimal_symbol_by_pc_section (sect_addr, osect).minsym))
      {
	const char *obj_name, *mapped, *sec_name, *msym_name;
	char *loc_string;
	struct cleanup *old_chain;

	matches = 1;
	offset = sect_addr - MSYMBOL_VALUE_ADDRESS (objfile, msymbol);
	mapped = section_is_mapped (osect) ? _("mapped") : _("unmapped");
	sec_name = osect->the_bfd_section->name;
	msym_name = MSYMBOL_PRINT_NAME (msymbol);

	/* Don't print the offset if it is zero.
	   We assume there's no need to handle i18n of "sym + offset".  */
	if (offset)
	  loc_string = xstrprintf ("%s + %u", msym_name, offset);
	else
	  loc_string = xstrprintf ("%s", msym_name);

	/* Use a cleanup to free loc_string in case the user quits
	   a pagination request inside printf_filtered.  */
	old_chain = make_cleanup (xfree, loc_string);

	gdb_assert (osect->objfile && objfile_name (osect->objfile));
	obj_name = objfile_name (osect->objfile);

	if (MULTI_OBJFILE_P ())
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
	      printf_filtered (_("%s in load address range of section %s\n"),
			       loc_string, sec_name);
	  else
	    if (section_is_overlay (osect))
	      printf_filtered (_("%s in %s overlay section %s\n"),
			       loc_string, mapped, sec_name);
	    else
	      printf_filtered (_("%s in section %s\n"),
			       loc_string, sec_name);

	do_cleanups (old_chain);
      }
  }
  if (matches == 0)
    printf_filtered (_("No symbol matches %s.\n"), arg);
}

static void
address_info (char *exp, int from_tty)
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

	  gdbarch = get_objfile_arch (objfile);
	  load_addr = BMSYMBOL_VALUE_ADDRESS (msymbol);

	  printf_filtered ("Symbol \"");
	  fprintf_symbol_filtered (gdb_stdout, exp,
				   current_language->la_language, DMGL_ANSI);
	  printf_filtered ("\" is at ");
	  fputs_filtered (paddress (gdbarch, load_addr), gdb_stdout);
	  printf_filtered (" in a file compiled without debugging");
	  section = MSYMBOL_OBJ_SECTION (objfile, msymbol.minsym);
	  if (section_is_overlay (section))
	    {
	      load_addr = overlay_unmapped_address (load_addr, section);
	      printf_filtered (",\n -- loaded at ");
	      fputs_filtered (paddress (gdbarch, load_addr), gdb_stdout);
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
  fprintf_symbol_filtered (gdb_stdout, SYMBOL_PRINT_NAME (sym),
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
      fputs_filtered (paddress (gdbarch, load_addr), gdb_stdout);
      if (section_is_overlay (section))
	{
	  load_addr = overlay_unmapped_address (load_addr, section);
	  printf_filtered (",\n -- loaded at ");
	  fputs_filtered (paddress (gdbarch, load_addr), gdb_stdout);
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
      fputs_filtered (paddress (gdbarch, load_addr), gdb_stdout);
      if (section_is_overlay (section))
	{
	  load_addr = overlay_unmapped_address (load_addr, section);
	  printf_filtered (_(",\n -- loaded at "));
	  fputs_filtered (paddress (gdbarch, load_addr), gdb_stdout);
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
      load_addr = BLOCK_START (SYMBOL_BLOCK_VALUE (sym));
      fputs_filtered (paddress (gdbarch, load_addr), gdb_stdout);
      if (section_is_overlay (section))
	{
	  load_addr = overlay_unmapped_address (load_addr, section);
	  printf_filtered (_(",\n -- loaded at "));
	  fputs_filtered (paddress (gdbarch, load_addr), gdb_stdout);
	  printf_filtered (_(" in overlay section %s"),
			   section->the_bfd_section->name);
	}
      break;

    case LOC_UNRESOLVED:
      {
	struct bound_minimal_symbol msym;

	msym = lookup_minimal_symbol_and_objfile (SYMBOL_LINKAGE_NAME (sym));
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
		fputs_filtered (paddress (gdbarch, load_addr), gdb_stdout);
		if (section_is_overlay (section))
		  {
		    load_addr = overlay_unmapped_address (load_addr, section);
		    printf_filtered (_(",\n -- loaded at "));
		    fputs_filtered (paddress (gdbarch, load_addr), gdb_stdout);
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
x_command (char *exp, int from_tty)
{
  struct expression *expr;
  struct format_data fmt;
  struct cleanup *old_chain;
  struct value *val;

  fmt.format = last_format ? last_format : 'x';
  fmt.size = last_size;
  fmt.count = 1;
  fmt.raw = 0;

  if (exp && *exp == '/')
    {
      const char *tmp = exp + 1;

      fmt = decode_format (&tmp, last_format, last_size);
      exp = (char *) tmp;
    }

  /* If we have an expression, evaluate it and use it as the address.  */

  if (exp != 0 && *exp != 0)
    {
      expr = parse_expression (exp);
      /* Cause expression not to be there any more if this command is
         repeated with Newline.  But don't clobber a user-defined
         command's definition.  */
      if (from_tty)
	*exp = 0;
      old_chain = make_cleanup (free_current_contents, &expr);
      val = evaluate_expression (expr);
      if (TYPE_CODE (value_type (val)) == TYPE_CODE_REF)
	val = coerce_ref (val);
      /* In rvalue contexts, such as this, functions are coerced into
         pointers to functions.  This makes "x/i main" work.  */
      if (/* last_format == 'i'  && */ 
	  TYPE_CODE (value_type (val)) == TYPE_CODE_FUNC
	   && VALUE_LVAL (val) == lval_memory)
	next_address = value_address (val);
      else
	next_address = value_as_address (val);

      next_gdbarch = expr->gdbarch;
      do_cleanups (old_chain);
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
  if (last_examine_value)
    {
      /* Make last address examined available to the user as $_.  Use
         the correct pointer type.  */
      struct type *pointer_type
	= lookup_pointer_type (value_type (last_examine_value));
      set_internalvar (lookup_internalvar ("_"),
		       value_from_pointer (pointer_type,
					   last_examine_address));

      /* Make contents of last address examined available to the user
	 as $__.  If the last value has not been fetched from memory
	 then don't fetch it now; instead mark it by voiding the $__
	 variable.  */
      if (value_lazy (last_examine_value))
	clear_internalvar (lookup_internalvar ("__"));
      else
	set_internalvar (lookup_internalvar ("__"), last_examine_value);
    }
}


/* Add an expression to the auto-display chain.
   Specify the expression.  */

static void
display_command (char *arg, int from_tty)
{
  struct format_data fmt;
  struct expression *expr;
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

  innermost_block = NULL;
  expr = parse_expression (exp);

  newobj = XNEW (struct display);

  newobj->exp_string = xstrdup (exp);
  newobj->exp = expr;
  newobj->block = innermost_block;
  newobj->pspace = current_program_space;
  newobj->number = ++display_number;
  newobj->format = fmt;
  newobj->enabled_p = 1;
  newobj->next = NULL;

  if (display_chain == NULL)
    display_chain = newobj;
  else
    {
      struct display *last;

      for (last = display_chain; last->next != NULL; last = last->next)
	;
      last->next = newobj;
    }

  if (from_tty)
    do_one_display (newobj);

  dont_repeat ();
}

static void
free_display (struct display *d)
{
  xfree (d->exp_string);
  xfree (d->exp);
  xfree (d);
}

/* Clear out the display_chain.  Done when new symtabs are loaded,
   since this invalidates the types stored in many expressions.  */

void
clear_displays (void)
{
  struct display *d;

  while ((d = display_chain) != NULL)
    {
      display_chain = d->next;
      free_display (d);
    }
}

/* Delete the auto-display DISPLAY.  */

static void
delete_display (struct display *display)
{
  struct display *d;

  gdb_assert (display != NULL);

  if (display_chain == display)
    display_chain = display->next;

  ALL_DISPLAYS (d)
    if (d->next == display)
      {
	d->next = display->next;
	break;
      }

  free_display (display);
}

/* Call FUNCTION on each of the displays whose numbers are given in
   ARGS.  DATA is passed unmodified to FUNCTION.  */

static void
map_display_numbers (char *args,
		     void (*function) (struct display *,
				       void *),
		     void *data)
{
  struct get_number_or_range_state state;
  int num;

  if (args == NULL)
    error_no_arg (_("one or more display numbers"));

  init_number_or_range (&state, args);

  while (!state.finished)
    {
      const char *p = state.string;

      num = get_number_or_range (&state);
      if (num == 0)
	warning (_("bad display number at or near '%s'"), p);
      else
	{
	  struct display *d, *tmp;

	  ALL_DISPLAYS_SAFE (d, tmp)
	    if (d->number == num)
	      break;
	  if (d == NULL)
	    printf_unfiltered (_("No display number %d.\n"), num);
	  else
	    function (d, data);
	}
    }
}

/* Callback for map_display_numbers, that deletes a display.  */

static void
do_delete_display (struct display *d, void *data)
{
  delete_display (d);
}

/* "undisplay" command.  */

static void
undisplay_command (char *args, int from_tty)
{
  if (args == NULL)
    {
      if (query (_("Delete all auto-display expressions? ")))
	clear_displays ();
      dont_repeat ();
      return;
    }

  map_display_numbers (args, do_delete_display, NULL);
  dont_repeat ();
}

/* Display a single auto-display.  
   Do nothing if the display cannot be printed in the current context,
   or if the display is disabled.  */

static void
do_one_display (struct display *d)
{
  struct cleanup *old_chain;
  int within_current_scope;

  if (d->enabled_p == 0)
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
      xfree (d->exp);
      d->exp = NULL;
      d->block = NULL;
    }

  if (d->exp == NULL)
    {

      TRY
	{
	  innermost_block = NULL;
	  d->exp = parse_expression (d->exp_string);
	  d->block = innermost_block;
	}
      CATCH (ex, RETURN_MASK_ALL)
	{
	  /* Can't re-parse the expression.  Disable this display item.  */
	  d->enabled_p = 0;
	  warning (_("Unable to display \"%s\": %s"),
		   d->exp_string, ex.message);
	  return;
	}
      END_CATCH
    }

  if (d->block)
    {
      if (d->pspace == current_program_space)
	within_current_scope = contained_in (get_selected_block (0), d->block);
      else
	within_current_scope = 0;
    }
  else
    within_current_scope = 1;
  if (!within_current_scope)
    return;

  old_chain = make_cleanup_restore_integer (&current_display_number);
  current_display_number = d->number;

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

      puts_filtered (d->exp_string);
      annotate_display_expression_end ();

      if (d->format.count != 1 || d->format.format == 'i')
	printf_filtered ("\n");
      else
	printf_filtered ("  ");

      annotate_display_value ();

      TRY
        {
	  struct value *val;
	  CORE_ADDR addr;

	  val = evaluate_expression (d->exp);
	  addr = value_as_address (val);
	  if (d->format.format == 'i')
	    addr = gdbarch_addr_bits_remove (d->exp->gdbarch, addr);
	  do_examine (d->format, d->exp->gdbarch, addr);
	}
      CATCH (ex, RETURN_MASK_ERROR)
	{
	  fprintf_filtered (gdb_stdout, _("<error: %s>\n"), ex.message);
	}
      END_CATCH
    }
  else
    {
      struct value_print_options opts;

      annotate_display_format ();

      if (d->format.format)
	printf_filtered ("/%c ", d->format.format);

      annotate_display_expression ();

      puts_filtered (d->exp_string);
      annotate_display_expression_end ();

      printf_filtered (" = ");

      annotate_display_expression ();

      get_formatted_print_options (&opts, d->format.format);
      opts.raw = d->format.raw;

      TRY
        {
	  struct value *val;

	  val = evaluate_expression (d->exp);
	  print_formatted (val, d->format.size, &opts, gdb_stdout);
	}
      CATCH (ex, RETURN_MASK_ERROR)
	{
	  fprintf_filtered (gdb_stdout, _("<error: %s>"), ex.message);
	}
      END_CATCH

      printf_filtered ("\n");
    }

  annotate_display_end ();

  gdb_flush (gdb_stdout);
  do_cleanups (old_chain);
}

/* Display all of the values on the auto-display chain which can be
   evaluated in the current scope.  */

void
do_displays (void)
{
  struct display *d;

  for (d = display_chain; d; d = d->next)
    do_one_display (d);
}

/* Delete the auto-display which we were in the process of displaying.
   This is done when there is an error or a signal.  */

void
disable_display (int num)
{
  struct display *d;

  for (d = display_chain; d; d = d->next)
    if (d->number == num)
      {
	d->enabled_p = 0;
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
display_info (char *ignore, int from_tty)
{
  struct display *d;

  if (!display_chain)
    printf_unfiltered (_("There are no auto-display expressions now.\n"));
  else
    printf_filtered (_("Auto-display expressions now in effect:\n\
Num Enb Expression\n"));

  for (d = display_chain; d; d = d->next)
    {
      printf_filtered ("%d:   %c  ", d->number, "ny"[(int) d->enabled_p]);
      if (d->format.size)
	printf_filtered ("/%d%c%c ", d->format.count, d->format.size,
			 d->format.format);
      else if (d->format.format)
	printf_filtered ("/%c ", d->format.format);
      puts_filtered (d->exp_string);
      if (d->block && !contained_in (get_selected_block (0), d->block))
	printf_filtered (_(" (cannot be evaluated in the current context)"));
      printf_filtered ("\n");
      gdb_flush (gdb_stdout);
    }
}

/* Callback fo map_display_numbers, that enables or disables the
   passed in display D.  */

static void
do_enable_disable_display (struct display *d, void *data)
{
  d->enabled_p = *(int *) data;
}

/* Implamentation of both the "disable display" and "enable display"
   commands.  ENABLE decides what to do.  */

static void
enable_disable_display_command (char *args, int from_tty, int enable)
{
  if (args == NULL)
    {
      struct display *d;

      ALL_DISPLAYS (d)
	d->enabled_p = enable;
      return;
    }

  map_display_numbers (args, do_enable_disable_display, &enable);
}

/* The "enable display" command.  */

static void
enable_display_command (char *args, int from_tty)
{
  enable_disable_display_command (args, from_tty, 1);
}

/* The "disable display" command.  */

static void
disable_display_command (char *args, int from_tty)
{
  enable_disable_display_command (args, from_tty, 0);
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
  struct display *d;
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

  for (d = display_chain; d != NULL; d = d->next)
    {
      if (d->pspace != pspace)
	continue;

      if (lookup_objfile_from_block (d->block) == objfile
	  || (d->exp && exp_uses_objfile (d->exp, objfile)))
      {
	xfree (d->exp);
	d->exp = NULL;
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
    name = SYMBOL_PRINT_NAME (var);

  fprintf_filtered (stream, "%s%s = ", n_spaces (2 * indent), name);
  TRY
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
  CATCH (except, RETURN_MASK_ERROR)
    {
      fprintf_filtered(stream, "<error reading variable %s (%s)>", name,
		       except.message);
    }
  END_CATCH

  fprintf_filtered (stream, "\n");
}

/* Subroutine of ui_printf to simplify it.
   Print VALUE to STREAM using FORMAT.
   VALUE is a C-style string on the target.  */

static void
printf_c_string (struct ui_file *stream, const char *format,
		 struct value *value)
{
  gdb_byte *str;
  CORE_ADDR tem;
  int j;

  tem = value_as_address (value);

  /* This is a %s argument.  Find the length of the string.  */
  for (j = 0;; j++)
    {
      gdb_byte c;

      QUIT;
      read_memory (tem + j, &c, 1);
      if (c == 0)
	break;
    }

  /* Copy the string contents into a string inside GDB.  */
  str = (gdb_byte *) alloca (j + 1);
  if (j != 0)
    read_memory (tem, str, j);
  str[j] = 0;

  fprintf_filtered (stream, format, (char *) str);
}

/* Subroutine of ui_printf to simplify it.
   Print VALUE to STREAM using FORMAT.
   VALUE is a wide C-style string on the target.  */

static void
printf_wide_c_string (struct ui_file *stream, const char *format,
		      struct value *value)
{
  gdb_byte *str;
  CORE_ADDR tem;
  int j;
  struct gdbarch *gdbarch = get_type_arch (value_type (value));
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct type *wctype = lookup_typename (current_language, gdbarch,
					 "wchar_t", NULL, 0);
  int wcwidth = TYPE_LENGTH (wctype);
  gdb_byte *buf = (gdb_byte *) alloca (wcwidth);
  struct obstack output;
  struct cleanup *inner_cleanup;

  tem = value_as_address (value);

  /* This is a %s argument.  Find the length of the string.  */
  for (j = 0;; j += wcwidth)
    {
      QUIT;
      read_memory (tem + j, buf, wcwidth);
      if (extract_unsigned_integer (buf, wcwidth, byte_order) == 0)
	break;
    }

  /* Copy the string contents into a string inside GDB.  */
  str = (gdb_byte *) alloca (j + wcwidth);
  if (j != 0)
    read_memory (tem, str, j);
  memset (&str[j], 0, wcwidth);

  obstack_init (&output);
  inner_cleanup = make_cleanup_obstack_free (&output);

  convert_between_encodings (target_wide_charset (gdbarch),
			     host_charset (),
			     str, j, wcwidth,
			     &output, translit_char);
  obstack_grow_str0 (&output, "");

  fprintf_filtered (stream, format, obstack_base (&output));
  do_cleanups (inner_cleanup);
}

/* Subroutine of ui_printf to simplify it.
   Print VALUE, a decimal floating point value, to STREAM using FORMAT.  */

static void
printf_decfloat (struct ui_file *stream, const char *format,
		 struct value *value)
{
  const gdb_byte *param_ptr = value_contents (value);

#if defined (PRINTF_HAS_DECFLOAT)
  /* If we have native support for Decimal floating
     printing, handle it here.  */
  fprintf_filtered (stream, format, param_ptr);
#else
  /* As a workaround until vasprintf has native support for DFP
     we convert the DFP values to string and print them using
     the %s format specifier.  */
  const char *p;

  /* Parameter data.  */
  struct type *param_type = value_type (value);
  struct gdbarch *gdbarch = get_type_arch (param_type);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* DFP output data.  */
  struct value *dfp_value = NULL;
  gdb_byte *dfp_ptr;
  int dfp_len = 16;
  gdb_byte dec[16];
  struct type *dfp_type = NULL;
  char decstr[MAX_DECIMAL_STRING];

  /* Points to the end of the string so that we can go back
     and check for DFP length modifiers.  */
  p = format + strlen (format);

  /* Look for the float/double format specifier.  */
  while (*p != 'f' && *p != 'e' && *p != 'E'
	 && *p != 'g' && *p != 'G')
    p--;

  /* Search for the '%' char and extract the size and type of
     the output decimal value based on its modifiers
     (%Hf, %Df, %DDf).  */
  while (*--p != '%')
    {
      if (*p == 'H')
	{
	  dfp_len = 4;
	  dfp_type = builtin_type (gdbarch)->builtin_decfloat;
	}
      else if (*p == 'D' && *(p - 1) == 'D')
	{
	  dfp_len = 16;
	  dfp_type = builtin_type (gdbarch)->builtin_declong;
	  p--;
	}
      else
	{
	  dfp_len = 8;
	  dfp_type = builtin_type (gdbarch)->builtin_decdouble;
	}
    }

  /* Conversion between different DFP types.  */
  if (TYPE_CODE (param_type) == TYPE_CODE_DECFLOAT)
    decimal_convert (param_ptr, TYPE_LENGTH (param_type),
		     byte_order, dec, dfp_len, byte_order);
  else
    /* If this is a non-trivial conversion, just output 0.
       A correct converted value can be displayed by explicitly
       casting to a DFP type.  */
    decimal_from_string (dec, dfp_len, byte_order, "0");

  dfp_value = value_from_decfloat (dfp_type, dec);

  dfp_ptr = (gdb_byte *) value_contents (dfp_value);

  decimal_to_string (dfp_ptr, dfp_len, byte_order, decstr);

  /* Print the DFP value.  */
  fprintf_filtered (stream, "%s", decstr);
#endif
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

  /* Copy any width.  */
  while (*p >= '0' && *p < '9')
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
      fprintf_filtered (stream, fmt, val);
    }
  else
    {
      *fmt_p++ = 's';
      *fmt_p++ = '\0';
      fprintf_filtered (stream, fmt, "(nil)");
    }
}

/* printf "printf format string" ARG to STREAM.  */

static void
ui_printf (const char *arg, struct ui_file *stream)
{
  struct format_piece *fpieces;
  const char *s = arg;
  struct value **val_args;
  int allocated_args = 20;
  struct cleanup *old_cleanups;

  val_args = XNEWVEC (struct value *, allocated_args);
  old_cleanups = make_cleanup (free_current_contents, &val_args);

  if (s == 0)
    error_no_arg (_("format-control string and values to print"));

  s = skip_spaces_const (s);

  /* A format string should follow, enveloped in double quotes.  */
  if (*s++ != '"')
    error (_("Bad format string, missing '\"'."));

  fpieces = parse_format_string (&s);

  make_cleanup (free_format_pieces_cleanup, &fpieces);

  if (*s++ != '"')
    error (_("Bad format string, non-terminated '\"'."));
  
  s = skip_spaces_const (s);

  if (*s != ',' && *s != 0)
    error (_("Invalid argument syntax"));

  if (*s == ',')
    s++;
  s = skip_spaces_const (s);

  {
    int nargs = 0;
    int nargs_wanted;
    int i, fr;
    char *current_substring;

    nargs_wanted = 0;
    for (fr = 0; fpieces[fr].string != NULL; fr++)
      if (fpieces[fr].argclass != literal_piece)
	++nargs_wanted;

    /* Now, parse all arguments and evaluate them.
       Store the VALUEs in VAL_ARGS.  */

    while (*s != '\0')
      {
	const char *s1;

	if (nargs == allocated_args)
	  val_args = (struct value **) xrealloc ((char *) val_args,
						 (allocated_args *= 2)
						 * sizeof (struct value *));
	s1 = s;
	val_args[nargs] = parse_to_comma_and_eval (&s1);

	nargs++;
	s = s1;
	if (*s == ',')
	  s++;
      }

    if (nargs != nargs_wanted)
      error (_("Wrong number of arguments for specified format-string"));

    /* Now actually print them.  */
    i = 0;
    for (fr = 0; fpieces[fr].string != NULL; fr++)
      {
	current_substring = fpieces[fr].string;
	switch (fpieces[fr].argclass)
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
	      struct type *wctype = lookup_typename (current_language, gdbarch,
						     "wchar_t", NULL, 0);
	      struct type *valtype;
	      struct obstack output;
	      struct cleanup *inner_cleanup;
	      const gdb_byte *bytes;

	      valtype = value_type (val_args[i]);
	      if (TYPE_LENGTH (valtype) != TYPE_LENGTH (wctype)
		  || TYPE_CODE (valtype) != TYPE_CODE_INT)
		error (_("expected wchar_t argument for %%lc"));

	      bytes = value_contents (val_args[i]);

	      obstack_init (&output);
	      inner_cleanup = make_cleanup_obstack_free (&output);

	      convert_between_encodings (target_wide_charset (gdbarch),
					 host_charset (),
					 bytes, TYPE_LENGTH (valtype),
					 TYPE_LENGTH (valtype),
					 &output, translit_char);
	      obstack_grow_str0 (&output, "");

	      fprintf_filtered (stream, current_substring,
                                obstack_base (&output));
	      do_cleanups (inner_cleanup);
	    }
	    break;
	  case double_arg:
	    {
	      struct type *type = value_type (val_args[i]);
	      DOUBLEST val;
	      int inv;

	      /* If format string wants a float, unchecked-convert the value
		 to floating point of the same size.  */
	      type = float_type_from_length (type);
	      val = unpack_double (type, value_contents (val_args[i]), &inv);
	      if (inv)
		error (_("Invalid floating value found in program."));

              fprintf_filtered (stream, current_substring, (double) val);
	      break;
	    }
	  case long_double_arg:
#ifdef HAVE_LONG_DOUBLE
	    {
	      struct type *type = value_type (val_args[i]);
	      DOUBLEST val;
	      int inv;

	      /* If format string wants a float, unchecked-convert the value
		 to floating point of the same size.  */
	      type = float_type_from_length (type);
	      val = unpack_double (type, value_contents (val_args[i]), &inv);
	      if (inv)
		error (_("Invalid floating value found in program."));

	      fprintf_filtered (stream, current_substring,
                                (long double) val);
	      break;
	    }
#else
	    error (_("long double not supported in printf"));
#endif
	  case long_long_arg:
#ifdef PRINTF_HAS_LONG_LONG
	    {
	      long long val = value_as_long (val_args[i]);

              fprintf_filtered (stream, current_substring, val);
	      break;
	    }
#else
	    error (_("long long not supported in printf"));
#endif
	  case int_arg:
	    {
	      int val = value_as_long (val_args[i]);

              fprintf_filtered (stream, current_substring, val);
	      break;
	    }
	  case long_arg:
	    {
	      long val = value_as_long (val_args[i]);

              fprintf_filtered (stream, current_substring, val);
	      break;
	    }
	  /* Handles decimal floating values.  */
	  case decfloat_arg:
	    printf_decfloat (stream, current_substring, val_args[i]);
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
	    fprintf_filtered (stream, current_substring, 0);
	    break;
	  default:
	    internal_error (__FILE__, __LINE__,
			    _("failed internal consistency check"));
	  }
	/* Maybe advance to the next argument.  */
	if (fpieces[fr].argclass != literal_piece)
	  ++i;
      }
  }
  do_cleanups (old_cleanups);
}

/* Implement the "printf" command.  */

static void
printf_command (char *arg, int from_tty)
{
  ui_printf (arg, gdb_stdout);
  gdb_flush (gdb_stdout);
}

/* Implement the "eval" command.  */

static void
eval_command (char *arg, int from_tty)
{
  struct ui_file *ui_out = mem_fileopen ();
  struct cleanup *cleanups = make_cleanup_ui_file_delete (ui_out);
  char *expanded;

  ui_printf (arg, ui_out);

  expanded = ui_file_xstrdup (ui_out, NULL);
  make_cleanup (xfree, expanded);

  execute_command (expanded, from_tty);

  do_cleanups (cleanups);
}

void
_initialize_printcmd (void)
{
  struct cmd_list_element *c;

  current_display_number = -1;

  observer_attach_free_objfile (clear_dangling_display_expressions);

  add_info ("address", address_info,
	    _("Describe where symbol SYM is stored."));

  add_info ("symbol", sym_info, _("\
Describe what symbol is at location ADDR.\n\
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

#if 0
  add_com ("whereis", class_vars, whereis_command,
	   _("Print line number and file of definition of variable."));
#endif

  add_info ("display", display_info, _("\
Expressions to display when program stops, with code numbers."));

  add_cmd ("undisplay", class_vars, undisplay_command, _("\
Cancel some expressions to be displayed when program stops.\n\
Arguments are the code numbers of the expressions to stop displaying.\n\
No argument means cancel all automatic-display expressions.\n\
\"delete display\" has the same effect as this command.\n\
Do \"info display\" to see current list of code numbers."),
	   &cmdlist);

  add_com ("display", class_vars, display_command, _("\
Print value of expression EXP each time the program stops.\n\
/FMT may be used before EXP as in the \"print\" command.\n\
/FMT \"i\" or \"s\" or including a size-letter is allowed,\n\
as in the \"x\" command, and then EXP is used to get the address to examine\n\
and examining is done as in the \"x\" command.\n\n\
With no argument, display all currently requested auto-display expressions.\n\
Use \"undisplay\" to cancel display requests previously made."));

  add_cmd ("display", class_vars, enable_display_command, _("\
Enable some expressions to be displayed when program stops.\n\
Arguments are the code numbers of the expressions to resume displaying.\n\
No argument means enable all automatic-display expressions.\n\
Do \"info display\" to see current list of code numbers."), &enablelist);

  add_cmd ("display", class_vars, disable_display_command, _("\
Disable some expressions to be displayed when program stops.\n\
Arguments are the code numbers of the expressions to stop displaying.\n\
No argument means disable all automatic-display expressions.\n\
Do \"info display\" to see current list of code numbers."), &disablelist);

  add_cmd ("display", class_vars, undisplay_command, _("\
Cancel some expressions to be displayed when program stops.\n\
Arguments are the code numbers of the expressions to stop displaying.\n\
No argument means cancel all automatic-display expressions.\n\
Do \"info display\" to see current list of code numbers."), &deletelist);

  add_com ("printf", class_vars, printf_command, _("\
printf \"printf format string\", arg1, arg2, arg3, ..., argn\n\
This is useful for formatted output in user-defined commands."));

  add_com ("output", class_vars, output_command, _("\
Like \"print\" but don't put in value history and don't print newline.\n\
This is useful in user-defined commands."));

  add_prefix_cmd ("set", class_vars, set_command, _("\
Evaluate expression EXP and assign result to variable VAR, using assignment\n\
syntax appropriate for the current language (VAR = EXP or VAR := EXP for\n\
example).  VAR may be a debugger \"convenience\" variable (names starting\n\
with $), a register (a few standard names starting with $), or an actual\n\
variable in the program being debugged.  EXP is any valid expression.\n\
Use \"set variable\" for variables with names identical to set subcommands.\n\
\n\
With a subcommand, this command modifies parts of the gdb environment.\n\
You can see these environment settings with the \"show\" command."),
		  &setlist, "set ", 1, &cmdlist);
  if (dbx_commands)
    add_com ("assign", class_vars, set_command, _("\
Evaluate expression EXP and assign result to variable VAR, using assignment\n\
syntax appropriate for the current language (VAR = EXP or VAR := EXP for\n\
example).  VAR may be a debugger \"convenience\" variable (names starting\n\
with $), a register (a few standard names starting with $), or an actual\n\
variable in the program being debugged.  EXP is any valid expression.\n\
Use \"set variable\" for variables with names identical to set subcommands.\n\
\nWith a subcommand, this command modifies parts of the gdb environment.\n\
You can see these environment settings with the \"show\" command."));

  /* "call" is the same as "set", but handy for dbx users to call fns.  */
  c = add_com ("call", class_vars, call_command, _("\
Call a function in the program.\n\
The argument is the function name and arguments, in the notation of the\n\
current working language.  The result is printed and saved in the value\n\
history, if it is not void."));
  set_cmd_completer (c, expression_completer);

  add_cmd ("variable", class_vars, set_command, _("\
Evaluate expression EXP and assign result to variable VAR, using assignment\n\
syntax appropriate for the current language (VAR = EXP or VAR := EXP for\n\
example).  VAR may be a debugger \"convenience\" variable (names starting\n\
with $), a register (a few standard names starting with $), or an actual\n\
variable in the program being debugged.  EXP is any valid expression.\n\
This may usually be abbreviated to simply \"set\"."),
	   &setlist);

  c = add_com ("print", class_vars, print_command, _("\
Print value of expression EXP.\n\
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
but no count or size letter (see \"x\" command)."));
  set_cmd_completer (c, expression_completer);
  add_com_alias ("p", "print", class_vars, 1);
  add_com_alias ("inspect", "print", class_vars, 1);

  add_setshow_uinteger_cmd ("max-symbolic-offset", no_class,
			    &max_symbolic_offset, _("\
Set the largest offset that will be printed in <symbol+1234> form."), _("\
Show the largest offset that will be printed in <symbol+1234> form."), _("\
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
Set printing of source filename and line number with <symbol>."), _("\
Show printing of source filename and line number with <symbol>."), NULL,
			   NULL,
			   show_print_symbol_filename,
			   &setprintlist, &showprintlist);

  add_com ("eval", no_class, eval_command, _("\
Convert \"printf format string\", arg1, arg2, arg3, ..., argn to\n\
a command line, and call it."));
}
