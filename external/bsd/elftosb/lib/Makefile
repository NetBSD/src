# $NetBSD: Makefile,v 1.1 2012/11/15 19:49:16 jkunz Exp $

LIBISPRIVATE=	yes

.if (${MACHINE} == "evbarm")
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
.endif

.include <bsd.lib.mk>

