# $NetBSD: Makefile,v 1.1 2009/01/20 07:12:16 agc Exp $

.include <bsd.own.mk>

#USE_FORT?= yes	# network protocol library
USE_FORT?= no	# network protocol library

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
WARNS=1

EXTDIST=	${NETBSDSRCDIR}/crypto/external/bsd/openpgpsdk/dist
.PATH:	${EXTDIST}/src/lib

.include <bsd.lib.mk>
