PROG=umserv
NOMAN=
LDADD+=-lrumpdev_ugenhc -lrumpdev_umass -lrumpdev_scsipi -lrumpdev_usb
LDADD+=-lrumpdev_disk -lrumpdev
LDADD+=-lrumpvfs
LDADD+=-lrump -lrumpuser -lpthread

CPPFLAGS+= -D_KERNTYPES

WARNS=4
DBG=-g

.include <bsd.prog.mk>
