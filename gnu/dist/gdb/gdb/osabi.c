/* OS ABI variant handling for GDB.
   Copyright 2001, 2002 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,  
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "gdb_string.h"
#include "osabi.h"

#include "elf-bfd.h"


/* This table matches the indices assigned to enum gdb_osabi.  Keep
   them in sync.  */
static const char * const gdb_osabi_names[] =
{
  "<unknown>",

  "SVR4",
  "GNU/Hurd",
  "Solaris",
  "OSF/1",
  "GNU/Linux",
  "FreeBSD a.out",
  "FreeBSD ELF",
  "NetBSD a.out",
  "NetBSD ELF",
  "Windows CE",
  "DJGPP",
  "NetWare",
  "Irix",
  "LynxOS",

  "ARM EABI v1",
  "ARM EABI v2",
  "ARM APCS",

  "<invalid>"
};

const char *
gdbarch_osabi_name (enum gdb_osabi osabi)
{
  if (osabi >= GDB_OSABI_UNKNOWN && osabi < GDB_OSABI_INVALID)
    return gdb_osabi_names[osabi];

  return gdb_osabi_names[GDB_OSABI_INVALID];
}

/* Handler for a given architecture/OS ABI pair.  There should be only
   one handler for a given OS ABI each architecture family.  */
struct gdb_osabi_handler  
{
  struct gdb_osabi_handler *next;
  enum bfd_architecture arch;
  enum gdb_osabi osabi;
  void (*init_osabi)(struct gdbarch_info, struct gdbarch *);
};

static struct gdb_osabi_handler *gdb_osabi_handler_list;

void
gdbarch_register_osabi (enum bfd_architecture arch, enum gdb_osabi osabi,
                        void (*init_osabi)(struct gdbarch_info,
					   struct gdbarch *))
{
  struct gdb_osabi_handler **handler_p;

  /* Registering an OS ABI handler for "unknown" is not allowed.  */
  if (osabi == GDB_OSABI_UNKNOWN)
    {
      internal_error
	(__FILE__, __LINE__,
	 "gdbarch_register_osabi: An attempt to register a handler for "
         "OS ABI \"%s\" for architecture %s was made.  The handler will "
	 "not be registered",
	 gdbarch_osabi_name (osabi),
	 bfd_printable_arch_mach (arch, 0));
      return;
    }

  for (handler_p = &gdb_osabi_handler_list; *handler_p != NULL;
       handler_p = &(*handler_p)->next)
    {
      if ((*handler_p)->arch == arch
	  && (*handler_p)->osabi == osabi)
	{
	  internal_error
	    (__FILE__, __LINE__,
	     "gdbarch_register_osabi: A handler for OS ABI \"%s\" "
	     "has already been registered for architecture %s",
	     gdbarch_osabi_name (osabi),
	     bfd_printable_arch_mach (arch, 0));
	  /* If user wants to continue, override previous definition.  */
	  (*handler_p)->init_osabi = init_osabi;
	  return;
	}
    }

  (*handler_p)
    = (struct gdb_osabi_handler *) xmalloc (sizeof (struct gdb_osabi_handler));
  (*handler_p)->next = NULL;
  (*handler_p)->arch = arch;
  (*handler_p)->osabi = osabi;
  (*handler_p)->init_osabi = init_osabi;
}


/* Sniffer to find the OS ABI for a given file's architecture and flavour. 
   It is legal to have multiple sniffers for each arch/flavour pair, to
   disambiguate one OS's a.out from another, for example.  The first sniffer
   to return something other than GDB_OSABI_UNKNOWN wins, so a sniffer should
   be careful to claim a file only if it knows for sure what it is.  */
struct gdb_osabi_sniffer
{
  struct gdb_osabi_sniffer *next;
  enum bfd_architecture arch;   /* bfd_arch_unknown == wildcard */
  enum bfd_flavour flavour;
  enum gdb_osabi (*sniffer)(bfd *);
};

static struct gdb_osabi_sniffer *gdb_osabi_sniffer_list;

void
gdbarch_register_osabi_sniffer (enum bfd_architecture arch,
                                enum bfd_flavour flavour,
				enum gdb_osabi (*sniffer_fn)(bfd *))
{
  struct gdb_osabi_sniffer *sniffer;

  sniffer =
    (struct gdb_osabi_sniffer *) xmalloc (sizeof (struct gdb_osabi_sniffer));
  sniffer->arch = arch;
  sniffer->flavour = flavour;
  sniffer->sniffer = sniffer_fn;

  sniffer->next = gdb_osabi_sniffer_list;
  gdb_osabi_sniffer_list = sniffer;
}


enum gdb_osabi
gdbarch_lookup_osabi (bfd *abfd)
{
  struct gdb_osabi_sniffer *sniffer;
  enum gdb_osabi osabi, match;
  int match_specific;

  match = GDB_OSABI_UNKNOWN;
  match_specific = 0;

  for (sniffer = gdb_osabi_sniffer_list; sniffer != NULL;
       sniffer = sniffer->next)
    {
      if ((sniffer->arch == bfd_arch_unknown /* wildcard */
	   || sniffer->arch == bfd_get_arch (abfd))
	  && sniffer->flavour == bfd_get_flavour (abfd))
	{
	  osabi = (*sniffer->sniffer) (abfd);
	  if (osabi < GDB_OSABI_UNKNOWN || osabi >= GDB_OSABI_INVALID)
	    {
	      internal_error
		(__FILE__, __LINE__,
		 "gdbarch_lookup_osabi: invalid OS ABI (%d) from sniffer "
		 "for architecture %s flavour %d",
		 (int) osabi,
		 bfd_printable_arch_mach (bfd_get_arch (abfd), 0),
		 (int) bfd_get_flavour (abfd));
	    }
	  else if (osabi != GDB_OSABI_UNKNOWN)
	    {
	      /* A specific sniffer always overrides a generic sniffer.
		 Croak on multiple match if the two matches are of the
		 same class.  If the user wishes to continue, we'll use
		 the first match.  */
	      if (match != GDB_OSABI_UNKNOWN)
		{
		  if ((match_specific && sniffer->arch != bfd_arch_unknown)
		   || (!match_specific && sniffer->arch == bfd_arch_unknown))
		    {
		      internal_error
		        (__FILE__, __LINE__,
		         "gdbarch_lookup_osabi: multiple %sspecific OS ABI "
			 "match for architecture %s flavour %d: first "
			 "match \"%s\", second match \"%s\"",
			 match_specific ? "" : "non-",
		         bfd_printable_arch_mach (bfd_get_arch (abfd), 0),
		         (int) bfd_get_flavour (abfd),
		         gdbarch_osabi_name (match),
		         gdbarch_osabi_name (osabi));
		    }
		  else if (sniffer->arch != bfd_arch_unknown)
		    {
		      match = osabi;
		      match_specific = 1;
		    }
		}
	      else
		{
		  match = osabi;
		  if (sniffer->arch != bfd_arch_unknown)
		    match_specific = 1;
		}
	    }
	}
    }

  return match;
}

void
gdbarch_init_osabi (struct gdbarch_info info, struct gdbarch *gdbarch,
                    enum gdb_osabi osabi)
{
  struct gdb_osabi_handler *handler;
  bfd *abfd = info.abfd;
  const struct bfd_arch_info *arch_info = gdbarch_bfd_arch_info (gdbarch);

  if (osabi == GDB_OSABI_UNKNOWN)
    {
      /* Don't complain about an unknown OSABI.  Assume the user knows
         what they are doing.  */
      return;
    }

  for (handler = gdb_osabi_handler_list; handler != NULL;
       handler = handler->next)
    {
      if (handler->arch == bfd_get_arch (abfd)
	  && handler->osabi == osabi)
	{
	  (*handler->init_osabi) (info, gdbarch);
	  return;
	}
    }

  /* We assume that if GDB_MULTI_ARCH is less than GDB_MULTI_ARCH_TM
     that an ABI variant can be supported by overriding definitions in
     the tm-file.  */
  if (GDB_MULTI_ARCH > GDB_MULTI_ARCH_PARTIAL)
    fprintf_filtered
      (gdb_stderr,
       "A handler for the OS ABI \"%s\" is not built into this "
       "configuration of GDB.  "
       "Attempting to continue with the default %s settings",
       gdbarch_osabi_name (osabi),
       bfd_printable_arch_mach (arch_info->arch, arch_info->mach));
}


/* Generic sniffer for ELF flavoured files.  */

void
generic_elf_osabi_sniff_abi_tag_sections (bfd *abfd, asection *sect, void *obj)
{
  enum gdb_osabi *os_ident_ptr = obj;
  const char *name;
  unsigned int sectsize;

  name = bfd_get_section_name (abfd, sect);
  sectsize = bfd_section_size (abfd, sect);

  /* .note.ABI-tag notes, used by GNU/Linux and FreeBSD.  */
  if (strcmp (name, ".note.ABI-tag") == 0 && sectsize > 0)
    {
      unsigned int name_length, data_length, note_type;
      char *note;

      /* If the section is larger than this, it's probably not what we are
	 looking for.  */
      if (sectsize > 128)
	sectsize = 128;

      note = alloca (sectsize);

      bfd_get_section_contents (abfd, sect, note,
				(file_ptr) 0, (bfd_size_type) sectsize);

      name_length = bfd_h_get_32 (abfd, note);
      data_length = bfd_h_get_32 (abfd, note + 4);
      note_type   = bfd_h_get_32 (abfd, note + 8);

      if (name_length == 4 && data_length == 16 && note_type == NT_GNU_ABI_TAG
	  && strcmp (note + 12, "GNU") == 0)
	{
	  int os_number = bfd_h_get_32 (abfd, note + 16);

	  switch (os_number)
	    {
	    case GNU_ABI_TAG_LINUX:
	      *os_ident_ptr = GDB_OSABI_LINUX;
	      break;

	    case GNU_ABI_TAG_HURD:
	      *os_ident_ptr = GDB_OSABI_HURD;
	      break;

	    case GNU_ABI_TAG_SOLARIS:
	      *os_ident_ptr = GDB_OSABI_SOLARIS;
	      break;

	    default:
	      internal_error
		(__FILE__, __LINE__,
		 "generic_elf_osabi_sniff_abi_tag_sections: unknown OS number %d",
		 os_number);
	    }
	  return;
	}
      else if (name_length == 8 && data_length == 4
	       && note_type == NT_FREEBSD_ABI_TAG
	       && strcmp (note + 12, "FreeBSD") == 0)
	{
	  /* XXX Should we check the version here?  Probably not
	     necessary yet.  */
	  *os_ident_ptr = GDB_OSABI_FREEBSD_ELF;
	}
      return;
    }

  /* .note.netbsd.ident notes, used by NetBSD.  */
  if (strcmp (name, ".note.netbsd.ident") == 0 && sectsize > 0)
    {
      unsigned int name_length, data_length, note_type;
      char *note;

      /* If the section is larger than this, it's probably not what we are
	 looking for.  */
      if (sectsize > 128) 
	sectsize = 128;

      note = alloca (sectsize);

      bfd_get_section_contents (abfd, sect, note,
				(file_ptr) 0, (bfd_size_type) sectsize);
      
      name_length = bfd_h_get_32 (abfd, note);
      data_length = bfd_h_get_32 (abfd, note + 4);
      note_type   = bfd_h_get_32 (abfd, note + 8);

      if (name_length == 7 && data_length == 4 && note_type == NT_NETBSD_IDENT
	  && strcmp (note + 12, "NetBSD") == 0)
	{
	  /* XXX Should we check the version here?  Probably not
	     necessary yet.  */
	  *os_ident_ptr = GDB_OSABI_NETBSD_ELF;
	}
      return;
    }
}

static enum gdb_osabi
generic_elf_osabi_sniffer (bfd *abfd)
{
  unsigned int elfosabi;
  enum gdb_osabi osabi = GDB_OSABI_UNKNOWN;

  elfosabi = elf_elfheader (abfd)->e_ident[EI_OSABI];

  switch (elfosabi)
    {
    case ELFOSABI_NONE:
      /* When elfosabi is ELFOSABI_NONE (0), then the ELF structures in the
         file are conforming to the base specification for that machine
	 (there are no OS-specific extensions).  In order to determine the
	 real OS in use we must look for OS notes that have been added.  */
      bfd_map_over_sections (abfd,
			     generic_elf_osabi_sniff_abi_tag_sections,
			     &osabi);
      break;

    case ELFOSABI_FREEBSD:
      osabi = GDB_OSABI_FREEBSD_ELF;
      break;

    case ELFOSABI_NETBSD:
      osabi = GDB_OSABI_NETBSD_ELF;
      break;

    case ELFOSABI_LINUX:
      osabi = GDB_OSABI_LINUX;
      break;

    case ELFOSABI_HURD:
      osabi = GDB_OSABI_HURD;
      break;

    case ELFOSABI_SOLARIS:
      osabi = GDB_OSABI_SOLARIS;
      break;
    }

  if (osabi == GDB_OSABI_UNKNOWN)
    {
      /* The FreeBSD folks have been naughty; they stored the string
         "FreeBSD" in the padding of the e_ident field of the ELF
         header to "brand" their ELF binaries in FreeBSD 3.x.  */
      if (strcmp (&elf_elfheader (abfd)->e_ident[8], "FreeBSD") == 0)
	osabi = GDB_OSABI_FREEBSD_ELF;
    }

  return osabi;
}


void
_initialize_gdb_osabi (void)
{
  if (strcmp (gdb_osabi_names[GDB_OSABI_INVALID], "<invalid>") != 0)
    internal_error
      (__FILE__, __LINE__,
       "_initialize_gdb_osabi: gdb_osabi_names[] is inconsistent");

  /* Register a generic sniffer for ELF flavoured files.  */
  gdbarch_register_osabi_sniffer (bfd_arch_unknown,
				  bfd_target_elf_flavour,
				  generic_elf_osabi_sniffer);
}
