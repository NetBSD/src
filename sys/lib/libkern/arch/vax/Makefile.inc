#	$NetBSD: Makefile.inc,v 1.22 2009/08/12 21:20:41 dsl Exp $
#
#	The rest of the lib routines are in machine/macros.h
#

SRCS+=	blkcpy.S blkset.S bswap16.S bswap32.S bswap64.S
SRCS+=	random.S
SRCS+=	udiv.S urem.S

SRCS+=	__assert.c
SRCS+=	__main.c memchr.c memcmp.c
SRCS+=	strcasecmp.c strcat.c strcmp.c strcpy.c strncasecmp.c strncmp.c
SRCS+=	strncpy.c strlen.c strtoul.c
