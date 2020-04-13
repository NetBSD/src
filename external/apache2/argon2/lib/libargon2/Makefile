USE_SHLIBDIR=	yes
SHLIB_MAJOR=  1
SHLIB_MINOR=  0

#DIST=../../dist/phc-winner-argon2

.PATH: ../../dist/phc-winner-argon2/src ../../dist/phc-winner-argon2/src/blake2 ../../dist/phc-winner-argon2/include

INCS=   	argon2.h
INCSDIR=	/usr/include

LIB=	argon2	
SRCS=	argon2.c core.c blake2b.c thread.c encoding.c ref.c

CPPFLAGS= -std=c89 -O3 -Wall -g -I../../dist/phc-winner-argon2/include -Isrc -fPIC -DARGON2_NO_THREADS

OPTTARGET ?= native
OPTTEST := $(shell $(CC) -Iinclude -Isrc -march=$(OPTTARGET) src/opt.c -c \
                        -o /dev/null 2>/dev/null; echo $$?)

install:	all incinstall
.include <bsd.lib.mk>
