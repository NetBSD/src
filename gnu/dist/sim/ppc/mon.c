/*  This file is part of the program psim.

    Copyright (C) 1994-1995, Andrew Cagney <cagney@highland.com.au>

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


#ifndef _MON_C_
#define _MON_C_

#include "basics.h"
#include "cpu.h"
#include "mon.h"
#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
int getrusage();
#endif

struct _cpu_mon {
  count_type issue_count[nr_itable_entries];
  count_type read_count;
  count_type write_count;
  count_type unaligned_read_count;
  count_type unaligned_write_count;
  count_type event_count[nr_mon_events];
};

struct _mon {
  int nr_cpus;
  cpu_mon cpu_monitor[MAX_NR_PROCESSORS];
};


INLINE_MON\
(mon *)
mon_create(void)
{
  mon *monitor = ZALLOC(mon);
  return monitor;
}


INLINE_MON\
(cpu_mon *)
mon_cpu(mon *monitor,
	int cpu_nr)
{
  if (cpu_nr < 0 || cpu_nr >= MAX_NR_PROCESSORS)
    error("mon_cpu() - invalid cpu number\n");
  return &monitor->cpu_monitor[cpu_nr];
}


INLINE_MON\
(void)
mon_init(mon *monitor,
	 int nr_cpus)
{
  memset(monitor, 0, sizeof(*monitor));
  monitor->nr_cpus = nr_cpus;
}


INLINE_MON\
(void)
mon_issue(itable_index index,
	  cpu *processor, 
	  unsigned_word cia)
{
  cpu_mon *monitor = cpu_monitor(processor);
  ASSERT(index <= nr_itable_entries);
  monitor->issue_count[index] += 1;
}


INLINE_MON\
(void)
mon_read(unsigned_word ea,
	 unsigned_word ra,
	 unsigned nr_bytes,
	 cpu *processor,
	 unsigned_word cia)
{
  cpu_mon *monitor = cpu_monitor(processor);
  monitor->read_count += 1;
  if ((nr_bytes - 1) & ea)
    monitor->unaligned_read_count += 1;
}


INLINE_MON\
(void)
mon_write(unsigned_word ea,
	  unsigned_word ra,
	  unsigned nr_bytes,
	  cpu *processor,
	  unsigned_word cia)
{
  cpu_mon *monitor = cpu_monitor(processor);
  monitor->write_count += 1;
  if ((nr_bytes - 1) & ea)
    monitor->unaligned_write_count += 1;
}

INLINE_MON\
(void)
mon_event(mon_events event,
	  cpu *processor,
	  unsigned_word cia)
{
  cpu_mon *monitor = cpu_monitor(processor);
  ASSERT(event >= 0 && event < nr_mon_events);
  monitor->event_count[event] += 1;
}

STATIC_INLINE_MON\
(count_type)
mon_get_number_of_insns(cpu_mon *monitor)
{
  itable_index index;
  count_type total_insns = 0;
  for (index = 0; index < nr_itable_entries; index++)
    total_insns += monitor->issue_count[index];
  return total_insns;
}

static int
mon_sort_instruction_names(const void *ptr_a, const void *ptr_b)
{
  itable_index a = *(const itable_index *)ptr_a;
  itable_index b = *(const itable_index *)ptr_b;

  return strcmp (itable[a].name, itable[b].name);
}

STATIC_INLINE_MON\
(char *)
mon_add_commas(char *buf,
	       int sizeof_buf,
	       count_type value)
{
  int comma = 3;
  char *endbuf = buf + sizeof_buf - 1;

  *--endbuf = '\0';
  do {
    if (comma-- == 0)
      {
	*--endbuf = ',';
	comma = 2;
      }

    *--endbuf = (value % 10) + '0';
  } while ((value /= 10) != 0);

  ASSERT(endbuf >= buf);
  return endbuf;
}


INLINE_MON\
(void)
mon_print_info(psim *system,
	       mon *monitor,
	       int verbose)
{
  char buffer[20];
  int cpu_nr;
  int len_cpu;
  int len_num = 0;
  int len;
  long total_insns = 0;
  long cpu_insns_second = 0;
  double cpu_time = 0.0;

  for (cpu_nr = 0; cpu_nr < monitor->nr_cpus; cpu_nr++) {
    count_type num_insns = mon_get_number_of_insns(&monitor->cpu_monitor[cpu_nr]);

    total_insns += num_insns;
    len = strlen (mon_add_commas(buffer, sizeof(buffer), num_insns));
    if (len_num < len)
      len_num = len;
  }

  sprintf (buffer, "%d", (int)monitor->nr_cpus + 1);
  len_cpu = strlen (buffer);

#ifdef HAVE_GETRUSAGE
  {
    struct rusage mytime;
    if (getrusage (RUSAGE_SELF, &mytime) == 0
	&& (mytime.ru_utime.tv_sec > 0 || mytime.ru_utime.tv_usec > 0)) {
      
      cpu_time = (double)mytime.ru_utime.tv_sec + (((double)mytime.ru_utime.tv_usec) / 1000000.0);
    }
  }
  if (cpu_time > 0 && total_insns > 0)
    cpu_insns_second = (long)(((double)total_insns / cpu_time) + 0.5);
#endif

  for (cpu_nr = 0; cpu_nr < monitor->nr_cpus; cpu_nr++) {

    if (verbose > 1) {
      itable_index sort_insns[nr_itable_entries];
      int nr_sort_insns = 0;
      itable_index index;
      int index2;

      if (cpu_nr)
	printf_filtered ("\n");

      for (index = 0; index < nr_itable_entries; index++) {
	if (monitor->cpu_monitor[cpu_nr].issue_count[index]) {
	  sort_insns[nr_sort_insns++] = index;
	}
      }

      qsort((void *)sort_insns, nr_sort_insns, sizeof(sort_insns[0]), mon_sort_instruction_names);

      for (index2 = 0; index2 < nr_sort_insns; index2++) {
	index = sort_insns[index2];
	printf_filtered("CPU #%*d executed %*s %s instruction%s.\n",
			len_cpu, cpu_nr+1,
			len_num, mon_add_commas(buffer,
						sizeof(buffer),
						monitor->cpu_monitor[cpu_nr].issue_count[index]),
			  itable[index].name,
			  (monitor->cpu_monitor[cpu_nr].issue_count[index] == 1) ? "" : "s");
      }

      printf_filtered ("\n");
    }

    if (CURRENT_MODEL_ISSUE > 0)
      {
	model_data *model_ptr = cpu_model(psim_cpu(system, cpu_nr));
	model_print *ptr = model_mon_info(model_ptr);
	model_print *orig_ptr = ptr;

	while (ptr) {
	  if (ptr->count)
	    printf_filtered("CPU #%*d executed %*s %s%s.\n",
			    len_cpu, cpu_nr+1,
			    len_num, mon_add_commas(buffer,
						    sizeof(buffer),
						    ptr->count),
			    ptr->name,
			    ((ptr->count == 1)
			     ? ptr->suffix_singular
			     : ptr->suffix_plural));

	  ptr = ptr->next;
	}

	model_mon_info_free(model_ptr, orig_ptr);
      }

    if (monitor->cpu_monitor[cpu_nr].read_count)
      printf_filtered ("CPU #%*d executed %*s data read%s.\n",
		       len_cpu, cpu_nr+1,
		       len_num, mon_add_commas(buffer,
					       sizeof(buffer),
					       monitor->cpu_monitor[cpu_nr].read_count),
		       (monitor->cpu_monitor[cpu_nr].read_count == 1) ? "" : "s");

    if (monitor->cpu_monitor[cpu_nr].write_count)
      printf_filtered ("CPU #%*d executed %*s data write%s.\n",
		       len_cpu, cpu_nr+1,
		       len_num, mon_add_commas(buffer,
					       sizeof(buffer),
					       monitor->cpu_monitor[cpu_nr].write_count),
		       (monitor->cpu_monitor[cpu_nr].write_count == 1) ? "" : "s");

    if (monitor->cpu_monitor[cpu_nr].unaligned_read_count)
      printf_filtered ("CPU #%*d executed %*s unaligned data read%s.\n",
		       len_cpu, cpu_nr+1,
		       len_num, mon_add_commas(buffer,
					       sizeof(buffer),
					       monitor->cpu_monitor[cpu_nr].read_count),
		       (monitor->cpu_monitor[cpu_nr].read_count == 1) ? "" : "s");

    if (monitor->cpu_monitor[cpu_nr].unaligned_write_count)
      printf_filtered ("CPU #%*d executed %*s unaligned data write%s.\n",
		       len_cpu, cpu_nr+1,
		       len_num, mon_add_commas(buffer,
					       sizeof(buffer),
					       monitor->cpu_monitor[cpu_nr].write_count),
		       (monitor->cpu_monitor[cpu_nr].write_count == 1) ? "" : "s");
    
    if (monitor->cpu_monitor[cpu_nr].event_count[mon_event_icache_miss])
      printf_filtered ("CPU #%*d executed %*s icache miss%s.\n",
		       len_cpu, cpu_nr+1,
		       len_num, mon_add_commas(buffer,
					       sizeof(buffer),
					       monitor->cpu_monitor[cpu_nr].event_count[mon_event_icache_miss]),
		       (monitor->cpu_monitor[cpu_nr].event_count[mon_event_icache_miss] == 1) ? "" : "es");

    {
      long nr_insns = mon_get_number_of_insns(&monitor->cpu_monitor[cpu_nr]);
      if (nr_insns > 0)
	printf_filtered("CPU #%*d executed %*s instructions in total.\n",
			len_cpu, cpu_nr+1,
			len_num, mon_add_commas(buffer,
						sizeof(buffer),
						nr_insns));
    }
  }

  if (monitor->nr_cpus > 1)
    printf_filtered("\nAll CPUs executed %s instructions in total.\n",
		    mon_add_commas(buffer, sizeof(buffer), total_insns));

  if (cpu_insns_second)
    printf_filtered ("%sSimulator speed was %s instructions/second\n",
		     (monitor->nr_cpus > 1) ? "" : "\n",
		     mon_add_commas(buffer, sizeof(buffer), cpu_insns_second));
  else if (cpu_time > 0.0)
    printf_filtered ("%sSimulator executed for %.2f seconds\n",
		     (monitor->nr_cpus > 1) ? "" : "\n", cpu_time);
}

#endif /* _MON_C_ */
