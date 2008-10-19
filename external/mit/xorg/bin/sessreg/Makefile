#	$NetBSD: Makefile,v 1.2.2.2 2008/10/19 22:40:59 haad Exp $

.include <bsd.own.mk>

PROG=	sessreg

.include "../xdm/Makefile.xdm"

CPPFLAGS=	-I${X11SRCDIR.${PROG}}/../include

X11EXTRAMANDEFS+=	-e 's,__ttys_file__,/etc/ttys,g; \
			s,__utmp_file__,/var/run/utmp,g; \
			s,__wtmp_file__,/var/log/wtmp,g'

.include <bsd.x11.mk>
.include <bsd.prog.mk>

.PATH:		${X11SRCDIR.${PROG}}
