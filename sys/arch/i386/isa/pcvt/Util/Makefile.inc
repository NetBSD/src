BINDIR	=	/usr/local/bin
FONTDIR	=	/usr/share/misc/vgafonts
LIBDIR	=	/usr/local/lib
LIBMODE	=	644

# for pcvt_ioctl.h

INCDIR	=	/usr/include/machine
INCOWN	=	bin
INCGRP	=	bin
INCMODE	=	444

${INCDIR}/pcvt_ioctl.h: ${.CURDIR}/../../pcvt_ioctl.h
	install ${COPY} -o ${INCOWN} -g ${INCGRP} -m ${INCMODE} \
	${.CURDIR}/../../pcvt_ioctl.h ${INCDIR}

.depend:	${INCDIR}/pcvt_ioctl.h

# for NetBSD or 386BSD (they use preformatted man pages)

MAN3EXT	=	0
MAN4EXT	=	0
MAN5EXT	=	0
MAN8EXT	=	0
MANDIR	=	/usr/local/man/cat
