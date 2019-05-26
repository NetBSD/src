/* Serial interface for raw TCP connections on Un*x like systems.

   Copyright (C) 1992-2017 Free Software Foundation, Inc.

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

#include "defs.h"
#include "serial.h"
#include "ser-base.h"
#include "ser-tcp.h"
#include "gdbcmd.h"
#include "cli/cli-decode.h"
#include "cli/cli-setshow.h"
#include "filestuff.h"

#include <sys/types.h>

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>  /* For FIONBIO.  */
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>  /* For FIONBIO.  */
#endif

#include "gdb_sys_time.h"

#ifdef USE_WIN32API
#include <winsock2.h>
#ifndef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#endif
#define close(fd) closesocket (fd)
#define ioctl ioctlsocket
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif

#include <signal.h>
#include "gdb_select.h"
#include <algorithm>

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif

void _initialize_ser_tcp (void);

/* For "set tcp" and "show tcp".  */

static struct cmd_list_element *tcp_set_cmdlist;
static struct cmd_list_element *tcp_show_cmdlist;

/* Whether to auto-retry refused connections.  */

static int tcp_auto_retry = 1;

/* Timeout period for connections, in seconds.  */

static unsigned int tcp_retry_limit = 15;

/* How many times per second to poll deprecated_ui_loop_hook.  */

#define POLL_INTERVAL 5

/* Helper function to wait a while.  If SCB is non-null, wait on its
   file descriptor.  Otherwise just wait on a timeout, updating *POLLS.
   Returns -1 on timeout or interrupt, otherwise the value of select.  */

static int
wait_for_connect (struct serial *scb, unsigned int *polls)
{
  struct timeval t;
  int n;

  /* While we wait for the connect to complete, 
     poll the UI so it can update or the user can 
     interrupt.  */
  if (deprecated_ui_loop_hook && deprecated_ui_loop_hook (0))
    {
      errno = EINTR;
      return -1;
    }

  /* Check for timeout.  */
  if (*polls > tcp_retry_limit * POLL_INTERVAL)
    {
      errno = ETIMEDOUT;
      return -1;
    }

  /* Back off to polling once per second after the first POLL_INTERVAL
     polls.  */
  if (*polls < POLL_INTERVAL)
    {
      t.tv_sec = 0;
      t.tv_usec = 1000000 / POLL_INTERVAL;
    }
  else
    {
      t.tv_sec = 1;
      t.tv_usec = 0;
    }

  if (scb)
    {
      fd_set rset, wset, eset;

      FD_ZERO (&rset);
      FD_SET (scb->fd, &rset);
      wset = rset;
      eset = rset;
	  
      /* POSIX systems return connection success or failure by signalling
	 wset.  Windows systems return success in wset and failure in
	 eset.
     
	 We must call select here, rather than gdb_select, because
	 the serial structure has not yet been initialized - the
	 MinGW select wrapper will not know that this FD refers
	 to a socket.  */
      n = select (scb->fd + 1, &rset, &wset, &eset, &t);
    }
  else
    /* Use gdb_select here, since we have no file descriptors, and on
       Windows, plain select doesn't work in that case.  */
    n = gdb_select (0, NULL, NULL, NULL, &t);

  /* If we didn't time out, only count it as one poll.  */
  if (n > 0 || *polls < POLL_INTERVAL)
    (*polls)++;
  else
    (*polls) += POLL_INTERVAL;

  return n;
}

/* Open a tcp socket.  */

int
net_open (struct serial *scb, const char *name)
{
  char hostname[100];
  const char *port_str;
  int n, port, tmp;
  int use_udp;
  struct hostent *hostent;
  struct sockaddr_in sockaddr;
#ifdef USE_WIN32API
  u_long ioarg;
#else
  int ioarg;
#endif
  unsigned int polls = 0;

  use_udp = 0;
  if (startswith (name, "udp:"))
    {
      use_udp = 1;
      name = name + 4;
    }
  else if (startswith (name, "tcp:"))
    name = name + 4;

  port_str = strchr (name, ':');

  if (!port_str)
    error (_("net_open: No colon in host name!"));  /* Shouldn't ever
						       happen.  */

  tmp = std::min (port_str - name, (ptrdiff_t) sizeof hostname - 1);
  strncpy (hostname, name, tmp);	/* Don't want colon.  */
  hostname[tmp] = '\000';	/* Tie off host name.  */
  port = atoi (port_str + 1);

  /* Default hostname is localhost.  */
  if (!hostname[0])
    strcpy (hostname, "localhost");

  hostent = gethostbyname (hostname);
  if (!hostent)
    {
      fprintf_unfiltered (gdb_stderr, "%s: unknown host\n", hostname);
      errno = ENOENT;
      return -1;
    }

  sockaddr.sin_family = PF_INET;
  sockaddr.sin_port = htons (port);
  memcpy (&sockaddr.sin_addr.s_addr, hostent->h_addr,
	  sizeof (struct in_addr));

 retry:

  if (use_udp)
    scb->fd = gdb_socket_cloexec (PF_INET, SOCK_DGRAM, 0);
  else
    scb->fd = gdb_socket_cloexec (PF_INET, SOCK_STREAM, 0);

  if (scb->fd == -1)
    return -1;
  
  /* Set socket nonblocking.  */
  ioarg = 1;
  ioctl (scb->fd, FIONBIO, &ioarg);

  /* Use Non-blocking connect.  connect() will return 0 if connected
     already.  */
  n = connect (scb->fd, (struct sockaddr *) &sockaddr, sizeof (sockaddr));

  if (n < 0)
    {
#ifdef USE_WIN32API
      int err = WSAGetLastError();
#else
      int err = errno;
#endif

      /* Maybe we're waiting for the remote target to become ready to
	 accept connections.  */
      if (tcp_auto_retry
#ifdef USE_WIN32API
	  && err == WSAECONNREFUSED
#else
	  && err == ECONNREFUSED
#endif
	  && wait_for_connect (NULL, &polls) >= 0)
	{
	  close (scb->fd);
	  goto retry;
	}

      if (
#ifdef USE_WIN32API
	  /* Under Windows, calling "connect" with a non-blocking socket
	     results in WSAEWOULDBLOCK, not WSAEINPROGRESS.  */
	  err != WSAEWOULDBLOCK
#else
	  err != EINPROGRESS
#endif
	  )
	{
	  errno = err;
	  net_close (scb);
	  return -1;
	}

      /* Looks like we need to wait for the connect.  */
      do 
	{
	  n = wait_for_connect (scb, &polls);
	} 
      while (n == 0);
      if (n < 0)
	{
	  net_close (scb);
	  return -1;
	}
    }

  /* Got something.  Is it an error?  */
  {
    int res, err;
    socklen_t len;

    len = sizeof (err);
    /* On Windows, the fourth parameter to getsockopt is a "char *";
       on UNIX systems it is generally "void *".  The cast to "char *"
       is OK everywhere, since in C++ any data pointer type can be
       implicitly converted to "void *".  */
    res = getsockopt (scb->fd, SOL_SOCKET, SO_ERROR, (char *) &err, &len);
    if (res < 0 || err)
      {
	/* Maybe the target still isn't ready to accept the connection.  */
	if (tcp_auto_retry
#ifdef USE_WIN32API
	    && err == WSAECONNREFUSED
#else
	    && err == ECONNREFUSED
#endif
	    && wait_for_connect (NULL, &polls) >= 0)
	  {
	    close (scb->fd);
	    goto retry;
	  }
	if (err)
	  errno = err;
	net_close (scb);
	return -1;
      }
  } 

  /* Turn off nonblocking.  */
  ioarg = 0;
  ioctl (scb->fd, FIONBIO, &ioarg);

  if (use_udp == 0)
    {
      /* Disable Nagle algorithm.  Needed in some cases.  */
      tmp = 1;
      setsockopt (scb->fd, IPPROTO_TCP, TCP_NODELAY,
		  (char *)&tmp, sizeof (tmp));
    }

#ifdef SIGPIPE
  /* If we don't do this, then GDB simply exits
     when the remote side dies.  */
  signal (SIGPIPE, SIG_IGN);
#endif

  return 0;
}

void
net_close (struct serial *scb)
{
  if (scb->fd == -1)
    return;

  close (scb->fd);
  scb->fd = -1;
}

int
net_read_prim (struct serial *scb, size_t count)
{
  /* Need to cast to silence -Wpointer-sign on MinGW, as Winsock's
     'recv' takes 'char *' as second argument, while 'scb->buf' is
     'unsigned char *'.  */
  return recv (scb->fd, (char *) scb->buf, count, 0);
}

int
net_write_prim (struct serial *scb, const void *buf, size_t count)
{
  /* On Windows, the second parameter to send is a "const char *"; on
     UNIX systems it is generally "const void *".  The cast to "const
     char *" is OK everywhere, since in C++ any data pointer type can
     be implicitly converted to "const void *".  */
  return send (scb->fd, (const char *) buf, count, 0);
}

int
ser_tcp_send_break (struct serial *scb)
{
  /* Send telnet IAC and BREAK characters.  */
  return (serial_write (scb, "\377\363", 2));
}

/* Support for "set tcp" and "show tcp" commands.  */

static void
set_tcp_cmd (char *args, int from_tty)
{
  help_list (tcp_set_cmdlist, "set tcp ", all_commands, gdb_stdout);
}

static void
show_tcp_cmd (char *args, int from_tty)
{
  help_list (tcp_show_cmdlist, "show tcp ", all_commands, gdb_stdout);
}

#ifndef USE_WIN32API

/* The TCP ops.  */

static const struct serial_ops tcp_ops =
{
  "tcp",
  net_open,
  net_close,
  NULL,
  ser_base_readchar,
  ser_base_write,
  ser_base_flush_output,
  ser_base_flush_input,
  ser_tcp_send_break,
  ser_base_raw,
  ser_base_get_tty_state,
  ser_base_copy_tty_state,
  ser_base_set_tty_state,
  ser_base_print_tty_state,
  ser_base_noflush_set_tty_state,
  ser_base_setbaudrate,
  ser_base_setstopbits,
  ser_base_setparity,
  ser_base_drain_output,
  ser_base_async,
  net_read_prim,
  net_write_prim
};

#endif /* USE_WIN32API */

void
_initialize_ser_tcp (void)
{
#ifdef USE_WIN32API
  /* Do nothing; the TCP serial operations will be initialized in
     ser-mingw.c.  */
#else
  serial_add_interface (&tcp_ops);
#endif /* USE_WIN32API */

  add_prefix_cmd ("tcp", class_maintenance, set_tcp_cmd, _("\
TCP protocol specific variables\n\
Configure variables specific to remote TCP connections"),
		  &tcp_set_cmdlist, "set tcp ",
		  0 /* allow-unknown */, &setlist);
  add_prefix_cmd ("tcp", class_maintenance, show_tcp_cmd, _("\
TCP protocol specific variables\n\
Configure variables specific to remote TCP connections"),
		  &tcp_show_cmdlist, "show tcp ",
		  0 /* allow-unknown */, &showlist);

  add_setshow_boolean_cmd ("auto-retry", class_obscure,
			   &tcp_auto_retry, _("\
Set auto-retry on socket connect"), _("\
Show auto-retry on socket connect"), 
			   NULL, NULL, NULL,
			   &tcp_set_cmdlist, &tcp_show_cmdlist);

  add_setshow_uinteger_cmd ("connect-timeout", class_obscure,
			    &tcp_retry_limit, _("\
Set timeout limit in seconds for socket connection"), _("\
Show timeout limit in seconds for socket connection"), _("\
If set to \"unlimited\", GDB will keep attempting to establish a\n\
connection forever, unless interrupted with Ctrl-c.\n\
The default is 15 seconds."),
			    NULL, NULL,
			    &tcp_set_cmdlist, &tcp_show_cmdlist);
}
