/* spu -- A program to make lots of random C/C++ code.
   Copyright (C) 1993, 1994, 2000 Free Software Foundation, Inc.
   Contributed by Cygnus Support.  Written by Stan Shebs.

This file is part of SPU.

SPU is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* This is a random program generator. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

char *version_string = "0.5";

/* These values are the builtin defaults, mainly useful for testing
   purposes, or if the user is uninterested in setting a value.  */

#define DEFAULT_NUM_FILES 5

#define DEFAULT_NUM_HEADER_FILES 1

#define DEFAULT_NUM_MACROS 10

#define DEFAULT_NUM_LIB_MACROS 30

#define DEFAULT_MAX_MACRO_ARGS 5

#define DEFAULT_NUM_ENUMS 10

#define DEFAULT_NUM_LIB_ENUMS 30

#define DEFAULT_NUM_ENUMERATORS 10

#define DEFAULT_NUM_STRUCTS 10

#define DEFAULT_NUM_LIB_STRUCTS 30

#define DEFAULT_NUM_FIELDS 20

#define DEFAULT_NUM_CLASSES 10

#define DEFAULT_NUM_LIB_CLASSES 30

#define DEFAULT_NUM_METHODS 20

#define DEFAULT_NUM_FUNCTIONS 100

#define DEFAULT_NUM_LIB_FUNCTIONS 300

#define DEFAULT_MAX_FUNCTION_ARGS 8

#define DEFAULT_FUNCTION_LENGTH 20

#define DEFAULT_FUNCTION_DEPTH 3

#define DEFAULT_LIB_PERCENT 10

/* Generic hash table.  */

struct hash_entry
{
  char *val;
  struct hash_entry *next;
};

struct hash_table
{
  struct hash_entry *entries[253];
  int numadds;
};

enum decl_types {
  d_nothing,
  d_macro,
  d_enum,
  d_struct,
  d_class,
  d_function
};

enum {
  t_nothing = 0,
  t_void = 1,
  t_int = 2,
  t_short = 3,
  t_char = 4,
  t_first_user = 100,
  t_char_ptr = 1000004
};

char *typenames[] = { "?", "void", "int", "short", "char" };

struct macro_desc
{
  int id;
  char *name;
  int numargs;
  char **args;
  int use;
};

struct enumerator_desc
{
  char *name;
};

struct enum_desc
{
  int id;
  char *name;
  int num_enumerators;
  struct enumerator_desc *enumerators;
  int use;
};

struct field_desc
{
  int type;
  char *name;
};

struct struct_desc
{
  int id;
  char *name;
  int numfields;
  struct field_desc *fields;
  int use;
};

/* (should add unions as type of struct) */

struct class_desc
{
  int id;
  char *name;
  int numfields;
  struct field_desc *fields;
  int nummethods;
  struct function_desc *methods;
  int use;
};

struct type_desc
{
  char *name;
};

struct arg_desc
{
  int type;
  char *name;
};

struct function_desc
{
  int id;
  char *name;
  int return_type;
  int numargs;
  struct arg_desc *args;
  struct class_desc *class;
  int use;
};

struct file_desc
{
  char *name;
};

struct decl_entry {
  enum decl_types type;
  union {
    struct macro_desc *macro_d;
    struct enum_desc *enum_d;
    struct struct_desc *struct_d;
    struct class_desc *class_d;
    struct function_desc *function_d;
  } decl;
  int seq;
  int order;
};

struct decl_table {
  int size;
  int nextentry;
  struct decl_entry *entries;
};

/* Function declarations.  */

void display_usage (void);

int hash_string (char *str);

struct hash_entry *get_hash_entry (void);

char *add_to_hash_table (char *buf, struct hash_table *table);

char *get_from_hash_table (char *buf, struct hash_table *table);

void init_xrandom (int seed);

int xrandom (int n);

int probability (int prob);

char *copy_string (char *str);

char *xmalloc (int n);

char *gen_unique_global_name (char *root, int upcase);

void gen_random_global_name (char *root, char *namebuf);

char *gen_random_local_name (int n, char **others);

void create_macros (void);

void create_macro (struct macro_desc *macrodesc);

char *gen_new_macro_name (void);

void create_enums (void);

void create_enum (struct enum_desc *enumdesc);

char *gen_random_enumerator_name (void);

void create_structs (void);

void create_struct (struct struct_desc *structdesc, int lib);

char *gen_random_field_name (int n);

void create_classes (void);

void create_class (struct class_desc *classdesc, int lib);

void create_functions (void);

void create_function (struct function_desc *fndesc, int lib);

void write_header_file (int n);

void write_lib_header_file (void);

void write_source_file (int n);

void write_lib_source_file (void);

void write_macro (FILE *fp, struct macro_desc *macrodesc);

void write_enum (FILE *fp, struct enum_desc *enumdesc);

void write_struct (FILE *fp, struct struct_desc *structdesc);

void write_class (FILE *fp, struct class_desc *classdesc);

void write_function_decl (FILE *fp, struct function_desc *fndesc);

void write_function (FILE *fp, struct function_desc *fndesc);

void write_lib_function (FILE *fp, int n);

void write_statement (FILE *fp, int depth, int max_depth);

void write_expression (FILE *fp, int rslttype, int depth, int max_depth,
		       int exclude_id);

void write_description_block (FILE *fp);

void write_makefile (void);

/* Global variables.  */

/* The possible languages. */

enum languages { knr, c, cpp, objc };

enum languages language = c;

/* Filename extensions to use with each language type.  */

char *extensions[] = { "c", "c", "cc", "m" };

/* Names for each language.  */

char *lang_names[] = { "K&R C", "standard C", "standard C++", "Objective-C" };

int num_files = DEFAULT_NUM_FILES;

int num_header_files = DEFAULT_NUM_HEADER_FILES;

char *file_base_name = "file";

int num_macros = DEFAULT_NUM_MACROS;

int num_lib_macros = DEFAULT_NUM_LIB_MACROS;

int num_enums = DEFAULT_NUM_ENUMS;

int num_lib_enums = DEFAULT_NUM_LIB_ENUMS;

int num_enumerators = DEFAULT_NUM_ENUMERATORS;

int num_structs = DEFAULT_NUM_STRUCTS;

int num_lib_structs = DEFAULT_NUM_LIB_STRUCTS;

int num_fields = DEFAULT_NUM_FIELDS;

int num_classes = DEFAULT_NUM_CLASSES;

int num_lib_classes = DEFAULT_NUM_LIB_CLASSES;

int num_methods = DEFAULT_NUM_METHODS;

int num_functions = DEFAULT_NUM_FUNCTIONS;

int num_lib_functions = DEFAULT_NUM_LIB_FUNCTIONS;

int max_function_args = DEFAULT_MAX_FUNCTION_ARGS;

int function_length = DEFAULT_FUNCTION_LENGTH;

int function_depth = DEFAULT_FUNCTION_DEPTH;

/* Percentage of library constructs that will be referenced.  */

int lib_percent = DEFAULT_LIB_PERCENT;

int randomize_order = 1;

int num_functions_per_file;

/* The amount of commenting in the source.  */

int commenting = 0;

/* Hash table for globally visible symbols.  */

struct hash_table *global_hash_table;

/* The seed for the random number generator.  */

int seed = -1;

int next_id = 1;

/* Space to record info about generated constructs.  */

struct macro_desc *macros;

struct macro_desc *lib_macros;

struct enum_desc *enums;

struct enum_desc *lib_enums;

struct struct_desc *structs;

struct struct_desc *lib_structs;

struct class_desc *classes;

struct class_desc *lib_classes;

struct function_desc *functions;

struct function_desc *lib_functions;

struct decl_table order;

struct decl_table lib_order;

int num_computer_terms;

/* Likely words to appear in names of things.  These must never be
   valid C/C++ keywords, since they may appear by themselves in some
   contexts.  */

char *computerese[] = {
  "add", "all", "alloc", "allocate", "area", "array", "at",
  "bogus", "buf", "buff", "buffer", "by", "btree",
  "ch", "chr", "clean", "cleanup", "count", "create", "cull",
  "data", "del", "delete_", "depth", "desc", "dest", "discard", "dismiss",
  "dma", "done", "dst",
  "fill", "find", "fn", "for_",
  "gc", "go", "goto_", "grok", "gronk", "group", "grovel",
  "hack", "hacked", "have", "heap",
  "in", "ind", "index", "ini", "init", "initial", "inside",
  "lab", "label", "last", "len", "length", "line", "lis", "list", "lose",
  "make", "mark", "mod", "modify", "more",
  "name", "nest", "nesting", "new_", "next", "node", "null", "num", "number",
  "part", "partial",
  "query", "queue",
  "ob", "obj", "object", "of",
  "pc", "pnt", "point", "pop", "pos", "position", "push",
  "raw", "recalc", "rect", "rectangle", "rel", "relative", "ret", "rslt",
  "remove", "reset", "rmv",
  "see", "set", "shape", "stack", "str", "string",
  "tab", "table", "tbl", "tag", "tree",
  "undel", "undo", "unmark", "use",
  "vary", "vec", "vect", "vector", "virt", "virtual_",
  "win", "wind", "window", "word",
  "zbuf",
  NULL
};

/* Return a word that commonly appears in programs. */

char *
random_computer_word (void)
{
  if (num_computer_terms == 0)
    {
      int i;

      for (i = 0; computerese[i] != NULL; ++i)
	;
      num_computer_terms = i;
    }
  return computerese[xrandom (num_computer_terms)];
}

int
main (int argc, char **argv)
{
  int i, num;
  char *arg;

  
  /* Parse all the arguments. */
  /* (should check on numeric values) */
  for (i = 1; i < argc; ++i)
    {
      arg = argv[i];
      if (strcmp(arg, "--basename") == 0)
	{
	  file_base_name = copy_string(argv[++i]);
	}
      else if (strcmp(arg, "--classes") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_classes = num;
	}
      else if (strcmp(arg, "--comments") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  commenting = num;
	}
      else if (strcmp(arg, "--enums") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_enums = num;
	}
      else if (strcmp(arg, "--enumerators") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_enumerators = num;
	}
      else if (strcmp(arg, "--fields") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_fields = num;
	}
      else if (strcmp(arg, "--files") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_files = num;
	}
      else if (strcmp(arg, "--functions") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_functions = num;
	}
      else if (strcmp(arg, "--function-length") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  function_length = num;
	}
      else if (strcmp(arg, "--function-depth") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  function_depth = num;
	}
      else if (strcmp(arg, "--header-files") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_header_files = num;
	}
      else if (strcmp(arg, "--help") == 0)
	{
	  display_usage ();
	  exit (0);
	}
      else if (strcmp(arg, "--language") == 0)
	{
	  if (strcmp (argv[i+1], "c") == 0)
	    language = c;
	  else if (strcmp (argv[i+1], "c++") == 0)
	    language = cpp;
	  else if (strcmp (argv[i+1], "knr") == 0)
	    language = knr;
	  else if (strcmp (argv[i+1], "objc") == 0)
	    language = objc;
	  ++i;
	}
      else if (strcmp(arg, "--lib-classes") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_lib_classes = num;
	}
      else if (strcmp(arg, "--lib-enums") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_lib_enums = num;
	}
      else if (strcmp(arg, "--lib-functions") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_lib_functions = num;
	}
      else if (strcmp(arg, "--lib-macros") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_lib_macros = num;
	}
      else if (strcmp(arg, "--lib-structs") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_lib_structs = num;
	}
      else if (strcmp(arg, "--macros") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_macros = num;
	}
      else if (strcmp(arg, "--methods") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_methods = num;
	}
      else if (strcmp(arg, "--seed") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  seed = num;
	}
      else if (strcmp(arg, "--structs") == 0)
	{
	  num = strtol (argv[++i], NULL, 10);
	  num_structs = num;
	}
      else if (strcmp(arg, "--version") == 0)
	{
	  fprintf (stderr, "SPU program generator version %s\n",
		   version_string);
	  exit (0);
	}
      else
	{
	  fprintf (stderr, "Usage: \"%s\" not valid, ignored\n", arg);
	  display_usage ();
	}
    }
  if (language != cpp)
    num_classes = num_lib_classes = 0;
  init_xrandom (seed);
  /* Round up the number of functions so each file gets the same
     number. */
  num_functions_per_file = (num_functions + num_files - 1) / num_files;
  num_functions = num_functions_per_file * num_files;
  /* Create the definitions of objects internally. */
  order.size =
    num_macros + num_enums + num_structs + num_classes + num_functions;
  order.nextentry = 0;
  order.entries =
    (struct decl_entry *) xmalloc (order.size * sizeof(struct decl_entry));
  lib_order.size = num_lib_macros + num_lib_enums + num_lib_structs + num_lib_classes + num_lib_functions;
  lib_order.nextentry = 0;
  lib_order.entries =
    (struct decl_entry *) xmalloc (lib_order.size * sizeof(struct decl_entry));
  create_macros ();
  create_enums ();
  create_structs ();
  create_functions ();
  create_classes ();
  /* Write out a bunch of files. */
  printf ("Writing %d header files...\n", num_header_files);
  for (i = 0; i < num_header_files; ++i)
    write_header_file (i);
  write_lib_header_file ();
  write_lib_source_file ();
  printf ("Writing %d files...\n", num_files);
  for (i = 0; i < num_files; ++i)
    write_source_file (i);
  /* Write out a makefile. */
  write_makefile ();
  /* Succeed if we actually wrote out a whole program correctly. */
  exit (0);
}

void
display_usage (void)
{
  fprintf (stderr, "Usage: spu [ ... options ... ]\n");
  fprintf (stderr, "\t--basename str (default \"%s\")\n", "file");
  fprintf (stderr, "\t--classes n (default %d)\n", DEFAULT_NUM_CLASSES);
  fprintf (stderr, "\t--comments n\n");
  fprintf (stderr, "\t--enums n (default %d)\n", DEFAULT_NUM_ENUMS);
  fprintf (stderr, "\t--enumerators n (default %d)\n", DEFAULT_NUM_ENUMERATORS);
  fprintf (stderr, "\t--fields n (default %d)\n", DEFAULT_NUM_FIELDS);
  fprintf (stderr, "\t--files n (default %d)\n", DEFAULT_NUM_FILES);
  fprintf (stderr, "\t--functions n (default %d)\n", DEFAULT_NUM_FUNCTIONS);
  fprintf (stderr, "\t--function-length n (default %d)\n", DEFAULT_FUNCTION_LENGTH);
  fprintf (stderr, "\t--function-depth n (default %d)\n", DEFAULT_FUNCTION_DEPTH);
  fprintf (stderr, "\t--header-files n (default %d)\n", DEFAULT_NUM_HEADER_FILES);
  fprintf (stderr, "\t--help\n");
  fprintf (stderr, "\t--language c|cpp|knr|objc (default c)\n");
  fprintf (stderr, "\t--lib-classes n (default %d)\n", DEFAULT_NUM_LIB_CLASSES);
  fprintf (stderr, "\t--lib-enums n (default %d)\n", DEFAULT_NUM_LIB_ENUMS);
  fprintf (stderr, "\t--lib-functions n (default %d)\n", DEFAULT_NUM_LIB_FUNCTIONS);
  fprintf (stderr, "\t--lib-macros n (default %d)\n", DEFAULT_NUM_LIB_MACROS);
  fprintf (stderr, "\t--lib-structs n (default %d)\n", DEFAULT_NUM_LIB_STRUCTS);
  fprintf (stderr, "\t--macros n (default %d)\n", DEFAULT_NUM_MACROS);
  fprintf (stderr, "\t--methods n (default %d)\n", DEFAULT_NUM_METHODS);
  fprintf (stderr, "\t--seed n\n");
  fprintf (stderr, "\t--structs n (default %d)\n", DEFAULT_NUM_STRUCTS);
  fprintf (stderr, "\t--version\n");
}

int
random_type (int libonly)
{
  int i, n;

  switch (xrandom (6))
    {
    case 0:
      return t_short;
    case 1:
      return t_char;
    case 2:
      return t_char_ptr;
    case 3:
      for (i = 0; i < 1000; ++i)
	{
	  n = xrandom (num_lib_structs);
	  if (lib_structs[n].id > 0 && lib_structs[n].use)
	    return t_first_user + lib_structs[n].id;
	}
      return t_int;
    case 4:
      if (libonly)
	return t_int;
      for (i = 0; i < 1000; ++i)
	{
	  n = xrandom (num_structs);
	  if (structs[n].id > 0)
	    return t_first_user + structs[n].id;
	}
      return t_int;
    default:
      return t_int;
    }
}

/* Given a numbered type, return its name.  */

char *
name_from_type (int n)
{
  int i;
  char tmpbuf[100];

  if (n < t_first_user)
    {
      return typenames[n];
    }
  else if (n >= 1000000 && (n - 1000000) < t_first_user)
    {
      sprintf (tmpbuf, "%s *", typenames[n - 1000000]);
      return copy_string (tmpbuf);
    }
  else
    {
      for (i = 0; i < num_structs; ++i)
	{
	  if (structs[i].id == (n - t_first_user))
	    {
	      sprintf (tmpbuf, "struct %s *", structs[i].name);
	      return copy_string (tmpbuf);
	    }
	}
      for (i = 0; i < num_lib_structs; ++i)
	{
	  if (lib_structs[i].id == (n - t_first_user))
	    {
	      sprintf (tmpbuf, "struct %s *", lib_structs[i].name);
	      return copy_string (tmpbuf);
	    }
	}
    }
  return "?type?";
}

void
add_decl_to_table (enum decl_types type, void *desc, struct decl_table *table)
{
  int n = table->nextentry++;

  table->entries[n].type = type;
  switch (type)
    {
    case d_macro:
      table->entries[n].decl.macro_d = (struct macro_desc *) desc;
      break;
    case d_enum:
      table->entries[n].decl.enum_d = (struct enum_desc *) desc;
      break;
    case d_struct:
      table->entries[n].decl.struct_d = (struct struct_desc *) desc;
      break;
    case d_class:
      table->entries[n].decl.class_d = (struct class_desc *) desc;
      break;
    case d_function:
      table->entries[n].decl.function_d = (struct function_desc *) desc;
      break;
    default:
      fprintf (stderr, "Unknown decl type %d in add_decl_to_table\n", type);
      break;
    }
  table->entries[n].seq = n;
  if (randomize_order)
    table->entries[n].order = xrandom (10000);
}

/* Create basic definitions of macros.  Negative number of arguments
   means a macros that looks like a constant instead of a function.  */

void
create_macros (void)
{
  int i;

  printf ("Creating %d macros...\n", num_macros);
  macros =
    (struct macro_desc *) xmalloc (num_macros * sizeof(struct macro_desc));
  for (i = 0; i < num_macros; ++i)
    {
      create_macro (&(macros[i]));
      add_decl_to_table (d_macro, &(macros[i]), &order);
    }

  /* It's important to create library macros second, so that their ids
     are higher than those for program macros.  This is so that
     library macro bodies are excluded from referencing any program
     macros (because of the exclude_id bits in write_expression).  */

  printf ("Creating %d library macros...\n", num_lib_macros);
  lib_macros =
    (struct macro_desc *) xmalloc (num_lib_macros * sizeof(struct macro_desc));
  for (i = 0; i < num_lib_macros; ++i)
    {
      create_macro (&(lib_macros[i]));
      if (!probability(lib_percent))
	lib_macros[i].use = 0;
      add_decl_to_table (d_macro, &(lib_macros[i]), &lib_order);
    }
}

void
create_macro (struct macro_desc *macrodesc)
{
  int j, numargs;

  macrodesc->id = next_id++;
  macrodesc->name = gen_new_macro_name ();
  numargs = xrandom (DEFAULT_MAX_MACRO_ARGS + 1);
  --numargs;
  if (numargs > 0)
    {
      macrodesc->args = (char **) xmalloc (numargs * sizeof(char *));
      for (j = 0; j < numargs; ++j)
	{
	  macrodesc->args[j] = gen_random_local_name (j, NULL);
	}
    }
  macrodesc->numargs = numargs;
  macrodesc->use = 1;
}

/* Generate a macro name.  */

char *
gen_new_macro_name (void)
{
  return gen_unique_global_name ("M", (xrandom (3) > 0));
}

/* Create definitions of the desired number of enums.  */

void
create_enums (void)
{
  int i;

  printf ("Creating %d enums...\n", num_enums);
  enums =
    (struct enum_desc *) xmalloc (num_enums * sizeof(struct enum_desc));
  for (i = 0; i < num_enums; ++i)
    {
      create_enum (&(enums[i]));
      add_decl_to_table (d_enum, &(enums[i]), &order);
    }
  printf ("Creating %d library enums...\n", num_lib_enums);
  lib_enums =
    (struct enum_desc *) xmalloc (num_lib_enums * sizeof(struct enum_desc));
  for (i = 0; i < num_lib_enums; ++i)
    {
      create_enum (&(lib_enums[i]));
      if (!probability(lib_percent))
	lib_enums[i].use = 0;
      add_decl_to_table (d_enum, &(lib_enums[i]), &lib_order);
    }
}

void
create_enum (struct enum_desc *enumdesc)
{
  int j, num;

  enumdesc->id = next_id++;
  /* Let some enums be anonymous. */
  if (xrandom (100) < 50)
    enumdesc->name = gen_unique_global_name (NULL, 0);
  num = num_enumerators / 2 + xrandom (num_enumerators);
  if (num <= 0)
    num = 1;
  enumdesc->enumerators =
    (struct enumerator_desc *) xmalloc (num * sizeof(struct enumerator_desc));
  for (j = 0; j < num; ++j)
    {
      enumdesc->enumerators[j].name = gen_random_enumerator_name ();
    }
  enumdesc->num_enumerators = j;
  enumdesc->use = 1;
}

/* Generate a unique enumerator within an enum.  */

char *
gen_random_enumerator_name (void)
{
  return gen_unique_global_name ("enum", 0);
}

/* Create definitions of the desired number of structures.  */

void
create_structs (void)
{
  int i;

  /* Do the library structs first, so that program structs may use
     them in their definitions.  */
  printf ("Creating %d library structs...\n", num_lib_structs);
  lib_structs =
    (struct struct_desc *) xmalloc (num_lib_structs * sizeof(struct struct_desc));
  for (i = 0; i < num_lib_structs; ++i)
    {
      create_struct (&(lib_structs[i]), 1);
      if (!probability(lib_percent))
	lib_structs[i].use = 0;
      add_decl_to_table (d_struct, &(lib_structs[i]), &lib_order);
    }

  printf ("Creating %d structs...\n", num_structs);
  structs =
    (struct struct_desc *) xmalloc (num_structs * sizeof(struct struct_desc));
  for (i = 0; i < num_structs; ++i)
    {
      create_struct (&(structs[i]), 0);
      add_decl_to_table (d_struct, &(structs[i]), &order);
    }
}

void
create_struct (struct struct_desc *structdesc, int lib)
{
  int j, numf;

  structdesc->id = next_id++;
  structdesc->name = gen_unique_global_name (NULL, 0);
  numf = xrandom (num_fields) + 1;
  structdesc->fields =
    (struct field_desc *) xmalloc (numf * sizeof(struct field_desc));
  for (j = 0; j < numf; ++j)
    {
      structdesc->fields[j].type = random_type (lib);
      structdesc->fields[j].name = gen_random_field_name (j);
    }
  structdesc->numfields = numf;
  structdesc->use = 1;
}

char *
gen_random_field_name (int n)
{
  char namebuf[100];

  /* (should have more variety) */
  sprintf (namebuf, "field%d", n);
  return copy_string (namebuf);
}

/* Create definitions of the desired number of classures.  */

void
create_classes (void)
{
  int i;

  /* Do the library classes first, so that program classes may use
     them in their definitions.  */
  printf ("Creating %d library classes...\n", num_lib_classes);
  lib_classes =
    (struct class_desc *) xmalloc (num_lib_classes * sizeof(struct class_desc));
  for (i = 0; i < num_lib_classes; ++i)
    {
      create_class (&(lib_classes[i]), 1);
      if (!probability(lib_percent))
	lib_classes[i].use = 0;
      add_decl_to_table (d_class, &(lib_classes[i]), &lib_order);
    }

  printf ("Creating %d classes...\n", num_classes);
  classes =
    (struct class_desc *) xmalloc (num_classes * sizeof(struct class_desc));
  for (i = 0; i < num_classes; ++i)
    {
      create_class (&(classes[i]), 0);
      add_decl_to_table (d_class, &(classes[i]), &order);
    }
}

void
create_class (struct class_desc *classdesc, int lib)
{
  int j, numf, numm;

  classdesc->id = next_id++;
  classdesc->name = gen_unique_global_name (NULL, 0);
  numf = xrandom (num_fields) + 1;
  classdesc->fields =
    (struct field_desc *) xmalloc (numf * sizeof(struct field_desc));
  for (j = 0; j < numf; ++j)
    {
      classdesc->fields[j].type = random_type (lib);
      classdesc->fields[j].name = gen_random_field_name (j);
    }
  classdesc->numfields = numf;
  numm = xrandom (num_methods + 1);
  classdesc->methods =
    (struct function_desc *) xmalloc (numm * sizeof(struct function_desc));
  for (j = 0; j < numm; ++j)
    {
      create_function (&(classdesc->methods[j]), lib);
      classdesc->methods[j].class = classdesc;
    }
  classdesc->nummethods = numm;
  classdesc->use = 1;
}

/* Create a number of functions with random numbers and types of
   arguments. */

void
create_functions (void)
{
  int i;

  printf ("Creating %d functions...\n", num_functions);
  functions =
    (struct function_desc *) xmalloc (num_functions * sizeof(struct function_desc));

  /* Generate the main program, as the first function.  */
  functions[0].id = next_id++;
  functions[0].name = "main";
  functions[0].return_type = t_int;
  functions[0].numargs = 0;
  functions[0].use = 1;
  add_decl_to_table (d_function, &(functions[0]), &order);

  /* Generate all the other functions.  */
  for (i = 1; i < num_functions; ++i)
    {
      create_function (&(functions[i]), 0);
      add_decl_to_table (d_function, &(functions[i]), &order);
#if 0 /* use forward decls for now instead */
      {
	int j, type;
	struct function_desc *fndesc = &(functions[i]);
      
	for (j = 0; j < fndesc->numargs; ++j)
	  {
	    type = fndesc->args[i].type;
	    if (type >= t_first_user)
	      {
		/* (should find arg types and increase fndesc->order) */
	      }
	  }
      }
#endif
    }

  printf ("Creating %d library functions...\n", num_lib_functions);
  lib_functions =
    (struct function_desc *) xmalloc (num_lib_functions * sizeof(struct function_desc));
  for (i = 0; i < num_lib_functions; ++i)
    {
      create_function (&(lib_functions[i]), 1);
      /* Mark some functions as not to be referenced from the program.  */
      if (!probability(lib_percent))
	lib_functions[i].use = 0;
      add_decl_to_table (d_function, &(lib_functions[i]), &lib_order);
    }
}

/* Generate the details of a single function.  */

void
create_function (struct function_desc *fndesc, int lib)
{
  int j, range, numargs;

  fndesc->id = next_id++;
  fndesc->name = gen_unique_global_name ("fn", 0);
  fndesc->return_type = ((xrandom (4) == 0) ? t_void : random_type (lib));
  /* Choose the number of arguments, preferring shorter argument lists
     by using a simple binomial distribution that is "folded" in the
     middle so zero-arg functions are the most common.  */
  range = 2 * (max_function_args + 1);
  numargs = 0;
  for (j = 0; j < 6; ++j)
    numargs += xrandom (range + 1);
  if (j > 0)
    numargs /= j;
  /* Shift distribution so 0 is in the middle.  */
  numargs -= max_function_args;
  /* Fold negative values over to positive side.  */
  if (numargs < 0)
    numargs = -numargs;
  if (numargs > max_function_args)
    numargs = max_function_args;
  if (numargs > 0)
    {
      fndesc->args =
	(struct arg_desc *) xmalloc (numargs * sizeof(struct arg_desc));
      for (j = 0; j < numargs; ++j)
	{
	  fndesc->args[j].type = random_type (lib);
	  fndesc->args[j].name = gen_random_local_name (j, NULL);
	}
    }
  fndesc->numargs = numargs;
  fndesc->class = NULL;
  fndesc->use = 1;
}

int
compare_entries(const void *x1, const void *x2)
{
  struct decl_entry *e1 = (struct decl_entry *) x1;
  struct decl_entry *e2 = (struct decl_entry *) x2;

  if (e1->order != e2->order)
    return (e1->order - e2->order);
  /* Randomized order may have pairs of matching numbers, so use this
     as fallback.  */
  return (e1->seq - e2->seq);
}

void
write_header_file (int n)
{
  int i;
  char tmpbuf[100];
  FILE *fp;

  sprintf (tmpbuf, "%s%d.h", file_base_name, n);
  fp = fopen (tmpbuf, "w");
  if (fp == NULL)
    return;
  write_description_block (fp);
  if (commenting > 0)
    fprintf (fp, "/* header */\n");
  /* Ensure that structure decls exist before functions mentioning them.  */
  if (randomize_order)
    {
      fprintf (fp, "/* forward decls */\n");
      for (i = 0; i < num_structs; ++i)
	fprintf (fp, "struct %s;\n", structs[i].name);
      for (i = 0; i < num_classes; ++i)
	fprintf (fp, "class %s;\n", classes[i].name);
      fprintf (fp, "\n");
    }
  qsort(order.entries, order.size, sizeof(struct decl_entry),
	compare_entries);
  for (i = 0; i < order.size; ++i)
    {
      switch (order.entries[i].type)
	{
	case d_macro:
	  write_macro (fp, order.entries[i].decl.macro_d);
	  break;
	case d_enum:
	  write_enum (fp, order.entries[i].decl.enum_d);
	  break;
	case d_struct:
	  write_struct (fp, order.entries[i].decl.struct_d);
	  break;
	case d_class:
	  write_class (fp, order.entries[i].decl.class_d);
	  break;
	case d_function:
	  write_function_decl (fp, order.entries[i].decl.function_d);
	  break;
	default:
	  fprintf (stderr, "Unknown decl type %d in write_header_file\n",
		   order.entries[i].type);
	  break;
	}
    }
  fclose (fp);
}

void
write_lib_header_file (void)
{
  int i;
  char tmpbuf[100];
  FILE *fp;

  sprintf (tmpbuf, "%slib.h", file_base_name);
  fp = fopen (tmpbuf, "w");
  if (fp == NULL)
    return;
  if (commenting > 0)
    fprintf (fp, "/* library header */\n");
  write_description_block (fp);
  /* Ensure that structure decls exist before functions mentioning them.  */
  if (randomize_order)
    {
      fprintf (fp, "/* forward decls */\n");
      for (i = 0; i < num_lib_structs; ++i)
	fprintf (fp, "struct %s;\n", lib_structs[i].name);
      for (i = 0; i < num_lib_classes; ++i)
	fprintf (fp, "class %s;\n", lib_classes[i].name);
      fprintf (fp, "\n");
    }
  qsort(lib_order.entries, lib_order.size, sizeof(struct decl_entry),
	compare_entries);
  for (i = 0; i < lib_order.size; ++i)
    {
      switch (lib_order.entries[i].type)
	{
	case d_macro:
	  write_macro (fp, lib_order.entries[i].decl.macro_d);
	  break;
	case d_enum:
	  write_enum (fp, lib_order.entries[i].decl.enum_d);
	  break;
	case d_struct:
	  write_struct (fp, lib_order.entries[i].decl.struct_d);
	  break;
	case d_class:
	  write_class (fp, lib_order.entries[i].decl.class_d);
	  break;
	case d_function:
	  write_function_decl (fp, lib_order.entries[i].decl.function_d);
	  break;
	default:
	  fprintf (stderr, "Unknown decl type %d in write_header_file\n",
		   lib_order.entries[i].type);
	  break;
	}
    }
  fclose (fp);
}

void
write_macro (FILE *fp, struct macro_desc *macrodesc)
{
  int j;

  fprintf (fp, "\n#define %s", macrodesc->name);
  /* Negative # arguments indicates an argumentless macro instead of
     one with zero arguments. */
  if (macrodesc->numargs >= 0)
    {
      fprintf (fp, "(");
      for (j = 0; j < macrodesc->numargs; ++j)
	{
	  if (j > 0)
	    fprintf (fp, ",");
	  fprintf (fp, "%s", macrodesc->args[j]);
	}
      fprintf (fp, ")");
    }
  /* Generate a macro body. */
  fprintf (fp, " (");
  switch (xrandom(4))
    {
    case 0:
      write_expression (fp, t_int, 0, 2, macrodesc->id);
      break;
    case 1:
      /* A very common expansion for macros.  */
      fprintf (fp, "0");
      break;
    case 2:
      /* Likewise.  */
      fprintf (fp, "1");
      break;
    default:
      fprintf (fp, "%d", xrandom (100));
      break;
    }
  fprintf (fp, ")");
  fprintf (fp, "\n\n");
}

/* Write out the definition of a enum. */

void
write_enum (FILE *fp, struct enum_desc *enumdesc)
{
  int j;

  fprintf (fp, "\nenum");
  if (enumdesc->name)
    fprintf (fp, " %s", enumdesc->name);
  fprintf (fp, " {");
  for (j = 0; j < enumdesc->num_enumerators; ++j)
    {
      if (j > 0)
	fprintf (fp, ",");
      fprintf (fp, "\n  %s", enumdesc->enumerators[j].name);
    }
  fprintf (fp, "\n};\n\n");
}

/* Write out the definition of a structure. */

void
write_struct (FILE *fp, struct struct_desc *structdesc)
{
  int j;

  fprintf (fp, "\nstruct %s {\n", structdesc->name);
  for (j = 0; j < structdesc->numfields; ++j)
    {
      fprintf (fp, "  %s %s;\n",
	       name_from_type (structdesc->fields[j].type),
	       structdesc->fields[j].name);
    }
  fprintf (fp, "};\n\n");
}

/* Write out the definition of a classure. */

void
write_class (FILE *fp, struct class_desc *classdesc)
{
  int j;

  fprintf (fp, "\nclass %s {\n", classdesc->name);
  fprintf (fp, "public:\n");
  for (j = 0; j < classdesc->numfields; ++j)
    {
      fprintf (fp, "  %s %s;\n",
	       name_from_type (classdesc->fields[j].type),
	       classdesc->fields[j].name);
    }
  for (j = 0; j < classdesc->nummethods; ++j)
    {
      write_function (fp, &(classdesc->methods[j]));
    }
  fprintf (fp, "};\n\n");
}

void
write_function_decl (FILE *fp, struct function_desc *fndesc)
{
  int i;

  fprintf (fp, "extern %s %s (",
	   name_from_type (fndesc->return_type), fndesc->name);
  if (language != knr)
    {
      for (i = 0; i < fndesc->numargs; ++i)
	{
	  fprintf (fp, "%s %s",
		   name_from_type (fndesc->args[i].type),
		   fndesc->args[i].name);
	  if (i + 1 < fndesc->numargs)
	    fprintf (fp, ", ");
	}
    }
  fprintf (fp, ");\n");
}

/* Write a complete source file. */

void
write_source_file (int n)
{
  char tmpbuf[100];
  int j;
  FILE *fp;

  sprintf (tmpbuf, "%s%d.%s",
	   file_base_name, n, extensions[language]);
  fp = fopen (tmpbuf, "w");
  if (fp == NULL)
    return;
  write_description_block (fp);
  if (1 /*num_lib_header_files*/ > 0)
    {
      for (j = 0; j < 1 /*num_header_files*/; ++j)
	{
	  fprintf (fp, "#include \"%slib.h\"\n", file_base_name);
	}
      fprintf (fp, "\n");
    }
  if (num_header_files > 0)
    {
      for (j = 0; j < num_header_files; ++j)
	{
	  fprintf (fp, "#include \"%s%d.h\"\n", file_base_name, j);
	}
      fprintf (fp, "\n");
    }

  if (n == 0)
    printf ("  (Each file contains %d functions)\n",
	    num_functions_per_file);

  for (j = 0; j < num_functions_per_file; ++j)
    {
      write_function (fp, &(functions[n * num_functions_per_file + j]));
    }
  fclose (fp);
}

/* (should add option to define methods separately) */

void
write_function (FILE *fp, struct function_desc *fndesc)
{
  int i, k;

  if (fndesc->class)
    fprintf (fp, "  ");
  fprintf (fp, "%s", name_from_type (fndesc->return_type));
  fprintf (fp, (fndesc->class ? " " : "\n"));
  fprintf (fp, "%s (", fndesc->name);
  for (i = 0; i < fndesc->numargs; ++i)
    {
      if (language != knr)
	{
	  fprintf (fp, "%s ", name_from_type (fndesc->args[i].type));
	}
      fprintf (fp, "%s", fndesc->args[i].name);
      if (i + 1 < fndesc->numargs)
	fprintf (fp, ", ");
    }
  fprintf(fp, ")");
  fprintf (fp, (fndesc->class ? " " : "\n"));
  if (language == knr)
    {
      for (i = 0; i < fndesc->numargs; ++i)
	{
	  fprintf (fp, "%s %s;\n",
		   name_from_type (fndesc->args[i].type),
		   fndesc->args[i].name);
	}
    }
  fprintf(fp, "{");
  fprintf (fp, (fndesc->class ? " " : "\n"));
  if (fndesc->class)
    {
      /* (should generate something for the method sometimes) */
    }
  else
    {
      /* Generate a plausible function body by writing a number of
	 statements. */
      for (k = 0; k < function_length; ++k)
	{
	  write_statement (fp, 1, function_depth - 1 + xrandom (3));
	}
    }
  /* Write a return statement if appropriate.  */
  if (fndesc->return_type != t_void)
    {
      fprintf (fp, "  return 0;");
      fprintf (fp, (fndesc->class ? " " : "\n"));
    }
  fprintf (fp, "}");
  if (fndesc->class)
    fprintf (fp, ";");
  fprintf (fp, "\n");
}

/* Write "library source", which really just means empty function
   bodies, done so the program will link.  */

void
write_lib_source_file (void)
{
  char tmpbuf[100];
  int j;
  FILE *fp;

  sprintf (tmpbuf, "%slib.%s", file_base_name, extensions[language]);
  fp = fopen (tmpbuf, "w");
  if (fp == NULL)
    return;
  write_description_block (fp);
  if (1 /*num_lib_header_files*/ > 0)
    {
      for (j = 0; j < 1 /*num_lib_header_files*/; ++j)
	{
	  fprintf (fp, "#include \"%slib.h\"\n", file_base_name);
	}
      fprintf (fp, "\n");
    }

  for (j = 0; j < num_lib_functions; ++j)
    {
      write_lib_function (fp, j);
    }
  fclose (fp);
}

/* Generate empty bodies for library function definitions.  */

void
write_lib_function (FILE *fp, int n)
{
  int i;

  fprintf (fp, "%s\n%s (",
	   name_from_type (lib_functions[n].return_type),
	   lib_functions[n].name);
  for (i = 0; i < lib_functions[n].numargs; ++i)
    {
      if (language != knr)
	{
	  fprintf (fp, "%s ", name_from_type (lib_functions[n].args[i].type));
	}
      fprintf (fp, "%s", lib_functions[n].args[i].name);
      if (i + 1 < lib_functions[n].numargs)
	fprintf (fp, ", ");
    }
  fprintf (fp, ")");
  if (!lib_functions[n].use)
    fprintf (fp, " /* unused */");
  fprintf (fp, "\n");
  if (language == knr)
    {
      for (i = 0; i < lib_functions[n].numargs; ++i)
	{
	  fprintf (fp, "%s %s;\n",
		   name_from_type (lib_functions[n].args[i].type),
		   lib_functions[n].args[i].name);
	}
    }
  fprintf (fp, "{\n");
  if (lib_functions[n].return_type != t_void)
    fprintf (fp, "  return 0;\n");
  fprintf (fp, "}\n\n");
}

void
write_statement (FILE *fp, int depth, int max_depth)
{
  int i;

  for (i = 0; i < depth; ++i)
    fprintf (fp, "  ");
  /* Always do non-recursive statements if going too deep. */
  if (depth >= max_depth)
    {
      write_expression (fp, t_void, 0, xrandom(4) + 1, 0);
      fprintf (fp, ";\n");
      return;
    }
  switch (xrandom(2))
    {
    case 0:
      write_expression (fp, t_void, 0, xrandom(4) + 1, 0);
      fprintf (fp, ";");
      break;
    case 1:
      fprintf (fp, "if (");
      write_expression (fp, t_int, 0, xrandom(2) + 1, 0);
      fprintf (fp, ") {\n");
      write_statement(fp, depth + 1, max_depth);
      for (i = 0; i < depth; ++i)
	fprintf (fp, "  ");
      fprintf (fp, "}");
      break;
    }
  fprintf(fp, "\n");
}

/* Write a single expression. */

void cast_integer_type (FILE *fp, int type);
struct function_desc *find_function (int rslttype,
				     struct function_desc *fns, int numfns);
struct macro_desc *find_macro (int rslttype,
			       struct macro_desc *macs, int nummacros,
			       int exclude_id);

void
write_expression (FILE *fp, int rslttype, int depth, int max_depth,
		  int exclude_id)
{
  int n, n2, j;
  struct macro_desc *macrodesc;
  struct function_desc *fndesc;

  /* Always do non-recursive statements if going too deep. */
  if (depth >= max_depth)
    {
      switch (xrandom(10))
	{
	case 7:
	  cast_integer_type (fp, rslttype);
	  fprintf (fp, "%d", xrandom (1000));
	  break;
	default:
	  cast_integer_type (fp, rslttype);
	  fprintf (fp, "%d", xrandom (127));
	  break;
	}
      return;
    }
  switch (xrandom(10))
    {
    case 0:
      fndesc = find_function (rslttype, lib_functions, num_lib_functions);
      if (fndesc == NULL)
	{
	  cast_integer_type (fp, rslttype);
	  fprintf (fp, "%d", xrandom (100));
	  return;
	}
      fprintf (fp, "%s (", fndesc->name);
      for (j = 0; j < fndesc->numargs; ++j)
	{
	  if (j > 0)
	    fprintf (fp, ", ");
	  write_expression (fp, fndesc->args[j].type, depth + 1,
			    max_depth - 1, exclude_id);
	}
      fprintf(fp, ")");
      break;
    case 7:
      fndesc = find_function (rslttype, functions, num_functions);
      if (fndesc == NULL)
	{
	  cast_integer_type (fp, rslttype);
	  fprintf (fp, "%d", xrandom (100));
	  return;
	}
      fprintf (fp, "%s (", fndesc->name);
      for (j = 0; j < fndesc->numargs; ++j)
	{
	  if (j > 0)
	    fprintf (fp, ", ");
	  write_expression (fp, fndesc->args[j].type, depth + 1,
			    max_depth - 1, exclude_id);
	}
      fprintf(fp, ")");
      break;
    case 1:
    case 6:
      macrodesc = find_macro (t_int, lib_macros, num_lib_macros, exclude_id);
      if (macrodesc == NULL)
	{
	  cast_integer_type (fp, rslttype);
	  fprintf (fp, "%d", xrandom (100));
	  return;
	}
      cast_integer_type (fp, rslttype);
      fprintf (fp, "%s", macrodesc->name);
      if (macrodesc->numargs >= 0)
	{
	  fprintf (fp, " (");
	  for (j = 0; j < macrodesc->numargs; ++j)
	    {
	      if (j > 0)
		fprintf (fp, ", ");
	      write_expression (fp, t_int, depth + 1, max_depth - 1,
				exclude_id);
	    }
	  fprintf (fp, ")");
	}
      break;
    case 8:
      macrodesc = find_macro (t_int, macros, num_macros, exclude_id);
      if (macrodesc == NULL)
	{
	  cast_integer_type (fp, rslttype);
	  fprintf (fp, "%d", xrandom (100));
	  return;
	}
      cast_integer_type (fp, rslttype);
      fprintf (fp, "%s", macrodesc->name);
      if (macrodesc->numargs >= 0)
	{
	  fprintf (fp, " (");
	  for (j = 0; j < macrodesc->numargs; ++j)
	    {
	      if (j > 0)
		fprintf (fp, ", ");
	      write_expression (fp, t_int, depth + 1, max_depth - 1,
				exclude_id);
	    }
	  fprintf (fp, ")");
	}
      break;
    case 2:
      cast_integer_type (fp, rslttype);
      fprintf (fp, "(");
      write_expression (fp, t_int, depth + 1, max_depth, exclude_id);
      fprintf (fp, " + ");
      write_expression (fp, t_int, depth + 1, max_depth, exclude_id);
      fprintf (fp, ")");
      break;
    case 3:
      cast_integer_type (fp, rslttype);
      fprintf (fp, "(");
      write_expression (fp, t_int, depth + 1, max_depth, exclude_id);
      fprintf (fp, " - ");
      write_expression (fp, t_int, depth + 1, max_depth, exclude_id);
      fprintf (fp, ")");
      break;
    case 4:
      cast_integer_type (fp, rslttype);
      fprintf (fp, "(");
      write_expression (fp, t_int, depth + 1, max_depth, exclude_id);
      fprintf (fp, " * ");
      write_expression (fp, t_int, depth + 1, max_depth, exclude_id);
      fprintf (fp, ")");
      break;
    case 5:
      cast_integer_type (fp, rslttype);
      n = xrandom (num_enums);
      n2 = xrandom (enums[n].num_enumerators);
      fprintf (fp, "%s", enums[n].enumerators[n2].name);
      break;
    default:
      cast_integer_type (fp, rslttype);
      fprintf (fp, "%d", xrandom (127));
      break;
    }
}

void
cast_integer_type (FILE *fp, int type)
{
  if (type == t_void || type == t_int)
    return;
  fprintf (fp, "(%s) ", name_from_type (type));
}

struct macro_desc *
find_macro (int rslttype, struct macro_desc *macs, int nummacros,
	    int exclude_id)
{
  int j, n;

  for (j = 0; j < 1000; ++j)
    {
      n = xrandom (nummacros);
      if (macs[n].use && macs[n].id > exclude_id)
	return &(macs[n]);
    }
  return NULL;
}

/* Find a function that has the right return type.  */

struct function_desc *
find_function (int rslttype, struct function_desc *fns, int numfns)
{
  int j, n;

  /* Try several times, but eventually give up.  */
  for (j = 0; j < 1000; ++j)
    {
      n = xrandom (numfns);
      if (fns[n].use && fns[n].return_type == rslttype)
	return &(fns[n]);
    }
  return NULL;
}

/* Write out a comment block that lists all the parameters used to
   generate this program.  */

void
write_description_block (FILE *fp)
{
  extern unsigned long initial_randstate;

  fprintf (fp, "/* A fine software product by SPU %s.  */\n",
	   version_string);
  fprintf (fp, "/* Written in %s. */\n", lang_names[language]);
  fprintf (fp, "/* Program: %d macros, %d enums, %d structs, %d classes, %d functions, */\n",
	   num_macros, num_enums, num_structs, num_classes, num_functions);
  fprintf (fp, "/* divided into %d source file(s) and %d header(s).  */\n",
	   num_files, num_header_files);
  fprintf (fp, "/* Library: %d macros, %d enums, %d structs, %d classes, %d functions. */\n",
	   num_lib_macros, num_lib_enums, num_lib_structs, num_lib_classes,
	   num_lib_functions);
  fprintf (fp, "/* Enumerators per enum range from %d to %d.  */\n",
	   num_enumerators / 2, num_enumerators + 1);
  fprintf (fp, "/* Fields per struct/class range from %d to %d.  */\n",
	   1, num_fields);
  if (num_classes > 0 || num_lib_classes > 0)
    fprintf (fp, "/* Methods per class range from %d to %d.  */\n",
	     0, num_methods);
  fprintf (fp, "/* Function length is %d statements, expression depth is %d.  */\n",
	   function_length, function_depth);
  fprintf (fp, "/* Random seed is %d.  */\n", (int) initial_randstate);
  fprintf (fp, "\n");
}

/* Write out a makefile that will compile the program just generated. */

void
write_makefile (void)
{
  char tmpbuf[100];
  int i, j;
  FILE *fp;

  sprintf (tmpbuf, "%s.mk", file_base_name);
  fp = fopen (tmpbuf, "w");
  if (fp)
    {
      /* Name the compiler so we can change it easily.  */
      fprintf (fp, "CC = cc\n\n");
      /* Write dependencies and action line for the executable.  */
      fprintf (fp, "%s.out:	", file_base_name);
      for (i = 0; i < num_files; ++i)
	fprintf (fp, " %s%d.o", file_base_name, i);
      fprintf (fp, " %slib.o", file_base_name);
      fprintf (fp, "\n");
      fprintf (fp, "\t$(CC) -o %s.out", file_base_name);
      for (i = 0; i < num_files; ++i)
	fprintf (fp, " %s%d.o", file_base_name, i);
      fprintf (fp, " %slib.o", file_base_name);
      fprintf (fp, "\n\n");
      /* Write dependencies for individual files. */
      for (i = 0; i < num_files; ++i)
	{
	  fprintf (fp, " %s%d.o:	%s%d.%s",
		   file_base_name, i, file_base_name, i,
		   extensions[language]);
	  for (j = 0; j < num_header_files; ++j)
	    fprintf (fp, " %s%d.h", file_base_name, j);
	  fprintf (fp, "\n");
	  fprintf (fp, "\t$(CC) -c %s%d.%s\n",
		   file_base_name, i, extensions[language]);
	}

      /* Library stuff.  */
      fprintf (fp, "\nlib:	%slib.o\n\n", file_base_name);
      fprintf (fp, " %slib.o:	%slib.%s %slib.h",
	       file_base_name, file_base_name, extensions[language],
	       file_base_name);
      fprintf (fp, "\n");
      fprintf (fp, "\t$(CC) -c %slib.%s\n",
	       file_base_name, extensions[language]);
      fclose (fp);
    }
}

/* Utility/general functions. */

/* Generate a name that is guaranteed to be a valid C/C++ identifier.  */

char *
gen_unique_global_name (char *root, int upcase)
{
  int i, j;
  char *str, namebuf[100];

  if (global_hash_table == NULL)
    {
      global_hash_table =
	(struct hash_table *) xmalloc (sizeof (struct hash_table));
    }
  str = NULL;
  /* Keep trying until we get a unique name. */
  for (i = 0; i < 10000; ++i)
    {
      gen_random_global_name (root, namebuf);
      if (upcase)
	{
	  for (j = 0; namebuf[j] != '\0'; ++j)
	    namebuf[j] = toupper (namebuf[j]);
	}
      if (get_from_hash_table (namebuf, global_hash_table) == NULL)
	{
	  str = add_to_hash_table (namebuf, global_hash_table);
	  break;
	}
    }
  if (str == NULL)
    {
      fprintf (stderr, "Can't get a unique name!\n");
      exit(1);
    }
  return str;
}

/* Synthesize a random name that is a valid C/C++ identifier and that
   "looks like" something one might see in a program.  */

void
gen_random_global_name (char *root, char *namebuf)
{
  char smallbuf[100];
  int n, i, len;

  namebuf[0] = '\0';
  switch (xrandom (4))
    {
    case 0:
      namebuf[0] = 'a' + xrandom (26);
      namebuf[1] = '\0';
      break;
    case 1:
      /* Convert a random number into a string, maybe with some
	 underscores thrown in for flavor.  */
      n = xrandom (10000);
      i = 0;
      while (n > 0)
	{
	  if (xrandom (6) == 0)
	    namebuf[i++] = '_';
	  namebuf[i++] = 'a' + (n % 26);
	  n /= 26;
	}
      namebuf[i] = '\0';
      break;
    default:
      strcat (namebuf, random_computer_word ());
      break;
    }
  if (root != NULL)
    {
      strcat (namebuf, "_");
      strcat (namebuf, root);
    }
  switch (xrandom (5))
    {
    case 0:
      strcat (namebuf, "_");
      len = strlen (namebuf);
      namebuf[len] = 'a' + xrandom (26);
      namebuf[len + 1] = '\0';
      break;
    case 1:
      strcat (namebuf, "_");
      sprintf (smallbuf, "%d", xrandom (10000));
      strcat (namebuf, smallbuf);
      break;
    case 2:
      n = xrandom (10000);
      len = strlen (namebuf);
      i = len;
      while (n > 0)
	{
	  if (xrandom (6) == 0)
	    namebuf[i++] = '_';
	  namebuf[i++] = 'a' + (n % 26);
	  n /= 26;
	}
      namebuf[i] = '\0';
      break;
    default:
      strcat (namebuf, "_");
      strcat (namebuf, random_computer_word ());
      break;
    }
  if (xrandom (5) == 0)
    {
      strcat (namebuf, "_");
      sprintf (smallbuf, "%d", xrandom (10000));
      strcat (namebuf, smallbuf);
    }
#if 0 /* enable to study random name distribution */
  printf ("Try %s\n", namebuf);
#endif
}

/* Generate a local variable name. */

char *
gen_random_local_name (int numothers, char **others)
{
  char namebuf[100];

  sprintf (namebuf, "%s%d",
	   (xrandom (2) == 0 ? random_computer_word () : "arg"),
	   numothers + 1);
  return copy_string (namebuf);
}

/* Generic hash table code. */

int
hash_string (char *str)
{
  int i, rslt;

  rslt = 0;
  for (i = 0; str[i] != '\0'; ++i)
    {
      rslt ^= str[i];
      rslt %= 253;
    }
  return rslt;
}

struct hash_entry *
get_hash_entry (void)
{
  return (struct hash_entry *) xmalloc (sizeof (struct hash_entry));
}

char *
add_to_hash_table (char *buf, struct hash_table *table)
{
  int hash;
  struct hash_entry *ent, *lastent;

  if (buf == NULL)
    buf = "";
  ++(table->numadds);
  hash = hash_string (buf);
  if (table->entries[hash] == NULL)
    {
      table->entries[hash] = get_hash_entry ();
      table->entries[hash]->val = copy_string (buf);
      return table->entries[hash]->val;
    }
  else
    {
      for (ent = table->entries[hash]; ent != NULL; ent = ent->next)
	{
	  if (ent->val == NULL)
	    return "null!?!";
	  if (strcmp (buf, ent->val) == 0)
	    return ent->val;
	  lastent = ent;
	}
      if (lastent != NULL)
	{
	  lastent->next = get_hash_entry ();
	  lastent->next->val = copy_string (buf);
	  return lastent->next->val;
	}
    }
  /* should never happen */
  return "?!hash!?";
}

char *
get_from_hash_table (char *buf, struct hash_table *table)
{
  int hash;
  struct hash_entry *ent, *lastent;

  if (buf == NULL)
    buf = "";
  hash = hash_string (buf);
  if (table->entries[hash] == NULL)
    {
      return NULL;
    }
  else
    {
      for (ent = table->entries[hash]; ent != NULL; ent = ent->next)
	{
	  if (ent->val == NULL)
	    return "null!?!";
	  if (strcmp (buf, ent->val) == 0)
	    return ent->val;
	  lastent = ent;
	}
      if (lastent != NULL)
	{
	  return NULL;
	}
    }
  /* should never happen */
  return "?!hash!?";
}

/* Random number handling is important but terrible/nonexistent in
   some systems.  Do it ourselves.  Also, this will give repeatable
   results across multiple platforms, which is important if this is
   being used to generate test cases. */

/* The random state *must* be at least 32 bits.  */

unsigned long initial_randstate = 0;

unsigned long randstate = 0;

/* Seed can come from elsewhere, for repeatability.  Otherwise, it comes
   from the current time, scaled down to where 32-bit arithmetic won't
   overflow.  */

void
init_xrandom (int seed)
{
  time_t tm;
    	
  if (seed > 0)
    {
      /* If the random state is already set, changes are somewhat
	 suspicious.  */
      if (randstate > 0)
	{
	  fprintf (stderr, "Randstate being changed from %lu to %d\n",
		   randstate, seed);
	}
      randstate = seed;
    }
  else
    {
      time (&tm);
      randstate = tm;
    }
  /* Whatever its source, put the randstate into known range (0 - 199999).  */
  randstate = abs (randstate);
  randstate %= 200000;
  /* This is kept around for the sake of error reporting. */
  initial_randstate = randstate;
}

/* Numbers lifted from Numerical Recipes, p. 198.  */
/* Arithmetic must be 32-bit.  */

int
xrandom (int m)
{
  int rslt;

  randstate = (9301 * randstate + 49297) % 233280;
  rslt = (m * randstate) / 233280;
#if 0 /* enable to study random number distribution */
  printf ("# %lu -> %d\n", randstate, rslt);
#endif
  return rslt;
}

/* Percentage probability, with bounds checking. */

int
probability(int prob)
{
  if (prob <= 0)
    return 0;
  if (prob >= 100)
    return 1;
  return (xrandom (100) < prob);
}

char *
xmalloc (int amt)
{
  char *value = (char *) malloc (amt);

  if (value == NULL)
    {
      /* This is pretty serious, have to get out quickly.  */
      fprintf (stderr, "Memory exhausted!!\n");
      exit (1);
    }
  /* Save callers from having to clear things themselves.  */
  memset (value, 0, amt);
  return value;
}

/* Copy a string to newly-allocated space.  The new space is never
   freed.  */

char *
copy_string (char *str)
{
  int len = strlen (str);
  char *rslt;
  
  rslt = xmalloc (len + 1);
  strcpy (rslt, str);
  return rslt;
}
