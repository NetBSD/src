#	$NetBSD: Makefile,v 1.2 1998/04/01 01:50:01 tv Exp $

PROG=	cc1obj
SRCS=	objc-parse.c objc-act.c 

CLEANFILES+=	objc-parse.c

# XXX needs to be before dist/gcc or tree.c comes from the
# wrong place..
.PATH: $(.OBJDIR) $(.CURDIR)/../../../dist/gcc/objc

.include "../Makefile.cc1"

CPPFLAGS+=	-I$(DIST)/objc
