# Makefile.am, by Harlan Stenn, from:
# Makefile for sntp
# N.M. Maclaren, October 2000.

AUTOMAKE_OPTIONS = foreign

NULL=

EXTRA_DIST=	bincheck.mf

bindir=	${exec_prefix}/${BINSUBDIR}
bin_PROGRAMS=	sntp

sntp_SOURCES = \
	header.h \
	internet.c \
	internet.h \
	kludges.h \
	main.c \
	socket.c \
	timing.c \
	unix.c \
	$(NULL)

dist_man_MANS=	sntp.1

include bincheck.mf
