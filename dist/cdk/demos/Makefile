# Generated automatically from Makefile.in by configure.
#
# Makefile for the examples directory.
#
SHELL		= /bin/sh

CC		= gcc
x		= 

prefix		= /usr/local
exec_prefix	= ${prefix}

srcdir		= .

LIBS		= -lcdk -lcurses 
LIBDIR		= -L.. 

CFLAGS		= -g -O2
EXTRA_CFLAGS	= 
CPPFLAGS	= -DHAVE_CONFIG_H  -I../include -I$(srcdir)/../include -I. 

BINS		= \
		fileview$x \
		rolodex$x \
		command$x \
		clock$x \
		stopSign$x \
		appointment$x \
		serial$x \
		vinstall$x

#
# If you have Sybase installed on your system, try making
# syb. It's a full screen interface to Sybase.
#
SYBROOT		= $(SYBASE)
SYBINCDIR	= -I$(SYBROOT)/include
SYBLIBDIR	= -L$(SYBROOT)/devlib
SYBLIBS		= -lnsl -lsybdb

all : $(BINS)

#
# Standard .c to .o compile line.
#
.c.o:
# compiling
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) -c $<
#
# Most of the examples are built from a single .c file using the same libraries
#
LINKIT	= $(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CPPFLAGS) $@.o -o $@ $(LIBDIR) $(LIBS)

# fileview    - Basic file selector/file viewer combination.
fileview$x :	fileview.o	; $(LINKIT)

# rolodex     - A fairly robust rolodex.
rolodex$x :	rolodex.o	; $(LINKIT)

# command     - A basic command interface with command history.
command$x :	command.o	; $(LINKIT)

# clock       - A simple little clock.
clock$x :	clock.o	; $(LINKIT)

# stopSign    - A simple demo I wrote after a couple of pints. :)
stopSign$x :	stopSign.o	; $(LINKIT)

# appointment - A basic appointment book/date keeper.
appointment$x :	appointment.o	; $(LINKIT)

# serial      - This probes the serial port.
serial$x :	serial.o	; $(LINKIT)

# keycheck    - This prints out the keys ASCII and Octal values.
keycheck$x :	keycheck.o	; $(LINKIT)

# vinstall    - CDK based installer.
vinstall$x :	vinstall.o	; $(LINKIT)

#
# A Sybase interface.
#
syb$x :		syb.c
	$(CC) $(CFLAGS) -o $@ $? $(CPPFLAGS) $(SYBINCDIR) $(LIBDIR) $(SYBLIBDIR) $(LIBS) $(SYBLIBS)

#
# Standard clean directives.
#
clean::
	-rm -f core *.o $(BINS)
distclean::	clean
	-rm -f Makefile
