#	from: @(#)bsd.own.mk	0.1 (RGrimes) 4/4/93
#	$Id: bsd.own.mk,v 1.4 1993/08/15 20:42:43 mycroft Exp $

BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	555
NONBINMODE?=	444

MANDIR?=	/usr/share/man/cat
MANGRP?=	${BINGRP}
MANOWN?=	${BINOWN}
MANMODE?=	${NONBINMODE}

LIBDIR?=	/usr/lib
LINTLIBDIR?=	/usr/libdata/lint
LIBGRP?=	${BINGRP}
LIBOWN?=	${BINOWN}
LIBMODE?=	${NONBINMODE}

DOCDIR?=        /usr/share/doc
DOCGRP?=	${DOCGRP}
DOCOWN?=	${DOCOWN}
DOCMODE?=       ${NONBINMODE}

COPY?=		-c
STRIP?=		-s

# Define SYS_INCLUDE to indicate whether you want symbolic links to the system
# source (``symlinks''), or a separate copy (``copies''); (latter useful
# in environments where it's not possible to keep /sys publicly readable)
#SYS_INCLUDE= 	symlinks
