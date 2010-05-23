#	$NetBSD: Makefile,v 1.3 2010/05/23 21:31:57 veego Exp $

.include <bsd.own.mk>

PROG=	sessreg

.include "../xdm/Makefile.xdm"

CPPFLAGS=	-I${X11SRCDIR.${PROG}}/../include

X11EXTRAMANDEFS+=	-e 's,__ttys_file__,/etc/ttys,g; \
			s,__utmp_file__,${XDMPIDDIR}/utmp,g; \
			s,__wtmp_file__,${XDMLOGDIR}/wtmp,g'

.include <bsd.x11.mk>
.include <bsd.prog.mk>

.PATH:		${X11SRCDIR.${PROG}}
