/* Remote utility routines for the remote server for GDB.
   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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

#include "server.h"
#include "terminal.h"
#include "target.h"
#include "gdbthread.h"
#include "tdesc.h"
#include "dll.h"

#include <stdio.h>
#include <string.h>
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/time.h>
#include <unistd.h>
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include <sys/stat.h>
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if USE_WIN32API
#include <winsock2.h>
#endif

#if __QNX__
#include <sys/iomgr.h>
#endif /* __QNX__ */

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif

#ifndef IN_PROCESS_AGENT

#if USE_WIN32API
# define INVALID_DESCRIPTOR INVALID_SOCKET
#else
# define INVALID_DESCRIPTOR -1
#endif

/* Extra value for readchar_callback.  */
enum {
  /* The callback is currently not scheduled.  */
  NOT_SCHEDULED = -1
};

/* Status of the readchar callback.
   Either NOT_SCHEDULED or the callback id.  */
static int readchar_callback = NOT_SCHEDULED;

static int readchar (void);
static void reset_readchar (void);
static void reschedule (void);

/* A cache entry for a successfully looked-up symbol.  */
struct sym_cache
{
  char *name;
  CORE_ADDR addr;
  struct sym_cache *next;
};

int remote_debug = 0;
struct ui_file *gdb_stdlog;

static int remote_is_stdio = 0;

static gdb_fildes_t remote_desc = INVALID_DESCRIPTOR;
static gdb_fildes_t listen_desc = INVALID_DESCRIPTOR;

/* FIXME headerize? */
extern int using_threads;
extern int debug_threads;

/* If true, then GDB has requested noack mode.  */
int noack_mode = 0;
/* If true, then we tell GDB to use noack mode by default.  */
int transport_is_reliable = 0;

#ifdef USE_WIN32API
# define read(fd, buf, len) recv (fd, (char *) buf, len, 0)
# define write(fd, buf, len) send (fd, (char *) buf, len, 0)
#endif

int
gdb_connected (void)
{
  return remote_desc != INVALID_DESCRIPTOR;
}

/* Return true if the remote connection is over stdio.  */

int
remote_connection_is_stdio (void)
{
  return remote_is_stdio;
}

static void
enable_async_notification (int fd)
{
#if defined(F_SETFL) && defined (FASYNC)
  int save_fcntl_flags;

  save_fcntl_flags = fcntl (fd, F_GETFL, 0);
  fcntl (fd, F_SETFL, save_fcntl_flags | FASYNC);
#if defined (F_SETOWN)
  fcntl (fd, F_SETOWN, getpid ());
#endif
#endif
}

static int
handle_accept_event (int err, gdb_client_data client_data)
{
  struct sockaddr_in sockaddr;
  socklen_t tmp;

  if (debug_threads)
    fprintf (stderr, "handling possible accept event\n");

  tmp = sizeof (sockaddr);
  remote_desc = accept (listen_desc, (struct sockaddr *) &sockaddr, &tmp);
  if (remote_desc == -1)
    perror_with_name ("Accept failed");

  /* Enable TCP keep alive process. */
  tmp = 1;
  setsockopt (remote_desc, SOL_SOCKET, SO_KEEPALIVE,
	      (char *) &tmp, sizeof (tmp));

  /* Tell TCP not to delay small packets.  This greatly speeds up
     interactive response. */
  tmp = 1;
  setsockopt (remote_desc, IPPROTO_TCP, TCP_NODELAY,
	      (char *) &tmp, sizeof (tmp));

#ifndef USE_WIN32API
  signal (SIGPIPE, SIG_IGN);	/* If we don't do this, then gdbserver simply
				   exits when the remote side dies.  */
#endif

  if (run_once)
    {
#ifndef USE_WIN32API
      close (listen_desc);		/* No longer need this */
#else
      closesocket (listen_desc);	/* No longer need this */
#endif
    }

  /* Even if !RUN_ONCE no longer notice new connections.  Still keep the
     descriptor open for add_file_handler to wait for a new connection.  */
  delete_file_handler (listen_desc);

  /* Convert IP address to string.  */
  fprintf (stderr, "Remote debugging from host %s\n",
	   inet_ntoa (sockaddr.sin_addr));

  enable_async_notification (remote_desc);

  /* Register the event loop handler.  */
  add_file_handler (remote_desc, handle_serial_event, NULL);

  /* We have a new GDB connection now.  If we were disconnected
     tracing, there's a window where the target could report a stop
     event to the event loop, and since we have a connection now, we'd
     try to send vStopped notifications to GDB.  But, don't do that
     until GDB as selected all-stop/non-stop, and has queried the
     threads' status ('?').  */
  target_async (0);

  return 0;
}

/* Prepare for a later connection to a remote debugger.
   NAME is the filename used for communication.  */

void
remote_prepare (char *name)
{
  char *port_str;
#ifdef USE_WIN32API
  static int winsock_initialized;
#endif
  int port;
  struct sockaddr_in sockaddr;
  socklen_t tmp;
  char *port_end;

  remote_is_stdio = 0;
  if (strcmp (name, STDIO_CONNECTION_NAME) == 0)
    {
      /* We need to record fact that we're using stdio sooner than the
	 call to remote_open so start_inferior knows the connection is
	 via stdio.  */
      remote_is_stdio = 1;
      transport_is_reliable = 1;
      return;
    }

  port_str = strchr (name, ':');
  if (port_str == NULL)
    {
      transport_is_reliable = 0;
      return;
    }

  port = strtoul (port_str + 1, &port_end, 10);
  if (port_str[1] == '\0' || *port_end != '\0')
    fatal ("Bad port argument: %s", name);

#ifdef USE_WIN32API
  if (!winsock_initialized)
    {
      WSADATA wsad;

      WSAStartup (MAKEWORD (1, 0), &wsad);
      winsock_initialized = 1;
    }
#endif

  listen_desc = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listen_desc == -1)
    perror_with_name ("Can't open socket");

  /* Allow rapid reuse of this port. */
  tmp = 1;
  setsockopt (listen_desc, SOL_SOCKET, SO_REUSEADDR, (char *) &tmp,
	      sizeof (tmp));

  sockaddr.sin_family = PF_INET;
  sockaddr.sin_port = htons (port);
  sockaddr.sin_addr.s_addr = INADDR_ANY;

  if (bind (listen_desc, (struct sockaddr *) &sockaddr, sizeof (sockaddr))
      || listen (listen_desc, 1))
    perror_with_name ("Can't bind address");

  transport_is_reliable = 1;
}

/* Open a connection to a remote debugger.
   NAME is the filename used for communication.  */

void
remote_open (char *name)
{
  char *port_str;

  port_str = strchr (name, ':');
#ifdef USE_WIN32API
  if (port_str == NULL)
    error ("Only <host>:<port> is supported on this platform.");
#endif

  if (strcmp (name, STDIO_CONNECTION_NAME) == 0)
    {
      fprintf (stderr, "Remote debugging using stdio\n");

      /* Use stdin as the handle of the connection.
	 We only select on reads, for example.  */
      remote_desc = fileno (stdin);

      enable_async_notification (remote_desc);

      /* Register the event loop handler.  */
      add_file_handler (remote_desc, handle_serial_event, NULL);
    }
#ifndef USE_WIN32API
  else if (port_str == NULL)
    {
      struct stat statbuf;

      if (stat (name, &statbuf) == 0
	  && (S_ISCHR (statbuf.st_mode) || S_ISFIFO (statbuf.st_mode)))
	remote_desc = open (name, O_RDWR);
      else
	{
	  errno = EINVAL;
	  remote_desc = -1;
	}

      if (remote_desc < 0)
	perror_with_name ("Could not open remote device");

#ifdef HAVE_TERMIOS
      {
	struct termios termios;
	tcgetattr (remote_desc, &termios);

	termios.c_iflag = 0;
	termios.c_oflag = 0;
	termios.c_lflag = 0;
	termios.c_cflag &= ~(CSIZE | PARENB);
	termios.c_cflag |= CLOCAL | CS8;
	termios.c_cc[VMIN] = 1;
	termios.c_cc[VTIME] = 0;

	tcsetattr (remote_desc, TCSANOW, &termios);
      }
#endif

#ifdef HAVE_TERMIO
      {
	struct termio termio;
	ioctl (remote_desc, TCGETA, &termio);

	termio.c_iflag = 0;
	termio.c_oflag = 0;
	termio.c_lflag = 0;
	termio.c_cflag &= ~(CSIZE | PARENB);
	termio.c_cflag |= CLOCAL | CS8;
	termio.c_cc[VMIN] = 1;
	termio.c_cc[VTIME] = 0;

	ioctl (remote_desc, TCSETA, &termio);
      }
#endif

#ifdef HAVE_SGTTY
      {
	struct sgttyb sg;

	ioctl (remote_desc, TIOCGETP, &sg);
	sg.sg_flags = RAW;
	ioctl (remote_desc, TIOCSETP, &sg);
      }
#endif

      fprintf (stderr, "Remote debugging using %s\n", name);

      enable_async_notification (remote_desc);

      /* Register the event loop handler.  */
      add_file_handler (remote_desc, handle_serial_event, NULL);
    }
#endif /* USE_WIN32API */
  else
    {
      int port;
      socklen_t len;
      struct sockaddr_in sockaddr;

      len = sizeof (sockaddr);
      if (getsockname (listen_desc,
		       (struct sockaddr *) &sockaddr, &len) < 0
	  || len < sizeof (sockaddr))
	perror_with_name ("Can't determine port");
      port = ntohs (sockaddr.sin_port);

      fprintf (stderr, "Listening on port %d\n", port);
      fflush (stderr);

      /* Register the event loop handler.  */
      add_file_handler (listen_desc, handle_accept_event, NULL);
    }
}

void
remote_close (void)
{
  delete_file_handler (remote_desc);

#ifdef USE_WIN32API
  closesocket (remote_desc);
#else
  if (! remote_connection_is_stdio ())
    close (remote_desc);
#endif
  remote_desc = INVALID_DESCRIPTOR;

  reset_readchar ();
}

/* Convert hex digit A to a number.  */

static int
fromhex (int a)
{
  if (a >= '0' && a <= '9')
    return a - '0';
  else if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  else
    error ("Reply contains invalid hex digit");
  return 0;
}

#endif

static const char hexchars[] = "0123456789abcdef";

static int
ishex (int ch, int *val)
{
  if ((ch >= 'a') && (ch <= 'f'))
    {
      *val = ch - 'a' + 10;
      return 1;
    }
  if ((ch >= 'A') && (ch <= 'F'))
    {
      *val = ch - 'A' + 10;
      return 1;
    }
  if ((ch >= '0') && (ch <= '9'))
    {
      *val = ch - '0';
      return 1;
    }
  return 0;
}

#ifndef IN_PROCESS_AGENT

int
unhexify (char *bin, const char *hex, int count)
{
  int i;

  for (i = 0; i < count; i++)
    {
      if (hex[0] == 0 || hex[1] == 0)
	{
	  /* Hex string is short, or of uneven length.
	     Return the count that has been converted so far. */
	  return i;
	}
      *bin++ = fromhex (hex[0]) * 16 + fromhex (hex[1]);
      hex += 2;
    }
  return i;
}

void
decode_address (CORE_ADDR *addrp, const char *start, int len)
{
  CORE_ADDR addr;
  char ch;
  int i;

  addr = 0;
  for (i = 0; i < len; i++)
    {
      ch = start[i];
      addr = addr << 4;
      addr = addr | (fromhex (ch) & 0x0f);
    }
  *addrp = addr;
}

const char *
decode_address_to_semicolon (CORE_ADDR *addrp, const char *start)
{
  const char *end;

  end = start;
  while (*end != '\0' && *end != ';')
    end++;

  decode_address (addrp, start, end - start);

  if (*end == ';')
    end++;
  return end;
}

#endif

/* Convert number NIB to a hex digit.  */

static int
tohex (int nib)
{
  if (nib < 10)
    return '0' + nib;
  else
    return 'a' + nib - 10;
}

#ifndef IN_PROCESS_AGENT

int
hexify (char *hex, const char *bin, int count)
{
  int i;

  /* May use a length, or a nul-terminated string as input. */
  if (count == 0)
    count = strlen (bin);

  for (i = 0; i < count; i++)
    {
      *hex++ = tohex ((*bin >> 4) & 0xf);
      *hex++ = tohex (*bin++ & 0xf);
    }
  *hex = 0;
  return i;
}

/* Convert BUFFER, binary data at least LEN bytes long, into escaped
   binary data in OUT_BUF.  Set *OUT_LEN to the length of the data
   encoded in OUT_BUF, and return the number of bytes in OUT_BUF
   (which may be more than *OUT_LEN due to escape characters).  The
   total number of bytes in the output buffer will be at most
   OUT_MAXLEN.  */

int
remote_escape_output (const gdb_byte *buffer, int len,
		      gdb_byte *out_buf, int *out_len,
		      int out_maxlen)
{
  int input_index, output_index;

  output_index = 0;
  for (input_index = 0; input_index < len; input_index++)
    {
      gdb_byte b = buffer[input_index];

      if (b == '$' || b == '#' || b == '}' || b == '*')
	{
	  /* These must be escaped.  */
	  if (output_index + 2 > out_maxlen)
	    break;
	  out_buf[output_index++] = '}';
	  out_buf[output_index++] = b ^ 0x20;
	}
      else
	{
	  if (output_index + 1 > out_maxlen)
	    break;
	  out_buf[output_index++] = b;
	}
    }

  *out_len = input_index;
  return output_index;
}

/* Convert BUFFER, escaped data LEN bytes long, into binary data
   in OUT_BUF.  Return the number of bytes written to OUT_BUF.
   Raise an error if the total number of bytes exceeds OUT_MAXLEN.

   This function reverses remote_escape_output.  It allows more
   escaped characters than that function does, in particular because
   '*' must be escaped to avoid the run-length encoding processing
   in reading packets.  */

static int
remote_unescape_input (const gdb_byte *buffer, int len,
		       gdb_byte *out_buf, int out_maxlen)
{
  int input_index, output_index;
  int escaped;

  output_index = 0;
  escaped = 0;
  for (input_index = 0; input_index < len; input_index++)
    {
      gdb_byte b = buffer[input_index];

      if (output_index + 1 > out_maxlen)
	error ("Received too much data from the target.");

      if (escaped)
	{
	  out_buf[output_index++] = b ^ 0x20;
	  escaped = 0;
	}
      else if (b == '}')
	escaped = 1;
      else
	out_buf[output_index++] = b;
    }

  if (escaped)
    error ("Unmatched escape character in target response.");

  return output_index;
}

/* Look for a sequence of characters which can be run-length encoded.
   If there are any, update *CSUM and *P.  Otherwise, output the
   single character.  Return the number of characters consumed.  */

static int
try_rle (char *buf, int remaining, unsigned char *csum, char **p)
{
  int n;

  /* Always output the character.  */
  *csum += buf[0];
  *(*p)++ = buf[0];

  /* Don't go past '~'.  */
  if (remaining > 97)
    remaining = 97;

  for (n = 1; n < remaining; n++)
    if (buf[n] != buf[0])
      break;

  /* N is the index of the first character not the same as buf[0].
     buf[0] is counted twice, so by decrementing N, we get the number
     of characters the RLE sequence will replace.  */
  n--;

  if (n < 3)
    return 1;

  /* Skip the frame characters.  The manual says to skip '+' and '-'
     also, but there's no reason to.  Unfortunately these two unusable
     characters double the encoded length of a four byte zero
     value.  */
  while (n + 29 == '$' || n + 29 == '#')
    n--;

  *csum += '*';
  *(*p)++ = '*';
  *csum += n + 29;
  *(*p)++ = n + 29;

  return n + 1;
}

#endif

char *
unpack_varlen_hex (char *buff,	/* packet to parse */
		   ULONGEST *result)
{
  int nibble;
  ULONGEST retval = 0;

  while (ishex (*buff, &nibble))
    {
      buff++;
      retval = retval << 4;
      retval |= nibble & 0x0f;
    }
  *result = retval;
  return buff;
}

#ifndef IN_PROCESS_AGENT

/* Write a PTID to BUF.  Returns BUF+CHARACTERS_WRITTEN.  */

char *
write_ptid (char *buf, ptid_t ptid)
{
  int pid, tid;

  if (multi_process)
    {
      pid = ptid_get_pid (ptid);
      if (pid < 0)
	buf += sprintf (buf, "p-%x.", -pid);
      else
	buf += sprintf (buf, "p%x.", pid);
    }
  tid = ptid_get_lwp (ptid);
  if (tid < 0)
    buf += sprintf (buf, "-%x", -tid);
  else
    buf += sprintf (buf, "%x", tid);

  return buf;
}

ULONGEST
hex_or_minus_one (char *buf, char **obuf)
{
  ULONGEST ret;

  if (strncmp (buf, "-1", 2) == 0)
    {
      ret = (ULONGEST) -1;
      buf += 2;
    }
  else
    buf = unpack_varlen_hex (buf, &ret);

  if (obuf)
    *obuf = buf;

  return ret;
}

/* Extract a PTID from BUF.  If non-null, OBUF is set to the to one
   passed the last parsed char.  Returns null_ptid on error.  */
ptid_t
read_ptid (char *buf, char **obuf)
{
  char *p = buf;
  char *pp;
  ULONGEST pid = 0, tid = 0;

  if (*p == 'p')
    {
      /* Multi-process ptid.  */
      pp = unpack_varlen_hex (p + 1, &pid);
      if (*pp != '.')
	error ("invalid remote ptid: %s\n", p);

      p = pp + 1;

      tid = hex_or_minus_one (p, &pp);

      if (obuf)
	*obuf = pp;
      return ptid_build (pid, tid, 0);
    }

  /* No multi-process.  Just a tid.  */
  tid = hex_or_minus_one (p, &pp);

  /* Since the stub is not sending a process id, then default to
     what's in the current inferior.  */
  pid = ptid_get_pid (current_ptid);

  if (obuf)
    *obuf = pp;
  return ptid_build (pid, tid, 0);
}

/* Write COUNT bytes in BUF to the client.
   The result is the number of bytes written or -1 if error.
   This may return less than COUNT.  */

static int
write_prim (const void *buf, int count)
{
  if (remote_connection_is_stdio ())
    return write (fileno (stdout), buf, count);
  else
    return write (remote_desc, buf, count);
}

/* Read COUNT bytes from the client and store in BUF.
   The result is the number of bytes read or -1 if error.
   This may return less than COUNT.  */

static int
read_prim (void *buf, int count)
{
  if (remote_connection_is_stdio ())
    return read (fileno (stdin), buf, count);
  else
    return read (remote_desc, buf, count);
}

/* Send a packet to the remote machine, with error checking.
   The data of the packet is in BUF, and the length of the
   packet is in CNT.  Returns >= 0 on success, -1 otherwise.  */

static int
putpkt_binary_1 (char *buf, int cnt, int is_notif)
{
  int i;
  unsigned char csum = 0;
  char *buf2;
  char *p;
  int cc;

  buf2 = xmalloc (strlen ("$") + cnt + strlen ("#nn") + 1);

  /* Copy the packet into buffer BUF2, encapsulating it
     and giving it a checksum.  */

  p = buf2;
  if (is_notif)
    *p++ = '%';
  else
    *p++ = '$';

  for (i = 0; i < cnt;)
    i += try_rle (buf + i, cnt - i, &csum, &p);

  *p++ = '#';
  *p++ = tohex ((csum >> 4) & 0xf);
  *p++ = tohex (csum & 0xf);

  *p = '\0';

  /* Send it over and over until we get a positive ack.  */

  do
    {
      if (write_prim (buf2, p - buf2) != p - buf2)
	{
	  perror ("putpkt(write)");
	  free (buf2);
	  return -1;
	}

      if (noack_mode || is_notif)
	{
	  /* Don't expect an ack then.  */
	  if (remote_debug)
	    {
	      if (is_notif)
		fprintf (stderr, "putpkt (\"%s\"); [notif]\n", buf2);
	      else
		fprintf (stderr, "putpkt (\"%s\"); [noack mode]\n", buf2);
	      fflush (stderr);
	    }
	  break;
	}

      if (remote_debug)
	{
	  fprintf (stderr, "putpkt (\"%s\"); [looking for ack]\n", buf2);
	  fflush (stderr);
	}

      cc = readchar ();

      if (cc < 0)
	{
	  free (buf2);
	  return -1;
	}

      if (remote_debug)
	{
	  fprintf (stderr, "[received '%c' (0x%x)]\n", cc, cc);
	  fflush (stderr);
	}

      /* Check for an input interrupt while we're here.  */
      if (cc == '\003' && current_inferior != NULL)
	(*the_target->request_interrupt) ();
    }
  while (cc != '+');

  free (buf2);
  return 1;			/* Success! */
}

int
putpkt_binary (char *buf, int cnt)
{
  return putpkt_binary_1 (buf, cnt, 0);
}

/* Send a packet to the remote machine, with error checking.  The data
   of the packet is in BUF, and the packet should be a NUL-terminated
   string.  Returns >= 0 on success, -1 otherwise.  */

int
putpkt (char *buf)
{
  return putpkt_binary (buf, strlen (buf));
}

int
putpkt_notif (char *buf)
{
  return putpkt_binary_1 (buf, strlen (buf), 1);
}

/* Come here when we get an input interrupt from the remote side.  This
   interrupt should only be active while we are waiting for the child to do
   something.  Thus this assumes readchar:bufcnt is 0.
   About the only thing that should come through is a ^C, which
   will cause us to request child interruption.  */

static void
input_interrupt (int unused)
{
  fd_set readset;
  struct timeval immediate = { 0, 0 };

  /* Protect against spurious interrupts.  This has been observed to
     be a problem under NetBSD 1.4 and 1.5.  */

  FD_ZERO (&readset);
  FD_SET (remote_desc, &readset);
  if (select (remote_desc + 1, &readset, 0, 0, &immediate) > 0)
    {
      int cc;
      char c = 0;

      cc = read_prim (&c, 1);

      if (cc != 1 || c != '\003' || current_inferior == NULL)
	{
	  fprintf (stderr, "input_interrupt, count = %d c = %d ('%c')\n",
		   cc, c, c);
	  return;
	}

      (*the_target->request_interrupt) ();
    }
}

/* Check if the remote side sent us an interrupt request (^C).  */
void
check_remote_input_interrupt_request (void)
{
  /* This function may be called before establishing communications,
     therefore we need to validate the remote descriptor.  */

  if (remote_desc == INVALID_DESCRIPTOR)
    return;

  input_interrupt (0);
}

/* Asynchronous I/O support.  SIGIO must be enabled when waiting, in order to
   accept Control-C from the client, and must be disabled when talking to
   the client.  */

static void
unblock_async_io (void)
{
#ifndef USE_WIN32API
  sigset_t sigio_set;

  sigemptyset (&sigio_set);
  sigaddset (&sigio_set, SIGIO);
  sigprocmask (SIG_UNBLOCK, &sigio_set, NULL);
#endif
}

#ifdef __QNX__
static void
nto_comctrl (int enable)
{
  struct sigevent event;

  if (enable)
    {
      event.sigev_notify = SIGEV_SIGNAL_THREAD;
      event.sigev_signo = SIGIO;
      event.sigev_code = 0;
      event.sigev_value.sival_ptr = NULL;
      event.sigev_priority = -1;
      ionotify (remote_desc, _NOTIFY_ACTION_POLLARM, _NOTIFY_COND_INPUT,
		&event);
    }
  else
    ionotify (remote_desc, _NOTIFY_ACTION_POLL, _NOTIFY_COND_INPUT, NULL);
}
#endif /* __QNX__ */


/* Current state of asynchronous I/O.  */
static int async_io_enabled;

/* Enable asynchronous I/O.  */
void
enable_async_io (void)
{
  if (async_io_enabled)
    return;

#ifndef USE_WIN32API
  signal (SIGIO, input_interrupt);
#endif
  async_io_enabled = 1;
#ifdef __QNX__
  nto_comctrl (1);
#endif /* __QNX__ */
}

/* Disable asynchronous I/O.  */
void
disable_async_io (void)
{
  if (!async_io_enabled)
    return;

#ifndef USE_WIN32API
  signal (SIGIO, SIG_IGN);
#endif
  async_io_enabled = 0;
#ifdef __QNX__
  nto_comctrl (0);
#endif /* __QNX__ */

}

void
initialize_async_io (void)
{
  /* Make sure that async I/O starts disabled.  */
  async_io_enabled = 1;
  disable_async_io ();

  /* Make sure the signal is unblocked.  */
  unblock_async_io ();
}

/* Internal buffer used by readchar.
   These are global to readchar because reschedule_remote needs to be
   able to tell whether the buffer is empty.  */

static unsigned char readchar_buf[BUFSIZ];
static int readchar_bufcnt = 0;
static unsigned char *readchar_bufp;

/* Returns next char from remote GDB.  -1 if error.  */

static int
readchar (void)
{
  int ch;

  if (readchar_bufcnt == 0)
    {
      readchar_bufcnt = read_prim (readchar_buf, sizeof (readchar_buf));

      if (readchar_bufcnt <= 0)
	{
	  if (readchar_bufcnt == 0)
	    fprintf (stderr, "readchar: Got EOF\n");
	  else
	    perror ("readchar");

	  return -1;
	}

      readchar_bufp = readchar_buf;
    }

  readchar_bufcnt--;
  ch = *readchar_bufp++;
  reschedule ();
  return ch;
}

/* Reset the readchar state machine.  */

static void
reset_readchar (void)
{
  readchar_bufcnt = 0;
  if (readchar_callback != NOT_SCHEDULED)
    {
      delete_callback_event (readchar_callback);
      readchar_callback = NOT_SCHEDULED;
    }
}

/* Process remaining data in readchar_buf.  */

static int
process_remaining (void *context)
{
  int res;

  /* This is a one-shot event.  */
  readchar_callback = NOT_SCHEDULED;

  if (readchar_bufcnt > 0)
    res = handle_serial_event (0, NULL);
  else
    res = 0;

  return res;
}

/* If there is still data in the buffer, queue another event to process it,
   we can't sleep in select yet.  */

static void
reschedule (void)
{
  if (readchar_bufcnt > 0 && readchar_callback == NOT_SCHEDULED)
    readchar_callback = append_callback_event (process_remaining, NULL);
}

/* Read a packet from the remote machine, with error checking,
   and store it in BUF.  Returns length of packet, or negative if error. */

int
getpkt (char *buf)
{
  char *bp;
  unsigned char csum, c1, c2;
  int c;

  while (1)
    {
      csum = 0;

      while (1)
	{
	  c = readchar ();
	  if (c == '$')
	    break;
	  if (remote_debug)
	    {
	      fprintf (stderr, "[getpkt: discarding char '%c']\n", c);
	      fflush (stderr);
	    }

	  if (c < 0)
	    return -1;
	}

      bp = buf;
      while (1)
	{
	  c = readchar ();
	  if (c < 0)
	    return -1;
	  if (c == '#')
	    break;
	  *bp++ = c;
	  csum += c;
	}
      *bp = 0;

      c1 = fromhex (readchar ());
      c2 = fromhex (readchar ());

      if (csum == (c1 << 4) + c2)
	break;

      if (noack_mode)
	{
	  fprintf (stderr,
		   "Bad checksum, sentsum=0x%x, csum=0x%x, "
		   "buf=%s [no-ack-mode, Bad medium?]\n",
		   (c1 << 4) + c2, csum, buf);
	  /* Not much we can do, GDB wasn't expecting an ack/nac.  */
	  break;
	}

      fprintf (stderr, "Bad checksum, sentsum=0x%x, csum=0x%x, buf=%s\n",
	       (c1 << 4) + c2, csum, buf);
      if (write_prim ("-", 1) != 1)
	return -1;
    }

  if (!noack_mode)
    {
      if (remote_debug)
	{
	  fprintf (stderr, "getpkt (\"%s\");  [sending ack] \n", buf);
	  fflush (stderr);
	}

      if (write_prim ("+", 1) != 1)
	return -1;

      if (remote_debug)
	{
	  fprintf (stderr, "[sent ack]\n");
	  fflush (stderr);
	}
    }
  else
    {
      if (remote_debug)
	{
	  fprintf (stderr, "getpkt (\"%s\");  [no ack sent] \n", buf);
	  fflush (stderr);
	}
    }

  return bp - buf;
}

void
write_ok (char *buf)
{
  buf[0] = 'O';
  buf[1] = 'K';
  buf[2] = '\0';
}

void
write_enn (char *buf)
{
  /* Some day, we should define the meanings of the error codes... */
  buf[0] = 'E';
  buf[1] = '0';
  buf[2] = '1';
  buf[3] = '\0';
}

#endif

void
convert_int_to_ascii (const unsigned char *from, char *to, int n)
{
  int nib;
  int ch;
  while (n--)
    {
      ch = *from++;
      nib = ((ch & 0xf0) >> 4) & 0x0f;
      *to++ = tohex (nib);
      nib = ch & 0x0f;
      *to++ = tohex (nib);
    }
  *to++ = 0;
}

#ifndef IN_PROCESS_AGENT

void
convert_ascii_to_int (const char *from, unsigned char *to, int n)
{
  int nib1, nib2;
  while (n--)
    {
      nib1 = fromhex (*from++);
      nib2 = fromhex (*from++);
      *to++ = (((nib1 & 0x0f) << 4) & 0xf0) | (nib2 & 0x0f);
    }
}

static char *
outreg (struct regcache *regcache, int regno, char *buf)
{
  if ((regno >> 12) != 0)
    *buf++ = tohex ((regno >> 12) & 0xf);
  if ((regno >> 8) != 0)
    *buf++ = tohex ((regno >> 8) & 0xf);
  *buf++ = tohex ((regno >> 4) & 0xf);
  *buf++ = tohex (regno & 0xf);
  *buf++ = ':';
  collect_register_as_string (regcache, regno, buf);
  buf += 2 * register_size (regcache->tdesc, regno);
  *buf++ = ';';

  return buf;
}

void
new_thread_notify (int id)
{
  char own_buf[256];

  /* The `n' response is not yet part of the remote protocol.  Do nothing.  */
  if (1)
    return;

  if (server_waiting == 0)
    return;

  sprintf (own_buf, "n%x", id);
  disable_async_io ();
  putpkt (own_buf);
  enable_async_io ();
}

void
dead_thread_notify (int id)
{
  char own_buf[256];

  /* The `x' response is not yet part of the remote protocol.  Do nothing.  */
  if (1)
    return;

  sprintf (own_buf, "x%x", id);
  disable_async_io ();
  putpkt (own_buf);
  enable_async_io ();
}

void
prepare_resume_reply (char *buf, ptid_t ptid,
		      struct target_waitstatus *status)
{
  if (debug_threads)
    fprintf (stderr, "Writing resume reply for %s:%d\n",
	     target_pid_to_str (ptid), status->kind);

  switch (status->kind)
    {
    case TARGET_WAITKIND_STOPPED:
      {
	struct thread_info *saved_inferior;
	const char **regp;
	struct regcache *regcache;

	sprintf (buf, "T%02x", status->value.sig);
	buf += strlen (buf);

	saved_inferior = current_inferior;

	current_inferior = find_thread_ptid (ptid);

	regp = current_target_desc ()->expedite_regs;

	regcache = get_thread_regcache (current_inferior, 1);

	if (the_target->stopped_by_watchpoint != NULL
	    && (*the_target->stopped_by_watchpoint) ())
	  {
	    CORE_ADDR addr;
	    int i;

	    strncpy (buf, "watch:", 6);
	    buf += 6;

	    addr = (*the_target->stopped_data_address) ();

	    /* Convert each byte of the address into two hexadecimal
	       chars.  Note that we take sizeof (void *) instead of
	       sizeof (addr); this is to avoid sending a 64-bit
	       address to a 32-bit GDB.  */
	    for (i = sizeof (void *) * 2; i > 0; i--)
	      *buf++ = tohex ((addr >> (i - 1) * 4) & 0xf);
	    *buf++ = ';';
	  }

	while (*regp)
	  {
	    buf = outreg (regcache, find_regno (regcache->tdesc, *regp), buf);
	    regp ++;
	  }
	*buf = '\0';

	/* Formerly, if the debugger had not used any thread features
	   we would not burden it with a thread status response.  This
	   was for the benefit of GDB 4.13 and older.  However, in
	   recent GDB versions the check (``if (cont_thread != 0)'')
	   does not have the desired effect because of sillyness in
	   the way that the remote protocol handles specifying a
	   thread.  Since thread support relies on qSymbol support
	   anyway, assume GDB can handle threads.  */

	if (using_threads && !disable_packet_Tthread)
	  {
	    /* This if (1) ought to be unnecessary.  But remote_wait
	       in GDB will claim this event belongs to inferior_ptid
	       if we do not specify a thread, and there's no way for
	       gdbserver to know what inferior_ptid is.  */
	    if (1 || !ptid_equal (general_thread, ptid))
	      {
		int core = -1;
		/* In non-stop, don't change the general thread behind
		   GDB's back.  */
		if (!non_stop)
		  general_thread = ptid;
		sprintf (buf, "thread:");
		buf += strlen (buf);
		buf = write_ptid (buf, ptid);
		strcat (buf, ";");
		buf += strlen (buf);

		core = target_core_of_thread (ptid);

		if (core != -1)
		  {
		    sprintf (buf, "core:");
		    buf += strlen (buf);
		    sprintf (buf, "%x", core);
		    strcat (buf, ";");
		    buf += strlen (buf);
		  }
	      }
	  }

	if (dlls_changed)
	  {
	    strcpy (buf, "library:;");
	    buf += strlen (buf);
	    dlls_changed = 0;
	  }

	current_inferior = saved_inferior;
      }
      break;
    case TARGET_WAITKIND_EXITED:
      if (multi_process)
	sprintf (buf, "W%x;process:%x",
		 status->value.integer, ptid_get_pid (ptid));
      else
	sprintf (buf, "W%02x", status->value.integer);
      break;
    case TARGET_WAITKIND_SIGNALLED:
      if (multi_process)
	sprintf (buf, "X%x;process:%x",
		 status->value.sig, ptid_get_pid (ptid));
      else
	sprintf (buf, "X%02x", status->value.sig);
      break;
    default:
      error ("unhandled waitkind");
      break;
    }
}

void
decode_m_packet (char *from, CORE_ADDR *mem_addr_ptr, unsigned int *len_ptr)
{
  int i = 0, j = 0;
  char ch;
  *mem_addr_ptr = *len_ptr = 0;

  while ((ch = from[i++]) != ',')
    {
      *mem_addr_ptr = *mem_addr_ptr << 4;
      *mem_addr_ptr |= fromhex (ch) & 0x0f;
    }

  for (j = 0; j < 4; j++)
    {
      if ((ch = from[i++]) == 0)
	break;
      *len_ptr = *len_ptr << 4;
      *len_ptr |= fromhex (ch) & 0x0f;
    }
}

void
decode_M_packet (char *from, CORE_ADDR *mem_addr_ptr, unsigned int *len_ptr,
		 unsigned char **to_p)
{
  int i = 0;
  char ch;
  *mem_addr_ptr = *len_ptr = 0;

  while ((ch = from[i++]) != ',')
    {
      *mem_addr_ptr = *mem_addr_ptr << 4;
      *mem_addr_ptr |= fromhex (ch) & 0x0f;
    }

  while ((ch = from[i++]) != ':')
    {
      *len_ptr = *len_ptr << 4;
      *len_ptr |= fromhex (ch) & 0x0f;
    }

  if (*to_p == NULL)
    *to_p = xmalloc (*len_ptr);

  convert_ascii_to_int (&from[i++], *to_p, *len_ptr);
}

int
decode_X_packet (char *from, int packet_len, CORE_ADDR *mem_addr_ptr,
		 unsigned int *len_ptr, unsigned char **to_p)
{
  int i = 0;
  char ch;
  *mem_addr_ptr = *len_ptr = 0;

  while ((ch = from[i++]) != ',')
    {
      *mem_addr_ptr = *mem_addr_ptr << 4;
      *mem_addr_ptr |= fromhex (ch) & 0x0f;
    }

  while ((ch = from[i++]) != ':')
    {
      *len_ptr = *len_ptr << 4;
      *len_ptr |= fromhex (ch) & 0x0f;
    }

  if (*to_p == NULL)
    *to_p = xmalloc (*len_ptr);

  if (remote_unescape_input ((const gdb_byte *) &from[i], packet_len - i,
			     *to_p, *len_ptr) != *len_ptr)
    return -1;

  return 0;
}

/* Decode a qXfer write request.  */

int
decode_xfer_write (char *buf, int packet_len, CORE_ADDR *offset,
		   unsigned int *len, unsigned char *data)
{
  char ch;
  char *b = buf;

  /* Extract the offset.  */
  *offset = 0;
  while ((ch = *buf++) != ':')
    {
      *offset = *offset << 4;
      *offset |= fromhex (ch) & 0x0f;
    }

  /* Get encoded data.  */
  packet_len -= buf - b;
  *len = remote_unescape_input ((const gdb_byte *) buf, packet_len,
				data, packet_len);
  return 0;
}

/* Decode the parameters of a qSearch:memory packet.  */

int
decode_search_memory_packet (const char *buf, int packet_len,
			     CORE_ADDR *start_addrp,
			     CORE_ADDR *search_space_lenp,
			     gdb_byte *pattern, unsigned int *pattern_lenp)
{
  const char *p = buf;

  p = decode_address_to_semicolon (start_addrp, p);
  p = decode_address_to_semicolon (search_space_lenp, p);
  packet_len -= p - buf;
  *pattern_lenp = remote_unescape_input ((const gdb_byte *) p, packet_len,
					 pattern, packet_len);
  return 0;
}

static void
free_sym_cache (struct sym_cache *sym)
{
  if (sym != NULL)
    {
      free (sym->name);
      free (sym);
    }
}

void
clear_symbol_cache (struct sym_cache **symcache_p)
{
  struct sym_cache *sym, *next;

  /* Check the cache first.  */
  for (sym = *symcache_p; sym; sym = next)
    {
      next = sym->next;
      free_sym_cache (sym);
    }

  *symcache_p = NULL;
}

/* Get the address of NAME, and return it in ADDRP if found.  if
   MAY_ASK_GDB is false, assume symbol cache misses are failures.
   Returns 1 if the symbol is found, 0 if it is not, -1 on error.  */

int
look_up_one_symbol (const char *name, CORE_ADDR *addrp, int may_ask_gdb)
{
  char own_buf[266], *p, *q;
  int len;
  struct sym_cache *sym;
  struct process_info *proc;

  proc = current_process ();

  /* Check the cache first.  */
  for (sym = proc->symbol_cache; sym; sym = sym->next)
    if (strcmp (name, sym->name) == 0)
      {
	*addrp = sym->addr;
	return 1;
      }

  /* It might not be an appropriate time to look up a symbol,
     e.g. while we're trying to fetch registers.  */
  if (!may_ask_gdb)
    return 0;

  /* Send the request.  */
  strcpy (own_buf, "qSymbol:");
  hexify (own_buf + strlen ("qSymbol:"), name, strlen (name));
  if (putpkt (own_buf) < 0)
    return -1;

  /* FIXME:  Eventually add buffer overflow checking (to getpkt?)  */
  len = getpkt (own_buf);
  if (len < 0)
    return -1;

  /* We ought to handle pretty much any packet at this point while we
     wait for the qSymbol "response".  That requires re-entering the
     main loop.  For now, this is an adequate approximation; allow
     GDB to read from memory while it figures out the address of the
     symbol.  */
  while (own_buf[0] == 'm')
    {
      CORE_ADDR mem_addr;
      unsigned char *mem_buf;
      unsigned int mem_len;

      decode_m_packet (&own_buf[1], &mem_addr, &mem_len);
      mem_buf = xmalloc (mem_len);
      if (read_inferior_memory (mem_addr, mem_buf, mem_len) == 0)
	convert_int_to_ascii (mem_buf, own_buf, mem_len);
      else
	write_enn (own_buf);
      free (mem_buf);
      if (putpkt (own_buf) < 0)
	return -1;
      len = getpkt (own_buf);
      if (len < 0)
	return -1;
    }

  if (strncmp (own_buf, "qSymbol:", strlen ("qSymbol:")) != 0)
    {
      warning ("Malformed response to qSymbol, ignoring: %s\n", own_buf);
      return -1;
    }

  p = own_buf + strlen ("qSymbol:");
  q = p;
  while (*q && *q != ':')
    q++;

  /* Make sure we found a value for the symbol.  */
  if (p == q || *q == '\0')
    return 0;

  decode_address (addrp, p, q - p);

  /* Save the symbol in our cache.  */
  sym = xmalloc (sizeof (*sym));
  sym->name = xstrdup (name);
  sym->addr = *addrp;
  sym->next = proc->symbol_cache;
  proc->symbol_cache = sym;

  return 1;
}

/* Relocate an instruction to execute at a different address.  OLDLOC
   is the address in the inferior memory where the instruction to
   relocate is currently at.  On input, TO points to the destination
   where we want the instruction to be copied (and possibly adjusted)
   to.  On output, it points to one past the end of the resulting
   instruction(s).  The effect of executing the instruction at TO
   shall be the same as if executing it at OLDLOC.  For example, call
   instructions that implicitly push the return address on the stack
   should be adjusted to return to the instruction after OLDLOC;
   relative branches, and other PC-relative instructions need the
   offset adjusted; etc.  Returns 0 on success, -1 on failure.  */

int
relocate_instruction (CORE_ADDR *to, CORE_ADDR oldloc)
{
  char own_buf[266];
  int len;
  ULONGEST written = 0;

  /* Send the request.  */
  strcpy (own_buf, "qRelocInsn:");
  sprintf (own_buf, "qRelocInsn:%s;%s", paddress (oldloc),
	   paddress (*to));
  if (putpkt (own_buf) < 0)
    return -1;

  /* FIXME:  Eventually add buffer overflow checking (to getpkt?)  */
  len = getpkt (own_buf);
  if (len < 0)
    return -1;

  /* We ought to handle pretty much any packet at this point while we
     wait for the qRelocInsn "response".  That requires re-entering
     the main loop.  For now, this is an adequate approximation; allow
     GDB to access memory.  */
  while (own_buf[0] == 'm' || own_buf[0] == 'M' || own_buf[0] == 'X')
    {
      CORE_ADDR mem_addr;
      unsigned char *mem_buf = NULL;
      unsigned int mem_len;

      if (own_buf[0] == 'm')
	{
	  decode_m_packet (&own_buf[1], &mem_addr, &mem_len);
	  mem_buf = xmalloc (mem_len);
	  if (read_inferior_memory (mem_addr, mem_buf, mem_len) == 0)
	    convert_int_to_ascii (mem_buf, own_buf, mem_len);
	  else
	    write_enn (own_buf);
	}
      else if (own_buf[0] == 'X')
	{
	  if (decode_X_packet (&own_buf[1], len - 1, &mem_addr,
			       &mem_len, &mem_buf) < 0
	      || write_inferior_memory (mem_addr, mem_buf, mem_len) != 0)
	    write_enn (own_buf);
	  else
	    write_ok (own_buf);
	}
      else
	{
	  decode_M_packet (&own_buf[1], &mem_addr, &mem_len, &mem_buf);
	  if (write_inferior_memory (mem_addr, mem_buf, mem_len) == 0)
	    write_ok (own_buf);
	  else
	    write_enn (own_buf);
	}
      free (mem_buf);
      if (putpkt (own_buf) < 0)
	return -1;
      len = getpkt (own_buf);
      if (len < 0)
	return -1;
    }

  if (own_buf[0] == 'E')
    {
      warning ("An error occurred while relocating an instruction: %s\n",
	       own_buf);
      return -1;
    }

  if (strncmp (own_buf, "qRelocInsn:", strlen ("qRelocInsn:")) != 0)
    {
      warning ("Malformed response to qRelocInsn, ignoring: %s\n",
	       own_buf);
      return -1;
    }

  unpack_varlen_hex (own_buf + strlen ("qRelocInsn:"), &written);

  *to += written;
  return 0;
}

void
monitor_output (const char *msg)
{
  char *buf = xmalloc (strlen (msg) * 2 + 2);

  buf[0] = 'O';
  hexify (buf + 1, msg, 0);

  putpkt (buf);
  free (buf);
}

#endif
