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


#ifndef _DEVICE_TABLE_H_
#define _DEVICE_TABLE_H_

#include <stdarg.h>

#include "basics.h"
#include "device.h"

typedef void (device_init_callback)
     (device *me,
      psim *system);
     
typedef void (device_config_address_callback)
     (device *me,
      const char *name,
      attach_type attach,
      int space,
      unsigned_word addr,
      unsigned nr_bytes,
      access_type access,
      device *who); /*callback/default*/

typedef unsigned (device_io_read_buffer_callback)
     (device *me,
      void *dest,
      int space,
      unsigned_word addr,
      unsigned nr_bytes,
      cpu *processor,
      unsigned_word cia);

typedef unsigned (device_io_write_buffer_callback)
     (device *me,
      const void *source,
      int space,
      unsigned_word addr,
      unsigned nr_bytes,
      cpu *processor,
      unsigned_word cia);

typedef unsigned (device_dma_read_buffer_callback)
     (device *me,
      void *dest,
      int space,
      unsigned_word addr,
      unsigned nr_bytes);

typedef unsigned (device_dma_write_buffer_callback)
     (device *me,
      const void *source,
      int space,
      unsigned_word addr,
      unsigned nr_bytes,
      int violate_read_only_section);


/* Interrupts */

typedef void (device_interrupt_event_callback)
     (device *me,
      int my_port,
      device *source,
      int source_port,
      int level,
      cpu *processor,
      unsigned_word cia);

typedef void (device_child_interrupt_event_callback)
     (device *me,
      device *parent,
      device *source,
      int source_port,
      int level,
      cpu *processor,
      unsigned_word cia);
      

/* bus address decoding */

typedef int (device_unit_decode_callback)
     (device *me,
      const char *unit,
      device_unit *address);

typedef int (device_unit_encode_callback)
     (device *me,
      const device_unit *unit_address,
      char *buf,
      int sizeof_buf);


/* instances */

typedef void *(device_instance_create_callback)
     (device *me,
      const char *args);

typedef void (device_instance_delete_callback)
     (device_instance *instance);

typedef int (device_instance_read_callback)
     (device_instance *instance,
      void *buf,
      unsigned_word len);

typedef int (device_instance_write_callback)
     (device_instance *instance,
      const void *buf,
      unsigned_word len);

typedef int (device_instance_seek_callback)
     (device_instance *instance,
      unsigned_word pos_hi,
      unsigned_word pos_lo);


/* all else fails */

typedef void (device_ioctl_callback)
     (device *me,
      psim *system,
      cpu *processor,
      unsigned_word cia,
      va_list ap);

typedef void (device_usage_callback)
     (int verbose);


/* the callbacks */

struct _device_callbacks {

  /* initialization */
  device_init_callback *init_address;
  device_init_callback *init_data;

  /* address/data config - from child */
  device_config_address_callback *attach_address;
  device_config_address_callback *detach_address;

  /* address/data transfer - to child */
  device_io_read_buffer_callback *io_read_buffer;
  device_io_write_buffer_callback *io_write_buffer;

  /* address/data transfer - from child */
  device_dma_read_buffer_callback *dma_read_buffer;
  device_dma_write_buffer_callback *dma_write_buffer;

  /* interrupt signalling */
  device_interrupt_event_callback *interrupt_event;
  device_child_interrupt_event_callback *child_interrupt_event;

  /* bus address decoding */
  device_unit_decode_callback *decode_unit;
  device_unit_encode_callback *encode_unit;

  /* instances */
  device_instance_create_callback *instance_create;
  device_instance_delete_callback *instance_delete;
  device_instance_read_callback *instance_read;
  device_instance_write_callback *instance_write;
  device_instance_seek_callback *instance_seek;

  /* back door to anything we've forgot */
  device_ioctl_callback *ioctl;
  device_usage_callback *usage;
};


/* Table of all the devices and a function to lookup/create a device
   from its name */

typedef void *(device_creator)
     (const char *name,
      const device_unit *unit_address,
      const char *args,
      device *parent);

typedef struct _device_descriptor device_descriptor;
struct _device_descriptor {
  const char *name;
  device_creator *creator;
  const device_callbacks *callbacks;
};

extern device_descriptor device_table[];


/* Unimplemented call back functions.  These abort the simulation */

extern device_init_callback unimp_device_init;
extern device_config_address_callback unimp_device_attach_address;
extern device_config_address_callback unimp_device_detach_address;
extern device_io_read_buffer_callback unimp_device_io_read_buffer;
extern device_io_write_buffer_callback unimp_device_io_write_buffer;
extern device_dma_read_buffer_callback unimp_device_dma_read_buffer;
extern device_dma_write_buffer_callback unimp_device_dma_write_buffer;
extern device_interrupt_event_callback unimp_device_interrupt_event;
extern device_child_interrupt_event_callback unimp_device_child_interrupt_event;
extern device_unit_decode_callback unimp_device_unit_decode;
extern device_unit_encode_callback unimp_device_unit_encode;
extern device_instance_create_callback unimp_device_instance_create;
extern device_instance_delete_callback unimp_device_instance_delete;
extern device_instance_read_callback unimp_device_instance_read;
extern device_instance_write_callback unimp_device_instance_write;
extern device_instance_seek_callback unimp_device_instance_seek;
extern device_ioctl_callback unimp_device_ioctl;


/* Pass through, ignore and generic callback functions.  A call going
   towards the root device are passed on up, local calls are ignored
   and call downs abort */

extern device_config_address_callback passthrough_device_attach_address;
extern device_config_address_callback passthrough_device_detach_address;
extern device_dma_read_buffer_callback passthrough_device_dma_read_buffer;
extern device_dma_write_buffer_callback passthrough_device_dma_write_buffer;

extern device_init_callback ignore_device_init;
extern device_unit_decode_callback ignore_device_unit_decode;

extern device_init_callback generic_device_init_address;
extern device_unit_decode_callback generic_device_unit_decode;
extern device_unit_encode_callback generic_device_unit_encode;


extern const device_callbacks passthrough_device_callbacks;


#endif /* _DEVICE_TABLE_H_ */
