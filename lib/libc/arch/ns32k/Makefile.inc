#	$NetBSD: Makefile.inc,v 1.9 1998/02/22 09:04:50 mycroft Exp $

KMINCLUDES= arch/ns32k/SYS.h
KMSRCS=	bcmp.S bzero.S ffs.S \
	memchr.S memcpy.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S strncmp.S strncpy.S \
	htonl.S htons.S ntohl.S ntohs.S

SRCS+=	${ASSRCS}
LSRCS+= ${ASSRCS:S/S$/c/g:C/^./Lint_&/g}
DPSRCS+=${ASSRCS:S/S$/c/g:C/^./Lint_&/g}
