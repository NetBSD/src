/* Plugin control for the GNU linker.
   Copyright 2010, 2011, 2012 Free Software Foundation, Inc.

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
#include "libiberty.h"
#include "bfd.h"
#include "bfdlink.h"
#include "bfdver.h"
#include "ld.h"
#include "ldmain.h"
#include "ldmisc.h"
#include "ldexp.h"
#include "ldlang.h"
#include "ldfile.h"
#include "plugin.h"
#include "plugin-api.h"
#include "elf-bfd.h"
#if !defined (HAVE_DLFCN_H) && defined (HAVE_WINDOWS_H)
#include <windows.h>
#endif

/* Report plugin symbols.  */
bfd_boolean report_plugin_symbols;

/* The suffix to append to the name of the real (claimed) object file
   when generating a dummy BFD to hold the IR symbols sent from the
   plugin.  For cosmetic use only; appears in maps, crefs etc.  */
#define IRONLY_SUFFIX " (symbol from plugin)"

/* Stores a single argument passed to a plugin.  */
typedef struct plugin_arg
{
  struct plugin_arg *next;
  const char *arg;
} plugin_arg_t;

/* Holds all details of a single plugin.  */
typedef struct plugin
{
  /* Next on the list of plugins, or NULL at end of chain.  */
  struct plugin *next;
  /* The argument string given to --plugin.  */
  const char *name;
  /* The shared library handle returned by dlopen.  */
  void *dlhandle;
  /* The list of argument string given to --plugin-opt.  */
  plugin_arg_t *args;
  /* Number of args in the list, for convenience.  */
  size_t n_args;
  /* The plugin's event handlers.  */
  ld_plugin_claim_file_handler claim_file_handler;
  ld_plugin_all_symbols_read_handler all_symbols_read_handler;
  ld_plugin_cleanup_handler cleanup_handler;
  /* TRUE if the cleanup handlers have been called.  */
  bfd_boolean cleanup_done;
} plugin_t;

/* The master list of all plugins.  */
static plugin_t *plugins_list = NULL;

/* We keep a tail pointer for easy linking on the end.  */
static plugin_t **plugins_tail_chain_ptr = &plugins_list;

/* The last plugin added to the list, for receiving args.  */
static plugin_t *last_plugin = NULL;

/* The tail of the arg chain of the last plugin added to the list.  */
static plugin_arg_t **last_plugin_args_tail_chain_ptr = NULL;

/* The plugin which is currently having a callback executed.  */
static plugin_t *called_plugin = NULL;

/* Last plugin to cause an error, if any.  */
static const char *error_plugin = NULL;

/* State of linker "notice" interface before we poked at it.  */
static bfd_boolean orig_notice_all;

/* Original linker callbacks, and the plugin version.  */
static const struct bfd_link_callbacks *orig_callbacks;
static struct bfd_link_callbacks plugin_callbacks;

/* Set at all symbols read time, to avoid recursively offering the plugin
   its own newly-added input files and libs to claim.  */
bfd_boolean no_more_claiming = FALSE;

/* List of tags to set in the constant leading part of the tv array. */
static const enum ld_plugin_tag tv_header_tags[] =
{
  LDPT_MESSAGE,
  LDPT_API_VERSION,
  LDPT_GNU_LD_VERSION,
  LDPT_LINKER_OUTPUT,
  LDPT_OUTPUT_NAME,
  LDPT_REGISTER_CLAIM_FILE_HOOK,
  LDPT_REGISTER_ALL_SYMBOLS_READ_HOOK,
  LDPT_REGISTER_CLEANUP_HOOK,
  LDPT_ADD_SYMBOLS,
  LDPT_GET_INPUT_FILE,
  LDPT_RELEASE_INPUT_FILE,
  LDPT_GET_SYMBOLS,
  LDPT_GET_SYMBOLS_V2,
  LDPT_ADD_INPUT_FILE,
  LDPT_ADD_INPUT_LIBRARY,
  LDPT_SET_EXTRA_LIBRARY_PATH
};

/* How many entries in the constant leading part of the tv array.  */
static const size_t tv_header_size = ARRAY_SIZE (tv_header_tags);

/* Forward references.  */
static bfd_boolean plugin_notice (struct bfd_link_info *,
				  struct bfd_link_hash_entry *, bfd *,
				  asection *, bfd_vma, flagword, const char *);

#if !defined (HAVE_DLFCN_H) && defined (HAVE_WINDOWS_H)

#define RTLD_NOW 0	/* Dummy value.  */

static void *
dlopen (const char *file, int mode ATTRIBUTE_UNUSED)
{
  return LoadLibrary (file);
}

static void *
dlsym (void *handle, const char *name)
{
  return GetProcAddress (handle, name);
}

static int
dlclose (void *handle)
{
  FreeLibrary (handle);
  return 0;
}

#endif /* !defined (HAVE_DLFCN_H) && defined (HAVE_WINDOWS_H)  */

#ifndef HAVE_DLFCN_H
static const char *
dlerror (void)
{
  return "";
}
#endif

/* Helper function for exiting with error status.  */
static int
set_plugin_error (const char *plugin)
{
  error_plugin = plugin;
  return -1;
}

/* Test if an error occurred.  */
static bfd_boolean
plugin_error_p (void)
{
  return error_plugin != NULL;
}

/* Return name of plugin which caused an error if any.  */
const char *
plugin_error_plugin (void)
{
  return error_plugin ? error_plugin : _("<no plugin>");
}

/* Handle -plugin arg: find and load plugin, or return error.  */
void
plugin_opt_plugin (const char *plugin)
{
  plugin_t *newplug;

  newplug = xmalloc (sizeof *newplug);
  memset (newplug, 0, sizeof *newplug);
  newplug->name = plugin;
  newplug->dlhandle = dlopen (plugin, RTLD_NOW);
  if (!newplug->dlhandle)
    einfo (_("%P%F: %s: error loading plugin: %s\n"), plugin, dlerror ());

  /* Chain on end, so when we run list it is in command-line order.  */
  *plugins_tail_chain_ptr = newplug;
  plugins_tail_chain_ptr = &newplug->next;

  /* Record it as current plugin for receiving args.  */
  last_plugin = newplug;
  last_plugin_args_tail_chain_ptr = &newplug->args;
}

/* Accumulate option arguments for last-loaded plugin, or return
   error if none.  */
int
plugin_opt_plugin_arg (const char *arg)
{
  plugin_arg_t *newarg;

  if (!last_plugin)
    return set_plugin_error (_("<no plugin>"));

  newarg = xmalloc (sizeof *newarg);
  newarg->arg = arg;
  newarg->next = NULL;

  /* Chain on end to preserve command-line order.  */
  *last_plugin_args_tail_chain_ptr = newarg;
  last_plugin_args_tail_chain_ptr = &newarg->next;
  last_plugin->n_args++;
  return 0;
}

/* Create a dummy BFD.  */
bfd *
plugin_get_ir_dummy_bfd (const char *name, bfd *srctemplate)
{
  bfd *abfd;

  bfd_use_reserved_id = 1;
  abfd = bfd_create (concat (name, IRONLY_SUFFIX, (const char *) NULL),
		     srctemplate);
  if (abfd != NULL)
    {
      abfd->flags |= BFD_LINKER_CREATED | BFD_PLUGIN;
      bfd_set_arch_info (abfd, bfd_get_arch_info (srctemplate));
      bfd_set_gp_size (abfd, bfd_get_gp_size (srctemplate));
      if (bfd_make_writable (abfd)
	  && bfd_copy_private_bfd_data (srctemplate, abfd))
	{
	  flagword flags;

	  /* Create section to own the symbols.  */
	  flags = (SEC_CODE | SEC_HAS_CONTENTS | SEC_READONLY
		   | SEC_ALLOC | SEC_LOAD | SEC_KEEP | SEC_EXCLUDE);
	  if (bfd_make_section_anyway_with_flags (abfd, ".text", flags))
	    return abfd;
	}
    }
  einfo (_("could not create dummy IR bfd: %F%E\n"));
  return NULL;
}

/* Check if the BFD passed in is an IR dummy object file.  */
static bfd_boolean
is_ir_dummy_bfd (const bfd *abfd)
{
  /* ABFD can sometimes legitimately be NULL, e.g. when called from one
     of the linker callbacks for a symbol in the *ABS* or *UND* sections.
     Likewise, the usrdata field may be NULL if ABFD was added by the
     backend without a corresponding input statement, as happens e.g.
     when processing DT_NEEDED dependencies.  */
  return (abfd
	  && abfd->usrdata
	  && ((lang_input_statement_type *)(abfd->usrdata))->flags.claimed);
}

/* Helpers to convert between BFD and GOLD symbol formats.  */
static enum ld_plugin_status
asymbol_from_plugin_symbol (bfd *abfd, asymbol *asym,
			    const struct ld_plugin_symbol *ldsym)
{
  flagword flags = BSF_NO_FLAGS;
  struct bfd_section *section;

  asym->the_bfd = abfd;
  asym->name = (ldsym->version
		? concat (ldsym->name, "@", ldsym->version, (const char *) NULL)
		: ldsym->name);
  asym->value = 0;
  switch (ldsym->def)
    {
    case LDPK_WEAKDEF:
      flags = BSF_WEAK;
      /* FALLTHRU */
    case LDPK_DEF:
      flags |= BSF_GLOBAL;
      if (ldsym->comdat_key)
	{
	  char *name = concat (".gnu.linkonce.t.", ldsym->comdat_key,
			       (const char *) NULL);
	  section = bfd_get_section_by_name (abfd, name);
	  if (section != NULL)
	    free (name);
	  else
	    {
	      flagword sflags;

	      sflags = (SEC_CODE | SEC_HAS_CONTENTS | SEC_READONLY
			| SEC_ALLOC | SEC_LOAD | SEC_KEEP | SEC_EXCLUDE
			| SEC_LINK_ONCE | SEC_LINK_DUPLICATES_DISCARD);
	      section = bfd_make_section_anyway_with_flags (abfd, name, sflags);
	      if (section == NULL)
		return LDPS_ERR;
	    }
	}
      else
	section = bfd_get_section_by_name (abfd, ".text");
      break;

    case LDPK_WEAKUNDEF:
      flags = BSF_WEAK;
      /* FALLTHRU */
    case LDPK_UNDEF:
      section = bfd_und_section_ptr;
      break;

    case LDPK_COMMON:
      flags = BSF_GLOBAL;
      section = bfd_com_section_ptr;
      asym->value = ldsym->size;
      /* For ELF targets, set alignment of common symbol to 1.  */
      if (bfd_get_flavour (abfd) == bfd_target_elf_flavour)
	{
	  ((elf_symbol_type *) asym)->internal_elf_sym.st_shndx = SHN_COMMON;
	  ((elf_symbol_type *) asym)->internal_elf_sym.st_value = 1;
	}
      break;

    default:
      return LDPS_ERR;
    }
  asym->flags = flags;
  asym->section = section;

  /* Visibility only applies on ELF targets.  */
  if (bfd_get_flavour (abfd) == bfd_target_elf_flavour)
    {
      elf_symbol_type *elfsym = elf_symbol_from (abfd, asym);
      unsigned char visibility;

      if (!elfsym)
	einfo (_("%P%F: %s: non-ELF symbol in ELF BFD!\n"), asym->name);
      switch (ldsym->visibility)
	{
	default:
	  einfo (_("%P%F: unknown ELF symbol visibility: %d!\n"),
		 ldsym->visibility);
	case LDPV_DEFAULT:
	  visibility = STV_DEFAULT;
	  break;
	case LDPV_PROTECTED:
	  visibility = STV_PROTECTED;
	  break;
	case LDPV_INTERNAL:
	  visibility = STV_INTERNAL;
	  break;
	case LDPV_HIDDEN:
	  visibility = STV_HIDDEN;
	  break;
	}
      elfsym->internal_elf_sym.st_other
	= (visibility | (elfsym->internal_elf_sym.st_other
			 & ~ELF_ST_VISIBILITY (-1)));
    }

  return LDPS_OK;
}

/* Register a claim-file handler.  */
static enum ld_plugin_status
register_claim_file (ld_plugin_claim_file_handler handler)
{
  ASSERT (called_plugin);
  called_plugin->claim_file_handler = handler;
  return LDPS_OK;
}

/* Register an all-symbols-read handler.  */
static enum ld_plugin_status
register_all_symbols_read (ld_plugin_all_symbols_read_handler handler)
{
  ASSERT (called_plugin);
  called_plugin->all_symbols_read_handler = handler;
  return LDPS_OK;
}

/* Register a cleanup handler.  */
static enum ld_plugin_status
register_cleanup (ld_plugin_cleanup_handler handler)
{
  ASSERT (called_plugin);
  called_plugin->cleanup_handler = handler;
  return LDPS_OK;
}

/* Add symbols from a plugin-claimed input file.  */
static enum ld_plugin_status
add_symbols (void *handle, int nsyms, const struct ld_plugin_symbol *syms)
{
  asymbol **symptrs;
  bfd *abfd = handle;
  int n;

  ASSERT (called_plugin);
  symptrs = xmalloc (nsyms * sizeof *symptrs);
  for (n = 0; n < nsyms; n++)
    {
      enum ld_plugin_status rv;
      asymbol *bfdsym;

      bfdsym = bfd_make_empty_symbol (abfd);
      symptrs[n] = bfdsym;
      rv = asymbol_from_plugin_symbol (abfd, bfdsym, syms + n);
      if (rv != LDPS_OK)
	return rv;
    }
  bfd_set_symtab (abfd, symptrs, nsyms);
  return LDPS_OK;
}

/* Get the input file information with an open (possibly re-opened)
   file descriptor.  */
static enum ld_plugin_status
get_input_file (const void *handle, struct ld_plugin_input_file *file)
{
  ASSERT (called_plugin);
  handle = handle;
  file = file;
  return LDPS_ERR;
}

/* Release the input file.  */
static enum ld_plugin_status
release_input_file (const void *handle)
{
  ASSERT (called_plugin);
  handle = handle;
  return LDPS_ERR;
}

/* Return TRUE if a defined symbol might be reachable from outside the
   universe of claimed objects.  */
static inline bfd_boolean
is_visible_from_outside (struct ld_plugin_symbol *lsym,
			 struct bfd_link_hash_entry *blhe)
{
  struct bfd_sym_chain *sym;

  if (link_info.relocatable)
    return TRUE;
  if (link_info.export_dynamic || !link_info.executable)
    {
      /* Check if symbol is hidden by version script.  */
      if (bfd_hide_sym_by_version (link_info.version_info,
				   blhe->root.string))
	return FALSE;
      /* Only ELF symbols really have visibility.  */
      if (bfd_get_flavour (link_info.output_bfd) == bfd_target_elf_flavour)
	{
	  struct elf_link_hash_entry *el = (struct elf_link_hash_entry *)blhe;
	  int vis = ELF_ST_VISIBILITY (el->other);
	  return vis == STV_DEFAULT || vis == STV_PROTECTED;
	}
      /* On non-ELF targets, we can safely make inferences by considering
	 what visibility the plugin would have liked to apply when it first
	 sent us the symbol.  During ELF symbol processing, visibility only
	 ever becomes more restrictive, not less, when symbols are merged,
	 so this is a conservative estimate; it may give false positives,
	 declaring something visible from outside when it in fact would
	 not have been, but this will only lead to missed optimisation
	 opportunities during LTRANS at worst; it will not give false
	 negatives, which can lead to the disastrous conclusion that the
	 related symbol is IRONLY.  (See GCC PR46319 for an example.)  */
      return (lsym->visibility == LDPV_DEFAULT
	      || lsym->visibility == LDPV_PROTECTED);
    }

  for (sym = &entry_symbol; sym != NULL; sym = sym->next)
    if (sym->name
	&& strcmp (sym->name, blhe->root.string) == 0)
      return TRUE;

  return FALSE;
}

/* Get the symbol resolution info for a plugin-claimed input file.  */
static enum ld_plugin_status
get_symbols (const void *handle, int nsyms, struct ld_plugin_symbol *syms,
	     int def_ironly_exp)
{
  const bfd *abfd = handle;
  int n;

  ASSERT (called_plugin);
  for (n = 0; n < nsyms; n++)
    {
      struct bfd_link_hash_entry *blhe;
      asection *owner_sec;
      int res;

      if (syms[n].def != LDPK_UNDEF)
	blhe = bfd_link_hash_lookup (link_info.hash, syms[n].name,
				     FALSE, FALSE, TRUE);
      else
	blhe = bfd_wrapped_link_hash_lookup (link_info.output_bfd, &link_info,
					     syms[n].name, FALSE, FALSE, TRUE);
      if (!blhe)
	{
	  res = LDPR_UNKNOWN;
	  goto report_symbol;
	}

      /* Determine resolution from blhe type and symbol's original type.  */
      if (blhe->type == bfd_link_hash_undefined
	  || blhe->type == bfd_link_hash_undefweak)
	{
	  res = LDPR_UNDEF;
	  goto report_symbol;
	}
      if (blhe->type != bfd_link_hash_defined
	  && blhe->type != bfd_link_hash_defweak
	  && blhe->type != bfd_link_hash_common)
	{
	  /* We should not have a new, indirect or warning symbol here.  */
	  einfo ("%P%F: %s: plugin symbol table corrupt (sym type %d)\n",
		 called_plugin->name, blhe->type);
	}

      /* Find out which section owns the symbol.  Since it's not undef,
	 it must have an owner; if it's not a common symbol, both defs
	 and weakdefs keep it in the same place. */
      owner_sec = (blhe->type == bfd_link_hash_common
		   ? blhe->u.c.p->section
		   : blhe->u.def.section);


      /* If it was originally undefined or common, then it has been
	 resolved; determine how.  */
      if (syms[n].def == LDPK_UNDEF
	  || syms[n].def == LDPK_WEAKUNDEF
	  || syms[n].def == LDPK_COMMON)
	{
	  if (owner_sec->owner == link_info.output_bfd)
	    res = LDPR_RESOLVED_EXEC;
	  else if (owner_sec->owner == abfd)
	    res = LDPR_PREVAILING_DEF_IRONLY;
	  else if (is_ir_dummy_bfd (owner_sec->owner))
	    res = LDPR_RESOLVED_IR;
	  else if (owner_sec->owner != NULL
		   && (owner_sec->owner->flags & DYNAMIC) != 0)
	    res = LDPR_RESOLVED_DYN;
	  else
	    res = LDPR_RESOLVED_EXEC;
	}

      /* Was originally def, or weakdef.  Does it prevail?  If the
	 owner is the original dummy bfd that supplied it, then this
	 is the definition that has prevailed.  */
      else if (owner_sec->owner == link_info.output_bfd)
	res = LDPR_PREEMPTED_REG;
      else if (owner_sec->owner == abfd)
	res = LDPR_PREVAILING_DEF_IRONLY;

      /* Was originally def, weakdef, or common, but has been pre-empted.  */
      else if (is_ir_dummy_bfd (owner_sec->owner))
	res = LDPR_PREEMPTED_IR;
      else
	res = LDPR_PREEMPTED_REG;

      if (res == LDPR_PREVAILING_DEF_IRONLY)
	{
	  /* We need to know if the sym is referenced from non-IR files.  Or
	     even potentially-referenced, perhaps in a future final link if
	     this is a partial one, perhaps dynamically at load-time if the
	     symbol is externally visible.  */
	  if (blhe->non_ir_ref)
	    res = LDPR_PREVAILING_DEF;
	  else if (is_visible_from_outside (&syms[n], blhe))
	    res = def_ironly_exp;
	}

    report_symbol:
      syms[n].resolution = res;
      if (report_plugin_symbols)
	einfo (_("%P: %B: symbol `%s' "
		 "definition: %d, visibility: %d, resolution: %d\n"),
	       abfd, syms[n].name,
	       syms[n].def, syms[n].visibility, res);
    }
  return LDPS_OK;
}

static enum ld_plugin_status
get_symbols_v1 (const void *handle, int nsyms, struct ld_plugin_symbol *syms)
{
  return get_symbols (handle, nsyms, syms, LDPR_PREVAILING_DEF);
}

static enum ld_plugin_status
get_symbols_v2 (const void *handle, int nsyms, struct ld_plugin_symbol *syms)
{
  return get_symbols (handle, nsyms, syms, LDPR_PREVAILING_DEF_IRONLY_EXP);
}

/* Add a new (real) input file generated by a plugin.  */
static enum ld_plugin_status
add_input_file (const char *pathname)
{
  ASSERT (called_plugin);
  if (!lang_add_input_file (xstrdup (pathname), lang_input_file_is_file_enum,
			    NULL))
    return LDPS_ERR;
  return LDPS_OK;
}

/* Add a new (real) library required by a plugin.  */
static enum ld_plugin_status
add_input_library (const char *pathname)
{
  ASSERT (called_plugin);
  if (!lang_add_input_file (xstrdup (pathname), lang_input_file_is_l_enum,
			    NULL))
    return LDPS_ERR;
  return LDPS_OK;
}

/* Set the extra library path to be used by libraries added via
   add_input_library.  */
static enum ld_plugin_status
set_extra_library_path (const char *path)
{
  ASSERT (called_plugin);
  ldfile_add_library_path (xstrdup (path), FALSE);
  return LDPS_OK;
}

/* Issue a diagnostic message from a plugin.  */
static enum ld_plugin_status
message (int level, const char *format, ...)
{
  va_list args;
  va_start (args, format);

  switch (level)
    {
    case LDPL_INFO:
      vfinfo (stdout, format, args, FALSE);
      putchar ('\n');
      break;
    case LDPL_WARNING:
      vfinfo (stdout, format, args, TRUE);
      putchar ('\n');
      break;
    case LDPL_FATAL:
    case LDPL_ERROR:
    default:
      {
	char *newfmt = ACONCAT ((level == LDPL_FATAL ? "%P%F: " : "%P%X: ",
				 format, "\n", (const char *) NULL));
	fflush (stdout);
	vfinfo (stderr, newfmt, args, TRUE);
	fflush (stderr);
      }
      break;
    }

  va_end (args);
  return LDPS_OK;
}

/* Helper to size leading part of tv array and set it up. */
static void
set_tv_header (struct ld_plugin_tv *tv)
{
  size_t i;

  /* Version info.  */
  static const unsigned int major = (unsigned)(BFD_VERSION / 100000000UL);
  static const unsigned int minor = (unsigned)(BFD_VERSION / 1000000UL) % 100;

  for (i = 0; i < tv_header_size; i++)
    {
      tv[i].tv_tag = tv_header_tags[i];
#define TVU(x) tv[i].tv_u.tv_ ## x
      switch (tv[i].tv_tag)
	{
	case LDPT_MESSAGE:
	  TVU(message) = message;
	  break;
	case LDPT_API_VERSION:
	  TVU(val) = LD_PLUGIN_API_VERSION;
	  break;
	case LDPT_GNU_LD_VERSION:
	  TVU(val) = major * 100 + minor;
	  break;
	case LDPT_LINKER_OUTPUT:
	  TVU(val) = (link_info.relocatable
		      ? LDPO_REL
		      : (link_info.executable
			 ? (link_info.pie ? LDPO_PIE : LDPO_EXEC)
			 : LDPO_DYN));
	  break;
	case LDPT_OUTPUT_NAME:
	  TVU(string) = output_filename;
	  break;
	case LDPT_REGISTER_CLAIM_FILE_HOOK:
	  TVU(register_claim_file) = register_claim_file;
	  break;
	case LDPT_REGISTER_ALL_SYMBOLS_READ_HOOK:
	  TVU(register_all_symbols_read) = register_all_symbols_read;
	  break;
	case LDPT_REGISTER_CLEANUP_HOOK:
	  TVU(register_cleanup) = register_cleanup;
	  break;
	case LDPT_ADD_SYMBOLS:
	  TVU(add_symbols) = add_symbols;
	  break;
	case LDPT_GET_INPUT_FILE:
	  TVU(get_input_file) = get_input_file;
	  break;
	case LDPT_RELEASE_INPUT_FILE:
	  TVU(release_input_file) = release_input_file;
	  break;
	case LDPT_GET_SYMBOLS:
	  TVU(get_symbols) = get_symbols_v1;
	  break;
	case LDPT_GET_SYMBOLS_V2:
	  TVU(get_symbols) = get_symbols_v2;
	  break;
	case LDPT_ADD_INPUT_FILE:
	  TVU(add_input_file) = add_input_file;
	  break;
	case LDPT_ADD_INPUT_LIBRARY:
	  TVU(add_input_library) = add_input_library;
	  break;
	case LDPT_SET_EXTRA_LIBRARY_PATH:
	  TVU(set_extra_library_path) = set_extra_library_path;
	  break;
	default:
	  /* Added a new entry to the array without adding
	     a new case to set up its value is a bug.  */
	  FAIL ();
	}
#undef TVU
    }
}

/* Append the per-plugin args list and trailing LDPT_NULL to tv.  */
static void
set_tv_plugin_args (plugin_t *plugin, struct ld_plugin_tv *tv)
{
  plugin_arg_t *arg = plugin->args;
  while (arg)
    {
      tv->tv_tag = LDPT_OPTION;
      tv->tv_u.tv_string = arg->arg;
      arg = arg->next;
      tv++;
    }
  tv->tv_tag = LDPT_NULL;
  tv->tv_u.tv_val = 0;
}

/* Return true if any plugins are active this run.  Only valid
   after options have been processed.  */
bfd_boolean
plugin_active_plugins_p (void)
{
  return plugins_list != NULL;
}

/* Load up and initialise all plugins after argument parsing.  */
void
plugin_load_plugins (void)
{
  struct ld_plugin_tv *my_tv;
  unsigned int max_args = 0;
  plugin_t *curplug = plugins_list;

  /* If there are no plugins, we need do nothing this run.  */
  if (!curplug)
    return;

  /* First pass over plugins to find max # args needed so that we
     can size and allocate the tv array.  */
  while (curplug)
    {
      if (curplug->n_args > max_args)
	max_args = curplug->n_args;
      curplug = curplug->next;
    }

  /* Allocate tv array and initialise constant part.  */
  my_tv = xmalloc ((max_args + 1 + tv_header_size) * sizeof *my_tv);
  set_tv_header (my_tv);

  /* Pass over plugins again, activating them.  */
  curplug = plugins_list;
  while (curplug)
    {
      enum ld_plugin_status rv;
      ld_plugin_onload onloadfn;

      onloadfn = (ld_plugin_onload) dlsym (curplug->dlhandle, "onload");
      if (!onloadfn)
	onloadfn = (ld_plugin_onload) dlsym (curplug->dlhandle, "_onload");
      if (!onloadfn)
        einfo (_("%P%F: %s: error loading plugin: %s\n"),
	       curplug->name, dlerror ());
      set_tv_plugin_args (curplug, &my_tv[tv_header_size]);
      called_plugin = curplug;
      rv = (*onloadfn) (my_tv);
      called_plugin = NULL;
      if (rv != LDPS_OK)
	einfo (_("%P%F: %s: plugin error: %d\n"), curplug->name, rv);
      curplug = curplug->next;
    }

  /* Since plugin(s) inited ok, assume they're going to want symbol
     resolutions, which needs us to track which symbols are referenced
     by non-IR files using the linker's notice callback.  */
  orig_notice_all = link_info.notice_all;
  orig_callbacks = link_info.callbacks;
  plugin_callbacks = *orig_callbacks;
  plugin_callbacks.notice = &plugin_notice;
  link_info.notice_all = TRUE;
  link_info.callbacks = &plugin_callbacks;
}

/* Call 'claim file' hook for all plugins.  */
static int
plugin_call_claim_file (const struct ld_plugin_input_file *file, int *claimed)
{
  plugin_t *curplug = plugins_list;
  *claimed = FALSE;
  if (no_more_claiming)
    return 0;
  while (curplug && !*claimed)
    {
      if (curplug->claim_file_handler)
	{
	  enum ld_plugin_status rv;
	  called_plugin = curplug;
	  rv = (*curplug->claim_file_handler) (file, claimed);
	  called_plugin = NULL;
	  if (rv != LDPS_OK)
	    set_plugin_error (curplug->name);
	}
      curplug = curplug->next;
    }
  return plugin_error_p () ? -1 : 0;
}

void
plugin_maybe_claim (struct ld_plugin_input_file *file,
		    lang_input_statement_type *entry)
{
  int claimed = 0;

  /* We create a dummy BFD, initially empty, to house whatever symbols
     the plugin may want to add.  */
  file->handle = plugin_get_ir_dummy_bfd (entry->the_bfd->filename,
					  entry->the_bfd);
  if (plugin_call_claim_file (file, &claimed))
    einfo (_("%P%F: %s: plugin reported error claiming file\n"),
	   plugin_error_plugin ());
  /* fd belongs to us, not the plugin; but we don't need it.  */
  close (file->fd);
  if (claimed)
    {
      /* Discard the real file's BFD and substitute the dummy one.  */

      /* BFD archive handling caches elements so we can't call
	 bfd_close for archives.  */
      if (entry->the_bfd->my_archive == NULL)
	bfd_close (entry->the_bfd);
      entry->the_bfd = file->handle;
      entry->flags.claimed = TRUE;
      bfd_make_readable (entry->the_bfd);
    }
  else
    {
      /* If plugin didn't claim the file, we don't need the dummy bfd.
	 Can't avoid speculatively creating it, alas.  */
      bfd_close_all_done (file->handle);
      entry->flags.claimed = FALSE;
    }
}

/* Call 'all symbols read' hook for all plugins.  */
int
plugin_call_all_symbols_read (void)
{
  plugin_t *curplug = plugins_list;

  /* Disable any further file-claiming.  */
  no_more_claiming = TRUE;

  while (curplug)
    {
      if (curplug->all_symbols_read_handler)
	{
	  enum ld_plugin_status rv;
	  called_plugin = curplug;
	  rv = (*curplug->all_symbols_read_handler) ();
	  called_plugin = NULL;
	  if (rv != LDPS_OK)
	    set_plugin_error (curplug->name);
	}
      curplug = curplug->next;
    }
  return plugin_error_p () ? -1 : 0;
}

/* Call 'cleanup' hook for all plugins at exit.  */
void
plugin_call_cleanup (void)
{
  plugin_t *curplug = plugins_list;
  while (curplug)
    {
      if (curplug->cleanup_handler && !curplug->cleanup_done)
	{
	  enum ld_plugin_status rv;
	  curplug->cleanup_done = TRUE;
	  called_plugin = curplug;
	  rv = (*curplug->cleanup_handler) ();
	  called_plugin = NULL;
	  if (rv != LDPS_OK)
	    info_msg (_("%P: %s: error in plugin cleanup: %d (ignored)\n"),
		      curplug->name, rv);
	  dlclose (curplug->dlhandle);
	}
      curplug = curplug->next;
    }
}

/* To determine which symbols should be resolved LDPR_PREVAILING_DEF
   and which LDPR_PREVAILING_DEF_IRONLY, we notice all the symbols as
   the linker adds them to the linker hash table.  Mark those
   referenced from a non-IR file with non_ir_ref.  We have to
   notice_all symbols, because we won't necessarily know until later
   which ones will be contributed by IR files.  */
static bfd_boolean
plugin_notice (struct bfd_link_info *info,
	       struct bfd_link_hash_entry *h,
	       bfd *abfd,
	       asection *section,
	       bfd_vma value,
	       flagword flags,
	       const char *string)
{
  if (h != NULL)
    {
      bfd *sym_bfd;

      /* No further processing if this def/ref is from an IR dummy BFD.  */
      if (is_ir_dummy_bfd (abfd))
	return TRUE;

      /* Making an indirect symbol counts as a reference unless this
	 is a brand new symbol.  */
      if (bfd_is_ind_section (section)
	  || (flags & BSF_INDIRECT) != 0)
	{
	  if (h->type != bfd_link_hash_new)
	    {
	      struct bfd_link_hash_entry *inh;

	      h->non_ir_ref = TRUE;
	      inh = bfd_wrapped_link_hash_lookup (abfd, info, string, FALSE,
						  FALSE, FALSE);
	      if (inh != NULL)
		inh->non_ir_ref = TRUE;
	    }
	}

      /* Nothing to do here for warning symbols.  */
      else if ((flags & BSF_WARNING) != 0)
	;

      /* Nothing to do here for constructor symbols.  */
      else if ((flags & BSF_CONSTRUCTOR) != 0)
	;

      /* If this is a ref, set non_ir_ref.  */
      else if (bfd_is_und_section (section))
	h->non_ir_ref = TRUE;

      /* Otherwise, it must be a new def.  Ensure any symbol defined
	 in an IR dummy BFD takes on a new value from a real BFD.
	 Weak symbols are not normally overridden by a new weak
	 definition, and strong symbols will normally cause multiple
	 definition errors.  Avoid this by making the symbol appear
	 to be undefined.  */
      else if (((h->type == bfd_link_hash_defweak
		 || h->type == bfd_link_hash_defined)
		&& is_ir_dummy_bfd (sym_bfd = h->u.def.section->owner))
	       || (h->type == bfd_link_hash_common
		   && is_ir_dummy_bfd (sym_bfd = h->u.c.p->section->owner)))
	{
	  h->type = bfd_link_hash_undefweak;
	  h->u.undef.abfd = sym_bfd;
	}
    }

  /* Continue with cref/nocrossref/trace-sym processing.  */
  if (h == NULL
      || orig_notice_all
      || (info->notice_hash != NULL
	  && bfd_hash_lookup (info->notice_hash, h->root.string,
			      FALSE, FALSE) != NULL))
    return (*orig_callbacks->notice) (info, h,
				      abfd, section, value, flags, string);
  return TRUE;
}

/* Return true if bfd is a dynamic library that should be reloaded.  */

bfd_boolean
plugin_should_reload (bfd *abfd)
{
  return ((abfd->flags & DYNAMIC) != 0
	  && bfd_get_flavour (abfd) == bfd_target_elf_flavour
	  && bfd_get_format (abfd) == bfd_object
	  && (elf_dyn_lib_class (abfd) & DYN_AS_NEEDED) != 0);
}
