#	$NetBSD: Makefile.inc,v 1.12 1998/08/04 03:35:32 perry Exp $

KMINCLUDES= arch/ns32k/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S strncmp.S strncpy.S \
	htonl.S htons.S ntohl.S ntohs.S

SRCS+=	${ASSRCS}
LSRCS+= ${ASSRCS:S/S$/c/g:C/^./Lint_&/g}
DPSRCS+=${ASSRCS:S/S$/c/g:C/^./Lint_&/g}
