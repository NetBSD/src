#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:28 wrstuden Exp $

NOMAN=		yes

.include <bsd.own.mk>

PROG=		ssh-askpass
SRCS=		drawing.c dynlist.c resources.c x11-ssh-askpass.c

APPDEFS=	SshAskpass.ad

BUILDSYMLINKS=	SshAskpass-default.ad SshAskpass.ad

FILESDIR=	${X11ROOTDIR}/share/examples/ssh-askpass
FILES=		SshAskpass-1337.ad SshAskpass-NeXTish.ad \
		SshAskpass.ad SshAskpass-green.ad SshAskpass-motif.ad

# XXX:	We use the following two lines instead of just having
# XXX:		FILES+= SshAskpass-default.ad
# XXX:	because of an obscure bug with "make -j" where it doesn't find
# XXX:	SshAskpass-default.ad in the .PATH correctly, probably because
# XXX:	of the rule created for it from BUILDSYMLINKS by <bsd.files.mk>.
# XXX:	This is not the correct long-term solution.
#
FILES+=			SshAskpass.ad
FILESNAME_SshAskpass.ad=SshAskpass-default.ad


CPPFLAGS+=	-I.
DPSRCS+=	SshAskpass_ad.h
CLEANFILES+=	SshAskpass_ad.h
SshAskpass_ad.h: SshAskpass.ad
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	sed -n '/^[^!]/s/.*/"&",/p' <${.ALLSRC} > ${.TARGET}

LDADD+=		-lXt -lSM -lICE -lXext -lX11
DPADD+=		${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:		${X11SRCDIR.local}/programs/x11-ssh-askpass

.include <bsd.x11.mk>
.include <bsd.prog.mk>
