#	$NetBSD: Makefile.inc,v 1.8 1997/12/07 00:19:22 matthias Exp $

KMINCLUDES= arch/ns32k/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S memcpy.S memset.S strcat.S strcmp.S \
	strcpy.S strlen.S strncmp.S strncpy.S htonl.S htons.S ntohl.S ntohs.S

SRCS+=	${ASSRCS}
LSRCS+= ${ASSRCS:S/S$/c/g:C/^./Lint_&/g}
DPSRCS+=${ASSRCS:S/S$/c/g:C/^./Lint_&/g}
