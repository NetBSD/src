#	$NetBSD: Makefile,v 1.2 2010/02/21 01:46:36 darran Exp $

# $FreeBSD: src/cddl/usr.bin/ctfmerge/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include <bsd.own.mk>

.include	"../../Makefile.inc"

PROG=		ctfmerge

SRCS=		alist.c \
		barrier.c \
		ctf.c \
		ctfmerge.c \
		fifo.c \
		hash.c \
		iidesc.c \
		input.c \
		list.c \
		memory.c \
		merge.c \
		output.c \
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

.ifndef HOSTPROG
LDADD+=		-L${OSNETDIR}/lib/libctf \
		-L${NETBSDSRCDIR}/exernal/bsd/libdwarf/lib \
		-L${NETBSDSRCDIR}/external/bsd/libelf/lib
LDADD+=		-lctf -ldwarf -lelf -lz -lpthread
.endif

.PATH:		${OPENSOLARIS_USR_DISTDIR}/tools/ctf/common
.PATH:		${OPENSOLARIS_USR_DISTDIR}/tools/ctf/cvt

MKMAN=		no

.include <bsd.prog.mk>
