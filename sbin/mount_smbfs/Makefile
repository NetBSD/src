#	$NetBSD: Makefile,v 1.4 2006/05/11 23:16:29 mrg Exp $

.include <bsd.own.mk>

PROG=	mount_smbfs
SRCS=	mount_smbfs.c
SRCS+=	rcfile.c ctx.c cfopt.c subr.c nls.c rap.c mbuf.c rq.c file.c \
	print.c kiconv.c nb.c nb_name.c nb_net.c nbns_rq.c
MAN=	mount_smbfs.8

SMBDIST=${NETBSDSRCDIR}/dist/smbfs

CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${SMBDIST}/include

.PATH:	${SMBDIST}/mount_smbfs ${SMBDIST}/lib/smb

DPADD+=${LIBUTIL}
LDADD+=-lutil

.if ${MKSHARE} != "no"
.PATH:	${SMBDIST}/examples
FILESDIR=	/usr/share/examples/smbfs
FILES=		dot.nsmbrc
.endif

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
.for f in ctx mbuf nb_name nbns_rq
COPTS.${f}.c+=       -Wno-pointer-sign
.endfor
.endif
