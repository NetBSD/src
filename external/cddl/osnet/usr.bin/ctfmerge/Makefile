#	$NetBSD: Makefile,v 1.3.12.1 2013/02/25 00:26:47 tls Exp $

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
LIBCTF_OBJDIR!=	cd ${.CURDIR}/../../lib/libctf && ${PRINTOBJDIR}
LDADD+=		-L${LIBCTF_OBJDIR} -lctf
LDADD+=		-ldwarf -lelf -lz -lpthread
.endif

.PATH:		${OPENSOLARIS_USR_DISTDIR}/tools/ctf/common
.PATH:		${OPENSOLARIS_USR_DISTDIR}/tools/ctf/cvt

.include <bsd.prog.mk>
