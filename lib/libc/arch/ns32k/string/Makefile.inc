#	$NetBSD: Makefile.inc,v 1.3 1997/12/07 00:19:42 matthias Exp $

# objects built from assembler sources (need lint stubs)
ASSRCS+=bcmp.S bcopy.S bzero.S ffs.S index.S memchr.S memcmp.S memcpy.S \
	memset.S memmove.S rindex.S strcat.S strchr.S strcmp.S strcpy.S \
	strlen.S strncat.S strncmp.S strncpy.S strrchr.S memccpy.S

# objects built from C sources
SRCS+=	strcspn.c strpbrk.c strsep.c strspn.c strstr.c swab.c

