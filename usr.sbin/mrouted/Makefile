#	$NetBSD: Makefile,v 1.20 2023/08/08 06:27:33 mrg Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

USE_FORT?= yes	# network server

PROG=	mrouted
SRCS=	callout.c cfparse.y config.c igmp.c inet.c kern.c main.c prune.c \
	route.c vif.c
MAN=	mrouted.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}
CPPFLAGS+= -I${.CURDIR}

CWARNFLAGS.clang+=	-Wno-format-extra-args
CWARNFLAGS.gcc+=        ${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

COPTS.cfparse.c+=	${CC_WNO_STRINGOP_TRUNCATION}
COPTS.vif.c+=		${CC_WNO_STRINGOP_TRUNCATION}
COPTS.route.c+=		${CC_WNO_ARRAY_BOUNDS}

.include <bsd.prog.mk>
