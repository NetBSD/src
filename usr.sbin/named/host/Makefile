#	@(#)Makefile            e07@nikhef.nl (Eric Wassenaar) 961012

# ----------------------------------------------------------------------
# Adapt the installation directories to your local standards.
# ----------------------------------------------------------------------

# This is where the host executable will go.
DESTBIN = /usr/local/bin

# This is where the host manual page will go.
DESTMAN = /usr/local/man

BINDIR = $(DESTBIN)
MANDIR = $(DESTMAN)/man1

# ----------------------------------------------------------------------
# Special compilation options may be needed only on a few platforms.
# See also the header file port.h for portability issues.
# ----------------------------------------------------------------------

#if defined(_AIX)
SYSDEFS = -D_BSD -D_BSD_INCLUDES -U__STR__ -DBIT_ZERO_ON_LEFT
#endif
 
#if defined(SCO) && You have either OpenDeskTop 3 or OpenServer 5
SYSDEFS = -DSYSV
#endif
 
#if defined(ultrix) && You are using the default ultrix <resolv.h>
SYSDEFS = -DULTRIX_RESOLV
#endif
 
#if defined(solaris) && You do not want to use BSD compatibility mode
SYSDEFS = -DSYSV
#endif
 
#if defined(solaris) && You are using its default broken resolver library
SYSDEFS = -DNO_YP_LOOKUP
#endif

SYSDEFS =

# ----------------------------------------------------------------------
# Configuration definitions.
# See also the header file conf.h for more configuration definitions.
# ----------------------------------------------------------------------

#if defined(BIND_48) && You want to use the default bind res_send()
CONFIGDEFS = -DBIND_RES_SEND
#endif

#if defined(BIND_49) && You want to use the special host res_send()
CONFIGDEFS = -DHOST_RES_SEND
#endif

# This is the default in either case if you compile stand-alone.
CONFIGDEFS = -DHOST_RES_SEND

# ----------------------------------------------------------------------
# Include file directories.
# This program must be compiled with the same include files that
# were used to build the resolver library you are linking with.
# ----------------------------------------------------------------------

INCL = ../../include
INCL = .

COMPINCL = ../../compat/include
COMPINCL = .

INCLUDES = -I$(INCL) -I$(COMPINCL)

# ----------------------------------------------------------------------
# Compilation definitions.
# ----------------------------------------------------------------------

DEFS = $(CONFIGDEFS) $(SYSDEFS) $(INCLUDES)

COPTS =
COPTS = -O

CFLAGS = $(COPTS) $(DEFS)

# Select your favorite compiler.
CC = /usr/ucb/cc			#if defined(solaris) && BSD
CC = /bin/cc -arch m68k -arch i386	#if defined(next)
CC = /bin/cc -Olimit 1000		#if defined(ultrix)
CC = /bin/cc
CC = cc

# ----------------------------------------------------------------------
# Linking definitions.
# libresolv.a should contain the resolver library of BIND 4.8.2 or later.
# Link it in only if your default library is different.
# SCO keeps its own default resolver library inside libsocket.a
#
# lib44bsd.a contains various utility routines, and comes with BIND 4.9.*
# You may need it if you link with the 4.9.* resolver library.
#
# libnet.a contains the getnet...() getserv...() getproto...() calls.
# It is safe to leave it out and use your default library.
# With BIND 4.9.3 the getnet...() calls are in the resolver library.
# ----------------------------------------------------------------------

RES = -lsocket				#if defined(SCO) && default
RES =
RES = ../../res/libresolv.a
RES = -lresolv

COMPLIB = ../../compat/lib/lib44bsd.a
COMPLIB = -lnet
COMPLIB =

LIBS = -lsocket -lnsl			#if defined(solaris) && not BSD
LIBS =

LIBRARIES = $(RES) $(COMPLIB) $(LIBS)

LDFLAGS =

# ----------------------------------------------------------------------
# Compatibility for compilation via the BIND master Makefile.
# ----------------------------------------------------------------------

# redefined by bind
CDEBUG = $(COPTS) $(CONFIGDEFS)
CDEFS = $(SYSDEFS) $(INCLUDES)
CFLAGS = $(CDEBUG) $(CDEFS)

# ----------------------------------------------------------------------
# Miscellaneous definitions.
# ----------------------------------------------------------------------

MAKE = make $(MFLAGS)

# This assumes the BSD install.
INSTALL = install -c

# Grrr
SHELL = /bin/sh

# ----------------------------------------------------------------------
# Files.
# ----------------------------------------------------------------------

PROG = host
HDRS = port.h conf.h exit.h type.h rrec.h defs.h
SRCS = host.c send.c vers.c
OBJS = host.o send.o vers.o
MANS = host.1
DOCS = RELEASE_NOTES

UTILS = nslookup mxlookup
MISCS = malloc.c

FILES = Makefile $(DOCS) $(HDRS) $(SRCS) $(MANS) $(UTILS) $(MISCS)

PACKAGE = host
TARFILE = $(PACKAGE).tar

CLEANUP = $(PROG) $(OBJS) $(TARFILE) $(TARFILE).Z

# ----------------------------------------------------------------------
# Rules for installation.
# ----------------------------------------------------------------------

all: $(PROG)

$(OBJS): $(SRCS) $(HDRS)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBRARIES)

install: $(PROG)
	$(INSTALL) -m 755 -s $(PROG) $(BINDIR)

man: $(MANS)
	$(INSTALL) -m 444 host.1 $(MANDIR)

clean:
	rm -f $(CLEANUP) *.o a.out core

# ----------------------------------------------------------------------
# host may be called with alternative names, querytype names and "zone".
# A few frequently used abbreviations are handy.
# ----------------------------------------------------------------------

ABBREVIATIONS = a ns cname soa wks ptr hinfo mx txt	# standard
ABBREVIATIONS = mb mg mr minfo				# deprecated
ABBREVIATIONS = md mf null				# obsolete
ABBREVIATIONS = rp afsdb x25 isdn rt nsap nsap-ptr	# new
ABBREVIATIONS = sig key px gpos aaaa loc nxt srv	# very new
ABBREVIATIONS = eid nimloc atma naptr			# draft
ABBREVIATIONS = uinfo uid gid unspec			# nonstandard
ABBREVIATIONS = maila mailb any				# filters

ABBREVIATIONS = mx ns soa zone

links:
	for i in $(ABBREVIATIONS) ; do \
		(cd $(BINDIR) ; ln -s $(PROG) $$i) ; \
	done

# ----------------------------------------------------------------------
# Rules for maintenance.
# ----------------------------------------------------------------------

lint:
	lint $(DEFS) $(SRCS)

alint:
	alint $(DEFS) $(SRCS)

llint:
	lint $(DEFS) $(SRCS) -lresolv

print:
	lpr -J $(PACKAGE) -p Makefile $(DOCS) $(HDRS) $(SRCS)

dist:
	tar cf $(TARFILE) $(FILES)
	compress $(TARFILE)

depend:
	mkdep $(DEFS) $(SRCS)

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
