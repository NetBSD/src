#	$NetBSD: Makefile,v 1.28 2003/05/09 01:09:13 lukem Exp $
#       @(#)Makefile	8.1 (Berkeley) 5/31/93

# To install on versions prior to BSD 4.4 the following may have to be
# defined with CPPFLAGS +=
#
# -DNET2_STAT	Use NET2 or older stat structure. The version of the
# 		stat structure is easily determined by looking at the
# 		basic type of an off_t (often defined in the file:
# 		/usr/include/sys/types.h). If off_t is a long (and is
# 		NOT A long long) then you must define NET2_STAT.
# 		This define is important, as if you do have a long long
# 		off_t and define NET2_STAT, pax will compile but will
# 		NOT RUN PROPERLY.
#
# -DNET2_FTS	Use the older NET2 fts. To identify the version,
# 		examine the file: /usr/include/fts.h. If FTS_COMFOLLOW
# 		is not defined then you must define NET2_FTS.
# 		Pax may not compile if this not (un)defined properly.
#
# -DNET2_REGEX	Use the older regexp.h not regex.h. The regex version
# 		is determined by looking at the value returned by
# 		regexec() (man 3 regexec). If regexec return a 1 for
# 		success (and NOT a 0 for success) you have the older
# 		regex routines and must define NET2_REGEX.
# 		Pax may not compile if this not (un)defined properly.

.include <bsd.own.mk>

PROG=   pax
SRCS=	ar_io.c ar_subs.c buf_subs.c cpio.c file_subs.c ftree.c\
	gen_subs.c getoldopt.c options.c pat_rep.c pax.c sel_subs.c tables.c\
	tar.c tty_subs.c

.if defined(SMALLPROG)
CPPFLAGS+=	-DSMALL
.else
SRCS+=		getid.c spec.c misc.c stat_flags.c pack_dev.c
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.sbin/mtree \
		-I${NETBSDSRCDIR}/sbin/mknod \
		-I${NETBSDSRCDIR}/bin/ls
.PATH:		${NETBSDSRCDIR}/usr.sbin/mtree \
		${NETBSDSRCDIR}/sbin/mknod \
		${NETBSDSRCDIR}/bin/ls
.endif

MAN=	pax.1 tar.1 cpio.1

.if defined(HOSTPROG)
CPPFLAGS+=	-DHOSTPROG
.else	# {	! HOSTPROG
LINKS+=	${BINDIR}/pax ${BINDIR}/tar
SYMLINKS+=${BINDIR}/tar /usr/bin/tar

LINKS+=	${BINDIR}/pax ${BINDIR}/cpio
SYMLINKS+=${BINDIR}/cpio /usr/bin/cpio
.endif	# }	! HOSTPROG

NOHTML=doc2html-error

.if !defined(HOSTPROG) && !defined(SMALLPROG)
CPPFLAGS+=	-DSUPPORT_RMT

LDADD+=	-lrmt
DPADD+=	${LIBRMT}
.endif

.if !defined(HOSTPROG)
.include <bsd.prog.mk>
.endif
