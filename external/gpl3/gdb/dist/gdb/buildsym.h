/* Build symbol tables in GDB's internal format.
   Copyright (C) 1986-2013 Free Software Foundation, Inc.

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

#if !defined (BUILDSYM_H)
#define BUILDSYM_H 1

struct objfile;
struct symbol;
struct addrmap;

/* This module provides definitions used for creating and adding to
   the symbol table.  These routines are called from various symbol-
   file-reading routines.

   They originated in dbxread.c of gdb-4.2, and were split out to
   make xcoffread.c more maintainable by sharing code.

   Variables declared in this file can be defined by #define-ing the
   name EXTERN to null.  It is used to declare variables that are
   normally extern, but which get defined in a single module using
   this technique.  */

struct block;
struct pending_block;

#ifndef EXTERN
#define	EXTERN extern
#endif

#define HASHSIZE 127		/* Size of things hashed via
				   hashname().  */

/* Core address of start of text of current source file.  This too
   comes from the N_SO symbol.  For Dwarf it typically comes from the
   DW_AT_low_pc attribute of a DW_TAG_compile_unit DIE.  */

EXTERN CORE_ADDR last_source_start_addr;

/* The list of sub-source-files within the current individual
   compilation.  Each file gets its own symtab with its own linetable
   and associated info, but they all share one blockvector.  */

struct subfile
  {
    struct subfile *next;
    char *name;
    char *dirname;
    struct linetable *line_vector;
    int line_vector_length;
    enum language language;
    const char *producer;
    const char *debugformat;
    struct symtab *symtab;
  };

EXTERN struct subfile *current_subfile;

/* Global variable which, when set, indicates that we are processing a
   .o file compiled with gcc */

EXTERN unsigned char processing_gcc_compilation;

/* When set, we are processing a .o file compiled by sun acc.  This is
   misnamed; it refers to all stabs-in-elf implementations which use
   N_UNDF the way Sun does, including Solaris gcc.  Hopefully all
   stabs-in-elf implementations ever invented will choose to be
   compatible.  */

EXTERN unsigned char processing_acc_compilation;

/* Count symbols as they are processed, for error messages.  */

EXTERN unsigned int symnum;

/* Record the symbols defined for each context in a list.  We don't
   create a struct block for the context until we know how long to
   make it.  */

#define PENDINGSIZE 100

struct pending
  {
    struct pending *next;
    int nsyms;
    struct symbol *symbol[PENDINGSIZE];
  };

/* Here are the three lists that symbols are put on.  */

/* static at top level, and types */

EXTERN struct pending *file_symbols;

/* global functions and variables */

EXTERN struct pending *global_symbols;

/* everything local to lexical context */

EXTERN struct pending *local_symbols;

/* "using" directives local to lexical context.  */

EXTERN struct using_direct *using_directives;

/* Stack representing unclosed lexical contexts (that will become
   blocks, eventually).  */

struct context_stack
  {
    /* Outer locals at the time we entered */

    struct pending *locals;

    /* Pending using directives at the time we entered.  */

    struct using_direct *using_directives;

    /* Pointer into blocklist as of entry */

    struct pending_block *old_blocks;

    /* Name of function, if any, defining context */

    struct symbol *name;

    /* PC where this context starts */

    CORE_ADDR start_addr;

    /* Temp slot for exception handling.  */

    CORE_ADDR end_addr;

    /* For error-checking matching push/pop */

    int depth;

  };

EXTERN struct context_stack *context_stack;

/* Index of first unused entry in context stack.  */

EXTERN int context_stack_depth;

/* Currently allocated size of context stack.  */

EXTERN int context_stack_size;

/* Non-zero if the context stack is empty.  */
#define outermost_context_p() (context_stack_depth == 0)

/* Nonzero if within a function (so symbols should be local, if
   nothing says specifically).  */

EXTERN int within_function;



struct subfile_stack
  {
    struct subfile_stack *next;
    char *name;
  };

EXTERN struct subfile_stack *subfile_stack;

#define next_symbol_text(objfile) (*next_symbol_text_func)(objfile)

/* Function to invoke get the next symbol.  Return the symbol name.  */

EXTERN char *(*next_symbol_text_func) (struct objfile *);

/* Vector of types defined so far, indexed by their type numbers.
   Used for both stabs and coff.  (In newer sun systems, dbx uses a
   pair of numbers in parens, as in "(SUBFILENUM,NUMWITHINSUBFILE)".
   Then these numbers must be translated through the type_translations
   hash table to get the index into the type vector.)  */

EXTERN struct type **type_vector;

/* Number of elements allocated for type_vector currently.  */

EXTERN int type_vector_length;

/* Initial size of type vector.  Is realloc'd larger if needed, and
   realloc'd down to the size actually used, when completed.  */

#define	INITIAL_TYPE_VECTOR_LENGTH	160

extern void add_symbol_to_list (struct symbol *symbol,
				struct pending **listhead);

extern struct symbol *find_symbol_in_list (struct pending *list,
					   char *name, int length);

extern struct block *finish_block (struct symbol *symbol,
                                   struct pending **listhead,
                                   struct pending_block *old_blocks,
                                   CORE_ADDR start, CORE_ADDR end,
                                   struct objfile *objfile);

extern void record_block_range (struct block *,
                                CORE_ADDR start, CORE_ADDR end_inclusive);

extern void really_free_pendings (void *dummy);

extern void start_subfile (const char *name, const char *dirname);

extern void patch_subfile_names (struct subfile *subfile, char *name);

extern void push_subfile (void);

extern char *pop_subfile (void);

extern struct block *end_symtab_get_static_block (CORE_ADDR end_addr,
						  struct objfile *objfile,
						  int expandable,
						  int required);

extern struct symtab *end_symtab_from_static_block (struct block *static_block,
						    struct objfile *objfile,
						    int section,
						    int expandable);

extern struct symtab *end_symtab (CORE_ADDR end_addr,
				  struct objfile *objfile, int section);

extern struct symtab *end_expandable_symtab (CORE_ADDR end_addr,
					     struct objfile *objfile,
					     int section);

extern void augment_type_symtab (struct objfile *objfile,
				 struct symtab *primary_symtab);

/* Defined in stabsread.c.  */

extern void scan_file_globals (struct objfile *objfile);

extern void buildsym_new_init (void);

extern void buildsym_init (void);

extern struct context_stack *push_context (int desc, CORE_ADDR valu);

extern struct context_stack *pop_context (void);

extern void record_line (struct subfile *subfile, int line, CORE_ADDR pc);

extern void start_symtab (const char *name, const char *dirname,
			  CORE_ADDR start_addr);

extern void restart_symtab (CORE_ADDR start_addr);

extern int hashname (const char *name);

extern void free_pending_blocks (void);

/* Record the name of the debug format in the current pending symbol
   table.  FORMAT must be a string with a lifetime at least as long as
   the symtab's objfile.  */

extern void record_debugformat (const char *format);

/* Record the name of the debuginfo producer (usually the compiler) in
   the current pending symbol table.  PRODUCER must be a string with a
   lifetime at least as long as the symtab's objfile.  */

extern void record_producer (const char *producer);

extern void merge_symbol_lists (struct pending **srclist,
				struct pending **targetlist);

/* Set the name of the last source file.  NAME is copied by this
   function.  */

extern void set_last_source_file (const char *name);

/* Fetch the name of the last source file.  */

extern const char *get_last_source_file (void);

/* The macro table for the compilation unit whose symbols we're
   currently reading.  All the symtabs for this CU will point to
   this.  */
EXTERN struct macro_table *pending_macros;

#undef EXTERN

#endif /* defined (BUILDSYM_H) */
