#	$NetBSD: Makefile.inc,v 1.18 2005/12/11 12:24:45 christos Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__assert.c __main.c blkcpy.S blkset.S bswap64.S random.S \
	strcasecmp.c strncasecmp.c strncmp.c strtoul.c udiv.S urem.S \
	strncpy.c memcmp.c strcat.c strcpy.c strcmp.c strlen.c memchr.c
