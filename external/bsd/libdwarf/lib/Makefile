#	$NetBSD: Makefile,v 1.1.12.2 2014/08/19 23:47:15 tls Exp $

.include <bsd.init.mk>

LIB=	dwarf

SRCS=	dwarf_abbrev.c		\
	dwarf_attr.c		\
	dwarf_attrval.c		\
	dwarf_cu.c		\
	dwarf_dealloc.c		\
	dwarf_die.c		\
	dwarf_dump.c		\
	dwarf_errmsg.c		\
	dwarf_errno.c		\
	dwarf_finish.c		\
	dwarf_form.c		\
	dwarf_func.c		\
	dwarf_init.c		\
	dwarf_loc.c

INCS=		dwarf.h libdwarf.h
INCSDIR=	/usr/include

CPPFLAGS+=	-I${.CURDIR}

WITHOUT_MAN=	yes

LIBDPLIBS+=	elf		${NETBSDSRCDIR}/external/bsd/libelf/lib

.include <bsd.lib.mk>
