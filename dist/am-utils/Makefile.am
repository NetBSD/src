## Process this file with automake to produce Makefile.in

# Package: am-utils
# Level: Top level Makefile
# Author: Erez Zadok

SUBDIRS =		\
	libamu		\
	amd		\
	amq		\
	\
	fixmount	\
	fsinfo		\
	hlfsd		\
	mk-amd-map	\
	wire-test	\
	\
	scripts		\
	doc

# temporary tests override SUBDIRS
#SUBDIRS = lib wire-test amq amd

# additional files to add to the distribution tar file
#	aux/aclocal
EXTRA_DIST_AUX =			\
	aux/GNUmakefile			\
	aux/amindent			\
	aux/autopat			\
	aux/chop-aclocal.pl		\
	aux/copy-if-newbig		\
	aux/mk-aclocal			\
	aux/mkconf			\
	aux/rmtspc			\
	aux/update_build_version	\
	\
	aux/macros/HEADER				\
	aux/macros/TRAILER				\
	aux/macros/c_void_p.m4				\
	aux/macros/cache_check_dynamic.m4		\
	aux/macros/check_amu_fs.m4			\
	aux/macros/check_checkmount_style.m4		\
	aux/macros/check_extern.m4			\
	aux/macros/check_fhandle.m4			\
	aux/macros/check_field.m4			\
	aux/macros/check_fs_headers.m4			\
	aux/macros/check_fs_mntent.m4			\
	aux/macros/check_hide_mount_type.m4		\
	aux/macros/check_lib2.m4			\
	aux/macros/check_map_funcs.m4			\
	aux/macros/check_mnt2_cdfs_opt.m4		\
	aux/macros/check_mnt2_gen_opt.m4		\
	aux/macros/check_mnt2_nfs_opt.m4		\
	aux/macros/check_mnttab_file_name.m4		\
	aux/macros/check_mnttab_location.m4		\
	aux/macros/check_mnttab_opt.m4			\
	aux/macros/check_mnttab_style.m4		\
	aux/macros/check_mnttab_type.m4			\
	aux/macros/check_mount_style.m4			\
	aux/macros/check_mount_trap.m4			\
	aux/macros/check_mount_type.m4			\
	aux/macros/check_mtype_printf_type.m4		\
	aux/macros/check_mtype_type.m4			\
	aux/macros/check_network_transport_type.m4	\
	aux/macros/check_nfs_fh_dref.m4			\
	aux/macros/check_nfs_hn_dref.m4			\
	aux/macros/check_nfs_prot_headers.m4		\
	aux/macros/check_nfs_sa_dref.m4			\
	aux/macros/check_nfs_socket_connection.m4	\
	aux/macros/check_os_libs.m4			\
	aux/macros/check_restartable_signal_handler.m4	\
	aux/macros/check_umount_style.m4		\
	aux/macros/check_unmount_args.m4		\
	aux/macros/check_unmount_call.m4		\
	aux/macros/expand_cpp_hex.m4			\
	aux/macros/expand_cpp_int.m4			\
	aux/macros/expand_cpp_string.m4			\
	aux/macros/expand_run_string.m4			\
	aux/macros/extern_optarg.m4			\
	aux/macros/extern_sys_errlist.m4		\
	aux/macros/field_mntent_t_mnt_time_string.m4	\
	aux/macros/func_bad_memcmp.m4			\
	aux/macros/func_bad_yp_all.m4			\
	aux/macros/host_macros.m4			\
	aux/macros/linux_headers.m4			\
	aux/macros/localconfig.m4			\
	aux/macros/mount_headers.m4			\
	aux/macros/msg.m4				\
	aux/macros/name_package.m4			\
	aux/macros/name_version.m4			\
	aux/macros/opt_amu_cflags.m4			\
	aux/macros/opt_cppflags.m4			\
	aux/macros/opt_debug.m4				\
	aux/macros/opt_ldflags.m4			\
	aux/macros/opt_libs.m4				\
	aux/macros/os_cflags.m4				\
	aux/macros/os_cppflags.m4			\
	aux/macros/os_ldflags.m4			\
	aux/macros/save_state.m4			\
	aux/macros/struct_field_nfs_fh.m4		\
	aux/macros/struct_mntent.m4			\
	aux/macros/struct_mnttab.m4			\
	aux/macros/struct_nfs_args.m4			\
	aux/macros/struct_nfs_fh.m4			\
	aux/macros/struct_nfs_fh3.m4			\
	aux/macros/struct_nfs_gfs_mount.m4		\
	aux/macros/try_compile_anyfs.m4			\
	aux/macros/try_compile_nfs.m4			\
	aux/macros/try_compile_rpc.m4			\
	aux/macros/type_auth_create_gidlist.m4		\
	aux/macros/type_autofs_args.m4			\
	aux/macros/type_cachefs_args.m4			\
	aux/macros/type_cdfs_args.m4			\
	aux/macros/type_efs_args.m4			\
	aux/macros/type_lofs_args.m4			\
	aux/macros/type_mfs_args.m4			\
	aux/macros/type_pcfs_args.m4			\
	aux/macros/type_recvfrom_fromlen.m4		\
	aux/macros/type_rfs_args.m4			\
	aux/macros/type_svc_in_arg.m4			\
	aux/macros/type_time_t.m4			\
	aux/macros/type_tmpfs_args.m4			\
	aux/macros/type_ufs_args.m4			\
	aux/macros/type_xdrproc_t.m4			\
	aux/macros/type_xfs_args.m4			\
	aux/macros/type_yp_order_outorder.m4

EXTRA_DIST_CONF =			\
	conf/checkmount/checkmount_aix.c	\
	conf/checkmount/checkmount_bsd44.c	\
	conf/checkmount/checkmount_default.c	\
	conf/checkmount/checkmount_osf.c	\
	conf/checkmount/checkmount_svr4.c	\
	conf/checkmount/checkmount_ultrix.c	\
	\
	conf/fh_dref/fh_dref_aix3.h		\
	conf/fh_dref/fh_dref_aix42.h		\
	conf/fh_dref/fh_dref_bsd44.h		\
	conf/fh_dref/fh_dref_default.h		\
	conf/fh_dref/fh_dref_freebsd22.h	\
	conf/fh_dref/fh_dref_hpux.h		\
	conf/fh_dref/fh_dref_irix.h		\
	conf/fh_dref/fh_dref_isc3.h		\
	conf/fh_dref/fh_dref_linux.h		\
	conf/fh_dref/fh_dref_nextstep.h		\
	conf/fh_dref/fh_dref_osf2.h		\
	conf/fh_dref/fh_dref_osf4.h		\
	conf/fh_dref/fh_dref_sunos3.h		\
	conf/fh_dref/fh_dref_sunos4.h		\
	conf/fh_dref/fh_dref_svr4.h		\
	\
	conf/hn_dref/hn_dref_default.h	\
	conf/hn_dref/hn_dref_isc3.h	\
	conf/hn_dref/hn_dref_linux.h	\
	\
	conf/mount/mount_aix.c		\
	conf/mount/mount_bsdi3.c	\
	conf/mount/mount_default.c	\
	conf/mount/mount_hpux.c		\
	conf/mount/mount_irix5.c	\
	conf/mount/mount_irix6.c	\
	conf/mount/mount_isc3.c		\
	conf/mount/mount_linux.c	\
	conf/mount/mount_mach3.c	\
	conf/mount/mount_stellix.c	\
	conf/mount/mount_svr4.c		\
	\
	conf/mtab/mtab_aix.c		\
	conf/mtab/mtab_bsd.c		\
	conf/mtab/mtab_file.c		\
	conf/mtab/mtab_isc3.c		\
	conf/mtab/mtab_mach3.c		\
	conf/mtab/mtab_osf.c		\
	conf/mtab/mtab_svr4.c		\
	conf/mtab/mtab_ultrix.c		\
	\
	conf/nfs_prot/nfs_prot_aix3.h		\
	conf/nfs_prot/nfs_prot_aix4.h		\
	conf/nfs_prot/nfs_prot_aix4_2.h		\
	conf/nfs_prot/nfs_prot_aix4_3.h		\
	conf/nfs_prot/nfs_prot_bsdi2.h		\
	conf/nfs_prot/nfs_prot_bsdi3.h		\
	conf/nfs_prot/nfs_prot_darwin.h		\
	conf/nfs_prot/nfs_prot_default.h	\
	conf/nfs_prot/nfs_prot_freebsd2.h	\
	conf/nfs_prot/nfs_prot_freebsd3.h	\
	conf/nfs_prot/nfs_prot_hpux.h		\
	conf/nfs_prot/nfs_prot_hpux11.h		\
	conf/nfs_prot/nfs_prot_irix5.h		\
	conf/nfs_prot/nfs_prot_irix6.h		\
	conf/nfs_prot/nfs_prot_linux.h		\
	conf/nfs_prot/nfs_prot_ncr2.h		\
	conf/nfs_prot/nfs_prot_nextstep.h	\
	conf/nfs_prot/nfs_prot_netbsd.h		\
	conf/nfs_prot/nfs_prot_netbsd1_3.h	\
	conf/nfs_prot/nfs_prot_netbsd1_4.h	\
	conf/nfs_prot/nfs_prot_openbsd.h	\
	conf/nfs_prot/nfs_prot_osf2.h		\
	conf/nfs_prot/nfs_prot_osf4.h		\
	conf/nfs_prot/nfs_prot_osf5_1.h		\
	conf/nfs_prot/nfs_prot_sunos3.h		\
	conf/nfs_prot/nfs_prot_sunos4.h		\
	conf/nfs_prot/nfs_prot_sunos5_3.h	\
	conf/nfs_prot/nfs_prot_sunos5_4.h	\
	conf/nfs_prot/nfs_prot_sunos5_5.h	\
	conf/nfs_prot/nfs_prot_sunos5_6.h	\
	conf/nfs_prot/nfs_prot_sunos5_7.h	\
	conf/nfs_prot/nfs_prot_sunos5_8.h	\
	conf/nfs_prot/nfs_prot_svr4.h		\
	conf/nfs_prot/nfs_prot_ultrix.h		\
	\
	conf/sa_dref/sa_dref_386bsd.h	\
	conf/sa_dref/sa_dref_aix3.h	\
	conf/sa_dref/sa_dref_aoi.h	\
	conf/sa_dref/sa_dref_bsd44.h	\
	conf/sa_dref/sa_dref_default.h	\
	conf/sa_dref/sa_dref_isc3.h	\
	conf/sa_dref/sa_dref_linux.h	\
	conf/sa_dref/sa_dref_svr4.h	\
	\
	conf/transp/transp_sockets.c	\
	conf/transp/transp_tli.c	\
	\
	conf/trap/trap_aix3.h		\
	conf/trap/trap_aux.h		\
	conf/trap/trap_default.h	\
	conf/trap/trap_dgux.h		\
	conf/trap/trap_hcx.h		\
	conf/trap/trap_hpux.h		\
	conf/trap/trap_irix.h		\
	conf/trap/trap_isc3.h		\
	conf/trap/trap_linux.h		\
	conf/trap/trap_mach3.h		\
	conf/trap/trap_news4.h		\
	conf/trap/trap_rtu6.h		\
	conf/trap/trap_stellix.h	\
	conf/trap/trap_svr4.h		\
	conf/trap/trap_ultrix.h		\
	\
	conf/umount/umount_bsd44.c	\
	conf/umount/umount_default.c	\
	conf/umount/umount_osf.c

EXTRA_DIST_INCLUDE =		\
	include/am_compat.h	\
	include/am_utils.h	\
	include/am_xdr_func.h	\
	include/am_defs.h	\
	include/amq_defs.h	\
	include/mount_headers1.h \
	include/mount_headers2.h

EXTRA_DIST =			\
	$(EXTRA_DIST_AUX)	\
	$(EXTRA_DIST_CONF)	\
	$(EXTRA_DIST_INCLUDE)	\
	\
	BUGS			\
	LSM.am-utils		\
	MIRRORS			\
	README.autofs		\
	README.ldap		\
	README.y2k		\
	buildall		\
	config.guess.long	\
	depcomp			\
	ldap-id.ms		\
	ldap-id.txt		\
	tasks

DISTCLEANFILES = amu_nfs_prot.h config.cache.old dbgcf.h

##############################################################################
# Personal maintainer rules by Erez Zadok

# update possibly new versions of autoconf/automake/libtool files
update:
	(cd $(top_srcdir) && ${MAKE} ${MFLAGS} -f ./aux/GNUmakefile update)
	${MAKE} ${MFLAGS} libtool
# remake libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

# make a snapshots and put them in the ftp snapshots directory
FTPTOP=/proj/bank/ftp/pub/am-utils
snapshot: dist install-snapshot
install-snapshot:
	snap=`/bin/ls -1 am-utils*.tar.gz` && \
	echo COPYING SNAPSHOTS: $$snap && \
	cp -pu $$snap $(FTPTOP)/snapshots && \
	rm -f $(FTPTOP)/am-utils-snapshot.tar.gz && \
	ln -s snapshots/$$snap $(FTPTOP)/am-utils-snapshot.tar.gz && \
	(cd $(FTPTOP) && make) && \
	(cd $(FTPTOP)/snapshots && make)
test-snapshot: dist
	mv -f am-utils*.tar.gz /proj/bank/ftp/private/ezk

# dist hook: change directory modes so they are not world writable
dist-hook:
	@echo "Protecting distribution directories..."
	chmod -R go-w $(distdir)
