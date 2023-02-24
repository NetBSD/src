#	$NetBSD: Makefile,v 1.3.22.1 2023/02/24 13:51:43 martin Exp $
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
