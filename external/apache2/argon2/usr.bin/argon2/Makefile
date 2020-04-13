PROG=   argon2

.include <bsd.init.mk>

BINDIR?=        /usr/bin

.PATH:  ../../dist/phc-winner-argon2/src ../../dist/phc-winner-argon2/src/blake2 ../../dist/phc-winner-argon2/man

SRCS=
SRCS=   argon2.c core.c blake2b.c thread.c encoding.c ref.c
SRCS+=   run.c

CPPFLAGS+= -DARGON2_NO_THREADS -std=c89 -O3 -Wall -g -I../../dist/phc-winner-argon2/include -Isrc 

MAN=argon2.1

.include <bsd.prog.mk>
