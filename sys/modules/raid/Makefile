#	$NetBSD: Makefile,v 1.2 2018/01/18 00:32:48 mrg Exp $

.include "../Makefile.inc"

IOCONF=	raid.ioconf

.PATH:	${S}/dev/raidframe

KMOD=	raid

SRCS+=	rf_acctrace.c           rf_alloclist.c         rf_aselect.c
SRCS+=	rf_callback.c           rf_chaindecluster.c    rf_copyback.c
SRCS+=	rf_cvscan.c             rf_dagdegrd.c          rf_dagdegwr.c
SRCS+=	rf_dagffrd.c            rf_dagffwr.c           rf_dagfuncs.c
SRCS+=	rf_dagutils.c           rf_debugMem.c          rf_debugprint.c
SRCS+=	rf_decluster.c          rf_declusterPQ.c       rf_diskqueue.c
SRCS+=	rf_disks.c              rf_driver.c            rf_engine.c
SRCS+=	rf_evenodd.c            rf_evenodd_dagfuncs.c  rf_evenodd_dags.c
SRCS+=	rf_fifo.c               rf_interdecluster.c    rf_invertq.c
SRCS+=	rf_layout.c             rf_map.c               rf_mcpair.c
SRCS+=	rf_netbsdkintf.c        rf_nwayxor.c           rf_options.c
SRCS+=	rf_paritylog.c          rf_paritylogDiskMgr.c  rf_paritylogging.c
SRCS+=	rf_parityloggingdags.c  rf_paritymap.c         rf_parityscan.c
SRCS+=	rf_pq.c                 rf_pqdeg.c             rf_pqdegdags.c
SRCS+=	rf_psstatus.c           rf_raid0.c             rf_raid1.c
SRCS+=	rf_raid4.c              rf_raid5.c             rf_raid5_rotatedspare.c
SRCS+=	rf_reconbuffer.c        rf_reconmap.c          rf_reconstruct.c
SRCS+=	rf_reconutil.c          rf_revent.c            rf_shutdown.c
SRCS+=	rf_sstf.c               rf_states.c            rf_stripelocks.c
SRCS+=	rf_strutils.c           rf_utils.c             rf_compat50.c
SRCS+=	rf_compat80.c

CPPFLAGS+=	-DRAID_AUTOCONFIG=1

# Include optional raid styles

CPPFLAGS+=	-DRF_INCLUDE_EVENODD=1
CPPFLAGS+=	-DRF_INCLUDE_RAID5_RS=1
CPPFLAGS+=	-DRF_INCLUDE_PARITYLOGGING=1      
CPPFLAGS+=	-DRF_INCLUDE_CHAINDECLUSTER=1     
CPPFLAGS+=	-DRF_INCLUDE_INTERDECLUSTER=1
CPPFLAGS+=	-DRF_INCLUDE_PARITY_DECLUSTERING=1
CPPFLAGS+=	-DRF_INCLUDE_PARITY_DECLUSTERING_DS=1

CPPFLAGS+=	-DCOMPAT_50
CPPFLAGS+=	-DCOMPAT_80

.if ${MACHINE_CPU} == "sparc64" || \
    ${MACHINE_CPU} == "x86_64" || \
    ${MACHINE_CPU} == "mips64"
SRCS+=	rf_compat32.c
CPPFLAGS+=	-DCOMPAT_NETBSD32
.endif


.include <bsd.kmodule.mk>
