/* Subroutines for the gcc driver.
   Copyright (C) 2009 Free Software Foundation, Inc.
   Contributed by Anatoly Sokolov <aesok@post.ru>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include <stdlib.h>

/* Current architecture.  */
const struct base_arch_s *avr_current_arch = NULL;

/* Current device.  */
const struct mcu_type_s *avr_current_device = NULL;

/* Initialize avr_current_arch and avr_current_device variables.  */

static void
avr_set_current_device (const char *name)
{
 
 if (NULL != avr_current_arch)
   return;
 
  for (avr_current_device = avr_mcu_types; avr_current_device->name;
       avr_current_device++)
    {
      if (strcmp (avr_current_device->name, name) == 0)
        break;
    }

  avr_current_arch = &avr_arch_types[avr_current_device->arch];
}

/* Returns command line parameters that describe the device architecture.  */

const char *
avr_device_to_arch (int argc, const char **argv)
{
  if (0 == argc)
    return;

  avr_set_current_device (argv[0]);

  return concat ("-m ", avr_current_arch->arch_name, NULL);
}

/* Returns command line parameters that describe start of date section.  */

const char *
avr_device_to_data_start (int argc, const char **argv)
{
  unsigned long data_section_start;
  char data_section_start_str[16];

  if (0 == argc)
    return;  

  avr_set_current_device (argv[0]);
  
  if (avr_current_device->data_section_start 
      == avr_current_arch->default_data_section_start)
    return NULL;
    
  data_section_start = 0x800000 + avr_current_device->data_section_start;
  
  snprintf (data_section_start_str, sizeof(data_section_start_str) - 1,
            "0x%lX", data_section_start);
  
  return concat ("-Tdata ", data_section_start_str, NULL);    
}

/* Returns command line parameters that describe the device startfile.  */

const char *
avr_device_to_startfiles (int argc, const char **argv)
{
  if (0 == argc)
    return;

  avr_set_current_device (argv[0]);

  return concat ("crt", avr_current_device->library_name, ".o%s", NULL);
}

/* Returns command line parameters that describe the device library.  */

const char *
avr_device_to_devicelib (int argc, const char **argv)
{
  if (0 == argc)
    return;

  avr_set_current_device (argv[0]);

  return concat ("-l", avr_current_device->library_name, NULL);
}

