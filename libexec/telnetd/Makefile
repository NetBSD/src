#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$Id: Makefile,v 1.4 1994/02/25 03:20:39 cgd Exp $

PROG=	telnetd
CFLAGS+=-DLINEMODE -DKLUDGELINEMODE -DUSE_TERMIO -DDIAGNOSTICS
CFLAGS+=-DOLD_ENVIRON -DENV_HACK
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
DPADD=	${LIBUTIL} ${LIBTERM}
LDADD=	-lutil -ltermcap -ltelnet
MAN8=	telnetd.0

# These are the sources that have encryption stuff in them.
CRYPT_SRC= authenc.c ext.h state.c telnetd.c termstat.c
CRYPT_SRC+= utility.c Makefile
NOCRYPT_DIR=${.CURDIR}/Nocrypt

.include <bsd.prog.mk>

nocrypt:
	@echo "Encryption code already removed."
