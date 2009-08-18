# This shell script emits a C file. -*- C -*-
#   Copyright 2006, 2007, 2008 Free Software Foundation, Inc.
#
# This file is part of the GNU Binutils.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
# MA 02110-1301, USA.
#

# This file is sourced from elf32.em, and defines extra spu specific
# features.
#
fragment <<EOF
#include "ldctor.h"
#include "elf32-spu.h"

/* Non-zero if no overlay processing should be done.  */
static int no_overlays = 0;

/* Non-zero if we want stubs on all calls out of overlay regions.  */
static int non_overlay_stubs = 0;

/* Whether to emit symbols for stubs.  */
static int emit_stub_syms = 0;

/* Non-zero to perform stack space analysis.  */
static int stack_analysis = 0;

/* Whether to emit symbols with stack requirements for each function.  */
static int emit_stack_syms = 0;

/* Range of valid addresses for loadable sections.  */
static bfd_vma local_store_lo = 0;
static bfd_vma local_store_hi = 0x3ffff;

/* Control --auto-overlay feature.  */
static int auto_overlay = 0;
static char *auto_overlay_file = 0;
static unsigned int auto_overlay_fixed = 0;
static unsigned int auto_overlay_reserved = 0;
static int extra_stack_space = 2000;
int my_argc;
char **my_argv;

static const char ovl_mgr[] = {
EOF

if ! cat ${srcdir}/emultempl/spu_ovl.o_c >> e${EMULATION_NAME}.c
then
  echo >&2 "Missing ${srcdir}/emultempl/spu_ovl.o_c"
  echo >&2 "You must build gas/as-new with --target=spu to build spu_ovl.o"
  exit 1
fi

fragment <<EOF
};

static const struct _ovl_stream ovl_mgr_stream = {
  ovl_mgr,
  ovl_mgr + sizeof (ovl_mgr)
};


static int
is_spu_target (void)
{
  extern const bfd_target bfd_elf32_spu_vec;

  return link_info.output_bfd->xvec == &bfd_elf32_spu_vec;
}

/* Create our note section.  */

static void
spu_after_open (void)
{
  if (is_spu_target ()
      && !link_info.relocatable
      && link_info.input_bfds != NULL
      && !spu_elf_create_sections (&link_info,
				   stack_analysis, emit_stack_syms))
    einfo ("%X%P: can not create note section: %E\n");

  gld${EMULATION_NAME}_after_open ();
}

/* If O is NULL, add section S at the end of output section OUTPUT_NAME.
   If O is not NULL, add section S at the beginning of output section O.

   Really, we should be duplicating ldlang.c map_input_to_output_sections
   logic here, ie. using the linker script to find where the section
   goes.  That's rather a lot of code, and we don't want to run
   map_input_to_output_sections again because most sections are already
   mapped.  So cheat, and put the section in a fixed place, ignoring any
   attempt via a linker script to put .stub, .ovtab, and built-in
   overlay manager code somewhere else.  */

static void
spu_place_special_section (asection *s, asection *o, const char *output_name)
{
  lang_output_section_statement_type *os;

  os = lang_output_section_find (o != NULL ? o->name : output_name);
  if (os == NULL)
    {
      const char *save = s->name;
      s->name = output_name;
      gld${EMULATION_NAME}_place_orphan (s);
      s->name = save;
    }
  else if (o != NULL && os->children.head != NULL)
    {
      lang_statement_list_type add;

      lang_list_init (&add);
      lang_add_section (&add, s, os);
      *add.tail = os->children.head;
      os->children.head = add.head;
    }
  else
    lang_add_section (&os->children, s, os);

  s->output_section->size += s->size;
}

/* Load built-in overlay manager, and tweak overlay section alignment.  */

static void
spu_elf_load_ovl_mgr (void)
{
  lang_output_section_statement_type *os;
  struct elf_link_hash_entry *h;

  h = elf_link_hash_lookup (elf_hash_table (&link_info),
			    "__ovly_load", FALSE, FALSE, FALSE);

  if (h != NULL
      && (h->root.type == bfd_link_hash_defined
	  || h->root.type == bfd_link_hash_defweak)
      && h->def_regular)
    {
      /* User supplied __ovly_load.  */
    }
  else if (ovl_mgr_stream.start == ovl_mgr_stream.end)
    einfo ("%F%P: no built-in overlay manager\n");
  else
    {
      lang_input_statement_type *ovl_is;

      ovl_is = lang_add_input_file ("builtin ovl_mgr",
				    lang_input_file_is_file_enum,
				    NULL);

      if (!spu_elf_open_builtin_lib (&ovl_is->the_bfd, &ovl_mgr_stream))
	einfo ("%X%P: can not open built-in overlay manager: %E\n");
      else
	{
	  asection *in;

	  if (!load_symbols (ovl_is, NULL))
	    einfo ("%X%P: can not load built-in overlay manager: %E\n");

	  /* Map overlay manager sections to output sections.  */
	  for (in = ovl_is->the_bfd->sections; in != NULL; in = in->next)
	    if ((in->flags & (SEC_ALLOC | SEC_LOAD))
		== (SEC_ALLOC | SEC_LOAD))
	      spu_place_special_section (in, NULL, ".text");
	}
    }

  /* Ensure alignment of overlay sections is sufficient.  */
  for (os = &lang_output_section_statement.head->output_section_statement;
       os != NULL;
       os = os->next)
    if (os->bfd_section != NULL
	&& spu_elf_section_data (os->bfd_section) != NULL
	&& spu_elf_section_data (os->bfd_section)->u.o.ovl_index != 0)
      {
	if (os->bfd_section->alignment_power < 4)
	  os->bfd_section->alignment_power = 4;

	/* Also ensure size rounds up.  */
	os->block_value = 16;
      }
}

/* Go find if we need to do anything special for overlays.  */

static void
spu_before_allocation (void)
{
  if (is_spu_target ()
      && !link_info.relocatable
      && !no_overlays)
    {
      /* Size the sections.  This is premature, but we need to know the
	 rough layout so that overlays can be found.  */
      expld.phase = lang_mark_phase_enum;
      expld.dataseg.phase = exp_dataseg_none;
      one_lang_size_sections_pass (NULL, TRUE);

      /* Find overlays by inspecting section vmas.  */
      if (spu_elf_find_overlays (&link_info))
	{
	  int ret;

	  if (auto_overlay != 0)
	    {
	      einfo ("%P: --auto-overlay ignored with user overlay script\n");
	      auto_overlay = 0;
	    }

	  ret = spu_elf_size_stubs (&link_info,
				    spu_place_special_section,
				    non_overlay_stubs);
	  if (ret == 0)
	    einfo ("%X%P: can not size overlay stubs: %E\n");
	  else if (ret == 2)
	    spu_elf_load_ovl_mgr ();
	}

      /* We must not cache anything from the preliminary sizing.  */
      lang_reset_memory_regions ();
    }

  gld${EMULATION_NAME}_before_allocation ();
}

struct tflist {
  struct tflist *next;
  char name[9];
};

static struct tflist *tmp_file_list;

static void clean_tmp (void)
{
  for (; tmp_file_list != NULL; tmp_file_list = tmp_file_list->next)
    unlink (tmp_file_list->name);
}

static int
new_tmp_file (char **fname)
{
  struct tflist *tf;
  int fd;

  if (tmp_file_list == NULL)
    atexit (clean_tmp);
  tf = xmalloc (sizeof (*tf));
  tf->next = tmp_file_list;
  tmp_file_list = tf;
  memcpy (tf->name, "ldXXXXXX", sizeof (tf->name));
  *fname = tf->name;
#ifdef HAVE_MKSTEMP
  fd = mkstemp (*fname);
#else
  *fname = mktemp (*fname);
  if (*fname == NULL)
    return -1;
  fd = open (fname, O_RDWR | O_CREAT | O_EXCL, 0600);
#endif
  return fd;
}

static FILE *
spu_elf_open_overlay_script (void)
{
  FILE *script = NULL;

  if (auto_overlay_file == NULL)
    {
      int fd = new_tmp_file (&auto_overlay_file);
      if (fd == -1)
	goto file_err;
      script = fdopen (fd, "w");
    }
  else
    script = fopen (auto_overlay_file, "w");

  if (script == NULL)
    {
    file_err:
      einfo ("%F%P: can not open script: %E\n");
    }
  return script;
}

static void
spu_elf_relink (void)
{
  char **argv = xmalloc ((my_argc + 4) * sizeof (*argv));

  memcpy (argv, my_argv, my_argc * sizeof (*argv));
  argv[my_argc++] = "--no-auto-overlay";
  if (tmp_file_list->name == auto_overlay_file)
    argv[my_argc - 1] = concat (argv[my_argc - 1], "=",
				auto_overlay_file, (const char *) NULL);
  argv[my_argc++] = "-T";
  argv[my_argc++] = auto_overlay_file;
  argv[my_argc] = 0;
  execvp (argv[0], (char *const *) argv);
  perror (argv[0]);
  _exit (127);
}

/* Final emulation specific call.  */

static void
gld${EMULATION_NAME}_finish (void)
{
  int need_laying_out;

  need_laying_out = bfd_elf_discard_info (link_info.output_bfd, &link_info);

  gld${EMULATION_NAME}_map_segments (need_laying_out);

  if (is_spu_target ())
    {
      if (local_store_lo < local_store_hi)
        {
	  asection *s;

	  s = spu_elf_check_vma (&link_info, auto_overlay,
				 local_store_lo, local_store_hi,
				 auto_overlay_fixed, auto_overlay_reserved,
				 extra_stack_space,
				 spu_elf_load_ovl_mgr,
				 spu_elf_open_overlay_script,
				 spu_elf_relink);
	  if (s != NULL && !auto_overlay)
	    einfo ("%X%P: %A exceeds local store range\n", s);
	}
      else if (auto_overlay)
	einfo ("%P: --auto-overlay ignored with zero local store range\n");

      if (!spu_elf_build_stubs (&link_info,
				emit_stub_syms || link_info.emitrelocations))
	einfo ("%F%P: can not build overlay stubs: %E\n");
    }

  finish_default ();
}

static char *
gld${EMULATION_NAME}_choose_target (int argc, char *argv[])
{
  my_argc = argc;
  my_argv = argv;
  return ldemul_default_target (argc, argv);
}

EOF

if grep -q 'ld_elf.*ppc.*_emulation' ldemul-list.h; then
  fragment <<EOF
#include "filenames.h"
#include <fcntl.h>
#include <sys/wait.h>

static const char *
base_name (const char *path)
{
  const char *file = strrchr (path, '/');
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
  {
    char *bslash = strrchr (path, '\\\\');

    if (file == NULL || (bslash != NULL && bslash > file))
      file = bslash;
    if (file == NULL
	&& path[0] != '\0'
	&& path[1] == ':')
      file = path + 1;
  }
#endif
  if (file == NULL)
    file = path;
  else
    ++file;
  return file;
}

/* This function is called when building a ppc32 or ppc64 executable
   to handle embedded spu images.  */
extern bfd_boolean embedded_spu_file (lang_input_statement_type *, const char *);

bfd_boolean
embedded_spu_file (lang_input_statement_type *entry, const char *flags)
{
  const char *cmd[6];
  const char *sym;
  char *handle, *p;
  char *oname;
  int fd;
  pid_t pid;
  int status;
  union lang_statement_union **old_stat_tail;
  union lang_statement_union **old_file_tail;
  union lang_statement_union *new_ent;
  lang_input_statement_type *search;

  if (entry->the_bfd->format != bfd_object
      || strcmp (entry->the_bfd->xvec->name, "elf32-spu") != 0
      || (entry->the_bfd->tdata.elf_obj_data->elf_header->e_type != ET_EXEC
	  && entry->the_bfd->tdata.elf_obj_data->elf_header->e_type != ET_DYN))
    return FALSE;

  /* Use the filename as the symbol marking the program handle struct.  */
  sym = base_name (entry->the_bfd->filename);

  handle = xstrdup (sym);
  for (p = handle; *p; ++p)
    if (!(ISALNUM (*p) || *p == '$' || *p == '.'))
      *p = '_';

  fd = new_tmp_file (&oname);
  if (fd == -1)
    return FALSE;
  close (fd);

  for (search = (lang_input_statement_type *) input_file_chain.head;
       search != NULL;
       search = (lang_input_statement_type *) search->next_real_file)
    if (search->filename != NULL)
      {
	const char *infile = base_name (search->filename);

	if (strncmp (infile, "crtbegin", 8) == 0)
	  {
	    if (infile[8] == 'S')
	      flags = concat (flags, " -fPIC", (const char *) NULL);
	    else if (infile[8] == 'T')
	      flags = concat (flags, " -fpie", (const char *) NULL);
	    break;
	  }
      }

  /* Use fork() and exec() rather than system() so that we don't
     need to worry about quoting args.  */
  cmd[0] = EMBEDSPU;
  cmd[1] = flags;
  cmd[2] = handle;
  cmd[3] = entry->the_bfd->filename;
  cmd[4] = oname;
  cmd[5] = NULL;
  if (trace_file_tries)
    {
      info_msg (_("running: %s \"%s\" \"%s\" \"%s\" \"%s\"\n"),
		cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
      fflush (stdout);
    }

  pid = fork ();
  if (pid == -1)
    return FALSE;
  if (pid == 0)
    {
      execvp (cmd[0], (char *const *) cmd);
      if (strcmp ("embedspu", EMBEDSPU) != 0)
	{
	  cmd[0] = "embedspu";
	  execvp (cmd[0], (char *const *) cmd);
	}
      perror (cmd[0]);
      _exit (127);
    }
#ifdef HAVE_WAITPID
#define WAITFOR(PID, STAT) waitpid (PID, STAT, 0)
#else
#define WAITFOR(PID, STAT) wait (STAT)
#endif
  if (WAITFOR (pid, &status) != pid
      || !WIFEXITED (status)
      || WEXITSTATUS (status) != 0)
    return FALSE;
#undef WAITFOR

  old_stat_tail = stat_ptr->tail;
  old_file_tail = input_file_chain.tail;
  if (lang_add_input_file (oname, lang_input_file_is_file_enum, NULL) == NULL)
    return FALSE;

  /* lang_add_input_file put the new list entry at the end of the statement
     and input file lists.  Move it to just after the current entry.  */
  new_ent = *old_stat_tail;
  *old_stat_tail = NULL;
  stat_ptr->tail = old_stat_tail;
  *old_file_tail = NULL;
  input_file_chain.tail = old_file_tail;
  new_ent->header.next = entry->header.next;
  entry->header.next = new_ent;
  new_ent->input_statement.next_real_file = entry->next_real_file;
  entry->next_real_file = new_ent;

  /* Ensure bfd sections are excluded from the output.  */
  bfd_section_list_clear (entry->the_bfd);
  entry->loaded = TRUE;
  return TRUE;
}

EOF
fi

# Define some shell vars to insert bits of code into the standard elf
# parse_args and list_options functions.
#
PARSE_AND_LIST_PROLOGUE='
#define OPTION_SPU_PLUGIN		301
#define OPTION_SPU_NO_OVERLAYS		(OPTION_SPU_PLUGIN + 1)
#define OPTION_SPU_STUB_SYMS		(OPTION_SPU_NO_OVERLAYS + 1)
#define OPTION_SPU_NON_OVERLAY_STUBS	(OPTION_SPU_STUB_SYMS + 1)
#define OPTION_SPU_LOCAL_STORE		(OPTION_SPU_NON_OVERLAY_STUBS + 1)
#define OPTION_SPU_STACK_ANALYSIS	(OPTION_SPU_LOCAL_STORE + 1)
#define OPTION_SPU_STACK_SYMS		(OPTION_SPU_STACK_ANALYSIS + 1)
#define OPTION_SPU_AUTO_OVERLAY		(OPTION_SPU_STACK_SYMS + 1)
#define OPTION_SPU_AUTO_RELINK		(OPTION_SPU_AUTO_OVERLAY + 1)
#define OPTION_SPU_OVERLAY_RODATA	(OPTION_SPU_AUTO_RELINK + 1)
#define OPTION_SPU_FIXED_SPACE		(OPTION_SPU_OVERLAY_RODATA + 1)
#define OPTION_SPU_RESERVED_SPACE	(OPTION_SPU_FIXED_SPACE + 1)
#define OPTION_SPU_EXTRA_STACK		(OPTION_SPU_RESERVED_SPACE + 1)
#define OPTION_SPU_NO_AUTO_OVERLAY	(OPTION_SPU_EXTRA_STACK + 1)
'

PARSE_AND_LIST_LONGOPTS='
  { "plugin", no_argument, NULL, OPTION_SPU_PLUGIN },
  { "no-overlays", no_argument, NULL, OPTION_SPU_NO_OVERLAYS },
  { "emit-stub-syms", no_argument, NULL, OPTION_SPU_STUB_SYMS },
  { "extra-overlay-stubs", no_argument, NULL, OPTION_SPU_NON_OVERLAY_STUBS },
  { "local-store", required_argument, NULL, OPTION_SPU_LOCAL_STORE },
  { "stack-analysis", no_argument, NULL, OPTION_SPU_STACK_ANALYSIS },
  { "emit-stack-syms", no_argument, NULL, OPTION_SPU_STACK_SYMS },
  { "auto-overlay", optional_argument, NULL, OPTION_SPU_AUTO_OVERLAY },
  { "auto-relink", no_argument, NULL, OPTION_SPU_AUTO_RELINK },
  { "overlay-rodata", no_argument, NULL, OPTION_SPU_OVERLAY_RODATA },
  { "fixed-space", required_argument, NULL, OPTION_SPU_FIXED_SPACE },
  { "reserved-space", required_argument, NULL, OPTION_SPU_RESERVED_SPACE },
  { "extra-stack-space", required_argument, NULL, OPTION_SPU_EXTRA_STACK },
  { "no-auto-overlay", optional_argument, NULL, OPTION_SPU_NO_AUTO_OVERLAY },
'

PARSE_AND_LIST_OPTIONS='
  fprintf (file, _("\
  --plugin                    Make SPU plugin.\n\
  --no-overlays               No overlay handling.\n\
  --emit-stub-syms            Add symbols on overlay call stubs.\n\
  --extra-overlay-stubs       Add stubs on all calls out of overlay regions.\n\
  --local-store=lo:hi         Valid address range.\n\
  --stack-analysis            Estimate maximum stack requirement.\n\
  --emit-stack-syms           Add sym giving stack needed for each func.\n\
  --auto-overlay [=filename]  Create an overlay script in filename if\n\
                              executable does not fit in local store.\n\
  --auto-relink               Rerun linker using auto-overlay script.\n\
  --overlay-rodata            Place read-only data with associated function\n\
                              code in overlays.\n\
  --fixed-space=bytes         Local store for non-overlay code and data.\n\
  --reserved-space=bytes      Local store for stack and heap.  If not specified\n\
                              ld will estimate stack size and assume no heap.\n\
  --extra-stack-space=bytes   Space for negative sp access (default 2000) if\n\
                              --reserved-space not given.\n"
		   ));
'

PARSE_AND_LIST_ARGS_CASES='
    case OPTION_SPU_PLUGIN:
      spu_elf_plugin (1);
      break;

    case OPTION_SPU_NO_OVERLAYS:
      no_overlays = 1;
      break;

    case OPTION_SPU_STUB_SYMS:
      emit_stub_syms = 1;
      break;

    case OPTION_SPU_NON_OVERLAY_STUBS:
      non_overlay_stubs = 1;
      break;

    case OPTION_SPU_LOCAL_STORE:
      {
	char *end;
	local_store_lo = strtoul (optarg, &end, 0);
	if (*end == '\'':'\'')
	  {
	    local_store_hi = strtoul (end + 1, &end, 0);
	    if (*end == 0)
	      break;
	  }
	einfo (_("%P%F: invalid --local-store address range `%s'\''\n"), optarg);
      }
      break;

    case OPTION_SPU_STACK_ANALYSIS:
      stack_analysis = 1;
      break;

    case OPTION_SPU_STACK_SYMS:
      emit_stack_syms = 1;
      break;

    case OPTION_SPU_AUTO_OVERLAY:
      auto_overlay |= 1;
      if (optarg != NULL)
	{
	  auto_overlay_file = optarg;
	  break;
	}
      /* Fall thru */

    case OPTION_SPU_AUTO_RELINK:
      auto_overlay |= 2;
      break;

    case OPTION_SPU_OVERLAY_RODATA:
      auto_overlay |= 4;
      break;

    case OPTION_SPU_FIXED_SPACE:
      {
	char *end;
	auto_overlay_fixed = strtoul (optarg, &end, 0);
	if (*end != 0)
	  einfo (_("%P%F: invalid --fixed-space value `%s'\''\n"), optarg);
      }
      break;

    case OPTION_SPU_RESERVED_SPACE:
      {
	char *end;
	auto_overlay_reserved = strtoul (optarg, &end, 0);
	if (*end != 0)
	  einfo (_("%P%F: invalid --reserved-space value `%s'\''\n"), optarg);
      }
      break;

    case OPTION_SPU_EXTRA_STACK:
      {
	char *end;
	extra_stack_space = strtol (optarg, &end, 0);
	if (*end != 0)
	  einfo (_("%P%F: invalid --extra-stack-space value `%s'\''\n"), optarg);
      }
      break;

    case OPTION_SPU_NO_AUTO_OVERLAY:
      auto_overlay = 0;
      if (optarg != NULL)
	{
	  struct tflist *tf;
	  size_t len;

	  if (tmp_file_list == NULL)
	    atexit (clean_tmp);

	  len = strlen (optarg) + 1;
	  tf = xmalloc (sizeof (*tf) - sizeof (tf->name) + len);
	  memcpy (tf->name, optarg, len);
	  tf->next = tmp_file_list;
	  tmp_file_list = tf;
	  break;
	}
      break;
'

LDEMUL_AFTER_OPEN=spu_after_open
LDEMUL_BEFORE_ALLOCATION=spu_before_allocation
LDEMUL_FINISH=gld${EMULATION_NAME}_finish
LDEMUL_CHOOSE_TARGET=gld${EMULATION_NAME}_choose_target
