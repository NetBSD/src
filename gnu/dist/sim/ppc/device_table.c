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


#ifndef _DEVICE_TABLE_C_
#define _DEVICE_TABLE_C_

#ifndef STATIC_INLINE_DEVICE_TABLE
#define STATIC_INLINE_DEVICE_TABLE STATIC_INLINE
#endif

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <ctype.h>

#include "device_table.h"

#include "events.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

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

#include "cpu.h"

#include "bfd.h"

/* Helper functions */

/* Generic device init: Attaches the device of size <nr_bytes> (taken
   from <name>@<int>,<nr_bytes>) to its parent at address zero and
   with read/write access. */

typedef struct _reg_spec {
  unsigned32 base;
  unsigned32 size;
} reg_spec;

void
generic_device_init_address(device *me,
			    psim *system)
{
  const device_property *reg = device_find_array_property(me, "reg");
  const reg_spec *spec = reg->array;
  int nr_entries = reg->sizeof_array / sizeof(reg_spec);

  if ((reg->sizeof_array % sizeof(reg_spec)) != 0)
    error("devices/%s reg property is of wrong size\n", device_name(me));
 
  while (nr_entries > 0) {
    device_attach_address(device_parent(me),
			  device_name(me),
			  attach_callback,
			  0 /*space*/,
			  BE2H_4(spec->base),
			  BE2H_4(spec->size),
			  access_read_write_exec,
			  me);
    spec++;
    nr_entries--;
  }
}

int
generic_device_unit_decode(device *me,
			   const char *unit,
			   device_unit *phys)
{
  memset(phys, 0, sizeof(device_unit));
  if (unit == NULL)
    return 0;
  else {
    char *pos = (char*)unit; /* force for strtoul() */
    while (1) {
      char *old_pos = pos;
      long int val = strtoul(pos, &pos, 0);
      if (old_pos == pos && *pos == '\0')
	return phys->nr_cells;
      if (old_pos == pos && *pos != '\0')
	return -1;
      if (phys->nr_cells == 4)
	return -1;
      phys->cells[phys->nr_cells] = val;
      phys->nr_cells++;
    }
  }
}

int
generic_device_unit_encode(device *me,
			   const device_unit *phys,
			   char *buf,
			   int sizeof_buf)
{
  int i;
  int len;
  char *pos = buf; /* force for strtoul() */
  for (i = 0; i < phys->nr_cells; i++) {
    if (pos != buf) {
      strcat(pos, ",");
      pos = strchr(pos, '\0');
    }
    sprintf(pos, "0x%lx", (unsigned long)phys->cells[i]);
    pos = strchr(pos, '\0');
  }
  len = pos - buf;
  if (len >= sizeof_buf)
    error("generic_unit_encode - buffer overflow\n");
  return len;
}

/* DMA a file into memory */
STATIC_INLINE_DEVICE_TABLE int
dma_file(device *me,
	 const char *file_name,
	 unsigned_word addr)
{
  int count;
  int inc;
  FILE *image;
  char buf[1024];

  /* get it open */
  image = fopen(file_name, "r");
  if (image == NULL)
    return -1;

  /* read it in slowly */
  count = 0;
  while (1) {
    inc = fread(buf, 1, sizeof(buf), image);
    if (feof(image) || ferror(image))
      break;
    if (device_dma_write_buffer(device_parent(me),
				buf,
				0 /*address-space*/,
				addr+count,
				inc /*nr-bytes*/,
				1 /*violate ro*/) != inc) {
      fclose(image);
      return -1;
    }
    count += inc;
  }

  /* close down again */
  fclose(image);

  return count;
}



/* inimplemented versions of each function */

void
unimp_device_init(device *me,
		  psim *system)
{
  error("device_init_callback for %s not implemented\n", device_name(me));
}

void
unimp_device_attach_address(device *me,
			    const char *name,
			    attach_type type,
			    int space,
			    unsigned_word addr,
			    unsigned nr_bytes,
			    access_type access,
			    device *who) /*callback/default*/
{
  error("device_attach_address_callback for %s not implemented\n", device_name(me));
}

void
unimp_device_detach_address(device *me,
			    const char *name,
			    attach_type type,
			    int space,
			    unsigned_word addr,
			    unsigned nr_bytes,
			    access_type access,
			    device *who) /*callback/default*/
{
  error("device_detach_address_callback for %s not implemented\n", device_name(me));
}

unsigned
unimp_device_io_read_buffer(device *me,
			    void *dest,
			    int space,
			    unsigned_word addr,
			    unsigned nr_bytes,
			    cpu *processor,
			    unsigned_word cia)
{
  error("device_io_read_buffer_callback for %s not implemented\n", device_name(me));
  return 0;
}

unsigned
unimp_device_io_write_buffer(device *me,
			     const void *source,
			     int space,
			     unsigned_word addr,
			     unsigned nr_bytes,
			     cpu *processor,
			     unsigned_word cia)
{
  error("device_io_write_buffer_callback for %s not implemented\n", device_name(me));
  return 0;
}

unsigned
unimp_device_dma_read_buffer(device *me,
			     void *target,
			     int space,
			     unsigned_word addr,
			     unsigned nr_bytes)
{
  error("device_dma_read_buffer_callback for %s not implemented\n", device_name(me));
  return 0;
}

unsigned
unimp_device_dma_write_buffer(device *me,
			      const void *source,
			      int space,
			      unsigned_word addr,
			      unsigned nr_bytes,
			      int violate_read_only_section)
{
  error("device_dma_write_buffer_callback for %s not implemented\n", device_name(me));
  return 0;
}

void
unimp_device_interrupt_event(device *me,
			     int my_port,
			     device *source,
			     int source_port,
			     int level,
			     cpu *processor,
			     unsigned_word cia)
{
  error("unimp_device_interrupt_event for %s unimplemented\n",
	device_name(me));
}

void
unimp_device_child_interrupt_event(device *me,
				   device *parent,
				   device *source,
				   int source_port,
				   int level,
				   cpu *processor,
				   unsigned_word cia)
{
  error("unimp_device_child_interrupt_event_callback for %s unimplemented\n",
	device_name(me));
}
      
int
unimp_device_unit_decode(device *me,
			 const char *unit,
			 device_unit *address)
{
  error("unimp_device_unit_decode_callback for %s unimplemented\n",
	device_name(me));
  return 0;
}

int
unimp_device_unit_encode(device *me,
			 const device_unit *unit_address,
			 char *buf,
			 int sizeof_buf)
{
  error("unimp_device_unit_encode_callback for %s unimplemented\n",
	device_name(me));
  return 0;
}

void *
unimp_device_instance_create(device *me,
			     const char *args)
{
  error("unimp_device_instance_create_callback for %s unimplemented\n",
	device_name(me));
  return 0;
}

void
unimp_device_instance_delete(device_instance *instance)
{
  error("unimp_device_instance_delete_callback for %s unimplemented\n",
	device_instance_name(instance));
}

int
unimp_device_instance_read(device_instance *instance,
			   void *buf,
			   unsigned_word len)
{
  error("unimp_device_instance_read_callback for %s unimplemented\n",
	device_instance_name(instance));
  return 0;
}

int
unimp_device_instance_write(device_instance *instance,
			    const void *buf,
			    unsigned_word len)
{
  error("unimp_device_instance_write_callback for %s unimplemented\n",
	device_instance_name(instance));
  return 0;
}

int
unimp_device_instance_seek(device_instance *instance,
			   unsigned_word pos_hi,
			   unsigned_word pos_lo)
{
  error("unimp_device_instance_seek_callback for %s unimplemented\n",
	device_instance_name(instance));
  return 0;
}


void
unimp_device_ioctl(device *me,
		   psim *system,
		   cpu *processor,
		   unsigned_word cia,
		   va_list ap)
{
  error("device_ioctl_callback for %s not implemented\n", device_name(me));
}



/* ignore/passthrough versions of each function */

void
ignore_device_init(device *me,
		   psim *system)
{
  /*null*/
}

void
passthrough_device_attach_address(device *me,
				  const char *name,
				  attach_type attach,
				  int space,
				  unsigned_word addr,
				  unsigned nr_bytes,
				  access_type access,
				  device *who) /*callback/default*/
{
  device_attach_address(device_parent(me), name, attach,
			space, addr, nr_bytes,
			access,
			who);
}

void
passthrough_device_detach_address(device *me,
				  const char *name,
				  attach_type attach,
				  int space,
				  unsigned_word addr,
				  unsigned nr_bytes,
				  access_type access,
				  device *who) /*callback/default*/
{
  device_detach_address(device_parent(me), name, attach,
			space, addr, nr_bytes, access,
			who);
}

unsigned
passthrough_device_dma_read_buffer(device *me,
				   void *dest,
				   int space,
				   unsigned_word addr,
				   unsigned nr_bytes)
{
  return device_dma_read_buffer(device_parent(me), dest,
				space, addr, nr_bytes);
}

unsigned
passthrough_device_dma_write_buffer(device *me,
			     const void *source,
			     int space,
			     unsigned_word addr,
			     unsigned nr_bytes,
			     int violate_read_only_section)
{
  return device_dma_write_buffer(device_parent(me), source,
				 space, addr,
				 nr_bytes,
				 violate_read_only_section);
}

int
ignore_device_unit_decode(device *me,
			  const char *unit,
			  device_unit *phys)
{
  memset(phys, 0, sizeof(device_unit));
  return 0;
}


static const device_callbacks passthrough_callbacks = {
  ignore_device_init,
  ignore_device_init,
  passthrough_device_attach_address,
  passthrough_device_detach_address,
  unimp_device_io_read_buffer,
  unimp_device_io_write_buffer,
  passthrough_device_dma_read_buffer,
  passthrough_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  generic_device_unit_decode,
  generic_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* Simple console device: console@<address>,16

   Input characters are taken from the keyboard, output characters
   sent to the terminal.  Echoing of characters is not disabled.

   The device has four registers:

   0x0: read
   0x4: read-status
   0x8: write
   0xC: write-status

   Where a nonzero status register indicates that the device is ready
   (input fifo contains a character or output fifo has space). */

typedef struct _console_buffer {
  char buffer;
  int status;
  event_entry_tag event_tag;
} console_buffer;

typedef struct _console_device {
  console_buffer input;
  console_buffer output;
} console_device;

typedef enum {
  console_read_buffer = 0,
  console_read_status = 4,
  console_write_buffer = 8,
  console_write_status = 12,
  console_offset_mask = 0xc,
  console_size = 16,
} console_offsets;

/* check the console for an available character */
static void
scan_console(console_device *console)
{ /* check for input */
  int flags;
  int status;
  /* get the old status */
  flags = fcntl(0, F_GETFL, 0);
  if (flags == -1) {
    perror("console");
    return;
  }
  /* temp, disable blocking IO */
  status = fcntl(0, F_SETFL, flags | O_NDELAY);
  if (status == -1) {
    perror("console");
    return;
  }
  /* try for input */
  status = read(0, &console->input.buffer, 1);
  if (status == 1) {
    console->input.status = 1;
  }
  else {
    console->input.status = 0;
  }
  /* return to regular vewing */
  flags = fcntl(0, F_SETFL, flags);
  if (flags == -1) {
    perror("console");
    return;
  }
}

/* write the character to the console */
static void
write_console(console_device *console,
	      char val)
{
  DTRACE(console, ("<%c:%d>", val, val));
  printf_filtered("%c", val) ;
  console->output.buffer = val;
  console->output.status = 1;
}

static unsigned
console_io_read_buffer_callback(device *me,
				void *dest,
				int space,
				unsigned_word addr,
				unsigned nr_bytes,
				cpu *processor,
				unsigned_word cia)
{
  console_device *console = (console_device*)device_data(me);
  unsigned_1 val;

  /* determine what was read */

  switch ((int)addr & console_offset_mask) {

  case console_read_buffer:
    val = console->input.buffer;
    break;

  case console_read_status:
    scan_console(console);
    val = console->input.status;
    break;

  case console_write_buffer:
    val = console->output.buffer;
    break;

  case console_write_status:
    val = console->output.status;
    break;

  default:
    error("console_read_callback() internal error\n");
    val = 0;
    break;

  }

  memset(dest, 0, nr_bytes);
  *(unsigned_1*)dest = val;
  return nr_bytes;
}

static unsigned
console_io_write_buffer_callback(device *me,
				 const void *source,
				 int space,
				 unsigned_word addr,
				 unsigned nr_bytes,
				 cpu *processor,
				 unsigned_word cia)
{
  console_device *console = (console_device*)device_data(me);
  unsigned_1 val = *(unsigned_1*)source;

  switch ((int)addr & console_offset_mask) {

  case console_read_buffer:
    console->input.buffer = val;
    break;

  case console_read_status:
    console->input.status = val;
    break;

  case console_write_buffer:
    write_console(console, val);
    break;

  case console_write_status:
    console->output.status = val;
    break;

  default:
    error("console_write_callback() internal error\n");

  }
	 
  return nr_bytes;
}

/* instances of the console device */
static void *
console_instance_create_callback(device *me,
				 const char *args)
{
  /* make life easier, attach the console data to the instance */
  return device_data(me);
}

static void
console_instance_delete_callback(device_instance *instance)
{
  /* nothing to delete, the console is attached to the device */
  return;
}

static int
console_instance_read_callback(device_instance *instance,
			       void *buf,
			       unsigned_word len)
{
  console_device *console = device_instance_data(instance);
  if (!console->input.status)
    scan_console(console);
  if (console->input.status) {
    *(char*)buf = console->input.buffer;
    console->input.status = 0;
    return 1;
  }
  else {
    return -2; /* not ready */
  }
}

static int
console_instance_write_callback(device_instance *instance,
				const void *buf,
				unsigned_word len)
{
  int i;
  const char *chp = buf;
  console_device *console = device_instance_data(instance);
  for (i = 0; i < len; i++)
    write_console(console, chp[i]);
  return i;
}

static device_callbacks const console_callbacks = {
  generic_device_init_address,
  ignore_device_init,
  unimp_device_attach_address,
  unimp_device_detach_address,
  console_io_read_buffer_callback,
  console_io_write_buffer_callback,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  unimp_device_unit_decode,
  unimp_device_unit_encode,
  console_instance_create_callback,
  console_instance_delete_callback,
  console_instance_read_callback,
  console_instance_write_callback,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};


static void *
console_create(const char *name,
	       const device_unit *unit_address,
	       const char *args,
	       device *parent)
{
  /* create the descriptor */
  console_device *console = ZALLOC(console_device);
  console->output.status = 1;
  console->output.buffer = '\0';
  console->input.status = 0;
  console->input.buffer = '\0';
  return console;
}



/* ICU device: icu@<address>

   <address> : read - processor nr
   <address> : write - interrupt processor nr
   <address> + 4 : read - nr processors

   Single byte registers that control a simple ICU.

   Illustrates passing of events to parent device. Passing of
   interrupts to an interrupt destination. */


static unsigned
icu_io_read_buffer_callback(device *me,
			    void *dest,
			    int space,
			    unsigned_word addr,
			    unsigned nr_bytes,
			    cpu *processor,
			    unsigned_word cia)
{
  memset(dest, 0, nr_bytes);
  switch (addr & 4) {
  case 0:
    *(unsigned_1*)dest = cpu_nr(processor);
    break;
  case 4:
    *(unsigned_1*)dest =
      device_find_integer_property(me, "/openprom/options/smp");
    break;
  }
  return nr_bytes;
}


static unsigned
icu_io_write_buffer_callback(device *me,
			     const void *source,
			     int space,
			     unsigned_word addr,
			     unsigned nr_bytes,
			     cpu *processor,
			     unsigned_word cia)
{
  unsigned_1 val = H2T_1(*(unsigned_1*)source);
  /* tell the parent device that the interrupt lines have changed.
     For this fake ICU.  The interrupt lines just indicate the cpu to
     interrupt next */
  device_interrupt_event(me,
			 val, /*my_port*/
			 val, /*val*/
			 processor, cia);
  return nr_bytes;
}

static void
icu_do_interrupt(event_queue *queue,
		 void *data)
{
  cpu *target = (cpu*)data;
  /* try to interrupt the processor.  If the attempt fails, try again
     on the next tick */
  if (!external_interrupt(target))
    event_queue_schedule(queue, 1, icu_do_interrupt, target);
}


static void
icu_interrupt_event_callback(device *me,
			     int my_port,
			     device *source,
			     int source_port,
			     int level,
			     cpu *processor,
			     unsigned_word cia)
{
  /* the interrupt controller can't interrupt a cpu at any time.
     Rather it must synchronize with the system clock before
     performing an interrupt on the given processor */
  psim *system = cpu_system(processor);
  cpu *target = psim_cpu(system, my_port);
  if (target != NULL) {
    event_queue *events = cpu_event_queue(target);
    event_queue_schedule(events, 1, icu_do_interrupt, target);
  }
}

static device_callbacks const icu_callbacks = {
  generic_device_init_address,
  ignore_device_init,
  unimp_device_attach_address,
  unimp_device_detach_address,
  icu_io_read_buffer_callback,
  icu_io_write_buffer_callback,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  icu_interrupt_event_callback,
  unimp_device_child_interrupt_event,
  unimp_device_unit_decode,
  unimp_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* HALT device: halt@0x<address>,4

   With real hardware, the processor operation is normally terminated
   through a reset.  This device illustrates how a reset device could
   be attached to an address */


static unsigned
halt_io_read_buffer_callback(device *me,
			     void *dest,
			     int space,
			     unsigned_word addr,
			     unsigned nr_bytes,
			     cpu *processor,
			     unsigned_word cia)
{
  cpu_halt(processor, cia, was_exited, 0);
  return 0;
}


static unsigned
halt_io_write_buffer_callback(device *me,
			      const void *source,
			      int space,
			      unsigned_word addr,
			      unsigned nr_bytes,
			      cpu *processor,
			      unsigned_word cia)
{
  cpu_halt(processor, cia, was_exited, *(unsigned_1*)source);
  return 0;
}


static device_callbacks const halt_callbacks = {
  generic_device_init_address,
  ignore_device_init,
  unimp_device_attach_address,
  unimp_device_detach_address,
  halt_io_read_buffer_callback,
  halt_io_write_buffer_callback,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  unimp_device_unit_decode,
  unimp_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* Register init device: register@<nothing>

   Properties attached to the register device specify the name/value
   initialization pair for cpu registers.

   A specific processor can be initialized by creating a property with
   a name like `0.pc'.

   Properties are normally processed old-to-new and this function
   needs to allow older (first in) properties to override new (last
   in) ones.  The suport function do_register_init() manages this. */

static void
do_register_init(device *me,
		 psim *system,
		 const device_property *prop)
{
  if (prop != NULL) {
    const char *name = prop->name;
    unsigned32 value = device_find_integer_property(me, name);
    int processor;

    do_register_init(me, system, device_next_property(prop));

    if (strchr(name, '.') == NULL) {
      processor = -1;
      DTRACE(register, ("%s=0x%lx\n", name, (unsigned long)value));
    }
    else {
      char *end;
      processor = strtoul(name, &end, 0);
      ASSERT(end[0] == '.');
      name = end+1;
      DTRACE(register, ("%d.%s=0x%lx\n", processor, name,
			(unsigned long)value));
    }    
    psim_write_register(system, processor, /* all processors */
			&value,
			name,
			cooked_transfer);
  }
}
		 

static void
register_init_data_callback(device *me,
			    psim *system)
{
  const device_property *prop = device_find_property(me, NULL);
  do_register_init(me, system, prop);
}


static device_callbacks const register_callbacks = {
  ignore_device_init,
  register_init_data_callback,
  unimp_device_attach_address,
  unimp_device_detach_address,
  unimp_device_io_read_buffer,
  unimp_device_io_write_buffer,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  unimp_device_unit_decode,
  unimp_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* Trace device:

   Properties attached to the trace device are names and values for
   the various trace variables.  When initialized trace goes through
   the propertie and sets the global trace variables so that they
   match what was specified in the device tree. */

static void
trace_init_data_callback(device *me,
			 psim *system)
{
  const device_property *prop = device_find_property(me, NULL);
  while (prop != NULL) {
    const char *name = prop->name;
    unsigned32 value = device_find_integer_property(me, name);
    trace_option(name, value);
    prop = device_next_property(prop);
  }
}


static device_callbacks const trace_callbacks = {
  ignore_device_init,
  trace_init_data_callback,
  unimp_device_attach_address,
  unimp_device_detach_address,
  unimp_device_io_read_buffer,
  unimp_device_io_write_buffer,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  unimp_device_unit_decode,
  unimp_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* VEA VM:

   vm@<stack-base>
     stack-base =
     nr-bytes =

   A VEA mode device. This sets its self up as the default memory
   device capturing all accesses (reads/writes) to currently unmapped
   addresses.  If the unmaped access falls within unallocated stack or
   heap address ranges then memory is allocated and the access is
   allowed to continue.

   During init phase, this device expects to receive `attach' requests
   from its children for the text/data/bss memory areas.  Typically,
   this would be done by the binary device.

   STACK: The location of the stack in memory is specified as part of
   the devices name.  Unmaped accesses that fall within the stack
   space result in the allocated stack being grown downwards so that
   it includes the page of the culprit access.

   HEAP: During initialization, the vm device monitors all `attach'
   operations from its children using this to determine the initial
   location of the heap.  The heap is then extended by system calls
   that frob the heap upper bound variable (see system.c). */


typedef struct _vm_device {
  /* area of memory valid for stack addresses */
  unsigned_word stack_base; /* min possible stack value */
  unsigned_word stack_bound;
  unsigned_word stack_lower_limit;
  /* area of memory valid for heap addresses */
  unsigned_word heap_base;
  unsigned_word heap_bound;
  unsigned_word heap_upper_limit;
} vm_device;


static void
vm_init_address_callback(device *me,
			 psim *system)
{
  vm_device *vm = (vm_device*)device_data(me);

  /* revert the stack/heap variables to their defaults */
  vm->stack_base = device_find_integer_property(me, "stack-base");
  vm->stack_bound = (vm->stack_base
		     + device_find_integer_property(me, "nr-bytes"));
  vm->stack_lower_limit = vm->stack_bound;
  vm->heap_base = 0;
  vm->heap_bound = 0;
  vm->heap_upper_limit = 0;

  /* establish this device as the default memory handler */
  device_attach_address(device_parent(me),
			device_name(me),
			attach_default,
			0 /*address space - ignore*/,
			0 /*addr - ignore*/,
			0 /*nr_bytes - ignore*/,
			access_read_write /*access*/,
			me);
}


static void
vm_attach_address(device *me,
		  const char *name,
		  attach_type attach,
		  int space,
		  unsigned_word addr,
		  unsigned nr_bytes,
		  access_type access,
		  device *who) /*callback/default*/
{
  vm_device *vm = (vm_device*)device_data(me);
  /* update end of bss if necessary */
  if (vm->heap_base < addr + nr_bytes) {
    vm->heap_base = addr + nr_bytes;
    vm->heap_bound = addr + nr_bytes;
    vm->heap_upper_limit = addr + nr_bytes;
  }
  device_attach_address(device_parent(me),
			"vm@0x0,0", /* stop remap */
			attach_raw_memory,
			0 /*address space*/,
			addr,
			nr_bytes,
			access,
			me);
}


STATIC_INLINE_DEVICE_TABLE unsigned
add_vm_space(device *me,
	     unsigned_word addr,
	     unsigned nr_bytes,
	     cpu *processor,
	     unsigned_word cia)
{
  vm_device *vm = (vm_device*)device_data(me);
  unsigned_word block_addr;
  unsigned block_nr_bytes;

  /* an address in the stack area, allocate just down to the addressed
     page */
  if (addr >= vm->stack_base && addr < vm->stack_lower_limit) {
    block_addr = FLOOR_PAGE(addr);
    block_nr_bytes = vm->stack_lower_limit - block_addr;
    vm->stack_lower_limit = block_addr;
  }
  /* an address in the heap area, allocate all of the required heap */
  else if (addr >= vm->heap_upper_limit && addr < vm->heap_bound) {
    block_addr = vm->heap_upper_limit;
    block_nr_bytes = vm->heap_bound - vm->heap_upper_limit;
    vm->heap_upper_limit = vm->heap_bound;
  }
  /* oops - an invalid address - abort the cpu */
  else if (processor != NULL) {
    cpu_halt(processor, cia, was_signalled, SIGSEGV);
    return 0;
  }
  /* 2*oops - an invalid address and no processor */
  else {
    return 0;
  }

  /* got the parameters, allocate the space */
  device_attach_address(device_parent(me),
			"vm@0x0,0", /* stop remap */
			attach_raw_memory,
			0 /*address space*/,
			block_addr,
			block_nr_bytes,
			access_read_write,
			me);
  return block_nr_bytes;
}


static unsigned
vm_io_read_buffer_callback(device *me,
			   void *dest,
			   int space,
			   unsigned_word addr,
			   unsigned nr_bytes,
			   cpu *processor,
			   unsigned_word cia)
{
  if (add_vm_space(me, addr, nr_bytes, processor, cia) >= nr_bytes) {
    memset(dest, 0, nr_bytes); /* always initialized to zero */
    return nr_bytes;
  }
  else 
    return 0;
}


static unsigned
vm_io_write_buffer_callback(device *me,
			    const void *source,
			    int space,
			    unsigned_word addr,
			    unsigned nr_bytes,
			    cpu *processor,
			    unsigned_word cia)
{
  if (add_vm_space(me, addr, nr_bytes, processor, cia) >= nr_bytes) {
    return device_dma_write_buffer(device_parent(me), source,
				   space, addr,
				   nr_bytes,
				   0/*violate_read_only*/);
  }
  else
    return 0;
}


static void
vm_ioctl_callback(device *me,
		  psim *system,
		  cpu *processor,
		  unsigned_word cia,
		  va_list ap)
{
  /* While the caller is notified that the heap has grown by the
     requested amount, the heap is infact extended out to a page
     boundary. */
  vm_device *vm = (vm_device*)device_data(me);
  unsigned_word new_break = ALIGN_8(cpu_registers(processor)->gpr[3]);
  unsigned_word old_break = vm->heap_bound;
  signed_word delta = new_break - old_break;
  if (delta > 0)
    vm->heap_bound = ALIGN_PAGE(new_break);
  cpu_registers(processor)->gpr[0] = 0;
  cpu_registers(processor)->gpr[3] = new_break;
}


static device_callbacks const vm_callbacks = {
  vm_init_address_callback,
  ignore_device_init,
  vm_attach_address,
  passthrough_device_detach_address,
  vm_io_read_buffer_callback,
  vm_io_write_buffer_callback,
  unimp_device_dma_read_buffer,
  passthrough_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  generic_device_unit_decode,
  generic_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  vm_ioctl_callback,
};


static void *
vea_vm_create(const char *name,
	      const device_unit *address,
	      const char *args,
	      device *parent)
{
  vm_device *vm = ZALLOC(vm_device);
  return vm;
}



/* Memory init device: memory@0x<addr>

   This strange device is used create sections of memory */

static void
memory_init_address_callback(device *me,
			     psim *system)
{
  const device_property *reg = device_find_array_property(me, "reg");
  const reg_spec *spec = reg->array;
  int nr_entries = reg->sizeof_array / sizeof(*spec);

  if ((reg->sizeof_array % sizeof(*spec)) != 0)
    error("devices/%s reg property of incorrect size\n", device_name(me));
  while (nr_entries > 0) {
    device_attach_address(device_parent(me),
			  device_name(me),
			  attach_raw_memory,
			  0 /*address space*/,
			  BE2H_4(spec->base),
			  BE2H_4(spec->size),
			  access_read_write_exec,
			  me);
    spec++;
    nr_entries--;
  }
}

static void *
memory_instance_create_callback(device *me,
				const char *args)
{
  return me; /* for want of any thing better */
}

static void
memory_instance_delete_callback(device_instance *instance)
{
  return;
}

static device_callbacks const memory_callbacks = {
  memory_init_address_callback,
  ignore_device_init,
  unimp_device_attach_address,
  unimp_device_detach_address,
  unimp_device_io_read_buffer,
  unimp_device_io_write_buffer,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  unimp_device_unit_decode,
  unimp_device_unit_encode,
  memory_instance_create_callback,
  memory_instance_delete_callback,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* IOBUS device: iobus@<address>

   Simple bus on which some IO devices live */

static void
iobus_attach_address_callback(device *me,
			      const char *name,
			      attach_type type,
			      int space,
			      unsigned_word addr,
			      unsigned nr_bytes,
			      access_type access,
			      device *who) /*callback/default*/
{
  unsigned_word iobus_addr;
  /* sanity check */
  if (type == attach_default)
    error("iobus_attach_address_callback() no default for %s/%s\n",
	  device_name(me), name);
  if (space != 0)
    error("iobus_attach_address_callback() no space for %s/%s\n",
	  device_name(me), name);
  /* get the bus address */
  if (device_unit_address(me)->nr_cells != 1)
    error("iobus_attach_address_callback() invalid address for %s\n",
	  device_name(me));
  iobus_addr = device_unit_address(me)->cells[0];
  device_attach_address(device_parent(me),
			device_name(me),
			type,
			0 /*space*/,
			iobus_addr + addr,
			nr_bytes,
			access,
			who);
}


static device_callbacks const iobus_callbacks = {
  ignore_device_init,
  ignore_device_init,
  iobus_attach_address_callback,
  unimp_device_detach_address,
  unimp_device_io_read_buffer,
  unimp_device_io_write_buffer,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  generic_device_unit_decode,
  generic_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* FILE device: file@0x<address>,<file-name>
   (later - file@0x<address>,<size>,<file-offset>,<file-name>)

   Specifies a file to read directly into memory starting at <address> */


static void
file_init_data_callback(device *me,
			psim *system)
{
  int count;
  const char *file_name = device_find_string_property(me, "file-name");
  unsigned_word addr = device_find_integer_property(me, "real-address");
  /* load the file */
  count = dma_file(me, file_name, addr);
  if (count < 0)
    error("device_table/%s - Problem loading file %s\n",
	  device_name(me), file_name);
}


static device_callbacks const file_callbacks = {
  ignore_device_init,
  file_init_data_callback,
  unimp_device_attach_address,
  unimp_device_detach_address,
  unimp_device_io_read_buffer,
  unimp_device_io_write_buffer,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  unimp_device_unit_decode,
  unimp_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* DATA device: data@<address>

     <data> - property containing the value to store
     <real-address> - address to store data at

   Store <data> at <address> using approperiate byte order */

static void
data_init_data_callback(device *me,
			psim *system)
{
  unsigned_word addr = device_find_integer_property(me, "real-address");
  const device_property *data = device_find_property(me, "data");
  if (data == NULL)
    error("devices/data - missing data property\n");
  switch (data->type) {
  case integer_property:
    {
      unsigned32 buf = device_find_integer_property(me, "data");
      H2T(buf);
      if (device_dma_write_buffer(device_parent(me),
				  &buf,
				  0 /*address-space*/,
				  addr,
				  sizeof(buf), /*nr-bytes*/
				  1 /*violate ro*/) != sizeof(buf))
	error("devices/%s - Problem storing integer 0x%x at 0x%lx\n",
	      device_name(me), (long)buf, (unsigned long)addr);
    }
    break;
  default:
    error("devices/%s - write of this data is not yet implemented\n", device_name(me));
    break;
  }
}


static device_callbacks const data_callbacks = {
  ignore_device_init,
  data_init_data_callback,
  unimp_device_attach_address,
  unimp_device_detach_address,
  unimp_device_io_read_buffer,
  unimp_device_io_write_buffer,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  unimp_device_unit_decode,
  unimp_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* HTAB:

   htab@<real-address>
     real-address =
     nr-bytes =

   pte@<real-address>
     real-address =
     virtual-address =
     nr-bytes =
     wimg =
     pp =

   pte@<real-address>
     real-address =
     file-name =
     wimg =
     pp =
     
   HTAB defines the location (in physical memory) of a HASH table.
   PTE (as a child of HTAB) defines a mapping that is to be entered
   into that table.

   NB: All the work in this device is done during init by the PTE.
   The pte, looks up its parent to determine the address of the HTAB
   and then uses DMA calls to establish the required mapping. */

STATIC_INLINE_DEVICE_TABLE void
htab_decode_hash_table(device *parent,
		       unsigned32 *htaborg,
		       unsigned32 *htabmask)
{
  unsigned_word htab_ra;
  unsigned htab_nr_bytes;
  unsigned n;
  /* determine the location/size of the hash table */
  if (parent == NULL
      || strcmp(device_name(parent), "htab") != 0)
    error("devices/htab - missing htab parent device\n");
  htab_ra = device_find_integer_property(parent, "real-address");
  htab_nr_bytes = device_find_integer_property(parent, "nr-bytes");
  for (n = htab_nr_bytes; n > 1; n = n / 2) {
    if (n % 2 != 0)
      error("devices/%s - htab size 0x%x not a power of two\n",
	    device_name(parent), htab_nr_bytes);
  }
  *htaborg = htab_ra;
  *htabmask = MASKED32(htab_nr_bytes - 1, 7, 31-6);
  if ((htab_ra & INSERTED32(*htabmask, 7, 15)) != 0) {
    error("devices/%s - htaborg 0x%x not aligned to htabmask 0x%x\n",
	  device_name(parent), *htaborg, *htabmask);
  }
  DTRACE(htab, ("htab - htaborg=0x%lx htabmask=0x%lx\n",
		(unsigned long)*htaborg, (unsigned long)*htabmask));
}

STATIC_INLINE void
htab_map_page(device *me,
	      unsigned_word ra,
	      unsigned64 va,
	      unsigned wimg,
	      unsigned pp,
	      unsigned32 htaborg,
	      unsigned32 htabmask)
{
  unsigned64 vpn = va << 12;
  unsigned32 vsid = INSERTED32(EXTRACTED64(vpn, 0, 23), 0, 23);
  unsigned32 page = INSERTED32(EXTRACTED64(vpn, 24, 39), 0, 15);
  unsigned32 hash = INSERTED32(EXTRACTED32(vsid, 5, 23)
			       ^ EXTRACTED32(page, 0, 15),
			       7, 31-6);
  int h;
  for (h = 0; h < 2; h++) {
    unsigned32 pteg = (htaborg | (hash & htabmask));
    int pti;
    for (pti = 0; pti < 8; pti++, pteg += 8) {
      unsigned32 current_target_pte0;
      unsigned32 current_pte0;
      if (device_dma_read_buffer(device_parent(me),
				 &current_target_pte0,
				 0, /*space*/
				 pteg,
				 sizeof(current_target_pte0)) != 4)
	error("htab_init_callback() failed to read a pte at 0x%x\n",
	      pteg);
      current_pte0 = T2H_4(current_target_pte0);
      if (!MASKED32(current_pte0, 0, 0)) {
	/* empty pte fill it */
	unsigned32 pte0 = (MASK32(0, 0)
			   | INSERTED32(EXTRACTED32(vsid, 0, 23), 1, 24)
			   | INSERTED32(h, 25, 25)
			   | INSERTED32(EXTRACTED32(page, 0, 5), 26, 31));
	unsigned32 target_pte0 = H2T_4(pte0);
	unsigned32 pte1 = (INSERTED32(EXTRACTED32(ra, 0, 19), 0, 19)
			   | INSERTED32(wimg, 25, 28)
			   | INSERTED32(pp, 30, 31));
	unsigned32 target_pte1 = H2T_4(pte1);
	if (device_dma_write_buffer(device_parent(me),
				    &target_pte0,
				    0, /*space*/
				    pteg,
				    sizeof(target_pte0),
				    1/*ro?*/) != 4
	    || device_dma_write_buffer(device_parent(me),
				       &target_pte1,
				       0, /*space*/
				       pteg + 4,
				       sizeof(target_pte1),
				       1/*ro?*/) != 4)
	  error("htab_init_callback() failed to write a pte a 0x%x\n",
		pteg);
	DTRACE(htab, ("map - va=0x%lx ra=0x%lx &pte0=0x%lx pte0=0x%lx pte1=0x%lx\n",
		      (unsigned long)va, (unsigned long)ra,
		      (unsigned long)pteg,
		      (unsigned long)pte0, (unsigned long)pte1));
	return;
      }
    }
    /* re-hash */
    hash = MASKED32(~hash, 0, 18);
  }
}

STATIC_INLINE_DEVICE_TABLE void
htab_map_region(device *me,
		unsigned_word pte_ra,
		unsigned_word pte_va,
		unsigned nr_bytes,
		unsigned wimg,
		unsigned pp,
		unsigned32 htaborg,
		unsigned32 htabmask)
{
  unsigned_word ra;
  unsigned64 va;
  /* go through all pages and create a pte for each */
  for (ra = pte_ra, va = (signed_word)pte_va;
       ra < pte_ra + nr_bytes;
       ra += 0x1000, va += 0x1000) {
    htab_map_page(me, ra, va, wimg, pp, htaborg, htabmask);
  }
}
  
typedef struct _htab_binary_sizes {
  unsigned_word text_ra;
  unsigned_word text_base;
  unsigned_word text_bound;
  unsigned_word data_ra;
  unsigned_word data_base;
  unsigned data_bound;
  device *me;
} htab_binary_sizes;

STATIC_INLINE_DEVICE_TABLE void
htab_sum_binary(bfd *abfd,
		sec_ptr sec,
		PTR data)
{
  htab_binary_sizes *sizes = (htab_binary_sizes*)data;
  unsigned_word size = bfd_get_section_size_before_reloc (sec);
  unsigned_word vma = bfd_get_section_vma (abfd, sec);

  /* skip the section if no memory to allocate */
  if (! (bfd_get_section_flags(abfd, sec) & SEC_ALLOC))
    return;

  if ((bfd_get_section_flags (abfd, sec) & SEC_CODE)
      || (bfd_get_section_flags (abfd, sec) & SEC_READONLY)) {
    if (sizes->text_bound < vma + size)
      sizes->text_bound = ALIGN_PAGE(vma + size);
    if (sizes->text_base > vma)
      sizes->text_base = FLOOR_PAGE(vma);
  }
  else if ((bfd_get_section_flags (abfd, sec) & SEC_DATA)
	   || (bfd_get_section_flags (abfd, sec) & SEC_ALLOC)) {
    if (sizes->data_bound < vma + size)
      sizes->data_bound = ALIGN_PAGE(vma + size);
    if (sizes->data_base > vma)
      sizes->data_base = FLOOR_PAGE(vma);
  }
}

STATIC_INLINE_DEVICE_TABLE void
htab_dma_binary(bfd *abfd,
		sec_ptr sec,
		PTR data)
{
  htab_binary_sizes *sizes = (htab_binary_sizes*)data;
  void *section_init;
  unsigned_word section_vma;
  unsigned_word section_size;
  unsigned_word section_ra;
  device *me = sizes->me;

  /* skip the section if no memory to allocate */
  if (! (bfd_get_section_flags(abfd, sec) & SEC_ALLOC))
    return;

  /* check/ignore any sections of size zero */
  section_size = bfd_get_section_size_before_reloc(sec);
  if (section_size == 0)
    return;

  /* if nothing to load, ignore this one */
  if (! (bfd_get_section_flags(abfd, sec) & SEC_LOAD))
    return;

  /* find where it is to go */
  section_vma = bfd_get_section_vma(abfd, sec);
  section_ra = 0;
  if ((bfd_get_section_flags (abfd, sec) & SEC_CODE)
      || (bfd_get_section_flags (abfd, sec) & SEC_READONLY))
    section_ra = (section_vma - sizes->text_base + sizes->text_ra);
  else if ((bfd_get_section_flags (abfd, sec) & SEC_DATA))
    section_ra = (section_vma - sizes->data_base + sizes->data_ra);
  else 
    return; /* just ignore it */

  DTRACE(htab,
	 ("load - name=%-7s vma=0x%.8lx size=%6ld ra=0x%.8lx flags=%3lx(%s%s%s%s%s )\n",
	  bfd_get_section_name(abfd, sec),
	  (long)section_vma,
	  (long)section_size,
	  (long)section_ra,
	  (long)bfd_get_section_flags(abfd, sec),
	  bfd_get_section_flags(abfd, sec) & SEC_LOAD ? " LOAD" : "",
	  bfd_get_section_flags(abfd, sec) & SEC_CODE ? " CODE" : "",
	  bfd_get_section_flags(abfd, sec) & SEC_DATA ? " DATA" : "",
	  bfd_get_section_flags(abfd, sec) & SEC_ALLOC ? " ALLOC" : "",
	  bfd_get_section_flags(abfd, sec) & SEC_READONLY ? " READONLY" : ""
	  ));

  /* dma in the sections data */
  section_init = zalloc(section_size);
  if (!bfd_get_section_contents(abfd,
				sec,
				section_init, 0,
				section_size)) {
    bfd_perror("devices/pte");
    error("devices/%s - no data loaded\n", device_name(me));
  }
  if (device_dma_write_buffer(device_parent(me),
			      section_init,
			      0 /*space*/,
			      section_ra,
			      section_size,
			      1 /*violate_read_only*/)
      != section_size)
    error("devices/%s - broken dma transfer\n", device_name(me));
  zfree(section_init); /* only free if load */
}

STATIC_INLINE_DEVICE_TABLE void
htab_map_binary(device *me,
		unsigned_word ra,
		unsigned wimg,
		unsigned pp,
		const char *file_name,
		unsigned32 htaborg,
		unsigned32 htabmask)
{
  htab_binary_sizes sizes;
  bfd *image;
  sizes.text_base = -1;
  sizes.data_base = -1;
  sizes.text_bound = 0;
  sizes.data_bound = 0;
  sizes.me = me;

  /* open the file */
  image = bfd_openr(file_name, NULL);
  if (image == NULL) {
    bfd_perror("devices/pte");
    error("devices/%s - the file %s not loaded\n", device_name(me), file_name);
  }

  /* check it is valid */
  if (!bfd_check_format(image, bfd_object)) {
    bfd_close(image);
    error("devices/%s - the file %s has an invalid binary format\n",
	  device_name(me), file_name);
  }

  /* determine the size of each of the files regions */
  bfd_map_over_sections (image, htab_sum_binary, (PTR) &sizes);

  /* determine the real addresses of the sections */
  sizes.text_ra = ra;
  sizes.data_ra = ALIGN_PAGE(sizes.text_ra + 
			     (sizes.text_bound - sizes.text_base));

  DTRACE(htab, ("text map - base=0x%lx bound=0x%lx ra=0x%lx\n",
		(unsigned long)sizes.text_base,
		(unsigned long)sizes.text_bound,
		(unsigned long)sizes.text_ra));
  DTRACE(htab, ("data map - base=0x%lx bound=0x%lx ra=0x%lx\n",
		(unsigned long)sizes.data_base,
		(unsigned long)sizes.data_bound,
		(unsigned long)sizes.data_ra));

  /* set up virtual memory maps for each of the regions */
  htab_map_region(me, sizes.text_ra, sizes.text_base,
		  sizes.text_bound - sizes.text_base,
		  wimg, pp,
		  htaborg, htabmask);
  htab_map_region(me, sizes.data_ra, sizes.data_base,
		  sizes.data_bound - sizes.data_base,
		  wimg, pp,
		  htaborg, htabmask);

  /* dma the sections into physical memory */
  bfd_map_over_sections (image, htab_dma_binary, (PTR) &sizes);
}

static void
htab_init_data_callback(device *me,
			psim *system)
{
  if (WITH_TARGET_WORD_BITSIZE != 32)
    error("devices/htab: only 32bit targets currently suported\n");

  /* only the pte does work */
  if (strcmp(device_name(me), "pte") == 0) {
    unsigned32 htaborg;
    unsigned32 htabmask;

    htab_decode_hash_table(device_parent(me), &htaborg, &htabmask);

    if (device_find_property(me, "file-name") != NULL) {
      /* map in a binary */
      unsigned32 pte_ra = device_find_integer_property(me, "real-address");
      unsigned pte_wimg = device_find_integer_property(me, "wimg");
      unsigned pte_pp = device_find_integer_property(me, "pp");
      const char *file_name = device_find_string_property(me, "file-name");
      DTRACE(htab, ("pte - ra=0x%lx, wimg=%ld, pp=%ld, file-name=%s\n",
		    (unsigned long)pte_ra,
		    (unsigned long)pte_wimg,
		    (long)pte_pp,
		    file_name));
      htab_map_binary(me, pte_ra, pte_wimg, pte_pp, file_name,
		      htaborg, htabmask);
    }
    else {
      /* handle a normal mapping definition */
      /* so that 0xff...0 is make 0xffffff00 */
      signed32 pte_va = device_find_integer_property(me, "virtual-address");
      unsigned32 pte_ra = device_find_integer_property(me, "real-address");
      unsigned pte_nr_bytes = device_find_integer_property(me, "nr-bytes");
      unsigned pte_wimg = device_find_integer_property(me, "wimg");
      unsigned pte_pp = device_find_integer_property(me, "pp");
      DTRACE(htab, ("pte - ra=0x%lx, wimg=%ld, pp=%ld, va=0x%lx, nr_bytes=%ld\n",
		    (unsigned long)pte_ra,
		    (long)pte_wimg,
		    (long)pte_pp,
		    (unsigned long)pte_va,
		    (long)pte_nr_bytes));
      htab_map_region(me, pte_ra, pte_va, pte_nr_bytes, pte_wimg, pte_pp,
		      htaborg, htabmask);
    }
  }
}


static device_callbacks const htab_callbacks = {
  ignore_device_init,
  htab_init_data_callback,
  unimp_device_attach_address,
  unimp_device_detach_address,
  unimp_device_io_read_buffer,
  unimp_device_io_write_buffer,
  passthrough_device_dma_read_buffer,
  passthrough_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  generic_device_unit_decode,
  generic_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* Load device: binary

   Single property the name of which specifies the file (understood by
   BFD) that is to be DMAed into memory as part of init */

STATIC_INLINE_DEVICE_TABLE void
update_for_binary_section(bfd *abfd,
			  asection *the_section,
			  PTR obj)
{
  unsigned_word section_vma;
  unsigned_word section_size;
  access_type access;
  device *me = (device*)obj;

  /* skip the section if no memory to allocate */
  if (! (bfd_get_section_flags(abfd, the_section) & SEC_ALLOC))
    return;

  /* check/ignore any sections of size zero */
  section_size = bfd_get_section_size_before_reloc(the_section);
  if (section_size == 0)
    return;

  /* find where it is to go */
  section_vma = bfd_get_section_vma(abfd, the_section);

  DTRACE(binary,
	 ("name=%-7s, vma=0x%.8lx, size=%6ld, flags=%3lx(%s%s%s%s%s )\n",
	  bfd_get_section_name(abfd, the_section),
	  (long)section_vma,
	  (long)section_size,
	  (long)bfd_get_section_flags(abfd, the_section),
	  bfd_get_section_flags(abfd, the_section) & SEC_LOAD ? " LOAD" : "",
	  bfd_get_section_flags(abfd, the_section) & SEC_CODE ? " CODE" : "",
	  bfd_get_section_flags(abfd, the_section) & SEC_DATA ? " DATA" : "",
	  bfd_get_section_flags(abfd, the_section) & SEC_ALLOC ? " ALLOC" : "",
	  bfd_get_section_flags(abfd, the_section) & SEC_READONLY ? " READONLY" : ""
	  ));

  /* determine the devices access */
  access = access_read;
  if (bfd_get_section_flags(abfd, the_section) & SEC_CODE)
    access |= access_exec;
  if (!(bfd_get_section_flags(abfd, the_section) & SEC_READONLY))
    access |= access_write;

  /* if a map, pass up a request to create the memory in core */
  if (strncmp(device_name(me), "map-binary", strlen("map-binary")) == 0)
    device_attach_address(device_parent(me),
			  device_name(me),
			  attach_raw_memory,
			  0 /*address space*/,
			  section_vma,
			  section_size,
			  access,
			  me);

  /* if a load dma in the required data */
  if (bfd_get_section_flags(abfd, the_section) & SEC_LOAD) {
    void *section_init = zalloc(section_size);
    if (!bfd_get_section_contents(abfd,
				  the_section,
				  section_init, 0,
				  section_size)) {
      bfd_perror("core:load_section()");
      error("load of data failed");
      return;
    }
    if (device_dma_write_buffer(device_parent(me),
				section_init,
				0 /*space*/,
				section_vma,
				section_size,
				1 /*violate_read_only*/)
	!= section_size)
      error("data_init_callback() broken transfer for %s\n", device_name(me));
    zfree(section_init); /* only free if load */
  }
}


static void
binary_init_data_callback(device *me,
			  psim *system)
{
  /* get the file name */
  const char *file_name = device_find_string_property(me, "file-name");
  bfd *image;

  /* open the file */
  image = bfd_openr(file_name, NULL);
  if (image == NULL) {
    bfd_perror("devices/binary");
    error("devices/%s - the file %s not loaded\n", device_name(me), file_name);
  }

  /* check it is valid */
  if (!bfd_check_format(image, bfd_object)) {
    bfd_close(image);
    error("devices/%s - the file %s has an invalid binary format\n",
	  device_name(me), file_name);
  }

  /* and the data sections */
  bfd_map_over_sections(image,
			update_for_binary_section,
			(PTR)me);

  bfd_close(image);
}


static device_callbacks const binary_callbacks = {
  ignore_device_init,
  binary_init_data_callback,
  unimp_device_attach_address,
  unimp_device_detach_address,
  unimp_device_io_read_buffer,
  unimp_device_io_write_buffer,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  unimp_device_unit_decode,
  unimp_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  unimp_device_ioctl,
};



/* Stack device: stack@<type>

   Has a single IOCTL to create a stack frame of the specified type.
   If <type> is elf or xcoff then a corresponding stack is created.
   Any other value of type is ignored.

   The IOCTL takes the additional arguments:

     unsigned_word stack_end -- where the stack should come down from
     char **argv -- ...
     char **envp -- ...

   */

STATIC_INLINE_DEVICE_TABLE int
sizeof_argument_strings(char **arg)
{
  int sizeof_strings = 0;

  /* robust */
  if (arg == NULL)
    return 0;

  /* add up all the string sizes (padding as we go) */
  for (; *arg != NULL; arg++) {
    int len = strlen(*arg) + 1;
    sizeof_strings += ALIGN_8(len);
  }

  return sizeof_strings;
}

STATIC_INLINE_DEVICE_TABLE int
number_of_arguments(char **arg)
{
  int nr;
  if (arg == NULL)
    return 0;
  for (nr = 0; *arg != NULL; arg++, nr++);
  return nr;
}

STATIC_INLINE_DEVICE_TABLE int
sizeof_arguments(char **arg)
{
  return ALIGN_8((number_of_arguments(arg) + 1) * sizeof(unsigned_word));
}

STATIC_INLINE_DEVICE_TABLE void
write_stack_arguments(psim *system,
		      char **arg,
		      unsigned_word start_block,
		      unsigned_word end_block,
		      unsigned_word start_arg,
		      unsigned_word end_arg)
{
  DTRACE(stack,
	("write_stack_arguments(system=0x%lx, arg=0x%lx, start_block=0x%lx, end_block=0x%lx, start_arg=0x%lx, end_arg=0x%lx)\n",
	 (long)system, (long)arg, (long)start_block, (long)end_block, (long)start_arg, (long)end_arg));
  if (arg == NULL)
    error("write_arguments: character array NULL\n");
  /* only copy in arguments, memory is already zero */
  for (; *arg != NULL; arg++) {
    int len = strlen(*arg)+1;
    unsigned_word target_start_block;
    DTRACE(stack,
	  ("write_stack_arguments() write %s=%s at %s=0x%lx %s=0x%lx %s=0x%lx\n",
	   "**arg", *arg, "start_block", (long)start_block,
	   "len", (long)len, "start_arg", (long)start_arg));
    if (psim_write_memory(system, 0, *arg,
			  start_block, len,
			  0/*violate_readonly*/) != len)
      error("write_stack_arguments() - write of **arg (%s) at 0x%x failed\n",
	    *arg, start_block);
    target_start_block = H2T_word(start_block);
    if (psim_write_memory(system, 0, &target_start_block,
			  start_arg, sizeof(target_start_block),
			  0) != sizeof(target_start_block))
      error("write_stack_arguments() - write of *arg failed\n");
    start_block += ALIGN_8(len);
    start_arg += sizeof(start_block);
  }
  start_arg += sizeof(start_block); /*the null at the end*/
  if (start_block != end_block
      || ALIGN_8(start_arg) != end_arg)
    error("write_stack_arguments - possible corruption\n");
  DTRACE(stack,
	 ("write_stack_arguments() = void\n"));
}

STATIC_INLINE_DEVICE_TABLE void
create_elf_stack_frame(psim *system,
		       unsigned_word bottom_of_stack,
		       char **argv,
		       char **envp)
{
  /* fixme - this is over aligned */

  /* information block */
  const unsigned sizeof_envp_block = sizeof_argument_strings(envp);
  const unsigned_word start_envp_block = bottom_of_stack - sizeof_envp_block;
  const unsigned sizeof_argv_block = sizeof_argument_strings(argv);
  const unsigned_word start_argv_block = start_envp_block - sizeof_argv_block;

  /* auxiliary vector - contains only one entry */
  const unsigned sizeof_aux_entry = 2*sizeof(unsigned_word); /* magic */
  const unsigned_word start_aux = start_argv_block - ALIGN_8(sizeof_aux_entry);

  /* environment points (including null sentinal) */
  const unsigned sizeof_envp = sizeof_arguments(envp);
  const unsigned_word start_envp = start_aux - sizeof_envp;

  /* argument pointers (including null sentinal) */
  const int argc = number_of_arguments(argv);
  const unsigned sizeof_argv = sizeof_arguments(argv);
  const unsigned_word start_argv = start_envp - sizeof_argv;

  /* link register save address - alligned to a 16byte boundary */
  const unsigned_word top_of_stack = ((start_argv
				       - 2 * sizeof(unsigned_word))
				      & ~0xf);

  /* install arguments on stack */
  write_stack_arguments(system, envp,
			start_envp_block, bottom_of_stack,
			start_envp, start_aux);
  write_stack_arguments(system, argv,
			start_argv_block, start_envp_block,
			start_argv, start_envp);

  /* set up the registers */
  psim_write_register(system, -1,
		      &top_of_stack, "sp", cooked_transfer);
  psim_write_register(system, -1,
		      &argc, "r3", cooked_transfer);
  psim_write_register(system, -1,
		      &start_argv, "r4", cooked_transfer);
  psim_write_register(system, -1,
		      &start_envp, "r5", cooked_transfer);
  psim_write_register(system, -1,
		      &start_aux, "r6", cooked_transfer);
}

STATIC_INLINE_DEVICE_TABLE void
create_aix_stack_frame(psim *system,
		       unsigned_word bottom_of_stack,
		       char **argv,
		       char **envp)
{
  unsigned_word core_envp;
  unsigned_word core_argv;
  unsigned_word core_argc;
  unsigned_word core_aux;
  unsigned_word top_of_stack;

  /* cheat - create an elf stack frame */
  create_elf_stack_frame(system, bottom_of_stack, argv, envp);
  
  /* extract argument addresses from registers */
  psim_read_register(system, 0, &top_of_stack, "r1", cooked_transfer);
  psim_read_register(system, 0, &core_argc, "r3", cooked_transfer);
  psim_read_register(system, 0, &core_argv, "r4", cooked_transfer);
  psim_read_register(system, 0, &core_envp, "r5", cooked_transfer);
  psim_read_register(system, 0, &core_aux, "r6", cooked_transfer);

  /* extract arguments from registers */
  error("create_aix_stack_frame() - what happens next?\n");
}



static void
stack_ioctl_callback(device *me,
		     psim *system,
		     cpu *processor,
		     unsigned_word cia,
		     va_list ap)
{
  unsigned_word stack_pointer;
  const char *stack_type;
  char **argv;
  char **envp;
  stack_pointer = va_arg(ap, unsigned_word);
  argv = va_arg(ap, char **);
  envp = va_arg(ap, char **);
  DTRACE(stack,
	 ("stack_ioctl_callback(me=0x%lx:%s, system=0x%lx, processor=0x%lx, cia=0x%lx, argv=0x%lx, envp=0x%lx)\n",
	  (long)me, device_name(me), (long)system, (long)processor, (long)cia, (long)argv, (long)envp));
  stack_type = device_find_string_property(me, "stack-type");
  if (strcmp(stack_type, "elf") == 0)
    create_elf_stack_frame(system, stack_pointer, argv, envp);
  else if (strcmp(stack_type, "xcoff") == 0)
    create_aix_stack_frame(system, stack_pointer, argv, envp);
  DTRACE(stack, 
	 ("stack_ioctl_callback() = void\n"));
}

static device_callbacks const stack_callbacks = {
  ignore_device_init,
  ignore_device_init,
  unimp_device_attach_address,
  unimp_device_detach_address,
  unimp_device_io_read_buffer,
  unimp_device_io_write_buffer,
  unimp_device_dma_read_buffer,
  unimp_device_dma_write_buffer,
  unimp_device_interrupt_event,
  unimp_device_child_interrupt_event,
  unimp_device_unit_decode,
  unimp_device_unit_encode,
  unimp_device_instance_create,
  unimp_device_instance_delete,
  unimp_device_instance_read,
  unimp_device_instance_write,
  unimp_device_instance_seek,
  stack_ioctl_callback,
};



device_descriptor device_table[] = {
  { "console", console_create, &console_callbacks },
  { "memory", NULL, &memory_callbacks },
  { "eeprom", NULL, &memory_callbacks },
  { "vm", vea_vm_create, &vm_callbacks },
  { "halt", NULL, &halt_callbacks },
  { "icu", NULL, &icu_callbacks },
  { "register", NULL, &register_callbacks },
  { "iobus", NULL, &iobus_callbacks },
  { "file", NULL, &file_callbacks },
  { "data", NULL, &data_callbacks },
  { "htab", NULL, &htab_callbacks },
  { "pte", NULL, &htab_callbacks }, /* yep - uses htab's table */
  { "stack", NULL, &stack_callbacks },
  { "load-binary", NULL, &binary_callbacks },
  { "map-binary", NULL, &binary_callbacks },
  /* standard OpenBoot devices */
  { "aliases", NULL, &passthrough_callbacks },
  { "options", NULL, &passthrough_callbacks },
  { "chosen", NULL, &passthrough_callbacks },
  { "packages", NULL, &passthrough_callbacks },
  { "cpus", NULL, &passthrough_callbacks },
  { "openprom", NULL, &passthrough_callbacks },
  { "init", NULL, &passthrough_callbacks },
  { "trace", NULL, &trace_callbacks },
  { NULL },
};

#endif /* _DEVICE_TABLE_C_ */
