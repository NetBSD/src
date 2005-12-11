#	$NetBSD: Makefile.inc,v 1.2 2005/12/11 12:24:44 christos Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__assert.c __main.c memset.c skpc.c scanc.c random.c \
	strcasecmp.c strncasecmp.c strncmp.c strtoul.c ffs.c \
	strncpy.c memcmp.c strcat.c strcpy.c strcmp.c strlen.c \
	memchr.c structcpy.c alloca.S
