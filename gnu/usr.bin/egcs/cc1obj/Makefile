#	$NetBSD: Makefile,v 1.1.1.1 1998/03/29 11:34:24 mrg Exp $

PROG=	cc1obj
SRCS=	objc-parse.c objc-act.c 

CLEANFILES+=	objc-parse.c

# XXX needs to be before dist/gcc or tree.c comes from the
# wrong place..
.PATH: $(.CURDIR)/../../../dist/gcc/objc

.include "../Makefile.cc1"

CPPFLAGS+=	-I$(DIST)/objc
