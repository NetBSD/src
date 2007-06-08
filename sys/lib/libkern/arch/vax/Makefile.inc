#	$NetBSD: Makefile.inc,v 1.18.30.1 2007/06/08 14:17:31 ad Exp $
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
