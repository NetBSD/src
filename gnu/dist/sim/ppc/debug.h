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


#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "filter_filename.h"

typedef enum {
  trace_invalid,
  trace_tbd,
  /**/
  trace_gdb,
  trace_os_emul,
  /**/
  trace_device_tree,
  trace_devices,
  trace_pass_device,
  trace_console_device,
  trace_icu_device,
  trace_halt_device,
  trace_register_device,
  trace_vm_device,
  trace_memory_device,
  trace_htab_device,
  trace_binary_device,
  trace_file_device,
  trace_core_device,
  trace_stack_device,
  /**/
  trace_semantics,
  trace_idecode,
  trace_alu,
  trace_load_store,
  trace_model,
  /**/
  trace_vm,
  trace_core,
  trace_psim,
  trace_device_init,
  trace_cpu,
  trace_breakpoint,
  trace_opts,
  trace_print_info,
  trace_print_device_tree,
  trace_dump_device_tree,
  nr_trace_options
} trace_options;



extern int ppc_trace[nr_trace_options];

#if WITH_TRACE
#define TRACE(OBJECT, ARGS) \
do { \
  if (ppc_trace[OBJECT]) { \
    printf_filtered("%s:%d: ", filter_filename(__FILE__), __LINE__); \
    printf_filtered ARGS; \
  } \
} while (0)
/* issue */
#define ITRACE(OBJECT, ARGS) \
do { \
  if (ppc_trace[OBJECT]) { \
    printf_filtered("%s:%d:0x%lx", my_prefix, cpu_nr(processor) + 1, (unsigned long)cia); \
    printf_filtered ARGS; \
  } \
} while (0)
/* device */
#define DTRACE(OBJECT, ARGS) \
do { \
  if (ppc_trace[trace_devices] || ppc_trace[trace_##OBJECT##_device]) { \
    printf_filtered("%s:%d:%s: ", filter_filename(__FILE__), __LINE__, #OBJECT); \
    printf_filtered ARGS; \
  } \
} while (0)
#else
#define TRACE(OBJECT, ARGS)
#define ITRACE(OBJECT, ARGS)
#define DTRACE(OBJECT, ARGS)
#endif

#if WITH_ASSERT
#define ASSERT(EXPRESSION) \
do { \
  if (!(EXPRESSION)) { \
    error("%s:%d: assertion failed - %s\n", \
	  filter_filename(__FILE__), __LINE__, #EXPRESSION); \
  } \
} while (0)
#else
#define ASSERT(EXPRESSION)
#endif

/* Parse OPTION updating the trace array */
extern void
trace_option(const char *option, int setting);

/* Output the list of trace options */
extern void trace_usage
(int verbose);


#endif /* _DEBUG_H_ */
