#	$Id: Makefile.inc,v 1.3 1994/02/25 07:40:47 phil Exp $

CFLAGS += -DIEEE_8087
KMINCLUDES= arch/ns32k/SYS.h arch/ns32k/DEFS.h
KMSRCS=	setjmp.S htonl.S htons.S ntohl.S ntohs.S
