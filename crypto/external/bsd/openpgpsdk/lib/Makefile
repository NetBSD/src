# $NetBSD: Makefile,v 1.7 2009/01/22 00:20:58 lukem Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network protocol library

LIB= openpgpsdk
SRCS+= accumulate.c compress.c create.c crypto.c errors.c fingerprint.c
SRCS+= hash.c keyring.c lists.c memory.c openssl_crypto.c packet-parse.c
SRCS+= packet-print.c packet-show.c random.c reader.c reader_armoured.c
SRCS+= reader_encrypted_se.c reader_encrypted_seip.c reader_fd.c
SRCS+= reader_hashed.c reader_mem.c readerwriter.c signature.c symmetric.c
SRCS+= util.c validate.c writer.c writer_armour.c writer_encrypt.c
SRCS+= writer_encrypt_se_ip.c writer_fd.c writer_memory.c
SRCS+= writer_skey_checksum.c writer_stream_encrypt_se_ip.c
CPPFLAGS+= -I${EXTDIST}/include
NOMAN= # defined
WARNS=4

EXTDIST=${NETBSDSRCDIR}/crypto/external/bsd/openpgpsdk/dist

.PATH: ${EXTDIST}/include/openpgpsdk ${EXTDIST}/src/lib

INCS+= accumulate.h armour.h callback.h compress.h configure.h
INCS+= create.h crypto.h defs.h errors.h
INCS+= final.h hash.h keyring.h lists.h memory.h
INCS+= packet-parse.h packet-show-cast.h packet-show.h packet.h
INCS+= random.h readerwriter.h signature.h std_print.h
INCS+= streamwriter.h types.h util.h
INCS+= validate.h version.h writer.h writer_armoured.h
INCSDIR=/usr/include/openpgpsdk

LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/lib/libcrypto
LIBDPLIBS+=	z	${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=	bz2	${NETBSDSRCDIR}/lib/libbz2

.include <bsd.lib.mk>
