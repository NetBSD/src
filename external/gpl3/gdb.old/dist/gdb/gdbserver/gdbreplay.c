/* Replay a remote debug session logfile for GDB.
   Copyright (C) 1996-2019 Free Software Foundation, Inc.
   Written by Fred Fish (fnf@cygnus.com) from pieces of gdbserver.

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

#include "common/common-defs.h"
#include "common/version.h"

#if HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#include <ctype.h>
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <unistd.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#if USE_WIN32API
#if _WIN32_WINNT < 0x0501
# undef _WIN32_WINNT
# define _WIN32_WINNT 0x0501
#endif
#include <ws2tcpip.h>
#endif

#include "common/netstuff.h"

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif

/* Sort of a hack... */
#define EOL (EOF - 1)

static int remote_desc;

#ifdef __MINGW32CE__

#ifndef COUNTOF
#define COUNTOF(STR) (sizeof (STR) / sizeof ((STR)[0]))
#endif

#define errno (GetLastError ())

char *
strerror (DWORD error)
{
  static char buf[1024];
  WCHAR *msgbuf;
  DWORD lasterr = GetLastError ();
  DWORD chars = FormatMessageW (FORMAT_MESSAGE_FROM_SYSTEM
				| FORMAT_MESSAGE_ALLOCATE_BUFFER,
				NULL,
				error,
				0, /* Default language */
				(LPVOID)&msgbuf,
				0,
				NULL);
  if (chars != 0)
    {
      /* If there is an \r\n appended, zap it.  */
      if (chars >= 2
	  && msgbuf[chars - 2] == '\r'
	  && msgbuf[chars - 1] == '\n')
	{
	  chars -= 2;
	  msgbuf[chars] = 0;
	}

      if (chars > ((COUNTOF (buf)) - 1))
	{
	  chars = COUNTOF (buf) - 1;
	  msgbuf [chars] = 0;
	}

      wcstombs (buf, msgbuf, chars + 1);
      LocalFree (msgbuf);
    }
  else
    sprintf (buf, "unknown win32 error (%ld)", error);

  SetLastError (lasterr);
  return buf;
}

#endif /* __MINGW32CE__ */

static void
sync_error (FILE *fp, const char *desc, int expect, int got)
{
  fprintf (stderr, "\n%s\n", desc);
  fprintf (stderr, "At logfile offset %ld, expected '0x%x' got '0x%x'\n",
	   ftell (fp), expect, got);
  fflush (stderr);
  exit (1);
}

static void
remote_error (const char *desc)
{
  fprintf (stderr, "\n%s\n", desc);
  fflush (stderr);
  exit (1);
}

static void
remote_close (void)
{
#ifdef USE_WIN32API
  closesocket (remote_desc);
#else
  close (remote_desc);
#endif
}

/* Open a connection to a remote debugger.
   NAME is the filename used for communication.  */

static void
remote_open (char *name)
{
  char *last_colon = strrchr (name, ':');

  if (last_colon == NULL)
    {
      fprintf (stderr, "%s: Must specify tcp connection as host:addr\n", name);
      fflush (stderr);
      exit (1);
    }

#ifdef USE_WIN32API
  static int winsock_initialized;
#endif
  int tmp;
  int tmp_desc;
  struct addrinfo hint;
  struct addrinfo *ainfo;

  memset (&hint, 0, sizeof (hint));
  /* Assume no prefix will be passed, therefore we should use
     AF_UNSPEC.  */
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = IPPROTO_TCP;

  parsed_connection_spec parsed = parse_connection_spec (name, &hint);

  if (parsed.port_str.empty ())
    error (_("Missing port on hostname '%s'"), name);

#ifdef USE_WIN32API
  if (!winsock_initialized)
    {
      WSADATA wsad;

      WSAStartup (MAKEWORD (1, 0), &wsad);
      winsock_initialized = 1;
    }
#endif

  int r = getaddrinfo (parsed.host_str.c_str (), parsed.port_str.c_str (),
		       &hint, &ainfo);

  if (r != 0)
    {
      fprintf (stderr, "%s:%s: cannot resolve name: %s\n",
	       parsed.host_str.c_str (), parsed.port_str.c_str (),
	       gai_strerror (r));
      fflush (stderr);
      exit (1);
    }

  scoped_free_addrinfo free_ainfo (ainfo);

  struct addrinfo *p;

  for (p = ainfo; p != NULL; p = p->ai_next)
    {
      tmp_desc = socket (p->ai_family, p->ai_socktype, p->ai_protocol);

      if (tmp_desc >= 0)
	break;
    }

  if (p == NULL)
    perror_with_name ("Cannot open socket");

  /* Allow rapid reuse of this port. */
  tmp = 1;
  setsockopt (tmp_desc, SOL_SOCKET, SO_REUSEADDR, (char *) &tmp,
	      sizeof (tmp));

  switch (p->ai_family)
    {
    case AF_INET:
      ((struct sockaddr_in *) p->ai_addr)->sin_addr.s_addr = INADDR_ANY;
      break;
    case AF_INET6:
      ((struct sockaddr_in6 *) p->ai_addr)->sin6_addr = in6addr_any;
      break;
    default:
      fprintf (stderr, "Invalid 'ai_family' %d\n", p->ai_family);
      exit (1);
    }

  if (bind (tmp_desc, p->ai_addr, p->ai_addrlen) != 0)
    perror_with_name ("Can't bind address");

  if (p->ai_socktype == SOCK_DGRAM)
    remote_desc = tmp_desc;
  else
    {
      struct sockaddr_storage sockaddr;
      socklen_t sockaddrsize = sizeof (sockaddr);
      char orig_host[GDB_NI_MAX_ADDR], orig_port[GDB_NI_MAX_PORT];

      if (listen (tmp_desc, 1) != 0)
	perror_with_name ("Can't listen on socket");

      remote_desc = accept (tmp_desc, (struct sockaddr *) &sockaddr,
			    &sockaddrsize);

      if (remote_desc == -1)
	perror_with_name ("Accept failed");

      /* Enable TCP keep alive process. */
      tmp = 1;
      setsockopt (tmp_desc, SOL_SOCKET, SO_KEEPALIVE,
		  (char *) &tmp, sizeof (tmp));

      /* Tell TCP not to delay small packets.  This greatly speeds up
	 interactive response. */
      tmp = 1;
      setsockopt (remote_desc, IPPROTO_TCP, TCP_NODELAY,
		  (char *) &tmp, sizeof (tmp));

      if (getnameinfo ((struct sockaddr *) &sockaddr, sockaddrsize,
		       orig_host, sizeof (orig_host),
		       orig_port, sizeof (orig_port),
		       NI_NUMERICHOST | NI_NUMERICSERV) == 0)
	{
	  fprintf (stderr, "Remote debugging from host %s, port %s\n",
		   orig_host, orig_port);
	  fflush (stderr);
	}

#ifndef USE_WIN32API
      close (tmp_desc);		/* No longer need this */

      signal (SIGPIPE, SIG_IGN);	/* If we don't do this, then
					   gdbreplay simply exits when
					   the remote side dies.  */
#else
      closesocket (tmp_desc);	/* No longer need this */
#endif
    }

#if defined(F_SETFL) && defined (FASYNC)
  fcntl (remote_desc, F_SETFL, FASYNC);
#endif

  fprintf (stderr, "Replay logfile using %s\n", name);
  fflush (stderr);
}

static int
fromhex (int ch)
{
  if (ch >= '0' && ch <= '9')
    {
      return (ch - '0');
    }
  if (ch >= 'A' && ch <= 'F')
    {
      return (ch - 'A' + 10);
    }
  if (ch >= 'a' && ch <= 'f')
    {
      return (ch - 'a' + 10);
    }
  fprintf (stderr, "\nInvalid hex digit '%c'\n", ch);
  fflush (stderr);
  exit (1);
}

static int
logchar (FILE *fp)
{
  int ch;
  int ch2;

  ch = fgetc (fp);
  fputc (ch, stdout);
  fflush (stdout);
  switch (ch)
    {
    case '\n':
      ch = EOL;
      break;
    case '\\':
      ch = fgetc (fp);
      fputc (ch, stdout);
      fflush (stdout);
      switch (ch)
	{
	case '\\':
	  break;
	case 'b':
	  ch = '\b';
	  break;
	case 'f':
	  ch = '\f';
	  break;
	case 'n':
	  ch = '\n';
	  break;
	case 'r':
	  ch = '\r';
	  break;
	case 't':
	  ch = '\t';
	  break;
	case 'v':
	  ch = '\v';
	  break;
	case 'x':
	  ch2 = fgetc (fp);
	  fputc (ch2, stdout);
	  fflush (stdout);
	  ch = fromhex (ch2) << 4;
	  ch2 = fgetc (fp);
	  fputc (ch2, stdout);
	  fflush (stdout);
	  ch |= fromhex (ch2);
	  break;
	default:
	  /* Treat any other char as just itself */
	  break;
	}
    default:
      break;
    }
  return (ch);
}

static int
gdbchar (int desc)
{
  unsigned char fromgdb;

  if (read (desc, &fromgdb, 1) != 1)
    return -1;
  else
    return fromgdb;
}

/* Accept input from gdb and match with chars from fp (after skipping one
   blank) up until a \n is read from fp (which is not matched) */

static void
expect (FILE *fp)
{
  int fromlog;
  int fromgdb;

  if ((fromlog = logchar (fp)) != ' ')
    {
      sync_error (fp, "Sync error during gdb read of leading blank", ' ',
		  fromlog);
    }
  do
    {
      fromlog = logchar (fp);
      if (fromlog == EOL)
	break;
      fromgdb = gdbchar (remote_desc);
      if (fromgdb < 0)
	remote_error ("Error during read from gdb");
    }
  while (fromlog == fromgdb);

  if (fromlog != EOL)
    {
      sync_error (fp, "Sync error during read of gdb packet from log", fromlog,
		  fromgdb);
    }
}

/* Play data back to gdb from fp (after skipping leading blank) up until a
   \n is read from fp (which is discarded and not sent to gdb). */

static void
play (FILE *fp)
{
  int fromlog;
  char ch;

  if ((fromlog = logchar (fp)) != ' ')
    {
      sync_error (fp, "Sync error skipping blank during write to gdb", ' ',
		  fromlog);
    }
  while ((fromlog = logchar (fp)) != EOL)
    {
      ch = fromlog;
      if (write (remote_desc, &ch, 1) != 1)
	remote_error ("Error during write to gdb");
    }
}

static void
gdbreplay_version (void)
{
  printf ("GNU gdbreplay %s%s\n"
	  "Copyright (C) 2019 Free Software Foundation, Inc.\n"
	  "gdbreplay is free software, covered by "
	  "the GNU General Public License.\n"
	  "This gdbreplay was configured as \"%s\"\n",
	  PKGVERSION, version, host_name);
}

static void
gdbreplay_usage (FILE *stream)
{
  fprintf (stream, "Usage:\tgdbreplay LOGFILE HOST:PORT\n");
  if (REPORT_BUGS_TO[0] && stream == stdout)
    fprintf (stream, "Report bugs to \"%s\".\n", REPORT_BUGS_TO);
}

/* Main function.  This is called by the real "main" function,
   wrapped in a TRY_CATCH that handles any uncaught exceptions.  */

static void ATTRIBUTE_NORETURN
captured_main (int argc, char *argv[])
{
  FILE *fp;
  int ch;

  if (argc >= 2 && strcmp (argv[1], "--version") == 0)
    {
      gdbreplay_version ();
      exit (0);
    }
  if (argc >= 2 && strcmp (argv[1], "--help") == 0)
    {
      gdbreplay_usage (stdout);
      exit (0);
    }

  if (argc < 3)
    {
      gdbreplay_usage (stderr);
      exit (1);
    }
  fp = fopen (argv[1], "r");
  if (fp == NULL)
    {
      perror_with_name (argv[1]);
    }
  remote_open (argv[2]);
  while ((ch = logchar (fp)) != EOF)
    {
      switch (ch)
	{
	case 'w':
	  /* data sent from gdb to gdbreplay, accept and match it */
	  expect (fp);
	  break;
	case 'r':
	  /* data sent from gdbreplay to gdb, play it */
	  play (fp);
	  break;
	case 'c':
	  /* Command executed by gdb */
	  while ((ch = logchar (fp)) != EOL);
	  break;
	}
    }
  remote_close ();
  exit (0);
}

int
main (int argc, char *argv[])
{
  TRY
    {
      captured_main (argc, argv);
    }
  CATCH (exception, RETURN_MASK_ALL)
    {
      if (exception.reason == RETURN_ERROR)
	{
	  fflush (stdout);
	  fprintf (stderr, "%s\n", exception.message);
	}

      exit (1);
    }
  END_CATCH

  gdb_assert_not_reached ("captured_main should never return");
}
