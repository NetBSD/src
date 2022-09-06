#	$NetBSD: Makefile,v 1.4 2022/09/06 17:50:18 phx Exp $
#
TARGET	= loadbsd

sdkprefix = 
CC	= $(sdkprefix)gcc
CFLAGS	= -D_STANDALONE -DNO_MID_CHECK -I./include -Os -fomit-frame-pointer -msmall-code -m68020
LDFLAGS	= -noixemul
LDLIBS	=

OBJS	= loadbsd.o loadfile.o loadfile_aout.o loadfile_elf32.o getopt.o

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
