#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.1 1999/07/12 03:20:17 darrenr Exp $

MAN=
LDSTATIC?=-static
PROG=	ktruss
CFLAGS+=-g -I${.CURDIR}/../ktrace -I/sys -DKTRUSS
SRCS=	ktrace.c dump.c subr.c ioctl.c misc.c
CLEANFILES+=misc.c misc.h ktrace.c subr.c ioctl.c

ktruss: links

links: ktrace.c subr.c ioctl.c

ktrace.c: ../ktrace/ktrace.c
	rm -f ktrace.c
	ln -s ../ktrace/ktrace.c .

subr.c: ../ktrace/subr.c
	rm -f subr.c
	ln -s ../ktrace/subr.c .

ioctl.c: ../kdump/ioctl.c
	rm -f ioctl.c
	ln -s ../kdump/ioctl.c .

../kdump/ioctl.c: ../kdump/mkioctls
	(cd ../kdump; make ioctl.c )

dump.c: misc.h

misc.c: /usr/include/sys/errno.h /usr/include/sys/signal.h
	./makeerrnos

misc.h: /usr/include/sys/errno.h /usr/include/sys/signal.h
	./makeerrnos

.include <bsd.prog.mk>
