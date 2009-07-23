#	$NetBSD: Makefile.inc,v 1.19.48.1 2009/07/23 23:32:45 jym Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__assert.c \
	blkcpy.S blkset.S bswap16.S bswap32.S bswap64.S \
	__main.c memchr.c memcmp.c \
	random.S \
	strcasecmp.c strcat.c strcmp.c strcpy.c strncasecmp.c strncmp.c \
	strncpy.c strlen.c strtoul.c \
	udiv.S urem.S \
