# $NetBSD: Makefile,v 1.3.2.3 2013/02/25 00:26:03 tls Exp $

.if ${MACHINE} == "evbarm" && ${MACHINE_ARCH:M*eb} == ""

LIBISPRIVATE=	yes
LIB=		elftosb
SRCS=		AESKey.cpp \
		Blob.cpp \
		DataSource.cpp \
		DataSourceImager.cpp \
		DataTarget.cpp \
		ELFSourceFile.cpp \
		EncoreBootImage.cpp \
		EvalContext.cpp \
		ExcludesListMatcher.cpp \
		GHSSecInfo.cpp \
		GlobMatcher.cpp \
		HexValues.cpp \
		IVTDataSource.cpp \
		Logging.cpp \
		Operation.cpp \
		OptionDictionary.cpp \
		OutputSection.cpp \
		Random.cpp \
		RijndaelCBCMAC.cpp \
		SHA1.cpp \
		SRecordSourceFile.cpp \
		SearchPath.cpp \
		SourceFile.cpp \
		StELFFile.cpp \
		StExecutableImage.cpp \
		StSRecordFile.cpp \
		Value.cpp \
		Version.cpp \
		crc.cpp \
		format_string.cpp \
		options.cpp \
		rijndael.cpp \
		stdafx.cpp
.include <bsd.lib.mk>
.else
${.TARGETS}:
.endif


