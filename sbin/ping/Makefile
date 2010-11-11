#	$NetBSD: Makefile,v 1.15 2010/11/11 22:56:38 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

USE_FORT?= yes	# setuid
PROG=	ping
MAN=	ping.8
BINOWN=	root
BINMODE=4555
LDADD=	-lm
DPADD=	${LIBM}
CPPFLAGS+=	-DIPSEC
LDADD+= -lipsec
DPADD+= ${LIBIPSEC}

#
# Compile-time debug flag.  If compiled with "make RUMP_ACTION=1",
# make rump system calls.  This allows to single-step ioctl commands
# to figure out where ioctl's go in the kernel.
#
.ifdef RUMP_ACTION
CPPFLAGS+=      -DRUMP_SYS_NETWORKING -DRUMP_SYS_IOCTL -DRUMP_SYS_CLOSE
CPPFLAGS+=      -DRUMP_ACTION
LDADD+=         -lrumpclient
DBG=            -g
.endif

.include <bsd.prog.mk>
