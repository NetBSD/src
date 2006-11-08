
WARNS=	4

CPPFLAGS+=-I/usr/include/ssp -D_FORTIFY_SOURCE=2
CFLAGS+=-fstack-protector-all -Wstack-protector
LDFLAGS+=-fstack-protector-all -Wstack-protector
