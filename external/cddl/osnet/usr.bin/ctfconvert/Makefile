#	$NetBSD: Makefile,v 1.2 2010/02/21 01:46:36 darran Exp $

# $FreeBSD: src/cddl/usr.bin/ctfconvert/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include <bsd.own.mk>

.include	"../../Makefile.inc"

DEBUG_FLAGS=	-g

PROG=		ctfconvert

SRCS=		alist.c \
		ctf.c \
		ctfconvert.c \
		dwarf.c \
		fixup_tdescs.c \
		hash.c \
		iidesc.c \
		input.c \
		list.c \
		memory.c \
		merge.c \
		output.c \
		st_parse.c \
		stabs.c \
		stack.c \
		strtab.c \
		symbol.c \
		tdata.c \
		traverse.c \
		util.c

CFLAGS+=	-I${OSNETDIR}/sys \
		-I${OSNETDIR}/include \
		-I${OPENSOLARIS_USR_DISTDIR} \
		-I${OPENSOLARIS_SYS_DISTDIR} \
		-I${OPENSOLARIS_USR_DISTDIR}/head \
		-I${OPENSOLARIS_USR_DISTDIR}/tools/ctf/common \
		-I${OPENSOLARIS_USR_DISTDIR}/tools/ctf/cvt \
		-I${OPENSOLARIS_SYS_DISTDIR}/uts/common \
		-g

.ifndef HOSTPROG
LDADD+=		-L${OSNETDIR}/lib/libctf \
		-L${NETBSDSRCDIR}/external/bsd/libdwarf/lib \
		-L${NETBSDSRCDIR}/external/bsd/libelf/lib
LDADD+=		-lctf -ldwarf -lelf -lz 
.endif

.PATH:		${.CURDIR}
.PATH:		${OPENSOLARIS_USR_DISTDIR}/tools/ctf/common
.PATH:		${OPENSOLARIS_USR_DISTDIR}/tools/ctf/cvt

MKMAN=		no

.include <bsd.prog.mk>
