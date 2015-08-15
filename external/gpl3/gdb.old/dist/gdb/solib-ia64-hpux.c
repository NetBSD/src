/* Copyright (C) 2010-2014 Free Software Foundation, Inc.

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
#include "ia64-tdep.h"
#include "ia64-hpux-tdep.h"
#include "solib-ia64-hpux.h"
#include "solist.h"
#include "solib.h"
#include "target.h"
#include "gdbtypes.h"
#include "inferior.h"
#include "gdbcore.h"
#include "regcache.h"
#include "opcode/ia64.h"
#include "symfile.h"
#include "objfiles.h"
#include "elf-bfd.h"
#include "exceptions.h"

/* Need to define the following macro in order to get the complete
   load_module_desc struct definition in dlfcn.h  Otherwise, it doesn't
   match the size of the struct the loader is providing us during load
   events.  */
#define _LOAD_MODULE_DESC_EXT

#include <sys/ttrace.h>
#include <dlfcn.h>
#include <elf.h>
#include <service_mgr.h>

/* The following is to have access to the definition of type load_info_t.  */
#include <crt0.h>

/* The r32 pseudo-register number.

   Like all stacked registers, r32 is treated as a pseudo-register,
   because it is not always available for read/write via the ttrace
   interface.  */
/* This is a bit of a hack, as we duplicate something hidden inside
   ia64-tdep.c, but oh well...  */
#define IA64_R32_PSEUDO_REGNUM (IA64_NAT127_REGNUM + 2)

/* Our struct so_list private data structure.  */

struct lm_info
{
  /* The shared library module descriptor.  We extract this structure
     from the loader at the time the shared library gets mapped.  */
  struct load_module_desc module_desc;

  /* The text segment address as defined in the shared library object
     (this is not the address where this segment got loaded).  This
     field is initially set to zero, and computed lazily.  */
  CORE_ADDR text_start;

  /* The data segment address as defined in the shared library object
     (this is not the address where this segment got loaded).  This
     field is initially set to zero, and computed lazily.  */
  CORE_ADDR data_start;
};

/* The list of shared libraries currently mapped by the inferior.  */

static struct so_list *so_list_head = NULL;

/* Create a new so_list element.  The result should be deallocated
   when no longer in use.  */

static struct so_list *
new_so_list (char *so_name, struct load_module_desc module_desc)
{
  struct so_list *new_so;

  new_so = (struct so_list *) XZALLOC (struct so_list);
  new_so->lm_info = (struct lm_info *) XZALLOC (struct lm_info);
  new_so->lm_info->module_desc = module_desc;

  strncpy (new_so->so_name, so_name, SO_NAME_MAX_PATH_SIZE - 1);
  new_so->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
  strcpy (new_so->so_original_name, new_so->so_name);

  return new_so;
}

/* Return non-zero if the instruction at the current PC is a breakpoint
   part of the dynamic loading process.

   We identify such instructions by checking that the instruction at
   the current pc is a break insn where no software breakpoint has been
   inserted by us.  We also verify that the operands have specific
   known values, to be extra certain.

   PTID is the ptid of the thread that should be checked, but this
   function also assumes that inferior_ptid is already equal to PTID.
   Ideally, we would like to avoid the requirement on inferior_ptid,
   but many routines still use the inferior_ptid global to access
   the relevant thread's register and memory.  We still have the ptid
   as parameter to be able to pass it to the routines that do take a ptid
   - that way we avoid increasing explicit uses of the inferior_ptid
   global.  */

static int
ia64_hpux_at_dld_breakpoint_1_p (ptid_t ptid)
{
  struct regcache *regcache = get_thread_regcache (ptid);
  CORE_ADDR pc = regcache_read_pc (regcache);
  struct address_space *aspace = get_regcache_aspace (regcache);
  ia64_insn t0, t1, slot[3], template, insn;
  int slotnum;
  bfd_byte bundle[16];

  /* If this is a regular breakpoint, then it can not be a dld one.  */
  if (breakpoint_inserted_here_p (aspace, pc))
    return 0;

  slotnum = ((long) pc) & 0xf;
  if (slotnum > 2)
    internal_error (__FILE__, __LINE__,
		    "invalid slot (%d) for address %s", slotnum,
		    paddress (get_regcache_arch (regcache), pc));

  pc -= (pc & 0xf);
  read_memory (pc, bundle, sizeof (bundle));

  /* bundles are always in little-endian byte order */
  t0 = bfd_getl64 (bundle);
  t1 = bfd_getl64 (bundle + 8);
  template = (t0 >> 1) & 0xf;
  slot[0] = (t0 >>  5) & 0x1ffffffffffLL;
  slot[1] = ((t0 >> 46) & 0x3ffff) | ((t1 & 0x7fffff) << 18);
  slot[2] = (t1 >> 23) & 0x1ffffffffffLL;

  if (template == 2 && slotnum == 1)
    {
      /* skip L slot in MLI template: */
      slotnum = 2;
    }

  insn = slot[slotnum];

  return (insn == 0x1c0c9c0       /* break.i 0x070327 */
          || insn == 0x3c0c9c0);  /* break.i 0x0f0327 */
}

/* Same as ia64_hpux_at_dld_breakpoint_1_p above, with the following
   differences: It temporarily sets inferior_ptid to PTID, and also
   contains any exception being raised.  */

int
ia64_hpux_at_dld_breakpoint_p (ptid_t ptid)
{
  volatile struct gdb_exception e;
  ptid_t saved_ptid = inferior_ptid;
  int result = 0;

  inferior_ptid = ptid;
  TRY_CATCH (e, RETURN_MASK_ALL)
    {
      result = ia64_hpux_at_dld_breakpoint_1_p (ptid);
    }
  inferior_ptid = saved_ptid;
  if (e.reason < 0)
    warning (_("error while checking for dld breakpoint: %s"), e.message);

  return result;
}

/* Handler for library load event: Read the information provided by
   the loader, and then use it to read the shared library symbols.  */

static void
ia64_hpux_handle_load_event (struct regcache *regcache)
{
  CORE_ADDR module_desc_addr;
  ULONGEST module_desc_size;
  CORE_ADDR so_path_addr;
  char so_path[PATH_MAX];
  struct load_module_desc module_desc;
  struct so_list *new_so;

  /* Extract the data provided by the loader as follow:
       - r33: Address of load_module_desc structure
       - r34: size of struct load_module_desc
       - r35: Address of string holding shared library path
   */
  regcache_cooked_read_unsigned (regcache, IA64_R32_PSEUDO_REGNUM + 1,
                                 &module_desc_addr);
  regcache_cooked_read_unsigned (regcache, IA64_R32_PSEUDO_REGNUM + 2,
                                 &module_desc_size);
  regcache_cooked_read_unsigned (regcache, IA64_R32_PSEUDO_REGNUM + 3,
                                 &so_path_addr);

  if (module_desc_size != sizeof (struct load_module_desc))
    warning (_("load_module_desc size (%ld) != size returned by kernel (%s)"),
             sizeof (struct load_module_desc),
	     pulongest (module_desc_size));

  read_memory_string (so_path_addr, so_path, PATH_MAX);
  read_memory (module_desc_addr, (gdb_byte *) &module_desc,
	       sizeof (module_desc));

  /* Create a new so_list element and insert it at the start of our
     so_list_head (we insert at the start of the list only because
     it is less work compared to inserting it elsewhere).  */
  new_so = new_so_list (so_path, module_desc);
  new_so->next = so_list_head;
  so_list_head = new_so;
}

/* Update the value of the PC to point to the begining of the next
   instruction bundle.  */

static void
ia64_hpux_move_pc_to_next_bundle (struct regcache *regcache)
{
  CORE_ADDR pc = regcache_read_pc (regcache);

  pc -= pc & 0xf;
  pc += 16;
  ia64_write_pc (regcache, pc);
}

/* Handle loader events.

   PTID is the ptid of the thread corresponding to the event being
   handled.  Similarly to ia64_hpux_at_dld_breakpoint_1_p, this
   function assumes that inferior_ptid is set to PTID.  */

static void
ia64_hpux_handle_dld_breakpoint_1 (ptid_t ptid)
{
  struct regcache *regcache = get_thread_regcache (ptid);
  ULONGEST arg0;

  /* The type of event is provided by the loaded via r32.  */
  regcache_cooked_read_unsigned (regcache, IA64_R32_PSEUDO_REGNUM, &arg0);
  switch (arg0)
    {
      case BREAK_DE_SVC_LOADED:
	/* Currently, the only service loads are uld and dld,
	   so we shouldn't need to do anything.  Just ignore.  */
	break;
      case BREAK_DE_LIB_LOADED:
	ia64_hpux_handle_load_event (regcache);
	solib_add (NULL, 0, &current_target, auto_solib_add);
	break;
      case BREAK_DE_LIB_UNLOADED:
      case BREAK_DE_LOAD_COMPLETE:
      case BREAK_DE_BOR:
	/* Ignore for now.  */
	break;
    }

  /* Now that we have handled the event, we can move the PC to
     the next instruction bundle, past the break instruction.  */
  ia64_hpux_move_pc_to_next_bundle (regcache);
}

/* Same as ia64_hpux_handle_dld_breakpoint_1 above, with the following
   differences: This function temporarily sets inferior_ptid to PTID,
   and also contains any exception.  */

void
ia64_hpux_handle_dld_breakpoint (ptid_t ptid)
{
  volatile struct gdb_exception e;
  ptid_t saved_ptid = inferior_ptid;

  inferior_ptid = ptid;
  TRY_CATCH (e, RETURN_MASK_ALL)
    {
      ia64_hpux_handle_dld_breakpoint_1 (ptid);
    }
  inferior_ptid = saved_ptid;
  if (e.reason < 0)
    warning (_("error detected while handling dld breakpoint: %s"), e.message);
}

/* Find the address of the code and data segments in ABFD, and update
   TEXT_START and DATA_START accordingly.  */

static void
ia64_hpux_find_start_vma (bfd *abfd, CORE_ADDR *text_start,
                          CORE_ADDR *data_start)
{
  Elf_Internal_Ehdr *i_ehdrp = elf_elfheader (abfd);
  Elf64_Phdr phdr;
  int i;

  *text_start = 0;
  *data_start = 0;

  if (bfd_seek (abfd, i_ehdrp->e_phoff, SEEK_SET) == -1)
    error (_("invalid program header offset in %s"), abfd->filename);

  for (i = 0; i < i_ehdrp->e_phnum; i++)
    {
      if (bfd_bread (&phdr, sizeof (phdr), abfd) != sizeof (phdr))
        error (_("failed to read segment %d in %s"), i, abfd->filename);

      if (phdr.p_flags & PF_X
          && (*text_start == 0 || phdr.p_vaddr < *text_start))
        *text_start = phdr.p_vaddr;

      if (phdr.p_flags & PF_W
          && (*data_start == 0 || phdr.p_vaddr < *data_start))
        *data_start = phdr.p_vaddr;
    }
}

/* The "relocate_section_addresses" target_so_ops routine for ia64-hpux.  */

static void
ia64_hpux_relocate_section_addresses (struct so_list *so,
				      struct target_section *sec)
{
  CORE_ADDR offset = 0;

  /* If we haven't computed the text & data segment addresses, do so now.
     We do this here, because we now have direct access to the associated
     bfd, whereas we would have had to open our own if we wanted to do it
     while processing the library-load event.  */
  if (so->lm_info->text_start == 0 && so->lm_info->data_start == 0)
    ia64_hpux_find_start_vma (sec->the_bfd_section->owner,
			      &so->lm_info->text_start,
			      &so->lm_info->data_start);

  /* Determine the relocation offset based on which segment
     the section belongs to.  */
  if ((so->lm_info->text_start < so->lm_info->data_start
       && sec->addr < so->lm_info->data_start)
      || (so->lm_info->text_start > so->lm_info->data_start
          && sec->addr >= so->lm_info->text_start))
    offset = so->lm_info->module_desc.text_base - so->lm_info->text_start;
  else if ((so->lm_info->text_start < so->lm_info->data_start
            && sec->addr >= so->lm_info->data_start)
           || (so->lm_info->text_start > so->lm_info->data_start
	       && sec->addr < so->lm_info->text_start))
    offset = so->lm_info->module_desc.data_base - so->lm_info->data_start;

  /* And now apply the relocation.  */
  sec->addr += offset;
  sec->endaddr += offset;

  /* Best effort to set addr_high/addr_low.  This is used only by
     'info sharedlibrary'.  */
  if (so->addr_low == 0 || sec->addr < so->addr_low)
    so->addr_low = sec->addr;

  if (so->addr_high == 0 || sec->endaddr > so->addr_high)
    so->addr_high = sec->endaddr;
}

/* The "free_so" target_so_ops routine for ia64-hpux.  */

static void
ia64_hpux_free_so (struct so_list *so)
{
  xfree (so->lm_info);
}

/* The "clear_solib" target_so_ops routine for ia64-hpux.  */

static void
ia64_hpux_clear_solib (void)
{
  struct so_list *so;

  while (so_list_head != NULL)
    {
      so = so_list_head;
      so_list_head = so_list_head->next;

      ia64_hpux_free_so (so);
      xfree (so);
    }
}

/* Assuming the inferior just stopped on an EXEC event, return
   the address of the load_info_t structure.  */

static CORE_ADDR
ia64_hpux_get_load_info_addr (void)
{
  struct type *data_ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  CORE_ADDR addr;
  int status;

  /* The address of the load_info_t structure is stored in the 4th
     argument passed to the initial thread of the process (in other
     words, in argv[3]).  So get the address of these arguments,
     and extract the 4th one.  */
  status = ttrace (TT_PROC_GET_ARGS, ptid_get_pid (inferior_ptid),
		   0, (uintptr_t) &addr, sizeof (CORE_ADDR), 0);
  if (status == -1 && errno)
    perror_with_name (_("Unable to get argument list"));
  return (read_memory_typed_address (addr + 3 * 8, data_ptr_type));
}

/* A structure used to aggregate some information extracted from
   the dynamic section of the main executable.  */

struct dld_info
{
  ULONGEST dld_flags;
  CORE_ADDR load_map;
};

/* Scan the ".dynamic" section referenced by ABFD and DYN_SECT,
   and extract the information needed to fill in INFO.  */

static void
ia64_hpux_read_dynamic_info (struct gdbarch *gdbarch, bfd *abfd,
			     asection *dyn_sect, struct dld_info *info)
{
  int sect_size;
  char *buf;
  char *buf_end;

  /* Make sure that info always has initialized data, even if we fail
     to read the syn_sect section.  */
  memset (info, 0, sizeof (struct dld_info));

  sect_size = bfd_section_size (abfd, dyn_sect);
  buf = alloca (sect_size);
  buf_end = buf + sect_size;

  if (bfd_seek (abfd, dyn_sect->filepos, SEEK_SET) != 0
      || bfd_bread (buf, sect_size, abfd) != sect_size)
    error (_("failed to read contents of .dynamic section"));

  for (; buf < buf_end; buf += sizeof (Elf64_Dyn))
    {
      Elf64_Dyn *dynp = (Elf64_Dyn *) buf;
      Elf64_Sxword d_tag;

      d_tag = bfd_h_get_64 (abfd, &dynp->d_tag);
      switch (d_tag)
        {
          case DT_HP_DLD_FLAGS:
            info->dld_flags = bfd_h_get_64 (abfd, &dynp->d_un);
            break;

          case DT_HP_LOAD_MAP:
            {
              CORE_ADDR load_map_addr = bfd_h_get_64 (abfd, &dynp->d_un.d_ptr);

              if (target_read_memory (load_map_addr,
				      (gdb_byte *) &info->load_map,
                                      sizeof (info->load_map)) != 0)
		error (_("failed to read load map at %s"),
		       paddress (gdbarch, load_map_addr));
            }
            break;
        }
    }
}

/* Wrapper around target_read_memory used with libdl.  */

static void *
ia64_hpux_read_tgt_mem (void *buffer, uint64_t ptr, size_t bufsiz, int ident)
{
  if (target_read_memory (ptr, (gdb_byte *) buffer, bufsiz) != 0)
    return 0;
  else
    return buffer;
}

/* Create a new so_list object for a shared library, and store that
   new so_list object in our SO_LIST_HEAD list.

   SO_INDEX is an index specifying the placement of the loaded shared
   library in the dynamic loader's search list.  Normally, this index
   is strictly positive, but an index of -1 refers to the loader itself.

   Return nonzero if the so_list object could be created.  A null
   return value with a positive SO_INDEX normally means that there are
   no more entries in the dynamic loader's search list at SO_INDEX or
   beyond.  */

static int
ia64_hpux_add_so_from_dld_info (struct dld_info info, int so_index)
{
  struct load_module_desc module_desc;
  uint64_t so_handle;
  char *so_path;
  struct so_list *so;

  so_handle = dlgetmodinfo (so_index, &module_desc, sizeof (module_desc),
			    ia64_hpux_read_tgt_mem, 0, info.load_map);

  if (so_handle == 0)
    /* No such entry.  We probably reached the end of the list.  */
    return 0;

  so_path = dlgetname (&module_desc, sizeof (module_desc),
                       ia64_hpux_read_tgt_mem, 0, info.load_map);
  if (so_path == NULL)
    {
      /* Should never happen, but let's not crash if it does.  */
      warning (_("unable to get shared library name, symbols not loaded"));
      return 0;
    }

  /* Create a new so_list and insert it at the start of our list.
     The order is not extremely important, but it's less work to do so
     at the end of the list.  */
  so = new_so_list (so_path, module_desc);
  so->next = so_list_head;
  so_list_head = so;

  return 1;
}

/* Assuming we just attached to a process, update our list of shared
   libraries (SO_LIST_HEAD) as well as GDB's list.  */

static void
ia64_hpux_solib_add_after_attach (void)
{
  bfd *abfd;
  asection *dyn_sect;
  struct dld_info info;
  int i;

  if (symfile_objfile == NULL)
    return;

  abfd = symfile_objfile->obfd;
  dyn_sect = bfd_get_section_by_name (abfd, ".dynamic");

  if (dyn_sect == NULL || bfd_section_size (abfd, dyn_sect) == 0)
    return;

  ia64_hpux_read_dynamic_info (get_objfile_arch (symfile_objfile), abfd,
			       dyn_sect, &info);

  if ((info.dld_flags & DT_HP_DEBUG_PRIVATE) == 0)
    {
      warning (_(
"The shared libraries were not privately mapped; setting a breakpoint\n\
in a shared library will not work until you rerun the program.\n\
Use the following command to enable debugging of shared libraries.\n\
chatr +dbg enable a.out"));
    }

  /* Read the symbols of the dynamic loader (dld.so).  */
  ia64_hpux_add_so_from_dld_info (info, -1);

  /* Read the symbols of all the other shared libraries.  */
  for (i = 1; ; i++)
    if (!ia64_hpux_add_so_from_dld_info (info, i))
      break;  /* End of list.  */

  /* Resync the library list at the core level.  */
  solib_add (NULL, 1, &current_target, auto_solib_add);
}

/* The "create_inferior_hook" target_so_ops routine for ia64-hpux.  */

static void
ia64_hpux_solib_create_inferior_hook (int from_tty)
{
  CORE_ADDR load_info_addr;
  load_info_t load_info;

  /* Initially, we were thinking about adding a check that the program
     (accessible through symfile_objfile) was linked against some shared
     libraries, by searching for a ".dynamic" section.  However, could
     this break in the case of a statically linked program that later
     uses dlopen?  Programs that are fully statically linked are very
     rare, and we will worry about them when we encounter one that
     causes trouble.  */

  /* Set the LI_TRACE flag in the load_info_t structure.  This enables
     notifications when shared libraries are being mapped.  */
  load_info_addr = ia64_hpux_get_load_info_addr ();
  read_memory (load_info_addr, (gdb_byte *) &load_info, sizeof (load_info));
  load_info.li_flags |= LI_TRACE;
  write_memory (load_info_addr, (gdb_byte *) &load_info, sizeof (load_info));

  /* If we just attached to our process, some shard libraries have
     already been mapped.  Find which ones they are...  */
  if (current_inferior ()->attach_flag)
    ia64_hpux_solib_add_after_attach ();
}

/* The "special_symbol_handling" target_so_ops routine for ia64-hpux.  */

static void
ia64_hpux_special_symbol_handling (void)
{
  /* Nothing to do.  */
}

/* The "current_sos" target_so_ops routine for ia64-hpux.  */

static struct so_list *
ia64_hpux_current_sos (void)
{
  /* Return a deep copy of our own list.  */
  struct so_list *new_head = NULL, *prev_new_so = NULL;
  struct so_list *our_so;

  for (our_so = so_list_head; our_so != NULL; our_so = our_so->next)
    {
      struct so_list *new_so;

      new_so = new_so_list (our_so->so_name, our_so->lm_info->module_desc);
      if (prev_new_so != NULL)
        prev_new_so->next = new_so;
      prev_new_so = new_so;
      if (new_head == NULL)
        new_head = new_so;
    }

  return new_head;
}

/* The "open_symbol_file_object" target_so_ops routine for ia64-hpux.  */

static int
ia64_hpux_open_symbol_file_object (void *from_ttyp)
{
  return 0;
}

/* The "in_dynsym_resolve_code" target_so_ops routine for ia64-hpux.  */

static int
ia64_hpux_in_dynsym_resolve_code (CORE_ADDR pc)
{
  return 0;
}

/* If FADDR is the address of a function inside one of the shared
   libraries, return the shared library linkage address.  */

CORE_ADDR
ia64_hpux_get_solib_linkage_addr (CORE_ADDR faddr)
{
  struct so_list *so = so_list_head;

  while (so != NULL)
    {
      struct load_module_desc module_desc = so->lm_info->module_desc;

      if (module_desc.text_base <= faddr
          && (module_desc.text_base + module_desc.text_size) > faddr)
        return module_desc.linkage_ptr;

      so = so->next;
    }

  return 0;
}

/* Create a new target_so_ops structure suitable for ia64-hpux, and
   return its address.  */

static struct target_so_ops *
ia64_hpux_target_so_ops (void)
{
  struct target_so_ops *ops = XZALLOC (struct target_so_ops);

  ops->relocate_section_addresses = ia64_hpux_relocate_section_addresses;
  ops->free_so = ia64_hpux_free_so;
  ops->clear_solib = ia64_hpux_clear_solib;
  ops->solib_create_inferior_hook = ia64_hpux_solib_create_inferior_hook;
  ops->special_symbol_handling = ia64_hpux_special_symbol_handling;
  ops->current_sos = ia64_hpux_current_sos;
  ops->open_symbol_file_object = ia64_hpux_open_symbol_file_object;
  ops->in_dynsym_resolve_code = ia64_hpux_in_dynsym_resolve_code;
  ops->bfd_open = solib_bfd_open;

  return ops;
}

/* Prevent warning from -Wmissing-prototypes.  */
void _initialize_solib_ia64_hpux (void);

void
_initialize_solib_ia64_hpux (void)
{
  ia64_hpux_so_ops = ia64_hpux_target_so_ops ();
}
