#	@(#)Makefile	5.11 (Berkeley) 2/19/91

PROG=	passwd
SRCS=	local_passwd.c passwd.c pw_copy.c pw_util.c
.PATH:  ${.CURDIR}/../../usr.bin/chpass ${.CURDIR}/../../usr.sbin/vipw \
	${.CURDIR}/../rlogin
CFLAGS+=-I${.CURDIR}
BINOWN=	root
BINMODE=4555
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>
