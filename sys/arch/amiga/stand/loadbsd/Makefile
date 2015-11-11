#	$NetBSD: Makefile,v 1.3 2015/11/11 16:08:52 phx Exp $
#
TARGET	= loadbsd

CC	= gcc
CFLAGS	= -D_STANDALONE -I./include -O -fomit-frame-pointer -msmall-code
LDFLAGS	= -noixemul
LDLIBS	=

OBJS	= loadbsd.o loadfile.o loadfile_aout.o loadfile_elf32.o getopt.o

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
