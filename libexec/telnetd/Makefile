#	$NetBSD: Makefile,v 1.12 1998/07/22 22:30:07 scottr Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

MAN=	telnetd.8

# XXX Keep telnetd(8) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../../domestic/libexec/telnetd) || \
    defined(EXPORTABLE_SYSTEM) || make(clean) || make(cleandir)

PROG=	telnetd
CPPFLAGS+=-DLINEMODE -DKLUDGELINEMODE -DUSE_TERMIO -DDIAGNOSTICS
CPPFLAGS+=-DOLD_ENVIRON -DENV_HACK -I${.CURDIR}
#CPPFLAGS+=-DAUTHENTICATION -I${.CURDIR}/../../lib
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
DPADD=	${LIBUTIL} ${LIBTERMCAP} ${LIBTELNET}
LDADD+=	-lutil -ltermcap -ltelnet

.include <bsd.prog.mk>

.else	# EXPORTABLE_SYSTEM
.include <bsd.man.mk>
.endif	# EXPORTABLE_SYSTEM

nocrypt:
	@echo "Encryption code already removed."
