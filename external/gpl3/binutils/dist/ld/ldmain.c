/* Main program of GNU linker.
   Copyright (C) 1991-2025 Free Software Foundation, Inc.
   Written by Steve Chamberlain steve@cygnus.com

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

#include "sysdep.h"
#include "bfd.h"
#include "bfdver.h"
#include "safe-ctype.h"
#include "libiberty.h"
#include "bfdlink.h"
#include "ctf-api.h"
#include "filenames.h"
#include "elf/common.h"

#include "ld.h"
#include "ldmain.h"
#include "ldmisc.h"
#include "ldwrite.h"
#include "ldexp.h"
#include "ldlang.h"
#include <ldgram.h>
#include "ldlex.h"
#include "ldfile.h"
#include "ldemul.h"
#include "ldctor.h"
#if BFD_SUPPORTS_PLUGINS
#include "plugin.h"
#include "plugin-api.h"
#endif /* BFD_SUPPORTS_PLUGINS */

/* Somewhere above, sys/stat.h got included.  */
#if !defined(S_ISDIR) && defined(S_IFDIR)
#define	S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#include <string.h>

#if defined (HAVE_GETRUSAGE)
#include <sys/resource.h>
#endif

#ifndef TARGET_SYSTEM_ROOT
#define TARGET_SYSTEM_ROOT ""
#endif

/* EXPORTS */

FILE *saved_script_handle = NULL;
FILE *previous_script_handle = NULL;
bool force_make_executable = false;

char *default_target;
const char *output_filename = "a.out";

/* Name this program was invoked by.  */
char *program_name;

/* The prefix for system library directories.  */
const char *ld_sysroot;

/* The canonical representation of ld_sysroot.  */
char *ld_canon_sysroot;
int ld_canon_sysroot_len;

/* Set by -G argument, for targets like MIPS ELF.  */
int g_switch_value = 8;

/* Nonzero means print names of input files as processed.  */
unsigned int trace_files;

/* Nonzero means report actions taken by the linker, and describe the linker script in use.  */
bool verbose;

/* Nonzero means version number was printed, so exit successfully
   instead of complaining if no input files are given.  */
bool version_printed;

/* TRUE if we should demangle symbol names.  */
bool demangling;

bool in_section_ordering;

args_type command_line;

ld_config_type config;

sort_type sort_section;

static const char *get_sysroot
  (int, char **);
static char *get_emulation
  (int, char **);
static bool add_archive_element
  (struct bfd_link_info *, bfd *, const char *, bfd **);
static void multiple_definition
  (struct bfd_link_info *, struct bfd_link_hash_entry *,
   bfd *, asection *, bfd_vma);
static void multiple_common
  (struct bfd_link_info *, struct bfd_link_hash_entry *,
   bfd *, enum bfd_link_hash_type, bfd_vma);
static void add_to_set
  (struct bfd_link_info *, struct bfd_link_hash_entry *,
   bfd_reloc_code_real_type, bfd *, asection *, bfd_vma);
static void constructor_callback
  (struct bfd_link_info *, bool, const char *, bfd *,
   asection *, bfd_vma);
static void warning_callback
  (struct bfd_link_info *, const char *, const char *, bfd *,
   asection *, bfd_vma);
static void warning_find_reloc
  (bfd *, asection *, void *);
static void undefined_symbol
  (struct bfd_link_info *, const char *, bfd *, asection *, bfd_vma,
   bool);
static void reloc_overflow
  (struct bfd_link_info *, struct bfd_link_hash_entry *, const char *,
   const char *, bfd_vma, bfd *, asection *, bfd_vma);
static void reloc_dangerous
  (struct bfd_link_info *, const char *, bfd *, asection *, bfd_vma);
static void unattached_reloc
  (struct bfd_link_info *, const char *, bfd *, asection *, bfd_vma);
static bool notice
  (struct bfd_link_info *, struct bfd_link_hash_entry *,
   struct bfd_link_hash_entry *, bfd *, asection *, bfd_vma, flagword);

static struct bfd_link_callbacks link_callbacks =
{
  add_archive_element,
  multiple_definition,
  multiple_common,
  add_to_set,
  constructor_callback,
  warning_callback,
  undefined_symbol,
  reloc_overflow,
  reloc_dangerous,
  unattached_reloc,
  notice,
  fatal,
  einfo,
  info_msg,
  minfo,
  ldlang_override_segment_assignment,
  ldlang_ctf_acquire_strings,
  NULL,
  ldlang_ctf_new_dynsym,
  ldlang_write_ctf_late
};

static bfd_assert_handler_type default_bfd_assert_handler;
static bfd_error_handler_type default_bfd_error_handler;

struct bfd_link_info link_info;

struct dependency_file
{
  struct dependency_file *next;
  char *name;
};

static struct dependency_file *dependency_files, *dependency_files_tail;

void
track_dependency_files (const char *filename)
{
  struct dependency_file *dep
    = (struct dependency_file *) xmalloc (sizeof (*dep));
  dep->name = xstrdup (filename);
  dep->next = NULL;
  if (dependency_files == NULL)
    dependency_files = dep;
  else
    dependency_files_tail->next = dep;
  dependency_files_tail = dep;
}

static void
write_dependency_file (void)
{
  FILE *out;
  struct dependency_file *dep;

  out = fopen (config.dependency_file, FOPEN_WT);
  if (out == NULL)
    {
      bfd_set_error (bfd_error_system_call);
      fatal (_("%P: cannot open dependency file %s: %E\n"),
	     config.dependency_file);
    }

  fprintf (out, "%s:", output_filename);

  for (dep = dependency_files; dep != NULL; dep = dep->next)
    fprintf (out, " \\\n  %s", dep->name);

  fprintf (out, "\n");
  for (dep = dependency_files; dep != NULL; dep = dep->next)
    fprintf (out, "\n%s:\n", dep->name);

  fclose (out);
}

static void
ld_cleanup (void)
{
  bfd *ibfd, *inext;
  if (link_info.output_bfd)
    bfd_close_all_done (link_info.output_bfd);
  for (ibfd = link_info.input_bfds; ibfd; ibfd = inext)
    {
      inext = ibfd->link.next;
      bfd_close_all_done (ibfd);
    }
#if BFD_SUPPORTS_PLUGINS
  /* Note - we do not call ld_plugin_start (PHASE_PLUGINS) here as this
     function is only called when the linker is exiting - ie after any
     stats may have been reported, and potentially in the middle of a
     phase where we have already started recording plugin stats.  */
  plugin_call_cleanup ();
#endif
  if (output_filename && delete_output_file_on_failure)
    unlink_if_ordinary (output_filename);
}

/* Hook to notice BFD assertions.  */

static void
ld_bfd_assert_handler (const char *fmt, const char *bfdver,
		       const char *file, int line)
{
  config.make_executable = false;
  (*default_bfd_assert_handler) (fmt, bfdver, file, line);
}

/* Hook the bfd error/warning handler for --fatal-warnings.  */

static void
ld_bfd_error_handler (const char *fmt, va_list ap)
{
  if (config.fatal_warnings)
    config.make_executable = false;
  (*default_bfd_error_handler) (fmt, ap);
}

static void
display_external_script (void)
{
  if (saved_script_handle == NULL)
    return;
  
  static const int ld_bufsz = 8193;
  size_t n;
  char *buf = (char *) xmalloc (ld_bufsz);

  rewind (saved_script_handle);
  while ((n = fread (buf, 1, ld_bufsz - 1, saved_script_handle)) > 0)
    {
      buf[n] = 0;
      info_msg ("%s", buf);
    }
  rewind (saved_script_handle);
  free (buf);
}

struct ld_phase_data
{
  const char *    name;

  unsigned long   start;
  unsigned long   duration;

  bool            started;
  bool            broken;

#if defined (HAVE_GETRUSAGE)
  struct rusage   begin;
  struct rusage   use;
#endif
};

static struct ld_phase_data phase_data [NUM_PHASES] =
{
  [PHASE_ALL]     = { .name = "ALL" },
  [PHASE_CTF]     = { .name = "ctf processing" },
  [PHASE_MERGE]   = { .name = "string merge" },
  [PHASE_PARSE]   = { .name = "parsing" },
  [PHASE_PLUGINS] = { .name = "plugins" },
  [PHASE_PROCESS] = { .name = "processing files" },
  [PHASE_WRITE]   = { .name = "write" },
};

void
ld_start_phase (ld_phase phase)
{
  struct ld_phase_data * pd = phase_data + phase;

  /* We record data even if config.stats_file is NULL.  This allows
     us to record data about phases that start before the command line
     arguments have been parsed.  ie PHASE_ALL and PHASE_PARSE.  */

  /* Do not overwrite the fields if we have already started recording.  */
  if (pd->started)
    {
      /* Since we do not queue phase starts and stops, if a phase is started
	 multiple times there is a likelyhood that it will be stopped multiple
	 times as well.  This is problematic as we will only record the data
	 for the first time the phase stops and ignore all of the other stops.

	 So let the user know.  Ideally real users will never actually see
	 this message, and instead only developers who are adding new phase
	 tracking code will ever encounter it.  */
      einfo ("%P: --stats: phase %s started twice - data may be unreliable\n",
	     pd->name);
      return;
    }

  /* It is OK if other phases are also active at this point.
     It just means that the phases overlap or that one phase is a sub-task
     of another.  Since we record resources on a per-phase basis, this
     should not matter.  */

  pd->started = true;
  pd->start = get_run_time ();

#if defined (HAVE_GETRUSAGE)
  /* Record the resource usage at the start of the phase.  */
  struct rusage usage;

  if (getrusage (RUSAGE_SELF, & usage) != 0)
    /* FIXME: Complain ?  */
    return;
  
  memcpy (& pd->begin, & usage, sizeof usage);
#endif
}

void
ld_stop_phase (ld_phase phase)
{
  struct ld_phase_data * pd = phase_data + phase;

  if (!pd->started)
    {
      /* We set the broken flag to indicate that the data
	 recorded for this phase is inconsistent.  */
      pd->broken = true;
      return;
    }

  pd->duration += get_run_time () - pd->start;
  pd->started = false;

#if defined (HAVE_GETRUSAGE)
  struct rusage usage;

  if (getrusage (RUSAGE_SELF, & usage) != 0)
    /* FIXME: Complain ?  */
    return;

  if (phase == PHASE_ALL)
    memcpy (& pd->use, & usage, sizeof usage);
  else
    {
      struct timeval t;

      /* For sub-phases we record the increase in specific fields.  */
      /* FIXME: Most rusage{} fields appear to be irrelevent to when considering
	 linker resource usage.  Currently we record maxrss and user and system
	 cpu times.  Are there any other fields that might be useful ?  */

#ifndef timeradd /* Macros copied from <sys/time.h>.  */
#define timeradd(a, b, result)					\
      do							\
	{							\
	  (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;		\
	  (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;	\
	  if ((result)->tv_usec >= 1000000)			\
	    {							\
	      ++(result)->tv_sec;				\
	      (result)->tv_usec -= 1000000;			\
	    }							\
	}							\
      while (0)
#endif
      
#ifndef timersub
#define timersub(a, b, result)					\
      do							\
	{							\
	  (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;		\
	  (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;	\
	  if ((result)->tv_usec < 0)				\
	    {							\
	      --(result)->tv_sec;				\
	      (result)->tv_usec += 1000000;			\
	    }							\
	}							\
      while (0)
#endif
      
      timersub (& usage.ru_utime, & pd->begin.ru_utime, & t);
      timeradd (& pd->use.ru_utime, &t, & pd->use.ru_utime);

      timersub (& usage.ru_stime, & pd->begin.ru_stime, & t);
      timeradd (& pd->use.ru_stime, &t, & pd->use.ru_stime);
		
      if (pd->begin.ru_maxrss < usage.ru_maxrss)
	pd->use.ru_maxrss += usage.ru_maxrss - pd->begin.ru_maxrss;
    }
#endif
}

static void
report_phases (FILE * file, time_t * start, char ** argv)
{
  unsigned long i;

  if (file == NULL)
    return;

  /* We might be writing to stdout, so make sure
     that we do not have any pending error output.  */
  fflush (stderr);

  /* We do not translate "Stats" as we provide this as a key
     word that can be searched for by grep and the like.  */
#define STATS_PREFIX "Stats: "

  fprintf (file, STATS_PREFIX "linker version: %s\n", BFD_VERSION_STRING);

  /* No \n at the end of the string as ctime() provides its own.  */
  fprintf (file, STATS_PREFIX "linker started: %s", ctime (start));

  /* We include the linker command line arguments since
     they can be hard to track down by other means.  */
  if (argv != NULL)
    {
      fprintf (file, STATS_PREFIX "args: ");
      for (i = 0; argv[i] != NULL; i++)
	fprintf (file, "%s ", argv[i]);
      fprintf (file, "\n\n");  /* Blank line to separate the args from the stats.  */
    }

  /* All of this song and dance with the column_info struct and printf
     formatting is so that we can have a nicely formated table with regular
     column spacing, whilst allowing for the column headers to be translated,
     and coping nicely with extra long strings or numbers.  */
  struct column_info
  {
    const char * header;
    const char * sub_header;
    int          width;
    int          pad;
  } columns[] =
#define COLUMNS_FIELD(HEADER,SUBHEADER) \
    { .header = N_( HEADER ), .sub_header = N_( SUBHEADER ) },
  {
    COLUMNS_FIELD ("phase", "name")
    COLUMNS_FIELD ("cpu time", "(microsec)")
#if defined (HAVE_GETRUSAGE)  
    /* Note: keep these columns in sync with the
       information recorded in ld_stop_phase().  */
    COLUMNS_FIELD ("memory", "(KiB)")
    COLUMNS_FIELD ("user time", "(seconds)")
    COLUMNS_FIELD ("system time", "(seconds)")
#endif
  };

#ifndef max
#define max(A,B) ((A) < (B) ? (B) : (A))
#endif

  size_t maxwidth = 1;
  for (i = 0; i < NUM_PHASES; i++)
    maxwidth = max (maxwidth, strlen (phase_data[i].name));

  fprintf (file, "%s", STATS_PREFIX);

  for (i = 0; i < ARRAY_SIZE (columns); i++)
    {
      int padding;

      if (i == 0)
	columns[i].width = fprintf (file, "%-*s", (int) maxwidth, columns[i].header);
      else
	columns[i].width = fprintf (file, "%s", columns[i].header);
      padding = columns[i].width % 8;
      if (padding < 4)
	padding = 4;
      columns[i].pad = fprintf (file, "%*c", padding, ' ');
    }

  fprintf (file, "\n");

  int bias = 0;
#define COLUMN_ENTRY(VAL, FORMAT, N)					\
  do									\
    {									\
      int l;								\
      									\
      if (N == 0) 							\
	l = fprintf (file, "%-*" FORMAT, columns[N].width, VAL);	\
      else								\
	l = fprintf (file, "%*" FORMAT, columns[N].width - bias, VAL);	\
      bias = 0;								\
      if (l < columns[N].width)						\
	l = columns[N].pad;						\
      else if (l < columns[N].width + columns[N].pad)			\
	l = columns[N].pad - (l - columns[N].width);			\
      else								\
	{								\
	  bias = l - (columns[N].width + columns[N].pad);		\
	  l = 0;							\
	}								\
      if (l)								\
	fprintf (file, "%*c", l, ' ');					\
    }									\
  while (0)

  fprintf (file, "%s", STATS_PREFIX);

  for (i = 0; i < ARRAY_SIZE (columns); i++)
    COLUMN_ENTRY (columns[i].sub_header, "s", i);

  fprintf (file, "\n");

  for (i = 0; i < NUM_PHASES; i++)
    {
      struct ld_phase_data * pd = phase_data + i;
      /* This should not be needed...  */      
      const char * name = pd->name ? pd->name : "<unnamed>";

      if (pd->broken)
	{
	  fprintf (file, "%s %s: %s",
		   STATS_PREFIX, name, _("WARNING: Data is unreliable!\n"));
	  continue;
	}

      fprintf (file, "%s", STATS_PREFIX);

      /* Care must be taken to keep the lines below in sync with
	 entries in the columns_info array.
	 FIXME: There ought to be a better way to do this...  */
      COLUMN_ENTRY (name, "s", 0);
      COLUMN_ENTRY (pd->duration, "ld", 1);
#if defined (HAVE_GETRUSAGE)
      COLUMN_ENTRY (pd->use.ru_maxrss, "ld", 2);
      COLUMN_ENTRY ((int64_t) pd->use.ru_utime.tv_sec, PRId64, 3);
      COLUMN_ENTRY ((int64_t) pd->use.ru_stime.tv_sec, PRId64, 4);
#endif
      fprintf (file, "\n");
    }

  fflush (file);
}

int
main (int argc, char **argv)
{
  char *emulation;
  long start_time = get_run_time ();
  time_t start_seconds = time (NULL);

#ifdef HAVE_LC_MESSAGES
  setlocale (LC_MESSAGES, "");
#endif
  setlocale (LC_CTYPE, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  program_name = argv[0];
  xmalloc_set_program_name (program_name);

  /* Check the LD_STATS environment variable before parsing the command line
     so that the --stats option, if used, can override the environment variable.  */
  char * stats_filename;
  if ((stats_filename = getenv ("LD_STATS")) != NULL)
    {
      if (ISPRINT (stats_filename[0]))
	config.stats_filename = stats_filename;
      else
	config.stats_filename = "-";
      config.stats = true;
    }

  ld_start_phase (PHASE_ALL);
  ld_start_phase (PHASE_PARSE);
  
  expandargv (&argc, &argv);
  char ** saved_argv = dupargv (argv);

  if (bfd_init () != BFD_INIT_MAGIC)
    fatal (_("%P: fatal error: libbfd ABI mismatch\n"));

  bfd_set_error_program_name (program_name);

  /* We want to notice and fail on those nasty BFD assertions which are
     likely to signal incorrect output being generated but otherwise may
     leave no trace.  */
  default_bfd_assert_handler = bfd_set_assert_handler (ld_bfd_assert_handler);

  /* Also hook the bfd error/warning handler for --fatal-warnings.  */
  default_bfd_error_handler = bfd_set_error_handler (ld_bfd_error_handler);

  xatexit (ld_cleanup);

  /* Remove temporary object-only files.  */
  xatexit (cmdline_remove_object_only_files);

  /* Set up the sysroot directory.  */
  ld_sysroot = get_sysroot (argc, argv);
  if (*ld_sysroot)
    ld_canon_sysroot = lrealpath (ld_sysroot);
  if (ld_canon_sysroot)
    {
      ld_canon_sysroot_len = strlen (ld_canon_sysroot);

      /* is_sysrooted_pathname() relies on no trailing dirsep.  */
      if (ld_canon_sysroot_len > 0
	  && IS_DIR_SEPARATOR (ld_canon_sysroot [ld_canon_sysroot_len - 1]))
	ld_canon_sysroot [--ld_canon_sysroot_len] = '\0';
    }
  else
    ld_canon_sysroot_len = -1;

  /* Set the default BFD target based on the configured target.  Doing
     this permits the linker to be configured for a particular target,
     and linked against a shared BFD library which was configured for
     a different target.  The macro TARGET is defined by Makefile.  */
  if (!bfd_set_default_target (TARGET))
    {
      einfo (_("%X%P: can't set BFD default target to `%s': %E\n"), TARGET);
      xexit (1);
    }

#if YYDEBUG
  {
    extern int yydebug;
    yydebug = 1;
  }
#endif

  config.build_constructors = true;
  config.rpath_separator = ':';
  config.split_by_reloc = (unsigned) -1;
  config.split_by_file = (bfd_size_type) -1;
  config.make_executable = true;
  config.magic_demand_paged = true;
  config.text_read_only = true;
  config.print_map_discarded = true;
  link_info.disable_target_specific_optimizations = -1;

  command_line.warn_mismatch = true;
  command_line.warn_search_mismatch = true;
  command_line.check_section_addresses = -1;

  /* We initialize DEMANGLING based on the environment variable
     COLLECT_NO_DEMANGLE.  The gcc collect2 program will demangle the
     output of the linker, unless COLLECT_NO_DEMANGLE is set in the
     environment.  Acting the same way here lets us provide the same
     interface by default.  */
  demangling = getenv ("COLLECT_NO_DEMANGLE") == NULL;

  link_info.allow_undefined_version = true;
  link_info.keep_memory = true;
  link_info.max_cache_size = (bfd_size_type) -1;
  link_info.combreloc = true;
  link_info.strip_discarded = true;
  link_info.prohibit_multiple_definition_absolute = false;
  link_info.textrel_check = DEFAULT_LD_TEXTREL_CHECK;
  link_info.emit_hash = DEFAULT_EMIT_SYSV_HASH;
  link_info.emit_gnu_hash = DEFAULT_EMIT_GNU_HASH;
  link_info.callbacks = &link_callbacks;
  link_info.input_bfds_tail = &link_info.input_bfds;
  /* SVR4 linkers seem to set DT_INIT and DT_FINI based on magic _init
     and _fini symbols.  We are compatible.  */
  link_info.init_function = "_init";
  link_info.fini_function = "_fini";
  link_info.relax_pass = 1;
  link_info.extern_protected_data = -1;
  link_info.dynamic_undefined_weak = -1;
  link_info.indirect_extern_access = -1;
  link_info.pei386_auto_import = -1;
  link_info.spare_dynamic_tags = 5;
  link_info.path_separator = ':';
#ifdef DEFAULT_FLAG_COMPRESS_DEBUG
  config.compress_debug = DEFAULT_COMPRESSED_DEBUG_ALGORITHM;
#endif
#ifdef DEFAULT_NEW_DTAGS
  link_info.new_dtags = DEFAULT_NEW_DTAGS;
#endif
  link_info.start_stop_gc = false;
  link_info.start_stop_visibility = STV_PROTECTED;

  ldfile_add_arch ("");
  emulation = get_emulation (argc, argv);
  ldemul_choose_mode (emulation);
  default_target = ldemul_choose_target (argc, argv);
  lang_init (false);
  ldexp_init (false);
  ldemul_before_parse ();
  lang_has_input_file = false;
  parse_args (argc, argv);

  if (config.hash_table_size != 0)
    bfd_hash_set_default_size (config.hash_table_size);

  ld_stop_phase (PHASE_PARSE);
  
#if BFD_SUPPORTS_PLUGINS
  ld_start_phase (PHASE_PLUGINS);
  /* Now all the plugin arguments have been gathered, we can load them.  */
  plugin_load_plugins ();
  ld_stop_phase (PHASE_PLUGINS);
#endif /* BFD_SUPPORTS_PLUGINS */

  ld_start_phase (PHASE_PARSE);

  ldemul_set_symbols ();

  /* If we have not already opened and parsed a linker script,
     try the default script from command line first.  */
  if (saved_script_handle == NULL
      && command_line.default_script != NULL)
    {
      ldfile_open_script_file (command_line.default_script);
      parser_input = input_script;
      yyparse ();
    }

  /* If we have not already opened and parsed a linker script
     read the emulation's appropriate default script.  */
  if (saved_script_handle == NULL)
    {
      int isfile;
      char *s = ldemul_get_script (&isfile);

      if (isfile)
	ldfile_open_default_command_file (s);
      else
	{
	  lex_string = s;
	  lex_redirect (s, _("built in linker script"), 1);
	}
      parser_input = input_script;
      yyparse ();
      lex_string = NULL;
    }

  if (verbose)
    {
      if (saved_script_handle)
	info_msg (_("using external linker script: %s"), processed_scripts->name);
      else
	info_msg (_("using internal linker script:"));
      info_msg ("\n==================================================\n");

      if (saved_script_handle)
	display_external_script ();
      else
	{
	  int isfile;

	  info_msg (ldemul_get_script (&isfile));
	}

      info_msg ("\n==================================================\n");
    }

  if (command_line.section_ordering_file)
    {
      FILE *hold_script_handle;

      hold_script_handle = saved_script_handle;
      ldfile_open_command_file (command_line.section_ordering_file);
      if (verbose)
	display_external_script ();
      saved_script_handle = hold_script_handle;
      in_section_ordering = true;
      parser_input = input_section_ordering_script;
      yyparse ();
      in_section_ordering = false;

    }

  if (command_line.force_group_allocation
      || !bfd_link_relocatable (&link_info))
    link_info.resolve_section_groups = true;
  else
    link_info.resolve_section_groups = false;

  if (command_line.print_output_format)
    info_msg ("%s\n", lang_get_output_target ());

  lang_final ();

  /* If the only command line argument has been -v or --version or --verbose
     then ignore any input files provided by linker scripts and exit now.
     We do not want to create an output file when the linker is just invoked
     to provide version information.  */
  if (argc == 2 && version_printed)
    xexit (0);

  if (link_info.inhibit_common_definition && !bfd_link_dll (&link_info))
    fatal (_("%P: --no-define-common may not be used without -shared\n"));

  if (!lang_has_input_file)
    {
      if (version_printed || command_line.print_output_format)
	xexit (0);
      output_unknown_cmdline_warnings ();
      fatal (_("%P: no input files\n"));
    }

  if (verbose)
    info_msg (_("%P: mode %s\n"), emulation);

  ldemul_after_parse ();

  output_unknown_cmdline_warnings ();

  if (config.map_filename)
    {
      if (strcmp (config.map_filename, "-") == 0)
	{
	  config.map_file = stdout;
	}
      else
	{
	  config.map_file = fopen (config.map_filename, FOPEN_WT);
	  if (config.map_file == (FILE *) NULL)
	    {
	      bfd_set_error (bfd_error_system_call);
	      einfo (_("%P: cannot open map file %s: %E\n"),
		     config.map_filename);
	    }
	}
      link_info.has_map_file = true;
    }

  if (config.stats_filename != NULL)
    {
      if (config.map_filename != NULL
	  && strcmp (config.stats_filename, config.map_filename) == 0)
	config.stats_file = NULL;
      else if (strcmp (config.stats_filename, "-") == 0)
	config.stats_file = stdout;
      else
	{
	  if (config.stats_filename[0] == '+')
	    config.stats_file = fopen (config.stats_filename + 1, "a");
	  else
	    config.stats_file = fopen (config.stats_filename, "w");

	  if (config.stats_file == NULL)
	    einfo ("%P: Warning: failed to open resource record file: %s\n",
		   config.stats_filename);
	}
    }

  ld_stop_phase (PHASE_PARSE);

  ld_start_phase (PHASE_PROCESS);
  lang_process ();
  ld_stop_phase (PHASE_PROCESS);

  /* Print error messages for any missing symbols, for any warning
     symbols, and possibly multiple definitions.  */
  if (bfd_link_relocatable (&link_info))
    link_info.output_bfd->flags &= ~EXEC_P;
  else
    link_info.output_bfd->flags |= EXEC_P;

  flagword flags = 0;
  switch (config.compress_debug)
    {
    case COMPRESS_DEBUG_GNU_ZLIB:
      flags = BFD_COMPRESS;
      break;
    case COMPRESS_DEBUG_GABI_ZLIB:
      flags = BFD_COMPRESS | BFD_COMPRESS_GABI;
      break;
    case COMPRESS_DEBUG_ZSTD:
      flags = BFD_COMPRESS | BFD_COMPRESS_GABI | BFD_COMPRESS_ZSTD;
      break;
    default:
      break;
    }
  link_info.output_bfd->flags
    |= flags & bfd_applicable_file_flags (link_info.output_bfd);


  ld_start_phase (PHASE_WRITE);
  ldwrite ();
  ld_stop_phase (PHASE_WRITE);


  if (config.map_file != NULL)
    lang_map ();
  if (command_line.cref)
    output_cref (config.map_file != NULL ? config.map_file : stdout);
  if (nocrossref_list != NULL)
    check_nocrossrefs ();
  if (command_line.print_memory_usage)
    lang_print_memory_usage ();
#if 0
  {
    struct bfd_link_hash_entry *h;

    h = bfd_link_hash_lookup (link_info.hash, "__image_base__", 0,0,1);
    fprintf (stderr, "lookup = %p val %lx\n", h, h ? h->u.def.value : 1);
  }
#endif
  ldexp_finish (false);
  lang_finish ();

  if (config.dependency_file != NULL)
    write_dependency_file ();

  /* Even if we're producing relocatable output, some non-fatal errors should
     be reported in the exit status.  (What non-fatal errors, if any, do we
     want to ignore for relocatable output?)  */
  if (!config.make_executable && !force_make_executable)
    {
      if (verbose)
	einfo (_("%P: link errors found, deleting executable `%s'\n"),
	       output_filename);

      /* The file will be removed by ld_cleanup.  */
      xexit (1);
    }
  else
    {
      bfd *obfd = link_info.output_bfd;
      link_info.output_bfd = NULL;
      if (!bfd_close (obfd))
	fatal (_("%P: %s: final close failed: %E\n"), output_filename);

      link_info.output_bfd = NULL;

      /* If the --force-exe-suffix is enabled, and we're making an
	 executable file and it doesn't end in .exe, copy it to one
	 which does.  */
      if (!bfd_link_relocatable (&link_info)
	  && command_line.force_exe_suffix)
	{
	  int len = strlen (output_filename);

	  if (len < 4
	      || (strcasecmp (output_filename + len - 4, ".exe") != 0
		  && strcasecmp (output_filename + len - 4, ".dll") != 0))
	    {
	      FILE *src;
	      FILE *dst;
	      const int bsize = 4096;
	      char *buf = (char *) xmalloc (bsize);
	      int l;
	      char *dst_name = (char *) xmalloc (len + 5);

	      strcpy (dst_name, output_filename);
	      strcat (dst_name, ".exe");
	      src = fopen (output_filename, FOPEN_RB);
	      dst = fopen (dst_name, FOPEN_WB);

	      if (!src)
		fatal (_("%P: unable to open for source of copy `%s'\n"),
		       output_filename);
	      if (!dst)
		fatal (_("%P: unable to open for destination of copy `%s'\n"),
		       dst_name);
	      while ((l = fread (buf, 1, bsize, src)) > 0)
		{
		  int done = fwrite (buf, 1, l, dst);

		  if (done != l)
		    einfo (_("%P: error writing file `%s'\n"), dst_name);
		}

	      fclose (src);
	      if (fclose (dst) == EOF)
		einfo (_("%P: error closing file `%s'\n"), dst_name);
	      free (dst_name);
	      free (buf);
	    }
	}
    }

  if (config.emit_gnu_object_only)
    cmdline_emit_object_only_section ();

  ld_stop_phase (PHASE_ALL);

  if (config.stats)
    {
      report_phases (config.map_file, & start_seconds, saved_argv);

      if (config.stats_filename)
	{
	  report_phases (config.stats_file, & start_seconds, saved_argv);

	  if (config.stats_file != stdout && config.stats_file != stderr)
	    {
	      fclose (config.stats_file);
	      config.stats_file = NULL;
	    }
	}
      else /* This is for backwards compatibility.  */
	{
	  long run_time = get_run_time () - start_time;

	  fflush (stdout);
	  fprintf (stderr, _("%s: total time in link: %ld.%06ld\n"),
		   program_name, run_time / 1000000, run_time % 1000000);
	  fflush (stderr);
	}
    }

  /* Prevent ld_cleanup from deleting the output file.  */
  output_filename = NULL;

  freeargv (saved_argv);

  xexit (0);
  return 0;
}

/* If the configured sysroot is relocatable, try relocating it based on
   default prefix FROM.  Return the relocated directory if it exists,
   otherwise return null.  */

static char *
get_relative_sysroot (const char *from ATTRIBUTE_UNUSED)
{
#ifdef TARGET_SYSTEM_ROOT_RELOCATABLE
  char *path;
  struct stat s;

  path = make_relative_prefix (program_name, from, TARGET_SYSTEM_ROOT);
  if (path)
    {
      if (stat (path, &s) == 0 && S_ISDIR (s.st_mode))
	return path;
      free (path);
    }
#endif
  return 0;
}

/* Return the sysroot directory.  Return "" if no sysroot is being used.  */

static const char *
get_sysroot (int argc, char **argv)
{
  int i;
  const char *path = NULL;

  for (i = 1; i < argc; i++)
    if (startswith (argv[i], "--sysroot="))
      path = argv[i] + strlen ("--sysroot=");

  if (!path)
    path = get_relative_sysroot (BINDIR);

  if (!path)
    path = get_relative_sysroot (TOOLBINDIR);

  if (!path)
    path = TARGET_SYSTEM_ROOT;

  if (IS_DIR_SEPARATOR (*path) && path[1] == 0)
    path = "";

  return path;
}

/* We need to find any explicitly given emulation in order to initialize the
   state that's needed by the lex&yacc argument parser (parse_args).  */

static char *
get_emulation (int argc, char **argv)
{
  char *emulation;
  int i;

  emulation = getenv (EMULATION_ENVIRON);
  if (emulation == NULL)
    emulation = DEFAULT_EMULATION;

  for (i = 1; i < argc; i++)
    {
      if (startswith (argv[i], "-m"))
	{
	  if (argv[i][2] == '\0')
	    {
	      /* -m EMUL */
	      if (i < argc - 1)
		{
		  emulation = argv[i + 1];
		  i++;
		}
	      else
		fatal (_("%P: missing argument to -m\n"));
	    }
	  else if (strcmp (argv[i], "-mips1") == 0
		   || strcmp (argv[i], "-mips2") == 0
		   || strcmp (argv[i], "-mips3") == 0
		   || strcmp (argv[i], "-mips4") == 0
		   || strcmp (argv[i], "-mips5") == 0
		   || strcmp (argv[i], "-mips32") == 0
		   || strcmp (argv[i], "-mips32r2") == 0
		   || strcmp (argv[i], "-mips32r3") == 0
		   || strcmp (argv[i], "-mips32r5") == 0
		   || strcmp (argv[i], "-mips32r6") == 0
		   || strcmp (argv[i], "-mips64") == 0
		   || strcmp (argv[i], "-mips64r2") == 0
		   || strcmp (argv[i], "-mips64r3") == 0
		   || strcmp (argv[i], "-mips64r5") == 0
		   || strcmp (argv[i], "-mips64r6") == 0)
	    {
	      /* FIXME: The arguments -mips1, -mips2, -mips3, etc. are
		 passed to the linker by some MIPS compilers.  They
		 generally tell the linker to use a slightly different
		 library path.  Perhaps someday these should be
		 implemented as emulations; until then, we just ignore
		 the arguments and hope that nobody ever creates
		 emulations named ips1, ips2 or ips3.  */
	    }
	  else if (strcmp (argv[i], "-m486") == 0)
	    {
	      /* FIXME: The argument -m486 is passed to the linker on
		 some Linux systems.  Hope that nobody creates an
		 emulation named 486.  */
	    }
	  else
	    {
	      /* -mEMUL */
	      emulation = &argv[i][2];
	    }
	}
    }

  return emulation;
}

void
add_ysym (const char *name)
{
  if (link_info.notice_hash == NULL)
    {
      link_info.notice_hash
	= (struct bfd_hash_table *) xmalloc (sizeof (struct bfd_hash_table));
      if (!bfd_hash_table_init_n (link_info.notice_hash,
				  bfd_hash_newfunc,
				  sizeof (struct bfd_hash_entry),
				  61))
	fatal (_("%P: bfd_hash_table_init failed: %E\n"));
    }

  if (bfd_hash_lookup (link_info.notice_hash, name, true, true) == NULL)
    fatal (_("%P: bfd_hash_lookup failed: %E\n"));
}

void
add_ignoresym (struct bfd_link_info *info, const char *name)
{
  if (info->ignore_hash == NULL)
    {
      info->ignore_hash = xmalloc (sizeof (struct bfd_hash_table));
      if (!bfd_hash_table_init_n (info->ignore_hash,
				  bfd_hash_newfunc,
				  sizeof (struct bfd_hash_entry),
				  61))
	fatal (_("%P: bfd_hash_table_init failed: %E\n"));
    }

  if (bfd_hash_lookup (info->ignore_hash, name, true, true) == NULL)
    fatal (_("%P: bfd_hash_lookup failed: %E\n"));
}

/* Record a symbol to be wrapped, from the --wrap option.  */

void
add_wrap (const char *name)
{
  if (link_info.wrap_hash == NULL)
    {
      link_info.wrap_hash
	= (struct bfd_hash_table *) xmalloc (sizeof (struct bfd_hash_table));
      if (!bfd_hash_table_init_n (link_info.wrap_hash,
				  bfd_hash_newfunc,
				  sizeof (struct bfd_hash_entry),
				  61))
	fatal (_("%P: bfd_hash_table_init failed: %E\n"));
    }

  if (bfd_hash_lookup (link_info.wrap_hash, name, true, true) == NULL)
    fatal (_("%P: bfd_hash_lookup failed: %E\n"));
}

/* Handle the -retain-symbols-file option.  */

void
add_keepsyms_file (const char *filename)
{
  FILE *file;
  char *buf;
  size_t bufsize;
  int c;

  if (link_info.strip == strip_some)
    einfo (_("%X%P: error: duplicate retain-symbols-file\n"));

  file = fopen (filename, "r");
  if (file == NULL)
    {
      bfd_set_error (bfd_error_system_call);
      einfo ("%X%P: %s: %E\n", filename);
      return;
    }

  link_info.keep_hash = (struct bfd_hash_table *)
      xmalloc (sizeof (struct bfd_hash_table));
  if (!bfd_hash_table_init (link_info.keep_hash, bfd_hash_newfunc,
			    sizeof (struct bfd_hash_entry)))
    fatal (_("%P: bfd_hash_table_init failed: %E\n"));

  bufsize = 100;
  buf = (char *) xmalloc (bufsize);

  c = getc (file);
  while (c != EOF)
    {
      while (ISSPACE (c))
	c = getc (file);

      if (c != EOF)
	{
	  size_t len = 0;

	  while (!ISSPACE (c) && c != EOF)
	    {
	      buf[len] = c;
	      ++len;
	      if (len >= bufsize)
		{
		  bufsize *= 2;
		  buf = (char *) xrealloc (buf, bufsize);
		}
	      c = getc (file);
	    }

	  buf[len] = '\0';

	  if (bfd_hash_lookup (link_info.keep_hash, buf, true, true) == NULL)
	    fatal (_("%P: bfd_hash_lookup for insertion failed: %E\n"));
	}
    }

  if (link_info.strip != strip_none)
    einfo (_("%P: `-retain-symbols-file' overrides `-s' and `-S'\n"));

  free (buf);
  link_info.strip = strip_some;
  fclose (file);
}

/* Callbacks from the BFD linker routines.  */

/* This is called when BFD has decided to include an archive member in
   a link.  */

static bool
add_archive_element (struct bfd_link_info *info,
		     bfd *abfd,
		     const char *name,
		     bfd **subsbfd ATTRIBUTE_UNUSED)
{
  lang_input_statement_type *input;
  lang_input_statement_type *parent;
  lang_input_statement_type orig_input;

  input = (lang_input_statement_type *)
      xcalloc (1, sizeof (lang_input_statement_type));
  input->header.type = lang_input_statement_enum;
  input->filename = bfd_get_filename (abfd);
  input->local_sym_name = bfd_get_filename (abfd);
  input->the_bfd = abfd;

  /* Save the original data for trace files/tries below, as plugins
     (if enabled) may possibly alter it to point to a replacement
     BFD, but we still want to output the original BFD filename.  */
  orig_input = *input;
#if BFD_SUPPORTS_PLUGINS
  /* Don't claim a fat IR object if no IR object should be claimed.  */
  if (link_info.lto_plugin_active
      && (!no_more_claiming
	  || bfd_get_lto_type (abfd) != lto_fat_ir_object))
    {
      ld_start_phase (PHASE_PLUGINS);
      /* We must offer this archive member to the plugins to claim.  */
      plugin_maybe_claim (input);
      ld_stop_phase (PHASE_PLUGINS);

      if (input->flags.claimed)
	{
	  if (no_more_claiming)
	    {
	      /* Don't claim new IR symbols after all IR symbols have
		 been claimed.  */
	      if (verbose)
		info_msg ("%pI: no new IR symbols to claim\n",
			  &orig_input);
	      input->flags.claimed = 0;
	      return false;
	    }
	  input->flags.claim_archive = true;
	  *subsbfd = input->the_bfd;
	}
    }
  else
    cmdline_check_object_only_section (input->the_bfd, false);
#endif /* BFD_SUPPORTS_PLUGINS */

  if (link_info.input_bfds_tail == &input->the_bfd->link.next
      || input->the_bfd->link.next != NULL)
    {
      /* We have already loaded this element, and are attempting to
	 load it again.  This can happen when the archive map doesn't
	 match actual symbols defined by the element.  */
      free (input);
      bfd_set_error (bfd_error_malformed_archive);
      return false;
    }

  /* Set the file_chain pointer of archives to the last element loaded
     from the archive.  See ldlang.c:find_rescan_insertion.  */
  parent = bfd_usrdata (abfd->my_archive);
  if (parent != NULL && !parent->flags.reload)
    parent->next = input;

  ldlang_add_file (input);

  if (config.map_file != NULL)
    {
      static bool header_printed;
      struct bfd_link_hash_entry *h;
      bfd *from;
      int len;

      h = bfd_link_hash_lookup (info->hash, name, false, false, true);
      if (h == NULL
	  && info->pei386_auto_import
	  && startswith (name, "__imp_"))
	h = bfd_link_hash_lookup (info->hash, name + 6, false, false, true);

      if (h == NULL)
	from = NULL;
      else
	{
	  switch (h->type)
	    {
	    default:
	      from = NULL;
	      break;

	    case bfd_link_hash_defined:
	    case bfd_link_hash_defweak:
	      from = h->u.def.section->owner;
	      break;

	    case bfd_link_hash_undefined:
	    case bfd_link_hash_undefweak:
	      from = h->u.undef.abfd;
	      break;

	    case bfd_link_hash_common:
	      from = h->u.c.p->section->owner;
	      break;
	    }
	}

      if (!header_printed)
	{
	  minfo (_("Archive member included to satisfy reference by file (symbol)\n\n"));
	  header_printed = true;
	}

      if (abfd->my_archive == NULL
	  || bfd_is_thin_archive (abfd->my_archive))
	{
	  minfo ("%s", bfd_get_filename (abfd));
	  len = strlen (bfd_get_filename (abfd));
	}
      else
	{
	  minfo ("%s(%s)", bfd_get_filename (abfd->my_archive),
		 bfd_get_filename (abfd));
	  len = (strlen (bfd_get_filename (abfd->my_archive))
		 + strlen (bfd_get_filename (abfd))
		 + 2);
	}

      if (len >= 29)
	{
	  print_nl ();
	  len = 0;
	}
      print_spaces (30 - len);

      if (from != NULL)
	minfo ("%pB ", from);
      if (h != NULL)
	minfo ("(%pT)\n", h->root.string);
      else
	minfo ("(%s)\n", name);
    }

  if (verbose
      || trace_files > 1
      || (trace_files && bfd_is_thin_archive (orig_input.the_bfd->my_archive)))
    info_msg ("%pI\n", &orig_input);
  return true;
}

/* This is called when BFD has discovered a symbol which is defined
   multiple times.  */

static void
multiple_definition (struct bfd_link_info *info,
		     struct bfd_link_hash_entry *h,
		     bfd *nbfd,
		     asection *nsec,
		     bfd_vma nval)
{
  const char *name;
  bfd *obfd;
  asection *osec;
  bfd_vma oval;

  if (info->allow_multiple_definition)
    return;

  switch (h->type)
    {
    case bfd_link_hash_defined:
      osec = h->u.def.section;
      oval = h->u.def.value;
      obfd = h->u.def.section->owner;
      break;
    case bfd_link_hash_indirect:
      osec = bfd_ind_section_ptr;
      oval = 0;
      obfd = NULL;
      break;
    default:
      abort ();
    }

  /* Ignore a redefinition of an absolute symbol to the
     same value; it's harmless.  */
  if (h->type == bfd_link_hash_defined
      && bfd_is_abs_section (osec)
      && bfd_is_abs_section (nsec)
      && nval == oval)
    return;

  /* If either section has the output_section field set to
     bfd_abs_section_ptr, it means that the section is being
     discarded, and this is not really a multiple definition at all.
     FIXME: It would be cleaner to somehow ignore symbols defined in
     sections which are being discarded.  */
  if (!info->prohibit_multiple_definition_absolute
      && ((osec->output_section != NULL
	   && ! bfd_is_abs_section (osec)
	   && bfd_is_abs_section (osec->output_section))
	  || (nsec->output_section != NULL
	      && !bfd_is_abs_section (nsec)
	      && bfd_is_abs_section (nsec->output_section))))
    return;

  name = h->root.string;
  if (nbfd == NULL)
    {
      nbfd = obfd;
      nsec = osec;
      nval = oval;
      obfd = NULL;
    }
  if (info->warn_multiple_definition)
    einfo (_("%P: %C: warning: multiple definition of `%pT'"),
	   nbfd, nsec, nval, name);
  else
    einfo (_("%X%P: %C: multiple definition of `%pT'"),
	   nbfd, nsec, nval, name);
  if (obfd != NULL)
    einfo (_("; %D: first defined here"), obfd, osec, oval);
  einfo ("\n");

  if (RELAXATION_ENABLED_BY_USER)
    {
      einfo (_("%P: disabling relaxation; it will not work with multiple definitions\n"));
      DISABLE_RELAXATION;
    }
}

/* This is called when there is a definition of a common symbol, or
   when a common symbol is found for a symbol that is already defined,
   or when two common symbols are found.  We only do something if
   -warn-common was used.  */

static void
multiple_common (struct bfd_link_info *info ATTRIBUTE_UNUSED,
		 struct bfd_link_hash_entry *h,
		 bfd *nbfd,
		 enum bfd_link_hash_type ntype,
		 bfd_vma nsize)
{
  const char *name;
  bfd *obfd;
  enum bfd_link_hash_type otype;
  bfd_vma osize;

  if (!config.warn_common)
    return;

  name = h->root.string;
  otype = h->type;
  if (otype == bfd_link_hash_common)
    {
      obfd = h->u.c.p->section->owner;
      osize = h->u.c.size;
    }
  else if (otype == bfd_link_hash_defined
	   || otype == bfd_link_hash_defweak)
    {
      obfd = h->u.def.section->owner;
      osize = 0;
    }
  else
    {
      /* FIXME: It would nice if we could report the BFD which defined
	 an indirect symbol, but we don't have anywhere to store the
	 information.  */
      obfd = NULL;
      osize = 0;
    }

  if (ntype == bfd_link_hash_defined
      || ntype == bfd_link_hash_defweak
      || ntype == bfd_link_hash_indirect)
    {
      ASSERT (otype == bfd_link_hash_common);
      if (obfd != NULL)
	einfo (_("%P: %pB: warning: definition of `%pT' overriding common"
		 " from %pB\n"),
	       nbfd, name, obfd);
      else
	einfo (_("%P: %pB: warning: definition of `%pT' overriding common\n"),
	       nbfd, name);
    }
  else if (otype == bfd_link_hash_defined
	   || otype == bfd_link_hash_defweak
	   || otype == bfd_link_hash_indirect)
    {
      ASSERT (ntype == bfd_link_hash_common);
      if (obfd != NULL)
	einfo (_("%P: %pB: warning: common of `%pT' overridden by definition"
		 " from %pB\n"),
	       nbfd, name, obfd);
      else
	einfo (_("%P: %pB: warning: common of `%pT' overridden by definition\n"),
	       nbfd, name);
    }
  else
    {
      ASSERT (otype == bfd_link_hash_common && ntype == bfd_link_hash_common);
      if (osize > nsize)
	{
	  if (obfd != NULL)
	    einfo (_("%P: %pB: warning: common of `%pT' overridden"
		     " by larger common from %pB\n"),
		   nbfd, name, obfd);
	  else
	    einfo (_("%P: %pB: warning: common of `%pT' overridden"
		     " by larger common\n"),
		   nbfd, name);
	}
      else if (nsize > osize)
	{
	  if (obfd != NULL)
	    einfo (_("%P: %pB: warning: common of `%pT' overriding"
		     " smaller common from %pB\n"),
		   nbfd, name, obfd);
	  else
	    einfo (_("%P: %pB: warning: common of `%pT' overriding"
		     " smaller common\n"),
		   nbfd, name);
	}
      else
	{
	  if (obfd != NULL)
	    einfo (_("%P: %pB and %pB: warning: multiple common of `%pT'\n"),
		   nbfd, obfd, name);
	  else
	    einfo (_("%P: %pB: warning: multiple common of `%pT'\n"),
		   nbfd, name);
	}
    }
}

/* This is called when BFD has discovered a set element.  H is the
   entry in the linker hash table for the set.  SECTION and VALUE
   represent a value which should be added to the set.  */

static void
add_to_set (struct bfd_link_info *info ATTRIBUTE_UNUSED,
	    struct bfd_link_hash_entry *h,
	    bfd_reloc_code_real_type reloc,
	    bfd *abfd,
	    asection *section,
	    bfd_vma value)
{
  if (config.warn_constructors)
    einfo (_("%P: warning: global constructor %s used\n"),
	   h->root.string);

  if (!config.build_constructors)
    return;

  ldctor_add_set_entry (h, reloc, NULL, section, value);

  if (h->type == bfd_link_hash_new)
    {
      h->type = bfd_link_hash_undefined;
      h->u.undef.abfd = abfd;
      /* We don't call bfd_link_add_undef to add this to the list of
	 undefined symbols because we are going to define it
	 ourselves.  */
    }
}

/* This is called when BFD has discovered a constructor.  This is only
   called for some object file formats--those which do not handle
   constructors in some more clever fashion.  This is similar to
   adding an element to a set, but less general.  */

static void
constructor_callback (struct bfd_link_info *info,
		      bool constructor,
		      const char *name,
		      bfd *abfd,
		      asection *section,
		      bfd_vma value)
{
  char *s;
  struct bfd_link_hash_entry *h;
  char set_name[1 + sizeof "__CTOR_LIST__"];

  if (config.warn_constructors)
    einfo (_("%P: warning: global constructor %s used\n"), name);

  if (!config.build_constructors)
    return;

  /* Ensure that BFD_RELOC_CTOR exists now, so that we can give a
     useful error message.  */
  if (bfd_reloc_type_lookup (info->output_bfd, BFD_RELOC_CTOR) == NULL
      && (bfd_link_relocatable (info)
	  || bfd_reloc_type_lookup (abfd, BFD_RELOC_CTOR) == NULL))
    fatal (_("%P: BFD backend error: BFD_RELOC_CTOR unsupported\n"));

  s = set_name;
  if (bfd_get_symbol_leading_char (abfd) != '\0')
    *s++ = bfd_get_symbol_leading_char (abfd);
  if (constructor)
    strcpy (s, "__CTOR_LIST__");
  else
    strcpy (s, "__DTOR_LIST__");

  h = bfd_link_hash_lookup (info->hash, set_name, true, true, true);
  if (h == (struct bfd_link_hash_entry *) NULL)
    fatal (_("%P: bfd_link_hash_lookup failed: %E\n"));
  if (h->type == bfd_link_hash_new)
    {
      h->type = bfd_link_hash_undefined;
      h->u.undef.abfd = abfd;
      /* We don't call bfd_link_add_undef to add this to the list of
	 undefined symbols because we are going to define it
	 ourselves.  */
    }

  ldctor_add_set_entry (h, BFD_RELOC_CTOR, name, section, value);
}

/* A structure used by warning_callback to pass information through
   bfd_map_over_sections.  */

struct warning_callback_info
{
  bool found;
  const char *warning;
  const char *symbol;
  asymbol **asymbols;
};

/* Look through the relocs to see if we can find a plausible address
   for SYMBOL in ABFD.  Return TRUE if found.  Otherwise return FALSE.  */

static bool
symbol_warning (const char *warning, const char *symbol, bfd *abfd)
{
  struct warning_callback_info cinfo;

  if (!bfd_generic_link_read_symbols (abfd))
    fatal (_("%P: %pB: could not read symbols: %E\n"), abfd);

  cinfo.found = false;
  cinfo.warning = warning;
  cinfo.symbol = symbol;
  cinfo.asymbols = bfd_get_outsymbols (abfd);
  bfd_map_over_sections (abfd, warning_find_reloc, &cinfo);
  return cinfo.found;
}

/* This is called when there is a reference to a warning symbol.  */

static void
warning_callback (struct bfd_link_info *info ATTRIBUTE_UNUSED,
		  const char *warning,
		  const char *symbol,
		  bfd *abfd,
		  asection *section,
		  bfd_vma address)
{
  /* This is a hack to support warn_multiple_gp.  FIXME: This should
     have a cleaner interface, but what?  */
  if (!config.warn_multiple_gp
      && strcmp (warning, "using multiple gp values") == 0)
    return;

  if (section != NULL)
    einfo ("%P: %C: %s%s\n", abfd, section, address, _("warning: "), warning);
  else if (abfd == NULL)
    einfo ("%P: %s%s\n", _("warning: "), warning);
  else if (symbol == NULL)
    einfo ("%P: %pB: %s%s\n", abfd, _("warning: "), warning);
  else if (!symbol_warning (warning, symbol, abfd))
    {
      bfd *b;
      /* Search all input files for a reference to SYMBOL.  */
      for (b = info->input_bfds; b; b = b->link.next)
	if (b != abfd && symbol_warning (warning, symbol, b))
	  return;
      einfo ("%P: %pB: %s%s\n", abfd, _("warning: "), warning);
    }
}

/* This is called by warning_callback for each section.  It checks the
   relocs of the section to see if it can find a reference to the
   symbol which triggered the warning.  If it can, it uses the reloc
   to give an error message with a file and line number.  */

static void
warning_find_reloc (bfd *abfd, asection *sec, void *iarg)
{
  struct warning_callback_info *info = (struct warning_callback_info *) iarg;
  long relsize;
  arelent **relpp;
  long relcount;
  arelent **p, **pend;

  if (info->found)
    return;

  relsize = bfd_get_reloc_upper_bound (abfd, sec);
  if (relsize < 0)
    fatal (_("%P: %pB: could not read relocs: %E\n"), abfd);
  if (relsize == 0)
    return;

  relpp = (arelent **) xmalloc (relsize);
  relcount = bfd_canonicalize_reloc (abfd, sec, relpp, info->asymbols);
  if (relcount < 0)
    fatal (_("%P: %pB: could not read relocs: %E\n"), abfd);

  p = relpp;
  pend = p + relcount;
  for (; p < pend && *p != NULL; p++)
    {
      arelent *q = *p;

      if (q->sym_ptr_ptr != NULL
	  && *q->sym_ptr_ptr != NULL
	  && strcmp (bfd_asymbol_name (*q->sym_ptr_ptr), info->symbol) == 0)
	{
	  /* We found a reloc for the symbol we are looking for.  */
	  einfo ("%P: %H: %s%s\n", abfd, sec, q->address, _("warning: "),
		 info->warning);
	  info->found = true;
	  break;
	}
    }

  free (relpp);
}

#if SUPPORT_ERROR_HANDLING_SCRIPT
char * error_handling_script = NULL;
#endif

/* This is called when an undefined symbol is found.  */

static void
undefined_symbol (struct bfd_link_info *info,
		  const char *name,
		  bfd *abfd,
		  asection *section,
		  bfd_vma address,
		  bool error)
{
  static char *error_name;
  static unsigned int error_count;

#define MAX_ERRORS_IN_A_ROW 5

  if (info->ignore_hash != NULL
      && bfd_hash_lookup (info->ignore_hash, name, false, false) != NULL)
    return;

  if (config.warn_once)
    {
      /* Only warn once about a particular undefined symbol.  */
      add_ignoresym (info, name);
    }

  /* We never print more than a reasonable number of errors in a row
     for a single symbol.  */
  if (error_name != NULL
      && strcmp (name, error_name) == 0)
    ++error_count;
  else
    {
      error_count = 0;
      free (error_name);
      error_name = xstrdup (name);
    }

#if SUPPORT_ERROR_HANDLING_SCRIPT
  if (error_handling_script != NULL
      && error_count < MAX_ERRORS_IN_A_ROW)
    {
      char *        argv[4];
      const char *  res;
      int           status, err;

      argv[0] = error_handling_script;
      argv[1] = "undefined-symbol";
      argv[2] = (char *) name;
      argv[3] = NULL;
      
      if (verbose)
	einfo (_("%P: About to run error handling script '%s' with arguments: '%s' '%s'\n"),
	       argv[0], argv[1], argv[2]);

      res = pex_one (PEX_SEARCH, error_handling_script, argv,
		     N_("error handling script"),
		     NULL /* Send stdout to random, temp file.  */,
		     NULL /* Write to stderr.  */,
		     &status, &err);
      if (res != NULL)
	{
	  einfo (_("%P: Failed to run error handling script '%s', reason: "),
		 error_handling_script);
	  /* FIXME: We assume here that errrno == err.  */
	  perror (res);
	}
      /* We ignore the return status of the script and
	 carry on to issue the normal error message.  */
    }
#endif /* SUPPORT_ERROR_HANDLING_SCRIPT */
  
  if (section != NULL)
    {
      if (error_count < MAX_ERRORS_IN_A_ROW)
	{
	  if (error)
	    einfo (_("%X%P: %H: undefined reference to `%pT'\n"),
		   abfd, section, address, name);
	  else
	    einfo (_("%P: %H: warning: undefined reference to `%pT'\n"),
		   abfd, section, address, name);
	}
      else if (error_count == MAX_ERRORS_IN_A_ROW)
	{
	  if (error)
	    einfo (_("%X%P: %D: more undefined references to `%pT' follow\n"),
		   abfd, section, address, name);
	  else
	    einfo (_("%P: %D: warning: more undefined references to `%pT' follow\n"),
		   abfd, section, address, name);
	}
      else if (error)
	einfo ("%X");
    }
  else
    {
      if (error_count < MAX_ERRORS_IN_A_ROW)
	{
	  if (error)
	    einfo (_("%X%P: %pB: undefined reference to `%pT'\n"),
		   abfd, name);
	  else
	    einfo (_("%P: %pB: warning: undefined reference to `%pT'\n"),
		   abfd, name);
	}
      else if (error_count == MAX_ERRORS_IN_A_ROW)
	{
	  if (error)
	    einfo (_("%X%P: %pB: more undefined references to `%pT' follow\n"),
		   abfd, name);
	  else
	    einfo (_("%P: %pB: warning: more undefined references to `%pT' follow\n"),
		   abfd, name);
	}
      else if (error)
	einfo ("%X");
    }
}

/* Counter to limit the number of relocation overflow error messages
   to print.  Errors are printed as it is decremented.  When it's
   called and the counter is zero, a final message is printed
   indicating more relocations were omitted.  When it gets to -1, no
   such errors are printed.  If it's initially set to a value less
   than -1, all such errors will be printed (--verbose does this).  */

int overflow_cutoff_limit = 10;

/* This is called when a reloc overflows.  */

static void
reloc_overflow (struct bfd_link_info *info,
		struct bfd_link_hash_entry *entry,
		const char *name,
		const char *reloc_name,
		bfd_vma addend,
		bfd *abfd,
		asection *section,
		bfd_vma address)
{
  if (overflow_cutoff_limit == -1)
    return;

  einfo ("%X%H:", abfd, section, address);

  if (overflow_cutoff_limit >= 0
      && overflow_cutoff_limit-- == 0)
    {
      einfo (_(" additional relocation overflows omitted from the output\n"));
      return;
    }

  if (entry)
    {
      while (entry->type == bfd_link_hash_indirect
	     || entry->type == bfd_link_hash_warning)
	entry = entry->u.i.link;
      switch (entry->type)
	{
	case bfd_link_hash_undefined:
	case bfd_link_hash_undefweak:
	  einfo (_(" relocation truncated to fit: "
		   "%s against undefined symbol `%pT'"),
		 reloc_name, entry->root.string);
	  break;
	case bfd_link_hash_defined:
	case bfd_link_hash_defweak:
	  einfo (_(" relocation truncated to fit: "
		   "%s against symbol `%pT' defined in %pA section in %pB"),
		 reloc_name, entry->root.string,
		 entry->u.def.section,
		 entry->u.def.section == bfd_abs_section_ptr
		 ? info->output_bfd : entry->u.def.section->owner);
	  break;
	default:
	  abort ();
	  break;
	}
    }
  else
    einfo (_(" relocation truncated to fit: %s against `%pT'"),
	   reloc_name, name);
  if (addend != 0)
    einfo ("+%v", addend);
  einfo ("\n");
}

/* This is called when a dangerous relocation is made.  */

static void
reloc_dangerous (struct bfd_link_info *info ATTRIBUTE_UNUSED,
		 const char *message,
		 bfd *abfd,
		 asection *section,
		 bfd_vma address)
{
  einfo (_("%X%H: dangerous relocation: %s\n"),
	 abfd, section, address, message);
}

/* This is called when a reloc is being generated attached to a symbol
   that is not being output.  */

static void
unattached_reloc (struct bfd_link_info *info ATTRIBUTE_UNUSED,
		  const char *name,
		  bfd *abfd,
		  asection *section,
		  bfd_vma address)
{
  einfo (_("%X%H: reloc refers to symbol `%pT' which is not being output\n"),
	 abfd, section, address, name);
}

/* This is called if link_info.notice_all is set, or when a symbol in
   link_info.notice_hash is found.  Symbols are put in notice_hash
   using the -y option, while notice_all is set if the --cref option
   has been supplied, or if there are any NOCROSSREFS sections in the
   linker script; and if plugins are active, since they need to monitor
   all references from non-IR files.  */

static bool
notice (struct bfd_link_info *info,
	struct bfd_link_hash_entry *h,
	struct bfd_link_hash_entry *inh ATTRIBUTE_UNUSED,
	bfd *abfd,
	asection *section,
	bfd_vma value,
	flagword flags ATTRIBUTE_UNUSED)
{
  const char *name;

  if (h == NULL)
    {
      if (command_line.cref || nocrossref_list != NULL)
	return handle_asneeded_cref (abfd, (enum notice_asneeded_action) value);
      return true;
    }

  name = h->root.string;
  if (info->notice_hash != NULL
      && bfd_hash_lookup (info->notice_hash, name, false, false) != NULL)
    {
      if (bfd_is_und_section (section))
	einfo (_("%P: %pB: reference to %s\n"), abfd, name);
      else
	einfo (_("%P: %pB: definition of %s\n"), abfd, name);
    }

  if (command_line.cref || nocrossref_list != NULL)
    add_cref (name, abfd, section, value);

  return true;
}
