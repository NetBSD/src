#	$NetBSD: Makefile.inc,v 1.16 2003/11/06 02:28:00 christos Exp $

KMINCLUDES= arch/ns32k/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S strncmp.S strncpy.S

SRCS+=	${ASSRCS}
LSRCS+= ${ASSRCS:S/S$/c/g:C/^./Lint_&/g}
DPSRCS+=${ASSRCS:S/S$/c/g:C/^./Lint_&/g}
CLEANFILES+=${ASSRCS:S/S$/c/g:C/^./Lint_&/g}

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S __sigtramp2.S
