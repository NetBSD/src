#	$NetBSD: Makefile,v 1.4 2009/10/29 14:42:54 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	amd

DIST=	${IDIST}/amd
.PATH:	${DIST}

SRCS=	am_ops.c amd.c amfs_auto.c amfs_generic.c amfs_direct.c \
	amfs_error.c amfs_host.c \
	amfs_link.c amfs_linkx.c amfs_nfsl.c amfs_nfsx.c \
	amfs_program.c amfs_root.c amfs_toplvl.c amfs_union.c amq_subr.c \
	amq_svc.c autil.c clock.c conf.c get_args.c info_exec.c info_file.c \
	info_ndbm.c info_passwd.c info_sun.c \
	info_union.c map.c mapc.c mntfs.c nfs_prot_svc.c nfs_start.c \
	nfs_subr.c ops_cdfs.c ops_efs.c \
	ops_mfs.c ops_nfs.c ops_nfs3.c ops_nullfs.c ops_pcfs.c \
	ops_tfs.c ops_tmpfs.c ops_udf.c ops_ufs.c ops_umapfs.c ops_unionfs.c \
	opts.c readdir.c restart.c rpc_fwd.c sched.c srvr_amfs_auto.c \
	srvr_nfs.c sun_map.c sun_map_parse.y sun_map_tok.l \
	conf_parse.y conf_tok.l

.if (${USE_HESIOD} != "no")
SRCS+=	info_hesiod.c
.endif

.if (${USE_LDAP} != "no")
SRCS+=	info_ldap.c

LDADD+=	-lldap
DPADD+=	${LIBLDAP}
.if (${MKPIC} == "no" || (defined(LDSTATIC) && ${LDSTATIC} != ""))
LDADD+=	-llber
DPADD+=	${LIBLBER}
.if ${MKCRYPTO} != "no" 
DPADD+=	${LIBSSL} ${LIBCRYPTO}
LDADD+=	-lssl -lcrypto
.endif
.endif
.endif

.if (${USE_YP} != "no")
SRCS+=	info_nis.c
.endif

.if ${HAVE_GCC} == 4
COPTS.amq_subr.c+=	-fno-strict-aliasing
.endif

# the following are not supported on NetBSD
#	info_ldap.c info_nisplus.c ops_cachefs.c ops_efs.c ops_lofs.c ops_xfs.c
MAN+=	amd.8

AMDOBJDIR!=cd ${.CURDIR} && ${PRINTOBJDIR}

CPPFLAGS+=	-I${DIST} -I${AMDOBJDIR}
LDADD+=		${LIBAMU} -lrpcsvc
DPADD+=		${LIBRPCSVC}
YHEADER=	1

YPREFIX.sun_map_parse.y =	sun_map_
LPREFIX.sun_map_tok.l =	sun_map_
CPPFLAGS.sun_map_tok.c += -Dyylval=sun_map_lval -Dyyerror=sun_map_error

YPREFIX.conf_parse.y =	conf_
LPREFIX.conf_tok.l =	conf_
CPPFLAGS.conf_tok.c += -Dyylval=conf_lval -Dyyerror=conf_error

CLEANFILES+=	build_version.h
DPSRCS+=	build_version.h

get_args.o get_args.d: build_version.h

build_version.h: get_args.c
	${_MKTARGET_CREATE}
	echo '#define AMU_BUILD_VERSION	1' > ${.TARGET}
	echo '#define BUILD_USER "'unknown'"' >> ${.TARGET}
	echo '#define BUILD_HOST "'unknown'"' >> ${.TARGET}
	echo '#define BUILD_DATE "'unknown'"' >> ${.TARGET}


.include <bsd.prog.mk>
