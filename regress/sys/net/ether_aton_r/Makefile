#	$NetBSD: Makefile,v 1.2 2010/12/20 16:13:44 joerg Exp $
#
# To use this, copy / link the cpu_in_cksum.S from the arch subdirectory
# and copy assym.h from the build directory of a kernel.
#

NOMAN=	# defined
.include <bsd.own.mk>

PROG=	ether_aton_r
SRCS=	ether_aton_r.c main.c
WARNS ?=	4

CLEANFILES+=ether_aton_r.c

ether_aton_r.c: autogen ${NETBSDSRCDIR}/sys/net/if_ethersubr.c
	${HOST_SH} ${.ALLSRC} ${.TARGET}

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

.include <bsd.prog.mk>
