#	$NetBSD: Makefile,v 1.13 1998/07/27 04:16:20 scottr Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM definition

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
.include <bsd.obj.mk>
.include <bsd.man.mk>
.endif	# EXPORTABLE_SYSTEM

nocrypt:
	@echo "Encryption code already removed."
