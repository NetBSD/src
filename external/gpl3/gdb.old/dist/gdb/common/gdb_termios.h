/* Common terminal interface definitions for GDB and gdbserver.
   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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

#ifndef GDB_TERMIOS_H
#define GDB_TERMIOS_H

/* If we're using autoconf, it will define HAVE_TERMIOS_H,
   HAVE_TERMIO_H and HAVE_SGTTY_H for us.  One day we can rewrite
   ser-unix.c and inflow.c to inspect those names instead of
   HAVE_TERMIOS, HAVE_TERMIO and the implicit HAVE_SGTTY (when neither
   HAVE_TERMIOS or HAVE_TERMIO is set).  Until then, make sure that
   nothing has already defined the one of the names, and do the right
   thing.  */

#if !defined (HAVE_TERMIOS) && !defined(HAVE_TERMIO) && !defined(HAVE_SGTTY)
#if defined(HAVE_TERMIOS_H)
#define HAVE_TERMIOS
#else /* ! defined (HAVE_TERMIOS_H) */
#if defined(HAVE_TERMIO_H)
#define HAVE_TERMIO
#else /* ! defined (HAVE_TERMIO_H) */
#if defined(HAVE_SGTTY_H)
#define HAVE_SGTTY
#endif /* ! defined (HAVE_SGTTY_H) */
#endif /* ! defined (HAVE_TERMIO_H) */
#endif /* ! defined (HAVE_TERMIOS_H) */
#endif /* !defined (HAVE_TERMIOS) && !defined (HAVE_TERMIO) &&
	  !defined (HAVE_SGTTY) */

#if defined(HAVE_TERMIOS)
#include <termios.h>
#endif

#if !defined(_WIN32) && !defined (HAVE_TERMIOS)

/* Define a common set of macros -- BSD based -- and redefine whatever
   the system offers to make it look like that.  FIXME: serial.h and
   ser-*.c deal with this in a much cleaner fashion; as soon as stuff
   is converted to use them, can get rid of this crap.  */

#ifdef HAVE_TERMIO

#include <termio.h>

#undef TIOCGETP
#define TIOCGETP TCGETA
#undef TIOCSETN
#define TIOCSETN TCSETA
#undef TIOCSETP
#define TIOCSETP TCSETAF
#define TERMINAL struct termio

#else /* sgtty */

#include <fcntl.h>
#include <sgtty.h>
#include <sys/ioctl.h>
#define TERMINAL struct sgttyb

#endif /* sgtty */
#endif

#endif /* ! GDB_TERMIOS_H */
