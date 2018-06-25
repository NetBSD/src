#	$NetBSD: Makefile,v 1.20.12.1 2018/06/25 07:25:35 pgoyette Exp $
#

NOSANITIZER=	# defined

.include <bsd.own.mk>

WARNS?=	5

LIB=	rt

SRCS=	sem.c shm.c pset.c

MAN+=	aio.3 aio_cancel.3 aio_error.3 aio_fsync.3 aio_read.3 aio_return.3 \
	aio_suspend.3 aio_write.3 lio_listio.3 \
	mq.3 mq_close.3 mq_getattr.3 mq_notify.3 mq_open.3 mq_receive.3 \
	mq_send.3 mq_setattr.3 mq_unlink.3 \
	pset.3 sched.3 shm_open.3 \
	sem_destroy.3 sem_getvalue.3 sem_init.3 sem_open.3 sem_post.3 \
	sem_wait.3

MLINKS+=	mq.3 mqueue.3
MLINKS+=	mq_receive.3 mq_timedreceive.3
MLINKS+=	mq_send.3 mq_timedsend.3

MLINKS+=	sem_open.3 sem_close.3
MLINKS+=	sem_open.3 sem_unlink.3
MLINKS+=	sem_wait.3 sem_timedwait.3
MLINKS+=	sem_wait.3 sem_trywait.3

MLINKS+=	pset.3 pset_create.3
MLINKS+=	pset.3 pset_assign.3
MLINKS+=	pset.3 pset_bind.3
MLINKS+=	pset.3 pset_destroy.3

MLINKS+=	sched.3 sched_setparam.3
MLINKS+=	sched.3 sched_getparam.3
MLINKS+=	sched.3 sched_protect.3
MLINKS+=	sched.3 sched_setscheduler.3
MLINKS+=	sched.3 sched_getscheduler.3
MLINKS+=	sched.3 sched_get_priority_max.3
MLINKS+=	sched.3 sched_get_priority_min.3
MLINKS+=	sched.3 sched_rr_get_interval.3
MLINKS+=	sched.3 sched_yield.3
MLINKS+=	sched.3 sched_setaffinity_np.3
MLINKS+=	sched.3 sched_getaffinity_np.3

MLINKS+=	shm_open.3 shm_unlink.3

.include "${.CURDIR}/../libc/libcincludes.mk"

AFLAGS+=-I${ARCHDIR}

.include "${.CURDIR}/sys/Makefile.inc"

.if ${MACHINE_ARCH} == "powerpc64"
# XXX: How come it does not resolve from libc?
CPPFLAGS+=-I${NETBSDSRCDIR}/lib/libc/include
.PATH.c: ${NETBSDSRCDIR}/lib/libc/gen
SRCS+= _errno.c
.endif

.include <bsd.lib.mk>
