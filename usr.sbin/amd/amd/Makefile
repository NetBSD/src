#	$NetBSD: Makefile,v 1.19 2001/01/07 08:00:54 enami Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	amd

DIST=	${IDIST}/amd
.PATH:	${DIST}

SRCS=	am_ops.c amd.c amfs_auto.c amfs_direct.c amfs_error.c amfs_host.c \
	amfs_inherit.c amfs_link.c amfs_linkx.c amfs_nfsl.c amfs_nfsx.c \
	amfs_program.c amfs_root.c amfs_toplvl.c amfs_union.c amq_subr.c \
	amq_svc.c autil.c clock.c conf.c get_args.c info_file.c info_hesiod.c \
	info_ndbm.c info_nis.c info_passwd.c \
	info_union.c map.c mapc.c mntfs.c nfs_prot_svc.c nfs_start.c \
	nfs_subr.c ops_autofs.c ops_cdfs.c \
	ops_mfs.c ops_nfs.c ops_nfs3.c ops_nullfs.c ops_pcfs.c \
	ops_tfs.c ops_tmpfs.c ops_ufs.c ops_umapfs.c ops_unionfs.c \
	opts.c restart.c rpc_fwd.c sched.c srvr_amfs_auto.c srvr_nfs.c \
	conf_parse.c conf_tok.c
# the following are not supported on NetBSD
#	info_ldap.c info_nisplus.c ops_cachefs.c ops_efs.c ops_lofs.c ops_xfs.c
MAN+=	amd.8

AMDOBJDIR!=cd ${.CURDIR} && ${PRINTOBJDIR}

CPPFLAGS+=	-I${DIST} -I${AMDOBJDIR}
LDADD+=		${LIBAMU} -lrpcsvc
DPADD+=		${LIBRPCSVC}
YHEADER=	1
CLEANFILES+=	conf_tok.c conf_parse.c conf_parse.h

conf_tok.o: conf_parse.h

.include <bsd.prog.mk>
