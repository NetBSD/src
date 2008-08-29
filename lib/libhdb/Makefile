# $NetBSD: Makefile,v 1.24 2008/08/29 00:02:22 gmcgarry Exp $

USE_FORT?= yes	# network protocol library

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/hdb

WARNS?=		1

LIB=		hdb

LIBDPLIBS+=	krb5	${.CURDIR}/../libkrb5 \
		asn1	${.CURDIR}/../libasn1 \
		roken	${.CURDIR}/../libroken

.if ${USETOOLS} != "yes"
COMPILEETOBJ!=		cd ${NETBSDSRCDIR}/lib/libcom_err/compile_et && ${PRINTOBJDIR}
TOOL_COMPILE_ET=	${COMPILEETOBJ}/compile_et

ASN1COMPILEOBJ!=	cd ${NETBSDSRCDIR}/lib/libasn1/asn1_compile && ${PRINTOBJDIR}
TOOL_ASN1_COMPILE=	${ASN1COMPILEOBJ}/asn1_compile
.endif

hdb_err.c hdb_err.h: hdb_err.et ${TOOL_COMPILE_ET}
	${TOOL_COMPILE_ET} ${DIST}/heimdal/lib/hdb/hdb_err.et

ERR_FILES=	hdb_err.c

gen_files= \
	asn1_Salt.x					\
	asn1_Key.x					\
	asn1_Event.x					\
	asn1_HDBFlags.x					\
	asn1_GENERATION.x				\
	asn1_HDB_Ext_PKINIT_acl.x			\
	asn1_HDB_Ext_PKINIT_hash.x			\
	asn1_HDB_Ext_Constrained_delegation_acl.x	\
	asn1_HDB_Ext_Lan_Manager_OWF.x			\
	asn1_HDB_Ext_Password.x				\
	asn1_HDB_Ext_Aliases.x				\
	asn1_HDB_extension.x				\
	asn1_HDB_extensions.x				\
	asn1_hdb_entry.x				\
	asn1_hdb_entry_alias.x

DPSRCS=		hdb_asn1.h hdb_err.h

BUILT_SOURCES=	${gen_files:.x=.c} hdb_err.c

SRCS= \
	common.c		\
	db.c			\
	db3.c			\
	ext.c			\
	hdb-ldap.c		\
	hdb.c			\
	keys.c			\
	keytab.c		\
	dbinfo.c		\
	mkey.c			\
	ndbm.c			\
	print.c			\
	$(BUILT_SOURCES)

INCS=		hdb.h hdb-protos.h hdb_err.h hdb_asn1.h
INCSDIR=	/usr/include/krb5

hdb_asn1_files: hdb.asn1 ${TOOL_ASN1_COMPILE}
	${TOOL_ASN1_COMPILE} ${DIST}/heimdal/lib/hdb/hdb.asn1 hdb_asn1

ASN1!=		cd ${NETBSDSRCDIR}/lib/libasn1 && ${PRINTOBJDIR}
KRB5!=		cd ${NETBSDSRCDIR}/lib/libkrb5 && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	 -I${DIST}/heimdal/lib/hdb		\
	 -I${ASN1}				\
	 -I${KRB5}				\
	 -I${DIST}/heimdal/lib/krb5		\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${NETBSDSRCDIR}/lib/libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHDB_DB_DIR=\"/var/heimdal\"          \
	 -DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
.for f in mkey
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif

${gen_files} hdb_asn1.hx: hdb_asn1_files

CLEANFILES=	${gen_files} ${BUILT_SOURCES} \
		hdb_asn1.h hdb_asn1.hx hdb_asn1_files hdb_err.h

.include <bsd.lib.mk>

.SUFFIXES: .hx .x

.hx.h:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

.x.c:
	@cmp -s $< $@ 2> /dev/null || cp $< $@

