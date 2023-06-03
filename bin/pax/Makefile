#	$NetBSD: Makefile,v 1.42 2023/06/03 09:09:01 lukem Exp $
#       @(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=   pax
SRCS=	ar_io.c ar_subs.c buf_subs.c file_subs.c ftree.c\
	gen_subs.c getoldopt.c options.c pat_rep.c pax.c sel_subs.c tables.c\
	tar.c tty_subs.c

.if defined(SMALLPROG)
CPPFLAGS+=	-DSMALL -DNO_CPIO
.else
SRCS+=		getid.c spec.c misc.c pack_dev.c cpio.c
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.sbin/mtree \
		-I${NETBSDSRCDIR}/sbin/mknod
.PATH:		${NETBSDSRCDIR}/usr.sbin/mtree \
		${NETBSDSRCDIR}/sbin/mknod

.if (${HOSTPROG:U} == "")
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif
.endif

MAN=	pax.1

.if defined(HOSTPROG)
CPPFLAGS+=	-DHOSTPROG
.else	# {	! HOSTPROG

# XXX: Interix does not have it; we need a conditional for it.
CPPFLAGS+=	-DHAVE_SYS_MTIO_H

.if ${MKBSDTAR} == "no"
LINKS+=	${BINDIR}/pax ${BINDIR}/tar
SYMLINKS+=${BINDIR}/tar /usr/bin/tar
MAN+=tar.1

LINKS+=	${BINDIR}/pax ${BINDIR}/cpio
SYMLINKS+=${BINDIR}/cpio /usr/bin/cpio
MAN+=cpio.1
.endif
.endif	# }	! HOSTPROG

.if !defined(HOSTPROG) && !defined(SMALLPROG)
CPPFLAGS+=	-DSUPPORT_RMT

LDADD+=	-lrmt
DPADD+=	${LIBRMT}
.endif

COPTS.tar.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
