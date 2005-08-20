#	$NetBSD: Makefile.inc,v 1.1 2005/08/20 16:14:34 elad Exp $

# hash functions
.PATH: ${.CURDIR}/hash/sha2

#INCSDIR=/usr/include/crypto

#INCS=	sha2.h

SRCS+=	sha2.c

MAN+=	sha2.3

MLINKS+=sha2.3 SHA256_Init.3 sha2.3 SHA256_Update.3 sha2.3 SHA256_Final.3
MLINKS+=sha2.3 SHA256_End.3  sha2.3 SHA256_File.3   sha2.3 SHA256_Data.3
MLINKS+=sha2.3 SHA256_Pad.3  sha2.3 SHA256_FileChunk.3
MLINKS+=sha2.3 SHA384_Init.3 sha2.3 SHA384_Update.3 sha2.3 SHA384_Final.3
MLINKS+=sha2.3 SHA384_End.3  sha2.3 SHA384_File.3   sha2.3 SHA384_Data.3
MLINKS+=sha2.3 SHA384_Pad.3  sha2.3 SHA384_FileChunk.3
MLINKS+=sha2.3 SHA512_Init.3 sha2.3 SHA512_Update.3 sha2.3 SHA512_Final.3
MLINKS+=sha2.3 SHA512_End.3  sha2.3 SHA512_File.3   sha2.3 SHA512_Data.3
MLINKS+=sha2.3 SHA512_Pad.3  sha2.3 SHA512_FileChunk.3
