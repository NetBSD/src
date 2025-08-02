/* Emulation of eBPF helpers.
   Copyright (C) 2020-2023 Free Software Foundation, Inc.

   This file is part of GDB, the GNU debugger.

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

/* BPF programs rely on the existence of several helper functions,
   which are provided by the kernel.  This simulator provides an
   implementation of the helpers, which can be customized by the
   user.  */

/* This must come before any other includes.  */
#include "defs.h"

#define WANT_CPU_BPFBF
#define WANT_CPU bpfbf

#include "sim-main.h"
#include "cgen-mem.h"
#include "cgen-ops.h"
#include "cpu.h"

#include "bpf-helpers.h"

/* bpf_trace_printk is a printk-like facility for debugging.

   In the kernel, it appends a line to the Linux's tracing debugging
   interface.

   In this simulator, it uses the simulator's tracing interface
   instead.

   The format tags recognized by this helper are:
   %d, %i, %u, %x, %ld, %li, %lu, %lx, %lld, %lli, %llu, %llx,
   %p, %s

   A maximum of three tags are supported.

   This helper returns the number of bytes written, or a negative
   value in case of failure.  */

int
bpf_trace_printk (SIM_CPU *current_cpu)
{
  va_list ap;
  SIM_DESC sd = CPU_STATE (current_cpu);

  DI fmt_address;
  uint32_t size, tags_processed;
  size_t i, bytes_written = 0;

  /* The first argument is the format string, which is passed as a
     pointer in %r1.  */
  fmt_address = GET_H_GPR (1);

  /* The second argument is the length of the format string, as an
     unsigned 32-bit number in %r2.  */
  size = GET_H_GPR (2);

  /* Read the format string from the memory pointed by %r2, printing
     out the stuff as we go.  There is a maximum of three format tags
     supported, which are read from %r3, %r4 and %r5 respectively.  */
  for (i = 0, tags_processed = 0; i < size;)
    {
      UDI value;
      QI c = GETMEMUQI (current_cpu, CPU_PC_GET (current_cpu),
                        fmt_address + i);

      switch (c)
        {
        case '%':
          /* Check we are not exceeding the limit of three format
             tags.  */
          if (tags_processed > 2)
            return -1; /* XXX look for kernel error code.  */

          /* Depending on the kind of tag, extract the value from the
             proper argument.  */
          if (i++ >= size)
            return -1; /* XXX look for kernel error code.  */

          value = GET_H_GPR (3 + tags_processed);

          switch ((GETMEMUQI (current_cpu, CPU_PC_GET (current_cpu),
                             fmt_address + i)))
            {
            case 'd':
              trace_printf (sd, current_cpu, "%d", (int) value);
              break;
            case 'i':
              trace_printf (sd, current_cpu, "%i", (int) value);
              break;
            case 'u':
              trace_printf (sd, current_cpu, "%u", (unsigned int) value);
              break;
            case 'x':
              trace_printf (sd, current_cpu, "%x", (unsigned int) value);
              break;
            case 'l':
              {
                if (i++ >= size)
                  return -1;
                switch (GETMEMUQI (current_cpu, CPU_PC_GET (current_cpu),
                             fmt_address + i))
                  {
                  case 'd':
                    trace_printf (sd, current_cpu, "%ld", (long) value);
                    break;
                  case 'i':
                    trace_printf (sd, current_cpu, "%li", (long) value);
                    break;
                  case 'u':
                    trace_printf (sd, current_cpu, "%lu", (unsigned long) value);
                    break;
                  case 'x':
                    trace_printf (sd, current_cpu, "%lx", (unsigned long) value);
                    break;
                  case 'l':
                    {
                      if (i++ >= size)
                        return -1;
                      switch (GETMEMUQI (current_cpu, CPU_PC_GET (current_cpu),
                                fmt_address + i)) {
                        case 'd':
                          trace_printf (sd, current_cpu, "%lld", (long long) value);
                          break;
                        case 'i':
                          trace_printf (sd, current_cpu, "%lli", (long long) value);
                          break;
                        case 'u':
                          trace_printf (sd, current_cpu, "%llu", (unsigned long long) value);
                          break;
                        case 'x':
                          trace_printf (sd, current_cpu, "%llx", (unsigned long long) value);
                          break;
                        default:
                          assert (0);
                          break;
                      }
                      break;
                    }
                  default:
                    assert (0);
                    break;
                }
                break;
              }
            default:
              /* XXX completeme */
              assert (0);
              break;
            }

          tags_processed++;
          i++;
          break;
        case '\0':
          i = size;
          break;
        default:
          trace_printf (sd, current_cpu, "%c", c);
          bytes_written++;
          i++;
          break;
        }
    }

  return bytes_written;
}
