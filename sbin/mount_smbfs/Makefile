#	$NetBSD: Makefile,v 1.7 2009/09/03 12:04:50 pooka Exp $

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=	mount_smbfs
SRCS=	mount_smbfs.c
SRCS+=	rcfile.c ctx.c cfopt.c subr.c nls.c rap.c mbuf.c rq.c file.c \
	print.c kiconv.c nb.c nb_name.c nb_net.c nbns_rq.c
MAN=	mount_smbfs.8

SMBDIST=${NETBSDSRCDIR}/dist/smbfs

CPPFLAGS+= -I${SMBDIST}/include

.PATH:	${SMBDIST}/mount_smbfs ${SMBDIST}/lib/smb

DPADD+=${LIBUTIL}
LDADD+=-lutil

.if ${MKSHARE} != "no"
.PATH:	${SMBDIST}/examples
FILESDIR=	/usr/share/examples/smbfs
FILES=		dot.nsmbrc
.endif

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
.for f in ctx mbuf nb_name nbns_rq
COPTS.${f}.c+=       -Wno-pointer-sign
.endfor
.endif
