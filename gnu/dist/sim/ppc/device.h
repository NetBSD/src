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


#ifndef _DEVICE_H_
#define _DEVICE_H_

#ifndef INLINE_DEVICE
#define INLINE_DEVICE
#endif

/* declared in basics.h, this object is used everywhere */
/* typedef struct _device device; */


/* Devices:

   OpenBoot documentation refers to devices, device nodes, packages,
   package instances, methods, static methods and properties.  This
   device implementation uses its own termonology. Where ever it
   exists, the notes will indicate a correspondance between PSIM terms
   and those found in OpenBoot.

   device:

   A device is the basic building block in this model.  A device can
   be further categorized into one of three classes - template, node
   and instance.

   device-node (aka device):

   The device tree is constructed from device-nodes.  Each node has
   both local state (data), a relationship with the device nodes
   around it and an address (unit-address) on the parents bus `bus' */

INLINE_DEVICE(device *) device_parent
(device *me);

INLINE_DEVICE(device *) device_sibling
(device *me);

INLINE_DEVICE(device *) device_child
(device *me);

INLINE_DEVICE(const char *) device_name
(device *me);

INLINE_DEVICE(const char *) device_path
(device *me);

INLINE_DEVICE(void *) device_data
(device *me);

typedef struct _device_unit {
  int nr_cells;
  unsigned32 cells[4]; /* unused cells are zero */
} device_unit;

INLINE_DEVICE(const device_unit *) device_unit_address
(device *me);

/* Each device-node normally corresponds to a hardware component of
   the system being modeled.  Leaf nodes matching external devices and
   intermediate nodes matching bridges and controllers.
   
   Device nodes also support methods that are an abstraction of the
   transactions that occure in real hardware.  These operations
   (io/dma read/writes and interrupts) are discussed separatly later.

   OpenBoot refers to device nodes by many names.  The most common are
   device, device node and package.


   device-template:

   A device node is created from its template.  The only valid
   operation on a template is to create a device node from it: */

INLINE_DEVICE(device *) device_template_create_device
(device *parent,
 const char *name,
 const char *unit_address,
 const char *args);

/* The create is paramaterized by both the devices unit address (a
   string that is converted into numeric form by the devices parent)
   and optionally extra argument information.

   The actual device node is constructed by a number of pieces provided
   by the template function: */

typedef struct _device_callbacks device_callbacks;

INLINE_DEVICE(device *) device_create_from
(const char *name,
 const device_unit *unit_address,
 void *data,
 const device_callbacks *callbacks,
 device *parent);

/* OpenBoot discusses the creation of packages (devices).




   device-instance:

   Devices support an abstract I/O model. A unique I/O instance can be
   created from a device node and then this instance used to perform
   I/O that is independant of other instances. */

INLINE_DEVICE(device_instance *)device_instance_create
(device *me,
 const char *device_specifier,
 int permanant);

INLINE_DEVICE(void) device_instance_delete
(device_instance *instance);

INLINE_DEVICE(int) device_instance_read
(device_instance *instance,
 void *addr,
 unsigned_word len);

INLINE_DEVICE(int) device_instance_write
(device_instance *instance,
 const void *addr,
 unsigned_word len);

INLINE_DEVICE(int) device_instance_seek
(device_instance *instance,
 unsigned_word pos_hi,
 unsigned_word pos_lo);

INLINE_DEVICE(device *) device_instance_device
(device_instance *instance);

INLINE_DEVICE(const char *) device_instance_name
(device_instance *instance);

INLINE_DEVICE(const char *) device_instance_path
(device_instance *instance);

INLINE_DEVICE(void *) device_instance_data
(device_instance *instance);

/* A device instance can be marked (when created) as being permenant.
   Such instances are assigned a reserved address and are *not*
   deleted between simulation runs.

   OpenBoot refers to a device instace as a package instance */


/* Device initialization:

   A device is created once (from its template) but initialized at the
   start of every simulation run.  This initialization is performed in
   stages:

	o	All attached addresses are detached
		Any non-permenant interrupts are detached
		Any non-permemant device instances are deleted

	o	the devices all initialize their address spaces
	
	o	the devices all initialize their data areas

   */

INLINE_DEVICE(void) device_init_address
(device *me,
 psim *system);

INLINE_DEVICE(void) device_init_data
(device *me,
 psim *system);

INLINE_DEVICE(void) device_tree_init
(device *root,
 psim *system);



/* Device Properties:

   Attached to a device node are its properties.  Properties describe
   the devices characteristics, for instance the address of its
   configuration and contrll registers. Unlike OpenBoot PSIM strictly
   types all properties.properties that profile the devices features.
   The below allow the manipulation of device properties */

typedef enum {
  array_property,
  boolean_property,
  ihandle_property,
  integer_property,
  phandle_property,
  string_property,
} device_property_type;

typedef struct _device_property device_property;
struct _device_property {
  device *owner;
  const char *name;
  device_property_type type;
  unsigned sizeof_array;
  const void *array;
  const device_property *original;
};


/* Locate a devices properties */

INLINE_DEVICE(const device_property *) device_next_property
(const device_property *previous);

INLINE_DEVICE(const device_property *) device_find_property
(device *me,
 const char *property); /* NULL for first property */


/* Similar to above except that the property *must* be in the device
   tree and *must* be of the specified type.

   If this isn't the case each function indicates is fail action */

INLINE_DEVICE(const device_property *) device_find_array_property
(device *me,
 const char *property);

INLINE_DEVICE(int) device_find_boolean_property
(device *me,
 const char *property);

INLINE_DEVICE(device_instance *) device_find_ihandle_property
(device *me,
 const char *property);

INLINE_DEVICE(signed_word) device_find_integer_property
(device *me,
 const char *property);

INLINE_DEVICE(device *) device_find_phandle_property
(device *me,
 const char *property);

INLINE_DEVICE(const char *) device_find_string_property
(device *me,
 const char *property);


/* INLINE_DEVICE void device_add_property
   No such external function, all properties, when added are explictly
   typed */

INLINE_DEVICE(void) device_add_duplicate_property
(device *me,
 const char *property,
 const device_property *original);

INLINE_DEVICE(void) device_add_array_property
(device *me,
 const char *property,
 const void *array,
 int sizeof_array);

INLINE_DEVICE(void) device_add_boolean_property
(device *me,
 const char *property,
 int bool);

INLINE_DEVICE(void) device_add_ihandle_property
(device *me,
 const char *property,
 device_instance *ihandle);

INLINE_DEVICE(void) device_add_integer_property
(device *me,
 const char *property,
 signed_word integer);

INLINE_DEVICE(void) device_add_phandle_property
(device *me,
 const char *property,
 device *phandle);

INLINE_DEVICE(void) device_add_string_property
(device *me,
 const char *property,
 const char *string);



/* Device Hardware:

   This model assumes that the data paths of the system being modeled
   have a tree topology.  That is, one or more processors sit at the
   top of a tree.  That tree containing leaf nodes (real devices) and
   branch nodes (bridges).

   For instance, consider the tree:

   /pci                    # PCI-HOST bridge
   /pci/pci1000,1@1        # A pci controller
   /pci/isa8086            # PCI-ISA bridge
   /pci/isa8086/fdc@300    # floppy disk controller on ISA bus

   A processor needing to access the device fdc@300 on the ISA bus
   would do so using a data path that goes through the pci-host bridge
   (pci)and the isa-pci bridge (isa8086) to finally reach the device
   fdc@300.  As the data transfer passes through each intermediate
   bridging node that bridge device is able to (just like with real
   hardware) manipulate either the address or data involved in the
   transfer. */

INLINE_DEVICE(unsigned) device_io_read_buffer
(device *me,
 void *dest,
 int space,
 unsigned_word addr,
 unsigned nr_bytes,
 cpu *processor,
 unsigned_word cia);

INLINE_DEVICE(unsigned) device_io_write_buffer
(device *me,
 const void *source,
 int space,
 unsigned_word addr,
 unsigned nr_bytes,
 cpu *processor,
 unsigned_word cia);


/* Conversly, the device pci1000,1@1 my need to perform a dma transfer
   into the cpu/memory core.  Just as I/O moves towards the leaves,
   dma transfers move towards the core via the initiating devices
   parent nodes.  The root device (special) converts the DMA transfer
   into reads/writes to memory */

INLINE_DEVICE(unsigned) device_dma_read_buffer
(device *me,
 void *dest,
 int space,
 unsigned_word addr,
 unsigned nr_bytes);

INLINE_DEVICE(unsigned) device_dma_write_buffer
(device *me,
 const void *source,
 int space,
 unsigned_word addr,
 unsigned nr_bytes,
 int violate_read_only_section);

/* To avoid the need for an intermediate (bridging) node to ask each
   of its child devices in turn if an IO access is intended for them,
   parent nodes maintain a table mapping addresses directly to
   specific devices.  When a device is `connected' to its bus it
   attaches its self to its parent. */

/* Address access attributes */
typedef enum _access_type {
  access_invalid = 0,
  access_read = 1,
  access_write = 2,
  access_read_write = 3,
  access_exec = 4,
  access_read_exec = 5,
  access_write_exec = 6,
  access_read_write_exec = 7,
} access_type;

/* Address attachement types */
typedef enum _attach_type {
  attach_invalid,
  attach_callback,
  attach_default,
  attach_raw_memory,
} attach_type;

INLINE_DEVICE(void) device_attach_address
(device *me,
 const char *name,
 attach_type attach,
 int space,
 unsigned_word addr,
 unsigned nr_bytes,
 access_type access,
 device *who); /*callback/default*/

INLINE_DEVICE(void) device_detach_address
(device *me,
 const char *name,
 attach_type attach,
 int space,
 unsigned_word addr,
 unsigned nr_bytes,
 access_type access,
 device *who); /*callback/default*/

/* where the attached address space can be any of:

   callback - all accesses to that range of addresses are past on to
   the attached child device.

   default - if no other device claims the access, it is passed on to
   this child device (giving subtractive decoding).

   memory - the specified address space contains RAM, the node that is
   having the ram attached is responsible for allocating space for and
   maintaining that space.  The device initiating the attach will not
   be notified of accesses to such an attachement.

   The last type of attachement is very important.  By giving the
   parent node the responsability (and freedom) of managing RAM, that
   node is able to implement memory spaces more efficiently.  For
   instance it could `cache' accesses or merge adjacent memory areas.


   In addition to I/O and DMA, devices interact with the rest of the
   system via interrupts.  Interrupts are discussed in the next
   section. */


/* Interrupts

   PSIM models interrupts and their wiring as a directed graph of
   connections between interrupt sources and destinations.  The source
   and destination are both a tupple consisting of a port number and
   device.  Both multiple destinations attached to a single source and
   multiple sources attached to a single destination are allowed.

   When a device drives an interrupt port with multiple destinations a
   broadcast of that interrupt event (message to all destinations)
   occures.  Each of those destination (device/port) are able to
   further propogate the interrupt until it reaches its ultimate
   destination.

   Normally an interrupt source would be a model of a real device
   (such as a keyboard) while an interrupt destination would be an
   interrupt controller.  The facility that allows an interrupt to be
   delivered to multiple devices and to be propogated from device to
   device was designed so that requirements specified by OpenPIC (ISA
   interrupts go to both OpenPIC and 8259), CHRP (8259 connected to
   OpenPIC) and hardware designs such as PCI-PCI bridges.


   Interrupt Source

   A device drives its interrupt line using the call: */

INLINE_DEVICE(void) device_interrupt_event
(device *me,
 int my_port,
 int value,
 cpu *processor,
 unsigned_word cia);

/* This interrupt event will then be propogated to any attached
   interrupt destinations.

   Any interpretation of PORT and VALUE is model dependant.  However
   as guidelines the following are recommended: PCI interrupts a-d
   correspond to lines 0-3; level sensative interrupts be requested
   with a value of one and withdrawn with a value of 0; edge sensative
   interrupts always have a value of 1, the event its self is treated
   as the interrupt.


   Interrupt Destinations

   Attached to each interrupt line of a device can be zero or more
   desitinations.  These destinations consist of a device/port pair.
   A destination is attached/detached to a device line using the
   attach and detach calls. */

INLINE_DEVICE(void) device_interrupt_attach
(device *me,
 int my_port,
 device *dest,
 int dest_port,
 int permenant);

INLINE_DEVICE(void) device_interrupt_detach
(device *me,
 int my_port,
 device *dest,
 int dest_port);

/* DESTINATION is attached (detached) to LINE of the device ME

   Should no destination be attached to a given devices interrupt line
   then that interrupt is propogated up the device tree (through
   parent nodes) until a parent that has an interrupt destination
   attached to its special child-interrupt line.  These are attached
   with: */

INLINE_DEVICE(void) device_child_interrupt_attach
(device *me,
 device *destination,
 int permenant);

INLINE_DEVICE(void) device_child_interrupt_detach
(device *me,
 device *destination);

/* It is an error for an interrupt to be propogated past the root node


   Interrupting a processor

   While it is possible for an interrupt destination device to
   directly interrupt a processor (using interrupt.h calls) it is not
   the norm.  Interrupting a processor midway through the cpu cycle
   would result in a simulation restart.  This restart may result in
   the interrupt not being delivered to all the intended destinations.

   Instead, an interrupt controller, when it has been determined that
   a processor should be interrupted should:

       o   Schedule an immediate timer event (using events.h)
           These events occure at the end of the cpu cycle.

       o   When this timer event is delivered (at the end of the
           cpu cycle) the interrupt controller can then deliver
	   its interrupt.

   It should be noted that interrupts can be delivered to an interrupt
   controller either during the middle of the cpu cycle or at the end.
   Further an interrupt could be delivered at the end of a cycle
   before and after an interrupt controller receives its own timer
   events.  An interrupt controller must be able to handle these cases
   gracefully. */


/* IOCTL:

   Very simply, a catch all for any thing that turns up that until now
   either hasn't been thought of or doesn't justify an extra function. */

EXTERN_DEVICE\
(void) device_ioctl
(device *me,
 psim *system,
 cpu *processor,
 unsigned_word cia,
 ...);


/* Tree utilities:

   In addition to the standard method of creating a device from a
   device template, the following sortcuts can be used.

   Create a device or property from a textual representation */

EXTERN_DEVICE(device *) device_tree_add_parsed
(device *current,
 const char *fmt,
 ...) __attribute__ ((format (printf, 2, 3)));

/* where FMT,... once formatted (using vsprintf) is used to locate and
   create either a device or property.  Its syntax is almost identical
   to that used in OpenBoot documentation - the only extension is in
   allowing properties and their values to be specified vis:

   "/pci/pci1000,1@1/disk@0,0"

   Path:

   The path to a device or property can either be absolute (leading
   `/') or relative (leading `.' or `..').  Relative paths start from
   the CURRENT node.  The new current node is returned as the result.
   In addition, a path may start with a leading alias (resolved by
   looking in /aliases).

   Device name:

   <name> "@" <unit> [ ":" <args> ]

   Where <name> is the name of the template device, <unit> is a
   textual specification of the devices unit address (that is
   converted into a numeric form by the devices parent) and <args> are
   optional additional information to be passed to the device-template
   when it creates the device.

   Properties:

   Properties are specified in a similar way to devices except that
   the last element on the path (which would have been the device) is
   the property name. This path is then followed by the property
   value. Unlike OpenBoot, the property values in the device tree are
   strongly typed.

   String property:

     <property-name> " " <text>
     <property-name> " " "\"" <text>
   
   Boolean property:

     <property-name> " " [ "true" | "false" ]
   Integer property or integer array property:

     <property-name> " " <number> { <number> }

   Phandle property:

     <property-name> " " "&" <path-to-device>

   Ihandle property:

     <property-name> " " "*" <path-to-device-to-open>

   Duplicate existing property:

     <property-name> " " "!" <path-to-original-property>


   In addition to properties, the wiring of interrupts can be
   specified:

   Attach interrupt <line> of <device> to <controller>:

     <device> " " ">" <my-port>  <dest-port> <dest-device>

   Attach child interrupt of <device> to <controller>:

     <device> " " "<" <controller>


   Once created, a device tree can be traversed in various orders: */

typedef void (device_tree_traverse_function)
     (device *device,
      void *data);

INLINE_DEVICE(void) device_tree_traverse
(device *root,
 device_tree_traverse_function *prefix,
 device_tree_traverse_function *postfix,
 void *data);

/* Or dumped out in a format that can be read back in using
   device_add_parsed() */

INLINE_DEVICE(void) device_tree_print_device
(device *device,
 void *ignore_data_argument);

/* Individual nodes can be located using */

INLINE_DEVICE(device *) device_tree_find_device
(device *root,
 const char *path);

/* And the current list of devices can be listed */

INLINE_DEVICE(void) device_usage
(int verbose);


/* External representation

   Both device nodes and device instances, in OpenBoot firmware have
   an external representation (phandles and ihandles) and these values
   are both stored in the device tree in property nodes and passed
   between the client program and the simulator during emulation
   calls.

   To limit the potential risk associated with trusing `data' from the
   client program, the following mapping operators `safely' convert
   between the two representations: */

INLINE_DEVICE(device *) external_to_device
(device *tree_member,
 unsigned32 phandle);

INLINE_DEVICE(unsigned32) device_to_external
(device *me);

INLINE_DEVICE(device_instance *) external_to_device_instance
(device *tree_member,
 unsigned32 ihandle);

INLINE_DEVICE(unsigned32) device_instance_to_external
(device_instance *me);

#endif /* _DEVICE_H_ */
