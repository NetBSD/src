#	$NetBSD: Makefile,v 1.7 2011/04/03 13:29:45 mbalmer Exp $

.include <bsd.own.mk>

PROG=		ssh-askpass
SRCS=		drawing.c dynlist.c resources.c x11-ssh-askpass.c

APPDEFS=	SshAskpass.ad
DATE=		February 14, 2001

BUILDSYMLINKS=	SshAskpass-default.ad SshAskpass.ad

FILESDIR=	${X11ROOTDIR}/share/examples/ssh-askpass
FILES=		SshAskpass-1337.ad SshAskpass-NeXTish.ad \
		SshAskpass-green.ad SshAskpass-motif.ad \
		SshAskpass-default.ad

CPPFLAGS+=	-I.
DPSRCS+=	SshAskpass_ad.h
CLEANFILES+=	SshAskpass_ad.h

SRCPATH=	${X11SRCDIR.local}/programs/x11-ssh-askpass

ssh-askpass.1:	x11-ssh-askpass.man.in
	sed -e 's#@NAME@#$(PROG)#g' -e 's#@VERSION@#$(VERSION)#g' \
	    -e 's#@DATE@#$(DATE)#g' < ${SRCPATH}/x11-ssh-askpass.man.in > $@

SshAskpass_ad.h: SshAskpass.ad
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${TOOL_SED} -n '/^[^!]/s/.*/"&",/p' <${.ALLSRC} > ${.TARGET}

LDADD+=		-lXt -lSM -lICE -lXinerama -lXext -lX11
DPADD+=		${LIBXT} ${LIBSM} ${LIBICE} ${LIBXINERAMA} ${LIBXEXT} ${LIBX11}

.PATH:		${SRCPATH}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
