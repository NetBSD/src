/*  This file is part of the program psim.

    Copyright (C) 1994-1996, Andrew Cagney <cagney@highland.com.au>

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
    */


#ifndef _DEVICE_C_
#define _DEVICE_C_

#include <stdio.h>

#include "device_table.h"
#include "cap.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#include <ctype.h>


/* property entries */

typedef struct _device_property_entry device_property_entry;
struct _device_property_entry {
  device_property_entry *next;
  device_property *value;
};


/* Interrupt edges */

typedef struct _device_interrupt_edge device_interrupt_edge;
struct _device_interrupt_edge {
  int my_port;
  device *dest;
  int dest_port;
  device_interrupt_edge *next;
  int permenant;
};

STATIC_INLINE_DEVICE\
(void)
attach_device_interrupt_edge(device_interrupt_edge **list,
			     int my_port,
			     device *dest,
			     int dest_port,
			     int permenant)
{
  device_interrupt_edge *new_edge = ZALLOC(device_interrupt_edge);
  new_edge->my_port = my_port;
  new_edge->dest = dest;
  new_edge->dest_port = dest_port;
  new_edge->next = *list;
  new_edge->permenant = permenant;
  *list = new_edge;
}

STATIC_INLINE_DEVICE\
(void)
detach_device_interrupt_edge(device_interrupt_edge **list,
			     int my_port,
			     device *dest,
			     int dest_port)
{
  while (*list != NULL) {
    device_interrupt_edge *old_edge = *list;
    if (old_edge->dest == dest
	&& old_edge->dest_port == dest_port
	&& old_edge->my_port == my_port) {
      if (old_edge->permenant)
	error("attempt to delete permenant interrupt\n");
      *list = old_edge->next;
      zfree(old_edge);
      return;
    }
  }
  error("detach_device_interrupt_edge: interrupt not attached\n");
}

STATIC_INLINE_DEVICE\
(void)
clean_device_interrupt_edges(device_interrupt_edge **list)
{
  while (*list != NULL) {
    device_interrupt_edge *old_edge = *list;
    if (old_edge->permenant) {
      list = &old_edge->next;
    }
    else {
      *list = old_edge->next;
      zfree(old_edge);
    }
  }
}


/* A device */

struct _device {

  /* my name is ... */
  const char *name;
  device_unit unit_address;
  const char *path;

  /* device tree */
  device *parent;
  device *children;
  device *sibling;

  /* its template methods */
  void *data; /* device specific data */
  const device_callbacks *callback;

  /* device properties */
  device_property_entry *properties;

  /* interrupts */
  device_interrupt_edge *interrupt_destinations;
  device_interrupt_edge *child_interrupt_destinations;

  /* any open instances of this device */
  device_instance *instances;

  /* the internal/external mappings */
  cap *ihandles;
  cap *phandles;
};


/* an instance of a device */
struct _device_instance {
  device *owner;
  void *data;
  char *args;
  char *name;
  char *path;
  int permenant;
  device_instance *next;
};



/* Device node: */

INLINE_DEVICE\
(device *)
device_parent(device *me)
{
  return me->parent;
}

INLINE_DEVICE\
(device *)
device_sibling(device *me)
{
  return me->sibling;
}

INLINE_DEVICE\
(device *)
device_child(device *me)
{
  return me->children;
}

INLINE_DEVICE\
(const char *)
device_name(device *me)
{
  return me->name;
}

INLINE_DEVICE\
(const char *)
device_path(device *me)
{
  return me->path;
}

INLINE_DEVICE\
(void *)
device_data(device *me)
{
  return me->data;
}

INLINE_DEVICE\
(const device_unit *)
device_unit_address(device *me)
{
  return &me->unit_address;
}



/* device template: */

/* determine the full name of the device.  If buf is specified it is
   stored in there.  Failing that, a safe area of memory is allocated */

STATIC_INLINE_DEVICE\
(const char *)
device_full_name(device *leaf,
		 char *buf,
		 unsigned sizeof_buf)
{
  /* get a buffer */
  char full_name[1024];
  if (buf == (char*)0) {
    buf = full_name;
    sizeof_buf = sizeof(full_name);
  }

  /* construct a name */
  if (leaf->parent == NULL) {
    if (sizeof_buf < 1)
      error("device_full_name() buffer overflow\n");
    *buf = '\0';
  }
  else {
    char unit[1024];
    device_full_name(leaf->parent, buf, sizeof_buf);
    if (leaf->parent != NULL
	&& leaf->parent->callback->encode_unit(leaf->parent,
					       &leaf->unit_address,
					       unit+1,
					       sizeof(unit)-1) > 0)
      unit[0] = '@';
    else
      unit[0] = '\0';
    if (strlen(buf) + strlen("/") + strlen(leaf->name) + strlen(unit)
	>= sizeof_buf)
      error("device_full_name() buffer overflow\n");
    strcat(buf, "/");
    strcat(buf, leaf->name);
    strcat (buf, unit);
  }
  
  /* return it usefully */
  if (buf == full_name)
    buf = strdup(full_name);
  return buf;
}

/* manipulate/lookup device names */

typedef struct _name_specifier {
  /* components in the full length name */
  char *path;
  char *property;
  char *value;
  /* current device */
  char *name;
  char *unit;
  char *args;
  /* previous device */
  char *last_name;
  char *last_unit;
  char *last_args;
  /* work area */
  char buf[1024];
} name_specifier;

/* perform basic processing of a full name to give a specifier */
STATIC_INLINE_DEVICE\
(int)
split_device_specifier(const char *device_specifier,
		       name_specifier *spec)
{
  char *chp;
  if (strlen(device_specifier) >= sizeof(spec->buf))
    error("device-specifier %s too long\n", device_specifier);

  /* expand aliases (later) */
  strcpy(spec->buf, device_specifier);

  /* strip the leading spaces and check that remainder isn't a comment */
  chp = spec->buf;
  while (*chp != '\0' && isspace(*chp))
    chp++;
  if (*chp == '\0' || *chp == '#')
    return 0;

  /* find the path and terminate it with null */
  spec->path = chp;
  while (*chp != '\0' && !isspace(*chp))
    chp++;
  if (*chp != '\0') {
    *chp = '\0';
    chp++;
  }

  /* and any value */
  while (*chp != '\0' && isspace(*chp))
    chp++;
  spec->value = chp;

  /* now go back and chop the property off of the path */
  if (spec->value[0] == '\0') {
    spec->property = NULL; /*not a property*/
    spec->value = NULL;
  }
  else if (spec->value[0] == '>'
	   || spec->value[0] == '<') {
    /* an interrupt spec */
    spec->property = NULL;
  }
  else {
    chp = strrchr(spec->path, '/');
    if (chp == NULL) {
      spec->property = spec->path;
      spec->path = strchr(spec->property, '\0');
    }
    else {
      *chp = '\0';
      spec->property = chp+1;
    }
  }

  /* and mark the rest as invalid */
  spec->name = NULL;
  spec->unit = NULL;
  spec->args = NULL;
  spec->last_name = NULL;
  spec->last_unit = NULL;
  spec->last_args = NULL;

  return 1;
}

/* parse the last device name on the path assuming that it is a
   property name */
STATIC_INLINE_DEVICE\
(int)
split_property_specifier(const char *property_specifier,
			 name_specifier *spec)
{
  if (split_device_specifier(property_specifier, spec)) {
    char *chp = strrchr(spec->path, '/');
    if (chp == NULL) {
      spec->property = spec->path;
      spec->path = strrchr(spec->property, '\0');;
    }
    else {
      *chp = '\0';
      spec->property = chp+1;
    }
    return 1;
  }
  else
    return 0;
}

/* parse the next device name and split it up */
STATIC_INLINE_DEVICE\
(int)
split_device_name(name_specifier *spec)
{
  char *chp;
  /* remember what came before */
  spec->last_name = spec->name;
  spec->last_unit = spec->unit;
  spec->last_args = spec->args;
  /* finished? */
  if (spec->path[0] == '\0') {
    spec->name = NULL;
    spec->unit = NULL;
    spec->args = NULL;
    return 0;
  }
  /* break the device name from the path */
  spec->name = spec->path;
  chp = strchr(spec->name, '/');
  if (chp == NULL)
    spec->path = strchr(spec->name, '\0');
  else {
    spec->path = chp+1;
    *chp = '\0';
  }
  /* now break out the unit */
  chp = strchr(spec->name, '@');
  if (chp == NULL) {
    spec->unit = NULL;
    chp = spec->name;
  }
  else {
    *chp = '\0';
    chp += 1;
    spec->unit = chp;
  }
  /* finally any args */
  chp = strchr(chp, ':');
  if (chp == NULL)
    spec->args = NULL;
  else {
    *chp = '\0';
    spec->args = chp+1;
  }
  return 1;
}

STATIC_INLINE_DEVICE\
(device *)
split_find_device(device *current,
		  name_specifier *spec)
{
  /* skip leading `.' etc */
  while (1) {
    if (strncmp(spec->path, "/", strlen("/")) == 0) {
      /* cd /... */
      while (current != NULL && current->parent != NULL)
	current = current->parent;
      spec->path += strlen("/");
    }
    else if (strncmp(spec->path, "./", strlen("./")) == 0) {
      /* cd ./... */
      current = current;
      spec->path += strlen("./");
    }
    else if (strncmp(spec->path, "../", strlen("../")) == 0) {
      /* cd ../... */
      if (current != NULL && current->parent != NULL)
	current = current->parent;
      spec->path += strlen("../");
    }
    else if (strcmp(spec->path, ".") == 0) {
      /* cd . */
      current = current;
      spec->path += strlen(".");
    }
    else if (strcmp(spec->path, "..") == 0) {
      /* cd . */
      if (current != NULL && current->parent != NULL)
	current = current->parent;
      spec->path += strlen("..");
    }
    else
      break;
  }

  /* now go through the path proper */
  while (split_device_name(spec)) {
    device_unit phys;
    device *child;
    current->callback->decode_unit(current, spec->unit, &phys);
    for (child = current->children; child != NULL; child = child->sibling) {
      if (strcmp(spec->name, child->name) == 0) {
	if (phys.nr_cells == 0
	    || memcmp(&phys, &child->unit_address, sizeof(device_unit)) == 0)
	  break;
      }
    }
    if (child == NULL)
      return current; /* search failed */
    current = child;
  }

  return current;
}


INLINE_DEVICE\
(device *)
device_template_create_device(device *parent,
			      const char *name,
			      const char *unit_address,
			      const char *args)
{
  device_descriptor *descr;
  int name_len;
  char *chp;
  chp = strchr(name, '@');
  name_len = (chp == NULL ? strlen(name) : chp - name);
  for (descr = device_table; descr->name != NULL; descr++) {
    if (strncmp(name, descr->name, name_len) == 0
	&& (descr->name[name_len] == '\0'
	    || descr->name[name_len] == '@')) {
      device_unit address = { 0 };
      void *data = NULL;
      if (parent->callback->decode_unit != NULL)
	parent->callback->decode_unit(parent, 
				      unit_address,
				      &address);
      if (descr->creator != NULL)
	data = descr->creator(name, &address, args, parent);
      return device_create_from(name, &address, data,
				descr->callbacks, parent);
    }
  }
  error("device_template_create_device() unknown device %s\n", name);
  return NULL;
  error("not implenented\n");
}

INLINE_DEVICE\
(device *)
device_create_from(const char *name,
		   const device_unit *unit_address,
		   void *data,
		   const device_callbacks *callbacks,
		   device *parent)
{
  device *new_device = ZALLOC(device);

  /* insert it into the device tree */
  new_device->parent = parent;
  new_device->children = NULL;
  if (parent != NULL) {
    device **sibling = &parent->children;
    while ((*sibling) != NULL)
      sibling = &(*sibling)->sibling;
    *sibling = new_device;
  }

  /* give it a name */
  new_device->name = strdup(name);
  new_device->unit_address = *unit_address;
  new_device->path = device_full_name(new_device, NULL, 0);

  /* its template */
  new_device->data = data;
  new_device->callback = callbacks;

  /* its properties - already null */
  /* interrupts - already null */

  /* mappings - if needed */
  if (parent == NULL) {
    new_device->ihandles = cap_create(name);
    new_device->phandles = cap_create(name);
  }

  return new_device;
}



/* device-instance: */

INLINE_DEVICE\
(device_instance *)
device_instance_create(device *me,
		       const char *device_specifier,
		       int permenant)
{
  void *data = NULL;
  device_instance *instance;
  /* find the device node */
  name_specifier spec;
  if (!split_device_specifier(device_specifier, &spec))
    return NULL;
  me = split_find_device(me, &spec);
  if (spec.name != NULL)
    return NULL;
  /* create the instance */
  data = me->callback->instance_create(me, spec.last_args);
  if (data == NULL)
    return NULL;
  instance = ZALLOC(device_instance);
  instance->owner = me;
  instance->data = data;
  instance->args = (spec.last_args == NULL ? NULL : strdup(spec.last_args));
  instance->path = strdup(device_specifier);
  instance->name = strdup(spec.last_name);
  instance->permenant = permenant;
  /*instance->unit*/
  /*instance->parent*/
  instance->next = me->instances;
  me->instances = instance;
  return instance;
}

STATIC_INLINE_DEVICE\
(void)
clean_device_instances(device *me)
{
  device_instance **instance = &me->instances;
  while (*instance != NULL) {
    device_instance *old_instance = *instance;
    if (old_instance->permenant) {
      instance = &old_instance->next;
    }
    else {
      device_instance_delete(old_instance);
      instance = &me->instances;
    }
  }
}

INLINE_DEVICE\
(void)
device_instance_delete(device_instance *instance)
{
  device *me = instance->owner;
  device_instance **curr;
  if (instance->permenant)
    error("device_instance_delete: attempt to delete a permenant instance\n");
  me->callback->instance_delete(instance);
  if (instance->args != NULL)
    zfree(instance->args);
  if (instance->path != NULL)
    zfree(instance->path);
  if (instance->name != NULL)
    zfree(instance->name);
  curr = &me->instances;
  while (*curr != NULL && *curr != instance)
    curr = &(*curr)->next;
  ASSERT(*curr != NULL);
  *curr = instance->next;
  zfree(instance);
}

INLINE_DEVICE\
(int)
device_instance_read(device_instance *instance,
		     void *addr,
		     unsigned_word len)
{
  return instance->owner->callback->instance_read(instance,
						  addr,
						  len);
}

INLINE_DEVICE\
(int)
device_instance_write(device_instance *instance,
		      const void *addr,
		      unsigned_word len)
{
  return instance->owner->callback->instance_write(instance,
						   addr,
						   len);
}

INLINE_DEVICE\
(int)
device_instance_seek(device_instance *instance,
		     unsigned_word pos_hi,
		     unsigned_word pos_lo)
{
  return instance->owner->callback->instance_seek(instance,
						  pos_hi,
						  pos_lo);
}

INLINE_DEVICE\
(device *)
device_instance_device(device_instance *instance)
{
  return instance->owner;
}

INLINE_DEVICE\
(const char *)
device_instance_name(device_instance *instance)
{
  return instance->name;
}

INLINE_DEVICE\
(const char *)
device_instance_path(device_instance *instance)
{
  return instance->path;
}

INLINE_DEVICE\
(void *)
device_instance_data(device_instance *instance)
{
  return instance->data;
}



/* Device initialization: */

INLINE_DEVICE\
(void)
device_init_address(device *me,
		    psim *system)
{
  TRACE(trace_device_init, ("device_init_address() initializing device=0x%lx (%s)\n",
			    (long)me, me->path));
  me->callback->init_address(me, system);
}

INLINE_DEVICE\
(void)
device_init_data(device *me,
		 psim *system)
{
  TRACE(trace_device_init, ("device_init_data() initializing device=0x%lx (%s)\n",
			    (long)me, me->path));
  me->callback->init_data(me, system);
}

STATIC_INLINE_DEVICE\
(void)
clean_device(device *root,
	     void *data)
{
  psim *system;
  system = (psim*)data;
  clean_device_interrupt_edges(&root->interrupt_destinations);
  clean_device_interrupt_edges(&root->child_interrupt_destinations);
  clean_device_instances(root);
}

STATIC_INLINE_DEVICE\
(void)
init_device_address(device *root,
		    void *data)
{
  psim *system = (psim*)data;
  device_init_address(root, system);
}

STATIC_INLINE_DEVICE\
(void)
init_device_data(device *root,
		 void *data)
{
  psim *system = (psim*)data;
  device_init_data(root, system);
}

INLINE_DEVICE\
(void)
device_tree_init(device *root,
		 psim *system)
{
  TRACE(trace_device_tree, ("device_tree_init(root=0x%lx, system=0x%lx)\n",
			    (long)root,
			    (long)system));
  /* remove the old, rebuild the new */
  device_tree_traverse(root, clean_device, NULL, system);
  device_tree_traverse(root, init_device_address, NULL, system);
  device_tree_traverse(root, init_device_data, NULL, system);
  TRACE(trace_device_tree, ("device_tree_init() = void\n"));
}



/* Device Properties: */

INLINE_DEVICE\
(const device_property *)
device_next_property(const device_property *property)
{
  /* find the property in the list */
  device *owner = property->owner;
  device_property_entry *entry = owner->properties;
  while (entry != NULL && entry->value != property)
    entry = entry->next;
  /* now return the following property */
  ASSERT(entry != NULL); /* must be a member! */
  if (entry->next != NULL)
    return entry->next->value;
  else
    return NULL;
}

INLINE_DEVICE\
(const device_property *)
device_find_property(device *me,
		     const char *property)
{
  name_specifier spec;
  if (me == NULL) {
    return NULL;
  }
  else if (property == NULL || strcmp(property, "") == 0) {
    if (me->properties == NULL)
      return NULL;
    else
      return me->properties->value;
  }
  else if (split_property_specifier(property, &spec)) {
    me = split_find_device(me, &spec);
    if (spec.name == NULL) { /*got to root*/
      device_property_entry *entry = me->properties;
      while (entry != NULL) {
	if (strcmp(entry->value->name, spec.property) == 0)
	  return entry->value;
	entry = entry->next;
      }
    }
  }
  return NULL;
}

/* return NULL if fail */
INLINE_DEVICE\
(const device_property *)
device_find_array_property(device *me,
			   const char *property)
{
  const device_property *node;
  TRACE(trace_devices,
	("device_find_integer(me=0x%lx, property=%s)\n",
	 (long)me, property));
  node = device_find_property(me, property);
  if (node == (device_property*)0
      || node->type != array_property)
    error("%s property %s not found or of wrong type\n",
	  me->name, property);
  return node;
}


INLINE_DEVICE\
(int)
device_find_boolean_property(device *me,
			     const char *property)
{
  const device_property *node;
  unsigned32 boolean;
  TRACE(trace_devices,
	("device_find_boolean(me=0x%lx, property=%s)\n",
	 (long)me, property));
  node = device_find_property(me, property);
  if (node == (device_property*)0
      || node->type != boolean_property)
    error("%s property %s not found or of wrong type\n",
	  me->name, property);
  ASSERT(sizeof(boolean) == node->sizeof_array);
  memcpy(&boolean, node->array, sizeof(boolean));
  return boolean;
}

INLINE_DEVICE\
(device_instance *)
device_find_ihandle_property(device *me,
			     const char *property)
{
  const device_property *node;
  unsigned32 ihandle;
  device_instance *instance;
  TRACE(trace_devices,
	("device_find_ihandle_property(me=0x%lx, property=%s)\n",
	 (long)me, property));
  node = device_find_property(me, property);
  if (node == (device_property*)0
      || node->type != ihandle_property)
    error("%s property %s not found or of wrong type\n",
	  me->name, property);
  ASSERT(sizeof(ihandle) == node->sizeof_array);
  memcpy(&ihandle, node->array, sizeof(ihandle));
  BE2H(ihandle);
  instance = external_to_device_instance(me, ihandle);
  ASSERT(instance != NULL);
  return instance;
}

INLINE_DEVICE\
(signed_word)
device_find_integer_property(device *me,
			     const char *property)
{
  const device_property *node;
  signed32 integer;
  TRACE(trace_devices,
	("device_find_integer(me=0x%lx, property=%s)\n",
	 (long)me, property));
  node = device_find_property(me, property);
  if (node == (device_property*)0
      || node->type != integer_property)
    error("%s property %s not found or of wrong type\n",
	  me->name, property);
  ASSERT(sizeof(integer) == node->sizeof_array);
  memcpy(&integer, node->array, sizeof(integer));
  BE2H(integer);
  return integer;
}

INLINE_DEVICE\
(device *)
device_find_phandle_property(device *me,
			     const char *property)
{
  error("device_find_ihandle_property unimplemented\n");
  return 0;
}

INLINE_DEVICE\
(const char *)
device_find_string_property(device *me,
			    const char *property)
{
  const device_property *node;
  const char *string;
  TRACE(trace_devices,
	("device_find_string(me=0x%lx, property=%s)\n",
	 (long)me, property));
  node = device_find_property(me, property);
  if (node == (device_property*)0
      || node->type != string_property)
    error("%s property %s not found or of wrong type\n",
	  me->name, property);
  string = node->array;
  ASSERT(strlen(string) + 1 == node->sizeof_array);
  return string;
}

/* local - not available externally */
STATIC_INLINE_DEVICE\
(device_property *)
device_add_property(device *me,
		    const char *property,
		    device_property_type type,
		    const void *array,
		    int sizeof_array,
		    const device_property *original)
{
  device_property_entry *new_entry = 0;
  device_property *new_value = 0;
  void *new_array = 0;
  /* find the list end */
  device_property_entry **insertion_point = &me->properties;
  while (*insertion_point != NULL) {
    if (strcmp((*insertion_point)->value->name, property) == 0)
      return (*insertion_point)->value;
    insertion_point = &(*insertion_point)->next;
  }
  /* alloc data for the new property */
  new_entry = ZALLOC(device_property_entry);
  new_value = ZALLOC(device_property);
  new_array = (sizeof_array > 0
	       ? zalloc(sizeof_array)
	       : (void*)0);
  /* insert the new property into the list */
  *insertion_point = new_entry;
  new_entry->value = new_value;
  new_value->name = strdup(property);
  new_value->type = type;
  new_value->sizeof_array = sizeof_array;
  new_value->array = new_array;
  new_value->owner = me;
  new_value->original = original;
  if (sizeof_array > 0)
    memcpy(new_array, array, sizeof_array);
  return new_value;
}

INLINE_DEVICE\
(void)
device_add_duplicate_property(device *me,
			      const char *property,
			      const device_property *original)
{
  TRACE(trace_devices,
	("device_add_duplicate_property(me=0x%lx, property=%s, ...)\n",
	 (long)me, property));
  device_add_property(me, property,
		      original->type,
		      original->array,
		      original->sizeof_array,
		      original);
}

INLINE_DEVICE\
(void)
device_add_array_property(device *me,
			  const char *property,
			  const void *array,
			  int sizeof_array)
{
  TRACE(trace_devices,
	("device_add_array_property(me=0x%lx, property=%s, ...)\n",
	 (long)me, property));
  device_add_property(me, property,
		      array_property, array, sizeof_array, NULL);
}

INLINE_DEVICE\
(void)
device_add_boolean_property(device *me,
			    const char *property,
			    int boolean)
{
  signed32 new_boolean = (boolean ? -1 : 0);
  TRACE(trace_devices,
	("device_add_boolean(me=0x%lx, property=%s, boolean=%d)\n",
	 (long)me, property, boolean));
  device_add_property(me, property, boolean_property,
		      &new_boolean, sizeof(new_boolean), NULL);
}

INLINE_DEVICE\
(void)
device_add_ihandle_property(device *me,
			    const char *property,
			    device_instance *instance)
{
  unsigned32 ihandle = H2BE_4(device_instance_to_external(instance));
  TRACE(trace_devices,
	("device_add_ihandle_property(me=0x%lx, property=%s, instance=0x%lx)\n",
	 (long)me, property, (unsigned long)instance));
  device_add_property(me, property, ihandle_property,
		      &ihandle, sizeof(ihandle), NULL);
}

INLINE_DEVICE\
(void)
device_add_integer_property(device *me,
			    const char *property,
			    signed32 integer)
{
  TRACE(trace_devices,
	("device_add_integer_property(me=0x%lx, property=%s, integer=%ld)\n",
	 (long)me, property, (long)integer));
  H2BE(integer);
  device_add_property(me, property, integer_property,
		      &integer, sizeof(integer), NULL);
}

INLINE_DEVICE\
(void)
device_add_phandle_property(device *me,
			    const char *property,
			    device *phandle)
{
  error("device_add_phandle_property unimplemented\n");
}

INLINE_DEVICE\
(void)
device_add_string_property(device *me,
			   const char *property,
			   const char *string)
{

  TRACE(trace_devices,
	("device_add_property(me=0x%lx, property=%s, string=%s)\n",
	 (long)me, property, string));
  device_add_property(me, property, string_property,
		      string, strlen(string) + 1, NULL);
}



/* Device Hardware: */

INLINE_DEVICE\
(unsigned)
device_io_read_buffer(device *me,
		      void *dest,
		      int space,
		      unsigned_word addr,
		      unsigned nr_bytes,
		      cpu *processor,
		      unsigned_word cia)
{
  return me->callback->io_read_buffer(me, dest, space,
				       addr, nr_bytes,
				       processor, cia);
}

INLINE_DEVICE\
(unsigned)
device_io_write_buffer(device *me,
		       const void *source,
		       int space,
		       unsigned_word addr,
		       unsigned nr_bytes,
		       cpu *processor,
		       unsigned_word cia)
{
  return me->callback->io_write_buffer(me, source, space,
				       addr, nr_bytes,
				       processor, cia);
}

INLINE_DEVICE\
(unsigned)
device_dma_read_buffer(device *me,
		       void *dest,
		       int space,
		       unsigned_word addr,
		       unsigned nr_bytes)
{
  return me->callback->dma_read_buffer(me, dest, space,
				       addr, nr_bytes);
}

INLINE_DEVICE\
(unsigned)
device_dma_write_buffer(device *me,
			const void *source,
			int space,
			unsigned_word addr,
			unsigned nr_bytes,
			int violate_read_only_section)
{
  return me->callback->dma_write_buffer(me, source, space,
					addr, nr_bytes,
					violate_read_only_section);
}

INLINE_DEVICE\
(void)
device_attach_address(device *me,
		      const char *name,
		      attach_type attach,
		      int space,
		      unsigned_word addr,
		      unsigned nr_bytes,
		      access_type access,
		      device *who) /*callback/default*/
{
  me->callback->attach_address(me, name, attach, space,
				addr, nr_bytes, access, who);
}

INLINE_DEVICE\
(void)
device_detach_address(device *me,
		      const char *name,
		      attach_type attach,
		      int space,
		      unsigned_word addr,
		      unsigned nr_bytes,
		      access_type access,
		      device *who) /*callback/default*/
{
  me->callback->detach_address(me, name, attach, space,
				addr, nr_bytes, access, who);
}



/* Interrupts: */

INLINE_DEVICE(void)
device_interrupt_event(device *me,
		       int my_port,
		       int level,
		       cpu *processor,
		       unsigned_word cia)
{
  int found_a_handler = 0;
  device_interrupt_edge *edge;
  /* device's interrupt lines directly connected */
  for (edge = me->interrupt_destinations;
       edge != NULL;
       edge = edge->next) {
    if (edge->my_port == my_port) {
      if (edge->dest->callback->interrupt_event == NULL)
	error("device_interrupt_event: interrupt callback null\n");
      edge->dest->callback->interrupt_event(edge->dest,
					    edge->dest_port,
					    me,
					    my_port,
					    level,
					    processor, cia);
      found_a_handler = 1;
    }
  }
  if (!found_a_handler) {
    /* device's interrupt lines hopefully on a parents bus */
    device *parent;
    for (parent = me->parent;
	 parent != NULL;
	 parent = parent->parent) {
      for (edge = parent->child_interrupt_destinations;
	   edge != NULL;
	   edge = edge->next) {
	if (edge->dest->callback->child_interrupt_event == NULL)
	  error("device_interrupt_event: child_interrupt callback null\n");
	edge->dest->callback->child_interrupt_event(edge->dest,
						    parent,
						    me,
						    my_port,
						    level,
						    processor, cia);
	return;
      }
    }
    error("device_interrupt_event: fell out of tree\n");
  }
}

INLINE_DEVICE\
(void)
device_interrupt_attach(device *me,
			int my_port,
			device *dest,
			int dest_port,
			int permenant)
{
  attach_device_interrupt_edge(&me->interrupt_destinations,
			       my_port,
			       dest,
			       dest_port,
			       permenant);
}

INLINE_DEVICE\
(void)
device_interrupt_detach(device *me,
			int my_port,
			device *dest,
			int dest_port)
{
  detach_device_interrupt_edge(&me->interrupt_destinations,
			       my_port,
			       dest,
			       dest_port);
}

INLINE_DEVICE\
(void)
device_child_interrupt_attach(device *me,
			      device *dest,
			      int permenant)
{
  attach_device_interrupt_edge(&me->interrupt_destinations,
			       0, /*my_port*/
			       dest,
			       0, /*dest_port*/
			       permenant);
}

INLINE_DEVICE\
(void)
device_child_interrupt_detach(device *me,
			      device *dest)
{
  detach_device_interrupt_edge(&me->interrupt_destinations,
			       0, /*my_port*/
			       dest,
			       0); /*dest_port*/
}



/* IOCTL: */

EXTERN_DEVICE\
(void)
device_ioctl(device *me,
	     psim *system,
	     cpu *processor,
	     unsigned_word cia,
	     ...)
{
  va_list ap;
  va_start(ap, cia);
  me->callback->ioctl(me, system, processor, cia, ap);
  va_end(ap);
}
      


/* Tree utilities: */

EXTERN_DEVICE\
(device *)
device_tree_add_parsed(device *current,
		       const char *fmt,
		       ...)
{
  char device_specifier[1024];
  name_specifier spec;

  /* format the path */
  {
    va_list ap;
    va_start(ap, fmt);
    vsprintf(device_specifier, fmt, ap);
    va_end(ap);
    if (strlen(device_specifier) >= sizeof(device_specifier))
      error("error in device_tree_add_parsed - buffer overflow\n");
  }

  /* break it up */
  if (!split_device_specifier(device_specifier, &spec))
    error("error parsing device %s\n", device_path);

  /* fill our tree with its contents */
  current = split_find_device(current, &spec);

  /* add any additional devices as needed */
  if (spec.name != NULL) {
    do {
      current =
	device_template_create_device(current, spec.name, spec.unit, spec.args);
    } while (split_device_name(&spec));
  }

  /* is there an interrupt spec */
  if (spec.property == NULL
      && spec.value != NULL) {
    switch (spec.value[0]) {
    case '>':
      {
	int my_port = strtoul(spec.value+1, &spec.value, 0);
	int dest_port  = strtoul(spec.value+1, &spec.value, 0);
	device *dest = device_tree_find_device(current, spec.value);
	device_interrupt_attach(current,
				my_port,
				dest,
				dest_port,
				1);/*permenant*/
      }
      break;
    case '<':
      spec.value++;
      device_child_interrupt_attach(current,
				    device_tree_find_device(current,
							    spec.value),
				    1);/*permenant*/
      break;
    default:
      error("unreconised interrupt spec %s\n", spec.value);
      break;
    }
  }

  /* is there a property */
  if (spec.property != NULL) {
    if (strcmp(spec.value, "true") == 0)
      device_add_boolean_property(current, spec.property, 1);
    else if (strcmp(spec.value, "false") == 0)
      device_add_boolean_property(current, spec.property, 0);
    else {
      const device_property *property;
      switch (spec.value[0]) {
      case '*':
	spec.value++;
	device_add_ihandle_property(current, spec.property,
				    device_instance_create(current,
							   spec.value,
							   1/*permenant*/));
	break;
      case '-': case '+':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	device_add_integer_property(current, spec.property,
				    strtoul(spec.value, &spec.value, 0));
	break;
      case '[':
	{
	  unsigned8 words[1024];
	  char *curr = spec.value + 1;
	  int nr_words = 0;
	  while (1) {
	    char *next;
	    words[nr_words] = H2BE_1(strtoul(curr, &next, 0));
	    if (curr == next)
	      break;
	    curr = next;
	    nr_words += 1;
	  }
	  device_add_array_property(current, spec.property,
				    words, sizeof(words[0]) * nr_words);
	}
	break;
      case '{':
	{
	  unsigned32 words[1024];
	  char *curr = spec.value + 1;
	  int nr_words = 0;
	  while (1) {
	    char *next;
	    words[nr_words] = H2BE_4(strtoul(curr, &next, 0));
	    if (curr == next)
	      break;
	    curr = next;
	    nr_words += 1;
	  }
	  device_add_array_property(current, spec.property,
				    words, sizeof(words[0]) * nr_words);
	}
	break;
      case '&':
	spec.value++;
	device_add_phandle_property(current, spec.property,
				    device_tree_find_device(current,
							    spec.value));
	break;
      case '"':
	spec.value++;
      default:
	device_add_string_property(current, spec.property, spec.value);
	break;
      case '!':
	spec.value++;
	property = device_find_property(current, spec.value);
	if (property == NULL)
	  error("Can not duplicate property, original %s not found\n",
		spec.value);
	device_add_duplicate_property(current,
				      spec.property,
				      property);
	break;
      }
    }
  }
  return current;
}

INLINE_DEVICE\
(void)
device_tree_traverse(device *root,
		     device_tree_traverse_function *prefix,
		     device_tree_traverse_function *postfix,
		     void *data)
{
  device *child;
  if (prefix != NULL)
    prefix(root, data);
  for (child = root->children; child != NULL; child = child->sibling) {
    device_tree_traverse(child, prefix, postfix, data);
  }
  if (postfix != NULL)
    postfix(root, data);
}

INLINE_DEVICE\
(void)
device_tree_print_device(device *me,
			 void *ignore_or_null)
{
  const device_property *property;
  device_interrupt_edge *interrupt_edge;
  /* output my name */
  printf_filtered("%s\n", me->path);
  /* properties */
  for (property = device_find_property(me, NULL);
       property != NULL;
       property = device_next_property(property)) {
    printf_filtered("%s/%s", me->path, property->name);
    if (property->original != NULL) {
      printf_filtered(" !");
      printf_filtered("%s/%s\n", property->original->owner->path,
		      property->original->name);
    }
    else {
      switch (property->type) {
      case array_property:
	{
	  if ((property->sizeof_array % sizeof(unsigned32)) == 0) {
	    unsigned32 *w = (unsigned32*)property->array;
	    printf_filtered(" {");
	    while ((char*)w - (char*)property->array < property->sizeof_array) {
	      printf_filtered(" 0x%lx", BE2H_4(*w));
	      w++;
	    }
	  }
	  else {
	    unsigned8 *w = (unsigned8*)property->array;
	    printf_filtered(" [");
	    while ((char*)w - (char*)property->array < property->sizeof_array) {
	      printf_filtered(" 0x%2x", BE2H_1(*w));
	      w++;
	    }
	  }
	  printf_filtered("\n");
	}
	break;
      case boolean_property:
	{
	  int b = device_find_boolean_property(me, property->name);
	  printf_filtered(" %s\n", b ? "true"  : "false");
	}
	break;
      case ihandle_property:
	{
	  device_instance *i = device_find_ihandle_property(me, property-> name);
	  printf_filtered(" *%s\n", i->path);
	}
	break;
      case integer_property:
	{
	  unsigned_word w = device_find_integer_property(me, property->name);
	  printf_filtered(" 0x%lx\n", (unsigned long)w);
	}
	break;
      case phandle_property:
	{
	  device *d = device_find_phandle_property(me, property->name);
	  printf_filtered(" *%s\n", d->path);
	}
	break;
      case string_property:
	{
	  const char *s = device_find_string_property(me, property->name);
	  printf_filtered(" \"%s\n", s);
	}
	break;
      }
    }
  }
  /* interrupts */
  for (interrupt_edge = me->interrupt_destinations;
       interrupt_edge != NULL;
       interrupt_edge = interrupt_edge->next) {
    printf_filtered("%s >%d %d %s\n",
		    me->path,
		    interrupt_edge->my_port,
		    interrupt_edge->dest_port,
		    interrupt_edge->dest->path);
  }
  /* child interrupts */
  for (interrupt_edge = me->child_interrupt_destinations;
       interrupt_edge != NULL;
       interrupt_edge = interrupt_edge->next) {
    printf_filtered("%s <%s\n", me->path, interrupt_edge->dest->path);
  }
}

INLINE_DEVICE\
(device *)
device_tree_find_device(device *root,
			const char *path)
{
  device *node;
  name_specifier spec;
  TRACE(trace_device_tree,
	("device_tree_find_device_tree(root=0x%lx, path=%s)\n",
	 (long)root, path));
  /* parse the path */
  split_device_specifier(path, &spec);
  if (spec.value != NULL)
    return NULL; /* something wierd */

  /* now find it */
  node = split_find_device(root, &spec);
  if (spec.name != NULL)
    return NULL; /* not a leaf */

  return node;
}

INLINE_DEVICE\
(void)
device_usage(int verbose)
{
  if (verbose == 1) {
    device_descriptor *descr;
    int pos;
    printf_filtered("\n");
    printf_filtered("A device/property specifier has the form:\n");
    printf_filtered("\n");
    printf_filtered("  /path/to/a/device [ property-value ]\n");
    printf_filtered("\n");
    printf_filtered("and a possible device is\n");
    printf_filtered("\n");
    pos = 0;
    for (descr = device_table; descr->name != NULL; descr++) {
      pos += strlen(descr->name) + 2;
	if (pos > 75) {
	  pos = strlen(descr->name) + 2;
	  printf_filtered("\n");
	}
	printf_filtered("  %s", descr->name);
      }
      printf_filtered("\n");
    }
  if (verbose > 1) {
    device_descriptor *descr;
    printf_filtered("\n");
    printf_filtered("A device/property specifier (<spec>) has the format:\n");
    printf_filtered("\n");
    printf_filtered("  <spec> ::= <path> [ <value> ] ;\n");
    printf_filtered("  <path> ::= { <prefix> } { <node> \"/\" } <node> ;\n");
    printf_filtered("  <prefix> ::= ( | \"/\" | \"../\" | \"./\" ) ;\n");
    printf_filtered("  <node> ::= <name> [ \"@\" <unit> ] [ \":\" <args> ] ;\n");
    printf_filtered("  <unit> ::= <number> { \",\" <number> } ;\n");
    printf_filtered("\n");
    printf_filtered("Where:\n");
    printf_filtered("\n");
    printf_filtered("  <name>  is the name of a device (list below)\n");
    printf_filtered("  <unit>  is the unit-address relative to the parent bus\n");
    printf_filtered("  <args>  additional arguments used when creating the device\n");
    printf_filtered("  <value> ::= ( <number> # integer property\n");
    printf_filtered("              | \"[\" { <number> } # array property (byte)\n");
    printf_filtered("              | \"{\" { <number> } # array property (cell)\n");
    printf_filtered("              | [ \"true\" | \"false\" ] # boolean property\n");
    printf_filtered("              | \"*\" <path> # ihandle property\n");
    printf_filtered("              | \"*\" <path> # ihandle property\n");
    printf_filtered("              | \"!\" <path> # copy property\n");
    printf_filtered("              | \">\" [ <number> ] <path> # attach interrupt\n");
    printf_filtered("              | \"<\" <path> # attach child interrupt\n");
    printf_filtered("              | \"\\\"\" <text> # string property\n");
    printf_filtered("              | <text> # string property\n");
    printf_filtered("              ) ;\n");
    printf_filtered("\n");
    printf_filtered("And the following are valid device names:\n");
    printf_filtered("\n");
    for (descr = device_table; descr->name != NULL; descr++) {
      if (descr->callbacks->usage != NULL)
	descr->callbacks->usage(verbose);
      else
	printf_filtered("  %s\n", descr->name);
    }
  }
}



/* External representation */

INLINE_DEVICE\
(device *)
external_to_device(device *tree_member,
		   unsigned32 phandle)
{
  device *root = device_tree_find_device(tree_member, "/");
  device *me = cap_internal(root->phandles, phandle);
  return me;
}

INLINE_DEVICE\
(unsigned32)
device_to_external(device *me)
{
  device *root = device_tree_find_device(me, "/");
  unsigned32 phandle = cap_external(root->phandles, me);
  return phandle;
}

INLINE_DEVICE\
(device_instance *)
external_to_device_instance(device *tree_member,
			    unsigned32 ihandle)
{
  device *root = device_tree_find_device(tree_member, "/");
  device_instance *instance = cap_internal(root->ihandles, ihandle);
  return instance;
}

INLINE_DEVICE\
(unsigned32)
device_instance_to_external(device_instance *instance)
{
  device *root = device_tree_find_device(instance->owner, "/");
  unsigned32 ihandle = cap_external(root->ihandles, instance);
  return ihandle;
}

#endif /* _DEVICE_C_ */
