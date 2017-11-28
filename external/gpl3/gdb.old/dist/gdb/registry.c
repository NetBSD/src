/* Support functions for general registry objects.

   Copyright (C) 2011-2016 Free Software Foundation, Inc.

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
#include "registry.h"
const struct registry_data *
register_data_with_cleanup (struct registry_data_registry *registry,
			    registry_data_callback save,
			    registry_data_callback free)
{
  struct registry_data_registration **curr;

  /* Append new registration.  */
  for (curr = &registry->registrations;
       *curr != NULL;
       curr = &(*curr)->next)
    ;

  *curr = XNEW (struct registry_data_registration);
  (*curr)->next = NULL;
  (*curr)->data = XNEW (struct registry_data);
  (*curr)->data->index = registry->num_registrations++;
  (*curr)->data->save = save;
  (*curr)->data->free = free;

  return (*curr)->data;
}

void
registry_alloc_data (struct registry_data_registry *registry,
		       struct registry_fields *fields)
{
  gdb_assert (fields->data == NULL);
  fields->num_data = registry->num_registrations;
  fields->data = XCNEWVEC (void *, fields->num_data);
}

void
registry_clear_data (struct registry_data_registry *data_registry,
		     registry_callback_adaptor adaptor,
		     struct registry_container *container,
		     struct registry_fields *fields)
{
  struct registry_data_registration *registration;
  int i;

  gdb_assert (fields->data != NULL);

  /* Process all the save handlers.  */

  for (registration = data_registry->registrations, i = 0;
       i < fields->num_data;
       registration = registration->next, i++)
    if (fields->data[i] != NULL && registration->data->save != NULL)
      adaptor (registration->data->save, container, fields->data[i]);

  /* Now process all the free handlers.  */

  for (registration = data_registry->registrations, i = 0;
       i < fields->num_data;
       registration = registration->next, i++)
    if (fields->data[i] != NULL && registration->data->free != NULL)
      adaptor (registration->data->free, container, fields->data[i]);

  memset (fields->data, 0, fields->num_data * sizeof (void *));
}

void
registry_container_free_data (struct registry_data_registry *data_registry,
			      registry_callback_adaptor adaptor,
			      struct registry_container *container,
			      struct registry_fields *fields)
{
  void ***rdata = &fields->data;
  gdb_assert (*rdata != NULL);
  registry_clear_data (data_registry, adaptor, container, fields);
  xfree (*rdata);
  *rdata = NULL;
}

void
registry_set_data (struct registry_fields *fields,
		   const struct registry_data *data,
		   void *value)
{
  gdb_assert (data->index < fields->num_data);
  fields->data[data->index] = value;
}

void *
registry_data (struct registry_fields *fields,
	       const struct registry_data *data)
{
  gdb_assert (data->index < fields->num_data);
  return fields->data[data->index];
}
