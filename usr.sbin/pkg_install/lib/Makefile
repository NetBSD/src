# $NetBSD: Makefile,v 1.4 1997/06/05 20:43:28 thorpej Exp $
# Original from FreeBSD, no rcs id.

LIB=		install
CFLAGS+=	-I${.CURDIR} ${DEBUG}
SRCS=		exec.c file.c global.c msg.c pen.c plist.c str.c
SRCS+=		ftpio.c ftperr.c 
CLEANFILES+=	ftperr.c

# Don't need to install ftpio.3
NOMAN=		yes

NOPROFILE=	yes
NOPIC=		yes

# only needed during build - prevent installation of library
libinstall::

ftperr.c:	ftp.errors
	@echo '#include <stdio.h>' > ${.TARGET}
	@echo '#include "ftpio.h"' >> ${.TARGET}
	@echo "struct ftperr ftpErrList[] = {" \ >>  ${.TARGET}
	@cat ${.ALLSRC} \
	  | grep -v ^# \
	  | sort \
	  | while read NUM STRING; do \
	    echo "  { $${NUM}, \"$${STRING}\" },"; \
	  done >> ${.TARGET}
	@echo "};" >> ${.TARGET}
	@echo -n "int const ftpErrListLength = " >> ${.TARGET}
	@echo "sizeof(ftpErrList) / sizeof(*ftpErrList);" >> ${.TARGET}

.include <bsd.lib.mk>
