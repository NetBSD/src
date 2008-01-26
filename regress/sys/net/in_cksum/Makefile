#	$NetBSD: Makefile,v 1.2 2008/01/26 02:07:13 chris Exp $
#
# To use this, copy / link the cpu_in_cksum.S from the arch subdirectory
# and copy assym.h from the build directory of a kernel.
#

NOMAN=	# defined

PROG=	in_cksum
SRCS=	in_cksum.c portable.c cpu_in_cksum.S
WARNS=	3
AFLAGS=	-g

CPPFLAGS.portable.c=	-Dcpu_in_cksum=portable_cpu_in_cksum
CLEANFILES+=	portable.c

.include <bsd.own.mk>

cpu_in_cksum.o: assym.h

portable.c: ${NETBSDSRCDIR}/sys/netinet/cpu_in_cksum.c
	cp ${NETBSDSRCDIR}/sys/netinet/cpu_in_cksum.c $@

TIMING_LOOPS=	10000

test:
	./in_cksum 16 1 ${TIMING_LOOPS} \
	    1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
	./in_cksum 16 1 ${TIMING_LOOPS} \
	    1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 \
	    1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
	./in_cksum 64 1 ${TIMING_LOOPS} \
	    1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 \
	    1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
	./in_cksum 16 1 ${TIMING_LOOPS}	\
	    1 3 1 3 1 3 1
	./in_cksum 16 1 ${TIMING_LOOPS}	40
	./in_cksum 16 0 ${TIMING_LOOPS}	2048
	./in_cksum 16 1 ${TIMING_LOOPS}	1536
	./in_cksum 16 1 ${TIMING_LOOPS}	576
	./in_cksum 16 1 ${TIMING_LOOPS}	1536 1536 1536 1536 1536 640

.include <bsd.prog.mk>
