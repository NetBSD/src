#	$NetBSD: Makefile,v 1.1 2003/02/18 20:35:30 jdolecek Exp $

.include <bsd.own.mk>

PROG=	mount_smbfs
SRCS=	mount_smbfs.c getmntopts.c
SRCS+=	rcfile.c ctx.c cfopt.c subr.c nls.c rap.c mbuf.c rq.c file.c \
	print.c kiconv.c nb.c nb_name.c nb_net.c nbns_rq.c
MAN=	mount_smbfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
SMBDIST=${NETBSDSRCDIR}/dist/smbfs

CPPFLAGS+= -I${MOUNT} -I${NETBSDSRCDIR}/sys -I${SMBDIST}/include

.PATH:	${MOUNT}
.PATH:	${SMBDIST}/mount_smbfs ${SMBDIST}/lib/smb

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
