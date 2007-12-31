#	$NetBSD: Makefile,v 1.106 2007/12/31 15:32:14 ad Exp $

.include <bsd.sys.mk>

INCSDIR= /usr/include/sys

INCS=	acct.h agpio.h aio.h ansi.h ataio.h atomic.h audioio.h \
	bitops.h bootblock.h bswap.h buf.h \
	callback.h callout.h cdefs.h cdefs_aout.h \
	cdefs_elf.h cdio.h chio.h clockctl.h condvar.h conf.h core.h \
	cpuio.h device.h dir.h dirent.h disk.h disklabel.h disklabel_acorn.h \
	disklabel_gpt.h \
	dkbad.h dkio.h dkstat.h domain.h drvctlio.h dvdio.h \
	endian.h envsys.h errno.h evcnt.h event.h exec.h exec_aout.h \
	exec_coff.h exec_ecoff.h exec_elf.h exec_script.h extattr.h extent.h \
	fcntl.h fd_set.h fdio.h featuretest.h file.h filedesc.h filio.h \
	float_ieee754.h fstypes.h gcq.h gmon.h gpio.h hash.h \
	ieee754.h inttypes.h ioccom.h ioctl.h ioctl_compat.h iostat.h ipc.h \
	joystick.h \
	kcore.h kgdb.h kmem.h ksem.h ksyms.h ktrace.h \
	lkm.h localedef.h lock.h lockf.h lwp.h lwpctl.h \
	malloc.h mallocvar.h mbuf.h md4.h \
	md5.h midiio.h mman.h mount.h mqueue.h msg.h msgbuf.h mtio.h mutex.h \
	namei.h null.h \
	param.h pipe.h pmc.h poll.h pool.h power.h proc.h \
	protosw.h ptrace.h queue.h \
	ras.h reboot.h radioio.h resource.h resourcevar.h rmd160.h rnd.h rwlock.h \
	scanio.h sched.h scsiio.h select.h selinfo.h sem.h sha1.h sha2.h \
	shm.h siginfo.h signal.h signalvar.h sigtypes.h simplelock.h \
	sleepq.h socket.h \
	socketvar.h sockio.h specificdata.h stat.h statvfs.h \
	syscall.h syscallargs.h sysctl.h stdint.h swap.h \
	syncobj.h syslimits.h syslog.h \
	tape.h termios.h time.h timeb.h timepps.h times.h \
	timex.h trace.h tree.h tty.h ttychars.h ttycom.h \
	ttydefaults.h ttydev.h types.h \
	ucontext.h ucred.h uio.h un.h unistd.h unpcb.h user.h utsname.h uuid.h \
	vadvise.h verified_exec.h vmmeter.h vnode.h vnode_if.h \
	wait.h wdog.h

INCSYMLINKS=\
	sys/exec_elf.h /usr/include/elf.h \
	sys/fcntl.h /usr/include/fcntl.h \
	sys/poll.h /usr/include/poll.h \
	sys/stdint.h /usr/include/stdint.h \
	sys/syslog.h /usr/include/syslog.h \
	sys/termios.h /usr/include/termios.h \
	sys/rmd160.h /usr/include/rmd160.h \
	sys/sha1.h /usr/include/sha1.h \
	sys/sha2.h /usr/include/sha2.h \
	sys/md4.h /usr/include/md4.h \
	sys/md5.h /usr/include/md5.h

INCSYMLINKS+=	../soundcard.h ${INCSDIR}/soundcard.h

namei: namei.src gennameih.awk
	${AWK} -f gennameih.awk < namei.src > namei.h

.include <bsd.kinc.mk>
