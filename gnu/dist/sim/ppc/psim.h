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


#ifndef _PSIM_H_
#define _PSIM_H_

#include "basics.h"


/* the system object */
/* typedef struct _psim psim; */
/* typedef struct _device device; */

/* when the `system' stops, find out why.  FIXME - at this point this
   is really a bit puzzling.  After all, how can there be a status
   when there several processors involved */

typedef struct _psim_status {
  int cpu_nr;
  stop_reason reason;
  int signal;
  unsigned_word program_counter;
} psim_status;


/* create an initial device tree and then populate it using
   information obtained from the command line */

extern device *psim_tree
(void);

extern char **psim_options
(device *root,
 char **argv);

extern void psim_usage
(int verbose);


/* create a new simulator */

extern psim *psim_create
(const char *file_name,
 device *root);


/* Given the created simulator (re) initialize it */

extern void psim_init
(psim *system);

extern void psim_stack
(psim *system,
 char **argv,
 char **envp);


/* Run/stop the system */

extern void psim_step
(psim *system);

extern void psim_run
(psim *system);

extern void psim_run_until_stop
(psim *system,
 volatile int *stop);

extern void psim_restart
(psim *system,
 int cpu_nr);

extern void psim_halt
(psim *system,
 int cpu_nr,
 unsigned_word cia,
 stop_reason reason,
 int signal);

extern psim_status psim_get_status
(psim *system);


/* reveal the internals of the simulation, giving access to cpu's and
   devices */

extern cpu *psim_cpu
(psim *system,
 int cpu_nr);

extern device *psim_device
(psim *system,
 const char *path);


/* manipulate the state (registers or memory) of a processor within
   the system.  In the case of memory, the read/write is performed
   using the specified processors address translation tables.

   Where applicable, WHICH_CPU == -1 indicates all processors and
   WHICH_CPU == <nr_cpus> indicates the `current' processor. */

extern void psim_read_register
(psim *system,
 int which_cpu,
 void *host_ordered_buf,
 const char reg[],
 transfer_mode mode);

extern void psim_write_register
(psim *system,
 int which_cpu,
 const void *buf,
 const char reg[],
 transfer_mode mode);

extern unsigned psim_read_memory
(psim *system,
 int which_cpu,
 void *buf,
 unsigned_word vaddr,
 unsigned len);

extern unsigned psim_write_memory
(psim *system,
 int which_cpu,
 const void *buf,
 unsigned_word vaddr,
 unsigned len,
 int violate_read_only_section);

extern void psim_print_info
(psim *system,
 int verbose);

/* FIXME: I do not want this here */
extern void psim_merge_device_file
(device *root,
 const char *file_name);

#endif /* _PSIM_H_ */
