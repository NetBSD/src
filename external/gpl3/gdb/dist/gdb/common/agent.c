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

#ifdef GDBSERVER
#include "server.h"
#else
#include "defs.h"
#include "target.h"
#include "inferior.h" /* for non_stop */
#endif

#include <string.h>
#include <unistd.h>
#include "agent.h"
#include "filestuff.h"

int debug_agent = 0;

#ifdef GDBSERVER
#define DEBUG_AGENT(fmt, args...)	\
  if (debug_agent)			\
    fprintf (stderr, fmt, ##args);
#else
#define DEBUG_AGENT(fmt, args...)	\
  if (debug_agent)			\
    fprintf_unfiltered (gdb_stdlog, fmt, ##args);
#endif

/* Global flag to determine using agent or not.  */
int use_agent = 0;

/* Addresses of in-process agent's symbols both GDB and GDBserver cares
   about.  */

struct ipa_sym_addresses
{
  CORE_ADDR addr_helper_thread_id;
  CORE_ADDR addr_cmd_buf;
  CORE_ADDR addr_capability;
};

/* Cache of the helper thread id.  FIXME: this global should be made
   per-process.  */
static unsigned int helper_thread_id = 0;

static struct
{
  const char *name;
  int offset;
  int required;
} symbol_list[] = {
  IPA_SYM(helper_thread_id),
  IPA_SYM(cmd_buf),
  IPA_SYM(capability),
};

static struct ipa_sym_addresses ipa_sym_addrs;

static int all_agent_symbols_looked_up = 0;

int
agent_loaded_p (void)
{
  return all_agent_symbols_looked_up;
}

/* Look up all symbols needed by agent.  Return 0 if all the symbols are
   found, return non-zero otherwise.  */

int
agent_look_up_symbols (void *arg)
{
  int i;

  all_agent_symbols_looked_up = 0;

  for (i = 0; i < sizeof (symbol_list) / sizeof (symbol_list[0]); i++)
    {
      CORE_ADDR *addrp =
	(CORE_ADDR *) ((char *) &ipa_sym_addrs + symbol_list[i].offset);
#ifdef GDBSERVER

      if (look_up_one_symbol (symbol_list[i].name, addrp, 1) == 0)
#else
      struct minimal_symbol *sym =
	lookup_minimal_symbol (symbol_list[i].name, NULL,
			       (struct objfile *) arg);

      if (sym != NULL)
	*addrp = SYMBOL_VALUE_ADDRESS (sym);
      else
#endif
	{
	  DEBUG_AGENT ("symbol `%s' not found\n", symbol_list[i].name);
	  return -1;
	}
    }

  all_agent_symbols_looked_up = 1;
  return 0;
}

static unsigned int
agent_get_helper_thread_id (void)
{
  if  (helper_thread_id == 0)
    {
#ifdef GDBSERVER
      if (read_inferior_memory (ipa_sym_addrs.addr_helper_thread_id,
				(unsigned char *) &helper_thread_id,
				sizeof helper_thread_id))
#else
      enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
      gdb_byte buf[4];

      if (target_read_memory (ipa_sym_addrs.addr_helper_thread_id,
			      buf, sizeof buf) == 0)
	helper_thread_id = extract_unsigned_integer (buf, sizeof buf,
						     byte_order);
      else
#endif
	{
	  warning (_("Error reading helper thread's id in lib"));
	}
    }

  return helper_thread_id;
}

#ifdef HAVE_SYS_UN_H
#include <sys/socket.h>
#include <sys/un.h>
#define SOCK_DIR P_tmpdir

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX sizeof(((struct sockaddr_un *) NULL)->sun_path)
#endif

#endif

/* Connects to synchronization socket.  PID is the pid of inferior, which is
   used to set up the connection socket.  */

static int
gdb_connect_sync_socket (int pid)
{
#ifdef HAVE_SYS_UN_H
  struct sockaddr_un addr;
  int res, fd;
  char path[UNIX_PATH_MAX];

  res = xsnprintf (path, UNIX_PATH_MAX, "%s/gdb_ust%d", P_tmpdir, pid);
  if (res >= UNIX_PATH_MAX)
    return -1;

  res = fd = gdb_socket_cloexec (PF_UNIX, SOCK_STREAM, 0);
  if (res == -1)
    {
      warning (_("error opening sync socket: %s"), strerror (errno));
      return -1;
    }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;

  res = xsnprintf (addr.sun_path, UNIX_PATH_MAX, "%s", path);
  if (res >= UNIX_PATH_MAX)
    {
      warning (_("string overflow allocating socket name"));
      close (fd);
      return -1;
    }

  res = connect (fd, (struct sockaddr *) &addr, sizeof (addr));
  if (res == -1)
    {
      warning (_("error connecting sync socket (%s): %s. "
		 "Make sure the directory exists and that it is writable."),
		 path, strerror (errno));
      close (fd);
      return -1;
    }

  return fd;
#else
  return -1;
#endif
}

/* Execute an agent command in the inferior.  PID is the value of pid of the
   inferior.  CMD is the buffer for command.  GDB or GDBserver will store the
   command into it and fetch the return result from CMD.  The interaction
   between GDB/GDBserver and the agent is synchronized by a synchronization
   socket.  Return zero if success, otherwise return non-zero.  */

int
agent_run_command (int pid, const char *cmd, int len)
{
  int fd;
  int tid = agent_get_helper_thread_id ();
  ptid_t ptid = ptid_build (pid, tid, 0);

#ifdef GDBSERVER
  int ret = write_inferior_memory (ipa_sym_addrs.addr_cmd_buf,
				   (const unsigned char *) cmd, len);
#else
  int ret = target_write_memory (ipa_sym_addrs.addr_cmd_buf,
				 (gdb_byte *) cmd, len);
#endif

  if (ret != 0)
    {
      warning (_("unable to write"));
      return -1;
    }

  DEBUG_AGENT ("agent: resumed helper thread\n");

  /* Resume helper thread.  */
#ifdef GDBSERVER
{
  struct thread_resume resume_info;

  resume_info.thread = ptid;
  resume_info.kind = resume_continue;
  resume_info.sig = GDB_SIGNAL_0;
  (*the_target->resume) (&resume_info, 1);
}
#else
 target_resume (ptid, 0, GDB_SIGNAL_0);
#endif

  fd = gdb_connect_sync_socket (pid);
  if (fd >= 0)
    {
      char buf[1] = "";
      int ret;

      DEBUG_AGENT ("agent: signalling helper thread\n");

      do
	{
	  ret = write (fd, buf, 1);
	} while (ret == -1 && errno == EINTR);

	DEBUG_AGENT ("agent: waiting for helper thread's response\n");

      do
	{
	  ret = read (fd, buf, 1);
	} while (ret == -1 && errno == EINTR);

      close (fd);

      DEBUG_AGENT ("agent: helper thread's response received\n");
    }
  else
    return -1;

  /* Need to read response with the inferior stopped.  */
  if (!ptid_equal (ptid, null_ptid))
    {
      struct target_waitstatus status;
      int was_non_stop = non_stop;
      /* Stop thread PTID.  */
      DEBUG_AGENT ("agent: stop helper thread\n");
#ifdef GDBSERVER
      {
	struct thread_resume resume_info;

	resume_info.thread = ptid;
	resume_info.kind = resume_stop;
	resume_info.sig = GDB_SIGNAL_0;
	(*the_target->resume) (&resume_info, 1);
      }

      non_stop = 1;
      mywait (ptid, &status, 0, 0);
#else
      non_stop = 1;
      target_stop (ptid);

      memset (&status, 0, sizeof (status));
      target_wait (ptid, &status, 0);
#endif
      non_stop = was_non_stop;
    }

  if (fd >= 0)
    {
#ifdef GDBSERVER
      if (read_inferior_memory (ipa_sym_addrs.addr_cmd_buf,
				(unsigned char *) cmd, IPA_CMD_BUF_SIZE))
#else
      if (target_read_memory (ipa_sym_addrs.addr_cmd_buf, (gdb_byte *) cmd,
			      IPA_CMD_BUF_SIZE))
#endif
	{
	  warning (_("Error reading command response"));
	  return -1;
	}
    }

  return 0;
}

/* Each bit of it stands for a capability of agent.  */
static unsigned int agent_capability = 0;

/* Return true if agent has capability AGENT_CAP, otherwise return false.  */

int
agent_capability_check (enum agent_capa agent_capa)
{
  if (agent_capability == 0)
    {
#ifdef GDBSERVER
      if (read_inferior_memory (ipa_sym_addrs.addr_capability,
				(unsigned char *) &agent_capability,
				sizeof agent_capability))
#else
      enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
      gdb_byte buf[4];

      if (target_read_memory (ipa_sym_addrs.addr_capability,
			      buf, sizeof buf) == 0)
	agent_capability = extract_unsigned_integer (buf, sizeof buf,
						     byte_order);
      else
#endif
	warning (_("Error reading capability of agent"));
    }
  return agent_capability & agent_capa;
}

/* Invalidate the cache of agent capability, so we'll read it from inferior
   again.  Call it when launches a new program or reconnect to remote stub.  */

void
agent_capability_invalidate (void)
{
  agent_capability = 0;
}
