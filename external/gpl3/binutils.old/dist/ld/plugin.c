/* Plugin control for the GNU linker.
   Copyright (C) 2010-2016 Free Software Foundation, Inc.

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
#include "libbfd.h"
#include "bfdlink.h"
#include "bfdver.h"
#include "ld.h"
#include "ldmain.h"
#include "ldmisc.h"
#include "ldexp.h"
#include "ldlang.h"
#include "ldfile.h"
#include "../bfd/plugin.h"
#include "plugin.h"
#include "plugin-api.h"
#include "elf-bfd.h"
#if HAVE_MMAP
# include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED ((void *) -1)
# endif
# ifndef PROT_READ
#  define PROT_READ 0
# endif
# ifndef MAP_PRIVATE
#  define MAP_PRIVATE 0
# endif
#endif
#include <errno.h>
#if !(defined(errno) || defined(_MSC_VER) && defined(_INC_ERRNO))
extern int errno;
#endif
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

typedef struct view_buffer
{
  char *addr;
  size_t filesize;
  off_t offset;
} view_buffer_t;

/* The internal version of struct ld_plugin_input_file with a BFD
   pointer.  */
typedef struct plugin_input_file
{
  bfd *abfd;
  view_buffer_t view_buffer;
  char *name;
  int fd;
  bfd_boolean use_mmap;
  off_t offset;
  off_t filesize;
} plugin_input_file_t;

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

#if HAVE_MMAP && HAVE_GETPAGESIZE
/* Page size used by mmap.  */
static off_t plugin_pagesize;
#endif

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
  LDPT_GET_VIEW,
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
				  struct bfd_link_hash_entry *,
				  struct bfd_link_hash_entry *,
				  bfd *, asection *, bfd_vma, flagword);

static const bfd_target * plugin_object_p (bfd *);

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

  /* Ignore -pass-through= from GCC driver.  */
  if (*arg == '-')
    {
      const char *p = arg + 1;

      if (*p == '-')
	++p;
      if (strncmp (p, "pass-through=", 13) == 0)
	return 0;
    }

  newarg = xmalloc (sizeof *newarg);
  newarg->arg = arg;
  newarg->next = NULL;

  /* Chain on end to preserve command-line order.  */
  *last_plugin_args_tail_chain_ptr = newarg;
  last_plugin_args_tail_chain_ptr = &newarg->next;
  last_plugin->n_args++;
  return 0;
}

/* Generate a dummy BFD to represent an IR file, for any callers of
   plugin_call_claim_file to use as the handle in the ld_plugin_input_file
   struct that they build to pass in.  The BFD is initially writable, so
   that symbols can be added to it; it must be made readable after the
   add_symbols hook has been called so that it can be read when linking.  */
static bfd *
plugin_get_ir_dummy_bfd (const char *name, bfd *srctemplate)
{
  bfd *abfd;
  bfd_boolean bfd_plugin_target;

  bfd_use_reserved_id = 1;
  bfd_plugin_target = bfd_plugin_target_p (srctemplate->xvec);
  abfd = bfd_create (concat (name, IRONLY_SUFFIX, (const char *) NULL),
		     bfd_plugin_target ? link_info.output_bfd : srctemplate);
  if (abfd != NULL)
    {
      abfd->flags |= BFD_LINKER_CREATED | BFD_PLUGIN;
      if (!bfd_make_writable (abfd))
	goto report_error;
      if (!bfd_plugin_target)
	{
	  bfd_set_arch_info (abfd, bfd_get_arch_info (srctemplate));
	  bfd_set_gp_size (abfd, bfd_get_gp_size (srctemplate));
	  if (!bfd_copy_private_bfd_data (srctemplate, abfd))
	    goto report_error;
	}
	{
	  flagword flags;

	  /* Create section to own the symbols.  */
	  flags = (SEC_CODE | SEC_HAS_CONTENTS | SEC_READONLY
		   | SEC_ALLOC | SEC_LOAD | SEC_KEEP | SEC_EXCLUDE);
	  if (bfd_make_section_anyway_with_flags (abfd, ".text", flags))
	    return abfd;
	}
    }
report_error:
  einfo (_("could not create dummy IR bfd: %F%E\n"));
  return NULL;
}

/* Check if the BFD passed in is an IR dummy object file.  */
static inline bfd_boolean
is_ir_dummy_bfd (const bfd *abfd)
{
  /* ABFD can sometimes legitimately be NULL, e.g. when called from one
     of the linker callbacks for a symbol in the *ABS* or *UND* sections.  */
  return abfd != NULL && (abfd->flags & BFD_PLUGIN) != 0;
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
  plugin_input_file_t *input = handle;
  bfd *abfd = input->abfd;
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
  const plugin_input_file_t *input = handle;

  ASSERT (called_plugin);

  file->name = input->name;
  file->offset = input->offset;
  file->filesize = input->filesize;
  file->handle = (void *) handle;

  return LDPS_OK;
}

/* Get view of the input file.  */
static enum ld_plugin_status
get_view (const void *handle, const void **viewp)
{
  plugin_input_file_t *input = (plugin_input_file_t *) handle;
  char *buffer;
  size_t size = input->filesize;
  off_t offset = input->offset;
#if HAVE_MMAP && HAVE_GETPAGESIZE
  off_t bias;
#endif

  ASSERT (called_plugin);

  /* FIXME: einfo should support %lld.  */
  if ((off_t) size != input->filesize)
    einfo (_("%P%F: unsupported input file size: %s (%ld bytes)\n"),
	   input->name, (long) input->filesize);

  /* Check the cached view buffer.  */
  if (input->view_buffer.addr != NULL
      && input->view_buffer.filesize == size
      && input->view_buffer.offset == offset)
    {
      *viewp = input->view_buffer.addr;
      return LDPS_OK;
    }

  input->view_buffer.filesize = size;
  input->view_buffer.offset = offset;

#if HAVE_MMAP
# if HAVE_GETPAGESIZE
  bias = offset % plugin_pagesize;
  offset -= bias;
  size += bias;
# endif
  buffer = mmap (NULL, size, PROT_READ, MAP_PRIVATE, input->fd, offset);
  if (buffer != MAP_FAILED)
    {
      input->use_mmap = TRUE;
# if HAVE_GETPAGESIZE
      buffer += bias;
# endif
    }
  else
#endif
    {
      char *p;

      input->use_mmap = FALSE;

      if (lseek (input->fd, offset, SEEK_SET) < 0)
	return LDPS_ERR;

      buffer = bfd_alloc (input->abfd, size);
      if (buffer == NULL)
	return LDPS_ERR;

      p = buffer;
      do
	{
	  ssize_t got = read (input->fd, p, size);
	  if (got == 0)
	    break;
	  else if (got > 0)
	    {
	      p += got;
	      size -= got;
	    }
	  else if (errno != EINTR)
	    return LDPS_ERR;
	}
      while (size > 0);
    }

  input->view_buffer.addr = buffer;
  *viewp = buffer;

  return LDPS_OK;
}

/* Release the input file.  */
static enum ld_plugin_status
release_input_file (const void *handle)
{
  plugin_input_file_t *input = (plugin_input_file_t *) handle;
  ASSERT (called_plugin);
  if (input->fd != -1)
    {
      close (input->fd);
      input->fd = -1;
    }
  return LDPS_OK;
}

/* Return TRUE if a defined symbol might be reachable from outside the
   universe of claimed objects.  */
static inline bfd_boolean
is_visible_from_outside (struct ld_plugin_symbol *lsym,
			 struct bfd_link_hash_entry *blhe)
{
  struct bfd_sym_chain *sym;

  if (bfd_link_relocatable (&link_info))
    return TRUE;
  if (link_info.export_dynamic || bfd_link_dll (&link_info))
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
  const plugin_input_file_t *input = handle;
  const bfd *abfd = (const bfd *) input->abfd;
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
	  /* The plugin is called to claim symbols in an archive element
	     from plugin_object_p.  But those symbols aren't needed to
	     create output.  They are defined and referenced only within
	     IR.  */
	  switch (syms[n].def)
	    {
	    default:
	      abort ();
	    case LDPK_UNDEF:
	    case LDPK_WEAKUNDEF:
	      res = LDPR_UNDEF;
	      break;
	    case LDPK_DEF:
	    case LDPK_WEAKDEF:
	    case LDPK_COMMON:
	      res = LDPR_PREVAILING_DEF_IRONLY;
	      break;
	    }
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
  lang_input_statement_type *is;

  ASSERT (called_plugin);
  is = lang_add_input_file (xstrdup (pathname), lang_input_file_is_file_enum,
			    NULL);
  if (!is)
    return LDPS_ERR;
  is->flags.lto_output = 1;
  return LDPS_OK;
}

/* Add a new (real) library required by a plugin.  */
static enum ld_plugin_status
add_input_library (const char *pathname)
{
  lang_input_statement_type *is;

  ASSERT (called_plugin);
  is = lang_add_input_file (xstrdup (pathname), lang_input_file_is_l_enum,
			    NULL);
  if (!is)
    return LDPS_ERR;
  is->flags.lto_output = 1;
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
      {
	char *newfmt = concat ("%P: warning: ", format, "\n",
			       (const char *) NULL);
	vfinfo (stdout, newfmt, args, TRUE);
	free (newfmt);
      }
      break;
    case LDPL_FATAL:
    case LDPL_ERROR:
    default:
      {
	char *newfmt = concat (level == LDPL_FATAL ? "%P%F" : "%P%X",
			       ": error: ", format, "\n",
			       (const char *) NULL);
	fflush (stdout);
	vfinfo (stderr, newfmt, args, TRUE);
	fflush (stderr);
	free (newfmt);
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
	  TVU(val) = (bfd_link_relocatable (&link_info) ? LDPO_REL
		      : bfd_link_pde (&link_info) ? LDPO_EXEC
		      : bfd_link_pie (&link_info) ? LDPO_PIE
		      : LDPO_DYN);
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
	case LDPT_GET_VIEW:
	  TVU(get_view) = get_view;
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
  link_info.lto_plugin_active = TRUE;
  link_info.callbacks = &plugin_callbacks;

  register_ld_plugin_object_p (plugin_object_p);

#if HAVE_MMAP && HAVE_GETPAGESIZE
  plugin_pagesize = getpagesize ();
#endif
}

/* Call 'claim file' hook for all plugins.  */
static int
plugin_call_claim_file (const struct ld_plugin_input_file *file, int *claimed)
{
  plugin_t *curplug = plugins_list;
  *claimed = FALSE;
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

/* Duplicates a character string with memory attached to ABFD.  */

static char *
plugin_strdup (bfd *abfd, const char *str)
{
  size_t strlength;
  char *copy;
  strlength = strlen (str) + 1;
  copy = bfd_alloc (abfd, strlength);
  if (copy == NULL)
    einfo (_("%P%F: plugin_strdup failed to allocate memory: %s\n"),
	   bfd_get_error ());
  memcpy (copy, str, strlength);
  return copy;
}

static const bfd_target *
plugin_object_p (bfd *ibfd)
{
  int claimed;
  plugin_input_file_t *input;
  off_t offset, filesize;
  struct ld_plugin_input_file file;
  bfd *abfd;
  bfd_boolean inarchive;
  const char *name;
  int fd;

  /* Don't try the dummy object file.  */
  if ((ibfd->flags & BFD_PLUGIN) != 0)
    return NULL;

  if (ibfd->plugin_format != bfd_plugin_unknown)
    {
      if (ibfd->plugin_format == bfd_plugin_yes)
	return ibfd->plugin_dummy_bfd->xvec;
      else
	return NULL;
    }

  inarchive = (ibfd->my_archive != NULL
	       && !bfd_is_thin_archive (ibfd->my_archive));
  name = inarchive ? ibfd->my_archive->filename : ibfd->filename;
  fd = open (name, O_RDONLY | O_BINARY);

  if (fd < 0)
    return NULL;

  /* We create a dummy BFD, initially empty, to house whatever symbols
     the plugin may want to add.  */
  abfd = plugin_get_ir_dummy_bfd (ibfd->filename, ibfd);

  input = bfd_alloc (abfd, sizeof (*input));
  if (input == NULL)
    einfo (_("%P%F: plugin failed to allocate memory for input: %s\n"),
	   bfd_get_error ());

  if (inarchive)
    {
      /* Offset and filesize must refer to the individual archive
	 member, not the whole file, and must exclude the header.
	 Fortunately for us, that is how the data is stored in the
	 origin field of the bfd and in the arelt_data.  */
      offset = ibfd->origin;
      filesize = arelt_size (ibfd);
    }
  else
    {
      offset = 0;
      filesize = lseek (fd, 0, SEEK_END);

      /* We must copy filename attached to ibfd if it is not an archive
	 member since it may be freed by bfd_close below.  */
      name = plugin_strdup (abfd, name);
    }

  file.name = name;
  file.offset = offset;
  file.filesize = filesize;
  file.fd = fd;
  file.handle = input;

  input->abfd = abfd;
  input->view_buffer.addr = NULL;
  input->view_buffer.filesize = 0;
  input->view_buffer.offset = 0;
  input->fd = fd;
  input->use_mmap = FALSE;
  input->offset = offset;
  input->filesize = filesize;
  input->name = plugin_strdup (abfd, ibfd->filename);

  claimed = 0;

  if (plugin_call_claim_file (&file, &claimed))
    einfo (_("%P%F: %s: plugin reported error claiming file\n"),
	   plugin_error_plugin ());

  if (input->fd != -1 && ! bfd_plugin_target_p (ibfd->xvec))
    {
      /* FIXME: fd belongs to us, not the plugin.  GCC plugin, which
	 doesn't need fd after plugin_call_claim_file, doesn't use
	 BFD plugin target vector.  Since GCC plugin doesn't call
	 release_input_file, we close it here.  LLVM plugin, which
	 needs fd after plugin_call_claim_file and calls
	 release_input_file after it is done, uses BFD plugin target
	 vector.  This scheme doesn't work when a plugin needs fd and
	 doesn't use BFD plugin target vector neither.  */
      close (fd);
      input->fd = -1;
    }

  if (claimed)
    {
      ibfd->plugin_format = bfd_plugin_yes;
      ibfd->plugin_dummy_bfd = abfd;
      bfd_make_readable (abfd);
      return abfd->xvec;
    }
  else
    {
#if HAVE_MMAP
      if (input->use_mmap)
	{
	  /* If plugin didn't claim the file, unmap the buffer.  */
	  char *addr = input->view_buffer.addr;
	  off_t size = input->view_buffer.filesize;
# if HAVE_GETPAGESIZE
	  off_t bias = input->view_buffer.offset % plugin_pagesize;
	  size += bias;
	  addr -= bias;
# endif
	  munmap (addr, size);
	}
#endif

      /* If plugin didn't claim the file, we don't need the dummy bfd.
	 Can't avoid speculatively creating it, alas.  */
      ibfd->plugin_format = bfd_plugin_no;
      bfd_close_all_done (abfd);
      return NULL;
    }
}

void
plugin_maybe_claim (lang_input_statement_type *entry)
{
  if (plugin_object_p (entry->the_bfd))
    {
      bfd *abfd = entry->the_bfd->plugin_dummy_bfd;

      /* Discard the real file's BFD and substitute the dummy one.  */

      /* We can't call bfd_close on archives.  BFD archive handling
	 caches elements, and add_archive_element keeps pointers to
	 the_bfd and the_bfd->filename in a lang_input_statement_type
	 linker script statement.  */
      if (entry->the_bfd->my_archive == NULL)
	bfd_close (entry->the_bfd);
      entry->the_bfd = abfd;
      entry->flags.claimed = 1;
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
	       struct bfd_link_hash_entry *inh,
	       bfd *abfd,
	       asection *section,
	       bfd_vma value,
	       flagword flags)
{
  struct bfd_link_hash_entry *orig_h = h;

  if (h != NULL)
    {
      bfd *sym_bfd;

      if (h->type == bfd_link_hash_warning)
	h = h->u.i.link;

      /* Nothing to do here if this def/ref is from an IR dummy BFD.  */
      if (is_ir_dummy_bfd (abfd))
	;

      /* Making an indirect symbol counts as a reference unless this
	 is a brand new symbol.  */
      else if (bfd_is_ind_section (section)
	       || (flags & BSF_INDIRECT) != 0)
	{
	  /* ??? Some of this is questionable.  See comments in
	     _bfd_generic_link_add_one_symbol for case IND.  */
	  if (h->type != bfd_link_hash_new)
	    {
	      h->non_ir_ref = TRUE;
	      inh->non_ir_ref = TRUE;
	    }
	  else if (inh->type == bfd_link_hash_new)
	    inh->non_ir_ref = TRUE;
	}

      /* Nothing to do here for warning symbols.  */
      else if ((flags & BSF_WARNING) != 0)
	;

      /* Nothing to do here for constructor symbols.  */
      else if ((flags & BSF_CONSTRUCTOR) != 0)
	;

      /* If this is a ref, set non_ir_ref.  */
      else if (bfd_is_und_section (section))
	{
	  /* Replace the undefined dummy bfd with the real one.  */
	  if ((h->type == bfd_link_hash_undefined
	       || h->type == bfd_link_hash_undefweak)
	      && (h->u.undef.abfd == NULL
		  || (h->u.undef.abfd->flags & BFD_PLUGIN) != 0))
	    h->u.undef.abfd = abfd;
	  h->non_ir_ref = TRUE;
	}

      /* Otherwise, it must be a new def.  */
      else
	{
	  /* A common symbol should be merged with other commons or
	     defs with the same name.  In particular, a common ought
	     to be overridden by a def in a -flto object.  In that
	     sense a common is also a ref.  */
	  if (bfd_is_com_section (section))
	    h->non_ir_ref = TRUE;

	  /* Ensure any symbol defined in an IR dummy BFD takes on a
	     new value from a real BFD.  Weak symbols are not normally
	     overridden by a new weak definition, and strong symbols
	     will normally cause multiple definition errors.  Avoid
	     this by making the symbol appear to be undefined.  */
	  if (((h->type == bfd_link_hash_defweak
		|| h->type == bfd_link_hash_defined)
	       && is_ir_dummy_bfd (sym_bfd = h->u.def.section->owner))
	      || (h->type == bfd_link_hash_common
		  && is_ir_dummy_bfd (sym_bfd = h->u.c.p->section->owner)))
	    {
	      h->type = bfd_link_hash_undefweak;
	      h->u.undef.abfd = sym_bfd;
	    }
	}
    }

  /* Continue with cref/nocrossref/trace-sym processing.  */
  if (orig_h == NULL
      || orig_notice_all
      || (info->notice_hash != NULL
	  && bfd_hash_lookup (info->notice_hash, orig_h->root.string,
			      FALSE, FALSE) != NULL))
    return (*orig_callbacks->notice) (info, orig_h, inh,
				      abfd, section, value, flags);
  return TRUE;
}
