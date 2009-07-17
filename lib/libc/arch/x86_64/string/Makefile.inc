#	$NetBSD: Makefile.inc,v 1.3 2009/07/17 19:37:55 dsl Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	bcmp.S bcopy.S bzero.S ffs.S memchr.S memcmp.S \
	memcpy.S memmove.S memset.S strcat.S strchr.S \
	strcmp.S strcpy.S strlen.S strncmp.S strrchr.S swab.S

# objects built from C sources
SRCS+=	strcspn.c strncat.c strncpy.c strpbrk.c strsep.c strspn.c \
	strstr.c

SRCS.x86_64.string=\
	Lint_bcmp.c Lint_bcopy.c Lint_bzero.c Lint_ffs.c Lint_index.c \
	Lint_memchr.c Lint_memcmp.c Lint_memcpy.c Lint_memmove.c \
	Lint_memset.c Lint_rindex.c Lint_strcat.c Lint_strchr.c \
	Lint_strcmp.c Lint_strcpy.c Lint_strlen.c Lint_strncmp.c \
	Lint_strrchr.c Lint_swab.c
LSRCS+=		${SRCS.x86_64.string}
DPSRCS+=	${SRCS.x86_64.string}
CLEANFILES+=	${SRCS.x86_64.string}
