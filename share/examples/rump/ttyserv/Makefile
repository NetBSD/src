PROG=ttyserv
NOMAN=
LDADD+=-lrumpdev_ucom -lrumpdev_ugenhc -lrumpdev_usb -lrumpdev
LDADD+=-lrumpkern_tty
LDADD+=-lrumpvfs
LDADD+=-lrump -lrumpuser -lpthread

CPPFLAGS+= -D_KERNTYPES

WARNS=4
DBG=-g

.include <bsd.prog.mk>
