/* Header file for modules that link with gcc.c
   Copyright (C) 1999-2016 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_GCC_H
#define GCC_GCC_H

#include "version.h"
#include "diagnostic-core.h"

/* The top-level "main" within the driver would be ~1000 lines long.
   This class breaks it up into smaller functions and contains some
   state shared by them.  */

class driver
{
 public:
  driver (bool can_finalize, bool debug);
  ~driver ();
  int main (int argc, char **argv);
  void finalize ();

 private:
  void set_progname (const char *argv0) const;
  void expand_at_files (int *argc, char ***argv) const;
  void decode_argv (int argc, const char **argv);
  void global_initializations ();
  void build_multilib_strings () const;
  void set_up_specs () const;
  void putenv_COLLECT_GCC (const char *argv0) const;
  void maybe_putenv_COLLECT_LTO_WRAPPER () const;
  void maybe_putenv_OFFLOAD_TARGETS () const;
  void build_option_suggestions (void);
  const char *suggest_option (const char *bad_opt);
  void handle_unrecognized_options ();
  int maybe_print_and_exit () const;
  bool prepare_infiles ();
  void do_spec_on_infiles () const;
  void maybe_run_linker (const char *argv0) const;
  void final_actions () const;
  int get_exit_code () const;

 private:
  char *explicit_link_files;
  struct cl_decoded_option *decoded_options;
  unsigned int decoded_options_count;
  auto_vec <char *> *m_option_suggestions;
};

/* The mapping of a spec function name to the C function that
   implements it.  */
struct spec_function
{
  const char *name;
  const char *(*func) (int, const char **);
};

/* This defines which switch letters take arguments.  */

#define DEFAULT_SWITCH_TAKES_ARG(CHAR) \
  ((CHAR) == 'D' || (CHAR) == 'U' || (CHAR) == 'o' \
   || (CHAR) == 'e' || (CHAR) == 'T' || (CHAR) == 'u' \
   || (CHAR) == 'I' || (CHAR) == 'J' || (CHAR) == 'm' \
   || (CHAR) == 'x' || (CHAR) == 'L' || (CHAR) == 'A' \
   || (CHAR) == 'V' || (CHAR) == 'B' || (CHAR) == 'b')

/* This defines which multi-letter switches take arguments.  */

#define DEFAULT_WORD_SWITCH_TAKES_ARG(STR)		\
 (!strcmp (STR, "Tdata") || !strcmp (STR, "Ttext")	\
  || !strcmp (STR, "Tbss") || !strcmp (STR, "include")	\
  || !strcmp (STR, "imacros") || !strcmp (STR, "aux-info") \
  || !strcmp (STR, "idirafter") || !strcmp (STR, "iprefix") \
  || !strcmp (STR, "iwithprefix") || !strcmp (STR, "iwithprefixbefore") \
  || !strcmp (STR, "iquote") || !strcmp (STR, "isystem") \
  || !strcmp (STR, "isysroot") \
  || !strcmp (STR, "cxx-isystem") || !strcmp (STR, "-iremap") \
  || !strcmp (STR, "-param") || !strcmp (STR, "specs") \
  || !strcmp (STR, "MF") || !strcmp (STR, "MT") || !strcmp (STR, "MQ") \
  || !strcmp (STR, "fintrinsic-modules-path") \
  || !strcmp (STR, "dumpbase") || !strcmp (STR, "dumpdir"))


/* These are exported by gcc.c.  */
extern int do_spec (const char *);
extern void record_temp_file (const char *, int, int);
extern void pfatal_with_name (const char *) ATTRIBUTE_NORETURN;
extern void set_input (const char *);

/* Spec files linked with gcc.c must provide definitions for these.  */

/* Called before processing to change/add/remove arguments.  */
extern void lang_specific_driver (struct cl_decoded_option **,
				  unsigned int *, int *);

/* Called before linking.  Returns 0 on success and -1 on failure.  */
extern int lang_specific_pre_link (void);

extern int n_infiles;

/* Number of extra output files that lang_specific_pre_link may generate.  */
extern int lang_specific_extra_outfiles;

/* A vector of corresponding output files is made up later.  */

extern const char **outfiles;

extern void
driver_get_configure_time_options (void (*cb)(const char *option,
					      void *user_data),
				   void *user_data);

#endif /* ! GCC_GCC_H */
