/* ldlex.h -
   Copyright (C) 1991-2015 Free Software Foundation, Inc.

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

#ifndef LDLEX_H
#define LDLEX_H

#include <stdio.h>

/* Codes used for the long options with no short synonyms.  150 isn't
   special; it's just an arbitrary non-ASCII char value.  */
enum option_values
{
  OPTION_ASSERT = 150,
  OPTION_CALL_SHARED,
  OPTION_CREF,
  OPTION_DEFSYM,
  OPTION_DEMANGLE,
  OPTION_DYNAMIC_LINKER,
  OPTION_NO_DYNAMIC_LINKER,
  OPTION_SYSROOT,
  OPTION_EB,
  OPTION_EL,
  OPTION_EMBEDDED_RELOCS,
  OPTION_EXPORT_DYNAMIC,
  OPTION_NO_EXPORT_DYNAMIC,
  OPTION_HELP,
  OPTION_IGNORE,
  OPTION_MAP,
  OPTION_NO_DEMANGLE,
  OPTION_NO_KEEP_MEMORY,
  OPTION_NO_WARN_MISMATCH,
  OPTION_NO_WARN_SEARCH_MISMATCH,
  OPTION_NOINHIBIT_EXEC,
  OPTION_NON_SHARED,
  OPTION_NO_WHOLE_ARCHIVE,
  OPTION_OFORMAT,
  OPTION_RELAX,
  OPTION_NO_RELAX,
  OPTION_RETAIN_SYMBOLS_FILE,
  OPTION_RPATH,
  OPTION_RPATH_LINK,
  OPTION_SHARED,
  OPTION_SONAME,
  OPTION_SORT_COMMON,
  OPTION_SORT_SECTION,
  OPTION_STATS,
  OPTION_SYMBOLIC,
  OPTION_SYMBOLIC_FUNCTIONS,
  OPTION_TASK_LINK,
  OPTION_TBSS,
  OPTION_TDATA,
  OPTION_TTEXT,
  OPTION_TTEXT_SEGMENT,
  OPTION_TRODATA_SEGMENT,
  OPTION_TLDATA_SEGMENT,
  OPTION_TRADITIONAL_FORMAT,
  OPTION_UR,
  OPTION_VERBOSE,
  OPTION_VERSION,
  OPTION_VERSION_SCRIPT,
  OPTION_VERSION_EXPORTS_SECTION,
  OPTION_DYNAMIC_LIST,
  OPTION_DYNAMIC_LIST_CPP_NEW,
  OPTION_DYNAMIC_LIST_CPP_TYPEINFO,
  OPTION_DYNAMIC_LIST_DATA,
  OPTION_WARN_COMMON,
  OPTION_WARN_CONSTRUCTORS,
  OPTION_WARN_FATAL,
  OPTION_NO_WARN_FATAL,
  OPTION_WARN_MULTIPLE_GP,
  OPTION_WARN_ONCE,
  OPTION_WARN_SECTION_ALIGN,
  OPTION_SPLIT_BY_RELOC,
  OPTION_SPLIT_BY_FILE ,
  OPTION_WHOLE_ARCHIVE,
  OPTION_ADD_DT_NEEDED_FOR_DYNAMIC,
  OPTION_NO_ADD_DT_NEEDED_FOR_DYNAMIC,
  OPTION_ADD_DT_NEEDED_FOR_REGULAR,
  OPTION_NO_ADD_DT_NEEDED_FOR_REGULAR,
  OPTION_WRAP,
  OPTION_FORCE_EXE_SUFFIX,
  OPTION_GC_SECTIONS,
  OPTION_NO_GC_SECTIONS,
  OPTION_PRINT_GC_SECTIONS,
  OPTION_NO_PRINT_GC_SECTIONS,
  OPTION_HASH_SIZE,
  OPTION_CHECK_SECTIONS,
  OPTION_NO_CHECK_SECTIONS,
  OPTION_NO_UNDEFINED,
  OPTION_INIT,
  OPTION_FINI,
  OPTION_SECTION_START,
  OPTION_UNIQUE,
  OPTION_TARGET_HELP,
  OPTION_ALLOW_SHLIB_UNDEFINED,
  OPTION_NO_ALLOW_SHLIB_UNDEFINED,
  OPTION_ALLOW_MULTIPLE_DEFINITION,
  OPTION_NO_UNDEFINED_VERSION,
  OPTION_DEFAULT_SYMVER,
  OPTION_DEFAULT_IMPORTED_SYMVER,
  OPTION_DISCARD_NONE,
  OPTION_SPARE_DYNAMIC_TAGS,
  OPTION_NO_DEFINE_COMMON,
  OPTION_NOSTDLIB,
  OPTION_NO_OMAGIC,
  OPTION_STRIP_DISCARDED,
  OPTION_NO_STRIP_DISCARDED,
  OPTION_ACCEPT_UNKNOWN_INPUT_ARCH,
  OPTION_NO_ACCEPT_UNKNOWN_INPUT_ARCH,
  OPTION_PIE,
  OPTION_UNRESOLVED_SYMBOLS,
  OPTION_WARN_UNRESOLVED_SYMBOLS,
  OPTION_ERROR_UNRESOLVED_SYMBOLS,
  OPTION_WARN_SHARED_TEXTREL,
  OPTION_WARN_ALTERNATE_EM,
  OPTION_REDUCE_MEMORY_OVERHEADS,
#ifdef ENABLE_PLUGINS
  OPTION_PLUGIN,
  OPTION_PLUGIN_OPT,
#endif /* ENABLE_PLUGINS */
  OPTION_DEFAULT_SCRIPT,
  OPTION_PRINT_OUTPUT_FORMAT,
  OPTION_PRINT_SYSROOT,
  OPTION_IGNORE_UNRESOLVED_SYMBOL,
  OPTION_PUSH_STATE,
  OPTION_POP_STATE,
  OPTION_PRINT_MEMORY_USAGE,
  OPTION_REQUIRE_DEFINED_SYMBOL,
  OPTION_ORPHAN_HANDLING,
};

/* The initial parser states.  */
typedef enum input_enum {
  input_selected,		/* We've set the initial state.  */
  input_script,
  input_mri_script,
  input_version_script,
  input_dynamic_list,
  input_defsym
} input_type;

extern input_type parser_input;

extern unsigned int lineno;
extern const char *lex_string;

/* In ldlex.l.  */
extern int yylex (void);
extern void lex_push_file (FILE *, const char *, unsigned int);
extern void lex_redirect (const char *, const char *, unsigned int);
extern void ldlex_script (void);
extern void ldlex_inputlist (void);
extern void ldlex_mri_script (void);
extern void ldlex_version_script (void);
extern void ldlex_version_file (void);
extern void ldlex_defsym (void);
extern void ldlex_expression (void);
extern void ldlex_both (void);
extern void ldlex_command (void);
extern void ldlex_popstate (void);
extern const char* ldlex_filename (void);

/* In lexsup.c.  */
extern int lex_input (void);
extern void lex_unput (int);
#ifndef yywrap
extern int yywrap (void);
#endif
extern void parse_args (unsigned, char **);

#endif
