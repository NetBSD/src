/* Serial interface for raw TCP connections on Un*x like systems
   Copyright 1992, 1993, 1998-1999 Free Software Foundation, Inc.

   This file is part of GDB.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "serial.h"
#include "ser-unix.h"

#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#ifndef __CYGWIN32__
#include <netinet/tcp.h>
#endif

#include "signals.h"
#include "gdb_string.h"

static int tcp_open (serial_t scb, const char *name);
static void tcp_close (serial_t scb);

void _initialize_ser_tcp (void);

/* Open up a raw tcp socket */

static int
tcp_open (serial_t scb, const char *name)
{
  char *port_str;
  int port;
  struct hostent *hostent;
  struct sockaddr_in sockaddr;
  int tmp;
  char hostname[100];
  struct protoent *protoent;
  int i;

  port_str = strchr (name, ':');

  if (!port_str)
    error ("tcp_open: No colon in host name!");		/* Shouldn't ever happen */

  tmp = min (port_str - name, (int) sizeof hostname - 1);
  strncpy (hostname, name, tmp);	/* Don't want colon */
  hostname[tmp] = '\000';	/* Tie off host name */
  port = atoi (port_str + 1);

  hostent = gethostbyname (hostname);

  if (!hostent)
    {
      fprintf_unfiltered (gdb_stderr, "%s: unknown host\n", hostname);
      errno = ENOENT;
      return -1;
    }

  for (i = 1; i <= 15; i++)
    {
      scb->fd = socket (PF_INET, SOCK_STREAM, 0);
      if (scb->fd < 0)
	return -1;

      /* Allow rapid reuse of this port. */
      tmp = 1;
      setsockopt (scb->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &tmp, sizeof (tmp));

      /* Enable TCP keep alive process. */
      tmp = 1;
      setsockopt (scb->fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &tmp, sizeof (tmp));

      sockaddr.sin_family = PF_INET;
      sockaddr.sin_port = htons (port);
      memcpy (&sockaddr.sin_addr.s_addr, hostent->h_addr,
	      sizeof (struct in_addr));

      if (!connect (scb->fd, (struct sockaddr *) &sockaddr, sizeof (sockaddr)))
	break;

      close (scb->fd);
      scb->fd = -1;

/* We retry for ECONNREFUSED because that is often a temporary condition, which
   happens when the server is being restarted.  */

      if (errno != ECONNREFUSED)
	return -1;

      sleep (1);
    }

  protoent = getprotobyname ("tcp");
  if (!protoent)
    return -1;

  tmp = 1;
  if (setsockopt (scb->fd, protoent->p_proto, TCP_NODELAY,
		  (char *) &tmp, sizeof (tmp)))
    return -1;

  signal (SIGPIPE, SIG_IGN);	/* If we don't do this, then GDB simply exits
				   when the remote side dies.  */

  return 0;
}

static void
tcp_close (serial_t scb)
{
  if (scb->fd < 0)
    return;

  close (scb->fd);
  scb->fd = -1;
}

void
_initialize_ser_tcp (void)
{
  struct serial_ops *ops = XMALLOC (struct serial_ops);
  memset (ops, sizeof (struct serial_ops), 0);
  ops->name = "tcp";
  ops->next = 0;
  ops->open = tcp_open;
  ops->close = tcp_close;
  ops->readchar = ser_unix_readchar;
  ops->write = ser_unix_write;
  ops->flush_output = ser_unix_nop_flush_output;
  ops->flush_input = ser_unix_flush_input;
  ops->send_break = ser_unix_nop_send_break;
  ops->go_raw = ser_unix_nop_raw;
  ops->get_tty_state = ser_unix_nop_get_tty_state;
  ops->set_tty_state = ser_unix_nop_set_tty_state;
  ops->print_tty_state = ser_unix_nop_print_tty_state;
  ops->noflush_set_tty_state = ser_unix_nop_noflush_set_tty_state;
  ops->setbaudrate = ser_unix_nop_setbaudrate;
  ops->setstopbits = ser_unix_nop_setstopbits;
  ops->drain_output = ser_unix_nop_drain_output;
  ops->async = ser_unix_async;
  serial_add_interface (ops);
}
