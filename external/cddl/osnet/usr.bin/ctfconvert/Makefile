#	$NetBSD: Makefile,v 1.3.6.1 2013/01/23 00:04:40 yamt Exp $

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
LIBCTF_OBJDIR!=	cd ${.CURDIR}/../../lib/libctf  && ${PRINTOBJDIR}

LDADD+=		-L${OSNETDIR}/lib/libctf \
		-L${NETBSDSRCDIR}/external/bsd/libdwarf/lib \
		-L${NETBSDSRCDIR}/external/bsd/libelf/lib \
		-L${LIBCTF_OBJDIR}
LDADD+=		-lctf -ldwarf -lelf -lz 
.endif

.PATH:		${.CURDIR}
.PATH:		${OPENSOLARIS_USR_DISTDIR}/tools/ctf/common
.PATH:		${OPENSOLARIS_USR_DISTDIR}/tools/ctf/cvt

.include <bsd.prog.mk>
