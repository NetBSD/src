#	$NetBSD: Makefile,v 1.5 1999/01/12 03:08:14 kleink Exp $

KDIR=	/sys/sys
INCSDIR= /usr/include/sys

INCS=	acct.h ataio.h audioio.h buf.h callout.h cdefs.h cdio.h chio.h \
	clist.h conf.h core.h device.h dir.h dirent.h disk.h disklabel.h \
	dkbad.h dkio.h dkstat.h dmap.h domain.h errno.h exec.h exec_aout.h \
	exec_ecoff.h exec_elf.h exec_script.h extent.h fcntl.h fdio.h \
	featuretest.h file.h filedesc.h filio.h gmon.h inttypes.h ioccom.h \
	ioctl.h ioctl_compat.h ipc.h kcore.h kernel.h kgdb.h ktrace.h lkm.h \
	localedef.h lock.h lockf.h malloc.h map.h mbuf.h md5.h midiio.h mman.h \
	mount.h msg.h msgbuf.h mtio.h namei.h param.h poll.h pool.h proc.h \
	protosw.h ptrace.h queue.h reboot.h resource.h resourcevar.h rnd.h \
	scanio.h scsiio.h select.h sem.h sha1.h shm.h signal.h signalvar.h \
	socket.h socketvar.h sockio.h stat.h syscall.h syscallargs.h sysctl.h \
	swap.h syslimits.h syslog.h systm.h tablet.h termios.h time.h timeb.h \
	timepps.h times.h timex.h tprintf.h trace.h tty.h ttychars.h \
	ttycom.h ttydefaults.h ttydev.h types.h ucred.h uio.h un.h unistd.h \
	unpcb.h user.h utsname.h vadvise.h vcmd.h vlimit.h vmmeter.h vnode.h \
	vnode_if.h vsio.h wait.h

SYMLINKS= sys/fcntl.h /usr/include/fcntl.h \
	sys/inttypes.h /usr/include/inttypes.h \
	sys/md5.h /usr/include/md5.h \
	sys/poll.h /usr/include/poll.h \
	sys/syslog.h /usr/include/syslog.h \
	sys/termios.h /usr/include/termios.h

.include <bsd.kinc.mk>
