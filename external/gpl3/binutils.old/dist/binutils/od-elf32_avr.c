/* od-avrelf.c -- dump information about an AVR elf object file.
   Copyright (C) 2011-2018 Free Software Foundation, Inc.
   Written by Senthil Kumar Selvaraj, Atmel.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stddef.h>
#include <time.h>
#include <stdint.h>
#include "safe-ctype.h"
#include "bfd.h"
#include "objdump.h"
#include "bucomm.h"
#include "bfdlink.h"
#include "bfd.h"
#include "elf/external.h"
#include "elf/internal.h"
#include "elf32-avr.h"

/* Index of the options in the options[] array.  */
#define OPT_MEMUSAGE 0
#define OPT_AVRPROP 1

/* List of actions.  */
static struct objdump_private_option options[] =
  {
    { "mem-usage", 0 },
    { "avr-prop",  0},
    { NULL, 0 }
  };

/* Display help.  */

static void
elf32_avr_help (FILE *stream)
{
  fprintf (stream, _("\
For AVR ELF files:\n\
  mem-usage   Display memory usage\n\
  avr-prop    Display contents of .avr.prop section\n\
"));
}

typedef struct tagDeviceInfo
{
    uint32_t flash_start;
    uint32_t flash_size;
    uint32_t ram_start;
    uint32_t ram_size;
    uint32_t eeprom_start;
    uint32_t eeprom_size;
    char * name;
} deviceinfo;


/* Return TRUE if ABFD is handled.  */

static int
elf32_avr_filter (bfd *abfd)
{
  return bfd_get_flavour (abfd) == bfd_target_elf_flavour;
}

static char*
elf32_avr_get_note_section_contents (bfd *abfd, bfd_size_type *size)
{
  asection *section;

  if ((section = bfd_get_section_by_name (abfd, ".note.gnu.avr.deviceinfo")) == NULL)
    return NULL;

  *size = bfd_get_section_size (section);
  char *contents = (char *) xmalloc (*size);
  bfd_get_section_contents (abfd, section, contents, 0, *size);

  return contents;
}

static char* elf32_avr_get_note_desc (bfd *abfd, char *contents,
        bfd_size_type size)
{
  Elf_External_Note *xnp = (Elf_External_Note *) contents;
  Elf_Internal_Note in;

  if (offsetof (Elf_External_Note, name) > size)
    return NULL;

  in.type = bfd_get_32 (abfd, xnp->type);
  in.namesz = bfd_get_32 (abfd, xnp->namesz);
  in.namedata = xnp->name;
  if (in.namesz > contents - in.namedata + size)
    return NULL;

  in.descsz = bfd_get_32 (abfd, xnp->descsz);
  in.descdata = in.namedata + align_power (in.namesz, 2);
  if (in.descsz != 0
        && (in.descdata >= contents + size
            || in.descsz > contents - in.descdata + size))
    return NULL;

  if (strcmp (in.namedata, "AVR") != 0)
    return NULL;

  return in.descdata;
}

static void
elf32_avr_get_device_info (bfd *abfd, char *description,
        deviceinfo *device)
{
  if (description == NULL)
    return;

  const bfd_size_type memory_sizes = 6;

  memcpy (device, description, memory_sizes * sizeof(uint32_t));
  device->name = NULL;

  uint32_t *stroffset_table = ((uint32_t *) description) + memory_sizes;
  bfd_size_type stroffset_table_size = bfd_get_32 (abfd, stroffset_table);
  char *str_table = ((char *) stroffset_table) + stroffset_table_size;

  /* If the only content is the size itself, there's nothing in the table */
  if (stroffset_table_size == 4)
    return;

  /* First entry is the device name index. */
  uint32_t device_name_index = bfd_get_32 (abfd, stroffset_table + 1);

  device->name = str_table + device_name_index;
}

static void
elf32_avr_get_memory_usage (bfd *abfd,
        bfd_size_type *text_usage,
        bfd_size_type *data_usage,
        bfd_size_type *eeprom_usage)
{

  bfd_size_type avr_datasize = 0;
  bfd_size_type avr_textsize = 0;
  bfd_size_type avr_bsssize = 0;
  bfd_size_type bootloadersize = 0;
  bfd_size_type noinitsize = 0;
  bfd_size_type eepromsize = 0;
  asection *section;

  if ((section = bfd_get_section_by_name (abfd, ".data")) != NULL)
    avr_datasize = bfd_section_size (abfd, section);
  if ((section = bfd_get_section_by_name (abfd, ".text")) != NULL)
    avr_textsize = bfd_section_size (abfd, section);
  if ((section = bfd_get_section_by_name (abfd, ".bss")) != NULL)
    avr_bsssize = bfd_section_size (abfd, section);
  if ((section = bfd_get_section_by_name (abfd, ".bootloader")) != NULL)
    bootloadersize = bfd_section_size (abfd, section);
  if ((section = bfd_get_section_by_name (abfd, ".noinit")) != NULL)
    noinitsize = bfd_section_size (abfd, section);
  if ((section = bfd_get_section_by_name (abfd, ".eeprom")) != NULL)
    eepromsize = bfd_section_size (abfd, section);

  *text_usage = avr_textsize + avr_datasize + bootloadersize;
  *data_usage = avr_datasize + avr_bsssize + noinitsize;
  *eeprom_usage = eepromsize;
}

static void
elf32_avr_dump_mem_usage (bfd *abfd)
{
  char *description = NULL;
  bfd_size_type note_section_size = 0;

  deviceinfo device = { 0, 0, 0, 0, 0, 0, NULL };
  device.name = "Unknown";

  bfd_size_type data_usage = 0;
  bfd_size_type text_usage = 0;
  bfd_size_type eeprom_usage = 0;

  char *contents = elf32_avr_get_note_section_contents (abfd,
    &note_section_size);

  if (contents != NULL)
    {
      description = elf32_avr_get_note_desc (abfd, contents, note_section_size);
      elf32_avr_get_device_info (abfd, description, &device);
    }

  elf32_avr_get_memory_usage (abfd, &text_usage, &data_usage,
     &eeprom_usage);

  printf ("AVR Memory Usage\n"
          "----------------\n"
          "Device: %s\n\n", device.name);

  /* Text size */
  printf ("Program:%8ld bytes", text_usage);
  if (device.flash_size > 0)
    printf (" (%2.1f%% Full)", ((float) text_usage / device.flash_size) * 100);

  printf ("\n(.text + .data + .bootloader)\n\n");

  /* Data size */
  printf ("Data:   %8ld bytes", data_usage);
  if (device.ram_size > 0)
    printf (" (%2.1f%% Full)", ((float) data_usage / device.ram_size) * 100);

  printf ("\n(.data + .bss + .noinit)\n\n");

  /* EEPROM size */
  if (eeprom_usage > 0)
    {
      printf ("EEPROM: %8ld bytes", eeprom_usage);
      if (device.eeprom_size > 0)
        printf (" (%2.1f%% Full)", ((float) eeprom_usage / device.eeprom_size) * 100);

      printf ("\n(.eeprom)\n\n");
    }

  if (contents != NULL)
    free (contents);

}

static void
elf32_avr_dump_avr_prop (bfd *abfd)
{
  struct avr_property_record_list *r_list;
  unsigned int i;

  r_list = avr_elf32_load_property_records (abfd);
  if (r_list == NULL)
    return;

  printf ("\nContents of `%s' section:\n\n", r_list->section->name);

  printf ("  Version: %d\n", r_list->version);
  printf ("  Flags:   %#x\n\n", r_list->flags);

  for (i = 0; i < r_list->record_count; ++i)
    {
      printf ("   %d %s @ %s + %#08lx (%#08lx)\n",
              i,
              avr_elf32_property_record_name (&r_list->records [i]),
              r_list->records [i].section->name,
              r_list->records [i].offset,
              (bfd_get_section_vma (abfd, r_list->records [i].section)
               + r_list->records [i].offset));
      switch (r_list->records [i].type)
        {
        case RECORD_ORG:
          /* Nothing else to print.  */
          break;
        case RECORD_ORG_AND_FILL:
          printf ("     Fill: %#08lx\n",
                  r_list->records [i].data.org.fill);
          break;
        case RECORD_ALIGN:
          printf ("     Align: %#08lx\n",
                  r_list->records [i].data.align.bytes);
          break;
        case RECORD_ALIGN_AND_FILL:
          printf ("     Align: %#08lx, Fill: %#08lx\n",
                  r_list->records [i].data.align.bytes,
                  r_list->records [i].data.align.fill);
          break;
        }
    }

  free (r_list);
}

static void
elf32_avr_dump (bfd *abfd)
{
  if (options[OPT_MEMUSAGE].selected)
    elf32_avr_dump_mem_usage (abfd);
  if (options[OPT_AVRPROP].selected)
    elf32_avr_dump_avr_prop (abfd);
}

const struct objdump_private_desc objdump_private_desc_elf32_avr =
  {
    elf32_avr_help,
    elf32_avr_filter,
    elf32_avr_dump,
    options
  };
