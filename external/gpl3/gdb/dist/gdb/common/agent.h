/* Shared utility routines for GDB to interact with agent.

   Copyright (C) 2009-2014 Free Software Foundation, Inc.

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

int agent_run_command (int pid, const char *cmd, int len);

int agent_look_up_symbols (void *);

#define STRINGIZE_1(STR) #STR
#define STRINGIZE(STR) STRINGIZE_1(STR)
#define IPA_SYM(SYM)                                   \
  {                                                    \
    STRINGIZE (gdb_agent_ ## SYM),			\
    offsetof (struct ipa_sym_addresses, addr_ ## SYM)	\
  }

/* The size in bytes of the buffer used to talk to the IPA helper
   thread.  */
#define IPA_CMD_BUF_SIZE 1024

int agent_loaded_p (void);

extern int debug_agent;

extern int use_agent;

/* Capability of agent.  Different agents may have different capabilities,
   such as installing fast tracepoint or evaluating breakpoint conditions.
   Capabilities are represented by bit-maps, and each capability occupies one
   bit.  */

enum agent_capa
{
  /* Capability to install fast tracepoint.  */
  AGENT_CAPA_FAST_TRACE = 0x1,
  /* Capability to install static tracepoint.  */
  AGENT_CAPA_STATIC_TRACE = (0x1 << 1),
};

int agent_capability_check (enum agent_capa);

void agent_capability_invalidate (void);
