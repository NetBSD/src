/*	$NetBSD: subr_copy.c,v 1.15 2022/02/11 17:53:28 riastradh Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999, 2002, 2007, 2008, 2019
 *	The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_subr.c	8.4 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_copy.c,v 1.15 2022/02/11 17:53:28 riastradh Exp $");

#define	__UFETCHSTORE_PRIVATE
#define	__UCAS_PRIVATE

#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

void
uio_setup_sysspace(struct uio *uio)
{

	uio->uio_vmspace = vmspace_kernel();
}

int
uiomove(void *buf, size_t n, struct uio *uio)
{
	struct vmspace *vm = uio->uio_vmspace;
	struct iovec *iov;
	size_t cnt;
	int error = 0;
	char *cp = buf;

	ASSERT_SLEEPABLE();

	KASSERT(uio->uio_rw == UIO_READ || uio->uio_rw == UIO_WRITE);
	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			KASSERT(uio->uio_iovcnt > 0);
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;
		if (!VMSPACE_IS_KERNEL_P(vm)) {
			preempt_point();
		}

		if (uio->uio_rw == UIO_READ) {
			error = copyout_vmspace(vm, cp, iov->iov_base,
			    cnt);
		} else {
			error = copyin_vmspace(vm, iov->iov_base, cp,
			    cnt);
		}
		if (error) {
			break;
		}
		iov->iov_base = (char *)iov->iov_base + cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		KDASSERT(cnt <= n);
		n -= cnt;
	}

	return (error);
}

/*
 * Wrapper for uiomove() that validates the arguments against a known-good
 * kernel buffer.
 */
int
uiomove_frombuf(void *buf, size_t buflen, struct uio *uio)
{
	size_t offset;

	if (uio->uio_offset < 0 || /* uio->uio_resid < 0 || */
	    (offset = uio->uio_offset) != uio->uio_offset)
		return (EINVAL);
	if (offset >= buflen)
		return (0);
	return (uiomove((char *)buf + offset, buflen - offset, uio));
}

/*
 * Give next character to user as result of read.
 */
int
ureadc(int c, struct uio *uio)
{
	struct iovec *iov;

	if (uio->uio_resid <= 0)
		panic("ureadc: non-positive resid");
again:
	if (uio->uio_iovcnt <= 0)
		panic("ureadc: non-positive iovcnt");
	iov = uio->uio_iov;
	if (iov->iov_len <= 0) {
		uio->uio_iovcnt--;
		uio->uio_iov++;
		goto again;
	}
	if (!VMSPACE_IS_KERNEL_P(uio->uio_vmspace)) {
		int error;
		if ((error = ustore_char(iov->iov_base, c)) != 0)
			return (error);
	} else {
		*(char *)iov->iov_base = c;
	}
	iov->iov_base = (char *)iov->iov_base + 1;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (0);
}

/*
 * Like copyin(), but operates on an arbitrary vmspace.
 */
int
copyin_vmspace(struct vmspace *vm, const void *uaddr, void *kaddr, size_t len)
{
	struct iovec iov;
	struct uio uio;
	int error;

	if (len == 0)
		return (0);

	if (VMSPACE_IS_KERNEL_P(vm)) {
		return kcopy(uaddr, kaddr, len);
	}
	if (__predict_true(vm == curproc->p_vmspace)) {
		return copyin(uaddr, kaddr, len);
	}

	iov.iov_base = kaddr;
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = (off_t)(uintptr_t)uaddr;
	uio.uio_resid = len;
	uio.uio_rw = UIO_READ;
	UIO_SETUP_SYSSPACE(&uio);
	error = uvm_io(&vm->vm_map, &uio, 0);

	return (error);
}

/*
 * Like copyout(), but operates on an arbitrary vmspace.
 */
int
copyout_vmspace(struct vmspace *vm, const void *kaddr, void *uaddr, size_t len)
{
	struct iovec iov;
	struct uio uio;
	int error;

	if (len == 0)
		return (0);

	if (VMSPACE_IS_KERNEL_P(vm)) {
		return kcopy(kaddr, uaddr, len);
	}
	if (__predict_true(vm == curproc->p_vmspace)) {
		return copyout(kaddr, uaddr, len);
	}

	iov.iov_base = __UNCONST(kaddr); /* XXXUNCONST cast away const */
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = (off_t)(uintptr_t)uaddr;
	uio.uio_resid = len;
	uio.uio_rw = UIO_WRITE;
	UIO_SETUP_SYSSPACE(&uio);
	error = uvm_io(&vm->vm_map, &uio, 0);

	return (error);
}

/*
 * Like copyin(), but operates on an arbitrary process.
 */
int
copyin_proc(struct proc *p, const void *uaddr, void *kaddr, size_t len)
{
	struct vmspace *vm;
	int error;

	error = proc_vmspace_getref(p, &vm);
	if (error) {
		return error;
	}
	error = copyin_vmspace(vm, uaddr, kaddr, len);
	uvmspace_free(vm);

	return error;
}

/*
 * Like copyout(), but operates on an arbitrary process.
 */
int
copyout_proc(struct proc *p, const void *kaddr, void *uaddr, size_t len)
{
	struct vmspace *vm;
	int error;

	error = proc_vmspace_getref(p, &vm);
	if (error) {
		return error;
	}
	error = copyout_vmspace(vm, kaddr, uaddr, len);
	uvmspace_free(vm);

	return error;
}

/*
 * Like copyin(), but operates on an arbitrary pid.
 */
int
copyin_pid(pid_t pid, const void *uaddr, void *kaddr, size_t len)
{
	struct proc *p;
	struct vmspace *vm;
	int error;

	mutex_enter(&proc_lock);
	p = proc_find(pid);
	if (p == NULL) {
		mutex_exit(&proc_lock);
		return ESRCH;
	}
	mutex_enter(p->p_lock);
	error = proc_vmspace_getref(p, &vm);
	mutex_exit(p->p_lock);
	mutex_exit(&proc_lock);

	if (error == 0) {
		error = copyin_vmspace(vm, uaddr, kaddr, len);
		uvmspace_free(vm);
	}
	return error;
}

/*
 * Like copyin(), except it operates on kernel addresses when the FKIOCTL
 * flag is passed in `ioctlflags' from the ioctl call.
 */
int
ioctl_copyin(int ioctlflags, const void *src, void *dst, size_t len)
{
	if (ioctlflags & FKIOCTL)
		return kcopy(src, dst, len);
	return copyin(src, dst, len);
}

/*
 * Like copyout(), except it operates on kernel addresses when the FKIOCTL
 * flag is passed in `ioctlflags' from the ioctl call.
 */
int
ioctl_copyout(int ioctlflags, const void *src, void *dst, size_t len)
{
	if (ioctlflags & FKIOCTL)
		return kcopy(src, dst, len);
	return copyout(src, dst, len);
}

/*
 * User-space CAS / fetch / store
 */

#ifdef __NO_STRICT_ALIGNMENT
#define	CHECK_ALIGNMENT(x)	__nothing
#else /* ! __NO_STRICT_ALIGNMENT */
static bool
ufetchstore_aligned(uintptr_t uaddr, size_t size)
{
	return (uaddr & (size - 1)) == 0;
}

#define	CHECK_ALIGNMENT()						\
do {									\
	if (!ufetchstore_aligned((uintptr_t)uaddr, sizeof(*uaddr)))	\
		return EFAULT;						\
} while (/*CONSTCOND*/0)
#endif /* __NO_STRICT_ALIGNMENT */

/*
 * __HAVE_UCAS_FULL platforms provide _ucas_32() and _ucas_64() themselves.
 * _RUMPKERNEL also provides it's own _ucas_32() and _ucas_64().
 *
 * In all other cases, we provide generic implementations that work on
 * all platforms.
 */

#if !defined(__HAVE_UCAS_FULL) && !defined(_RUMPKERNEL)
#if !defined(__HAVE_UCAS_MP) && defined(MULTIPROCESSOR)
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/once.h>
#include <sys/mutex.h>
#include <sys/ipi.h>

static int ucas_critical_splcookie;
static volatile u_int ucas_critical_pausing_cpus;
static u_int ucas_critical_ipi;
static ONCE_DECL(ucas_critical_init_once)

static void
ucas_critical_cpu_gate(void *arg __unused)
{
	int count = SPINLOCK_BACKOFF_MIN;

	KASSERT(atomic_load_relaxed(&ucas_critical_pausing_cpus) > 0);

	/*
	 * Notify ucas_critical_wait that we have stopped.  Using
	 * store-release ensures all our memory operations up to the
	 * IPI happen before the ucas -- no buffered stores on our end
	 * can clobber it later on, for instance.
	 *
	 * Matches atomic_load_acquire in ucas_critical_wait -- turns
	 * the following atomic_dec_uint into a store-release.
	 */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_exit();
#endif
	atomic_dec_uint(&ucas_critical_pausing_cpus);

	/*
	 * Wait for ucas_critical_exit to reopen the gate and let us
	 * proceed.  Using a load-acquire ensures the ucas happens
	 * before any of our memory operations when we return from the
	 * IPI and proceed -- we won't observe any stale cached value
	 * that the ucas overwrote, for instance.
	 *
	 * Matches atomic_store_release in ucas_critical_exit.
	 */
	while (atomic_load_acquire(&ucas_critical_pausing_cpus) != (u_int)-1) {
		SPINLOCK_BACKOFF(count);
	}
}

static int
ucas_critical_init(void)
{

	ucas_critical_ipi = ipi_register(ucas_critical_cpu_gate, NULL);
	return 0;
}

static void
ucas_critical_wait(void)
{
	int count = SPINLOCK_BACKOFF_MIN;

	/*
	 * Wait for all CPUs to stop at the gate.  Using a load-acquire
	 * ensures all memory operations before they stop at the gate
	 * happen before the ucas -- no buffered stores in other CPUs
	 * can clobber it later on, for instance.
	 *
	 * Matches membar_exit/atomic_dec_uint (store-release) in
	 * ucas_critical_cpu_gate.
	 */
	while (atomic_load_acquire(&ucas_critical_pausing_cpus) > 0) {
		SPINLOCK_BACKOFF(count);
	}
}
#endif /* ! __HAVE_UCAS_MP && MULTIPROCESSOR */

static inline void
ucas_critical_enter(lwp_t * const l)
{

#if !defined(__HAVE_UCAS_MP) && defined(MULTIPROCESSOR)
	if (ncpu > 1) {
		RUN_ONCE(&ucas_critical_init_once, ucas_critical_init);

		/*
		 * Acquire the mutex first, then go to splhigh() and
		 * broadcast the IPI to lock all of the other CPUs
		 * behind the gate.
		 *
		 * N.B. Going to splhigh() implicitly disables preemption,
		 * so there's no need to do it explicitly.
		 */
		mutex_enter(&cpu_lock);
		ucas_critical_splcookie = splhigh();
		ucas_critical_pausing_cpus = ncpu - 1;
		ipi_trigger_broadcast(ucas_critical_ipi, true);
		ucas_critical_wait();
		return;
	}
#endif /* ! __HAVE_UCAS_MP && MULTIPROCESSOR */

	KPREEMPT_DISABLE(l);
}

static inline void
ucas_critical_exit(lwp_t * const l)
{

#if !defined(__HAVE_UCAS_MP) && defined(MULTIPROCESSOR)
	if (ncpu > 1) {
		/*
		 * Open the gate and notify all CPUs in
		 * ucas_critical_cpu_gate that they can now proceed.
		 * Using a store-release ensures the ucas happens
		 * before any memory operations they issue after the
		 * IPI -- they won't observe any stale cache of the
		 * target word, for instance.
		 *
		 * Matches atomic_load_acquire in ucas_critical_cpu_gate.
		 */
		atomic_store_release(&ucas_critical_pausing_cpus, (u_int)-1);
		splx(ucas_critical_splcookie);
		mutex_exit(&cpu_lock);
		return;
	}
#endif /* ! __HAVE_UCAS_MP && MULTIPROCESSOR */

	KPREEMPT_ENABLE(l);
}

int
_ucas_32(volatile uint32_t *uaddr, uint32_t old, uint32_t new, uint32_t *ret)
{
	lwp_t * const l = curlwp;
	uint32_t *uva = ((void *)(uintptr_t)uaddr);
	int error;

	/*
	 * Wire the user address down to avoid taking a page fault during
	 * the critical section.
	 */
	error = uvm_vslock(l->l_proc->p_vmspace, uva, sizeof(*uaddr),
			   VM_PROT_READ | VM_PROT_WRITE);
	if (error)
		return error;

	ucas_critical_enter(l);
	error = _ufetch_32(uva, ret);
	if (error == 0 && *ret == old) {
		error = _ustore_32(uva, new);
	}
	ucas_critical_exit(l);

	uvm_vsunlock(l->l_proc->p_vmspace, uva, sizeof(*uaddr));

	return error;
}

#ifdef _LP64
int
_ucas_64(volatile uint64_t *uaddr, uint64_t old, uint64_t new, uint64_t *ret)
{
	lwp_t * const l = curlwp;
	uint64_t *uva = ((void *)(uintptr_t)uaddr);
	int error;

	/*
	 * Wire the user address down to avoid taking a page fault during
	 * the critical section.
	 */
	error = uvm_vslock(l->l_proc->p_vmspace, uva, sizeof(*uaddr),
			   VM_PROT_READ | VM_PROT_WRITE);
	if (error)
		return error;

	ucas_critical_enter(l);
	error = _ufetch_64(uva, ret);
	if (error == 0 && *ret == old) {
		error = _ustore_64(uva, new);
	}
	ucas_critical_exit(l);

	uvm_vsunlock(l->l_proc->p_vmspace, uva, sizeof(*uaddr));

	return error;
}
#endif /* _LP64 */
#endif /* ! __HAVE_UCAS_FULL && ! _RUMPKERNEL */

int
ucas_32(volatile uint32_t *uaddr, uint32_t old, uint32_t new, uint32_t *ret)
{

	ASSERT_SLEEPABLE();
	CHECK_ALIGNMENT();
#if (defined(__HAVE_UCAS_MP) && defined(MULTIPROCESSOR)) && \
    !defined(_RUMPKERNEL)
	if (ncpu > 1) {
		return _ucas_32_mp(uaddr, old, new, ret);
	}
#endif /* __HAVE_UCAS_MP && MULTIPROCESSOR */
	return _ucas_32(uaddr, old, new, ret);
}

#ifdef _LP64
int
ucas_64(volatile uint64_t *uaddr, uint64_t old, uint64_t new, uint64_t *ret)
{

	ASSERT_SLEEPABLE();
	CHECK_ALIGNMENT();
#if (defined(__HAVE_UCAS_MP) && defined(MULTIPROCESSOR)) && \
    !defined(_RUMPKERNEL)
	if (ncpu > 1) {
		return _ucas_64_mp(uaddr, old, new, ret);
	}
#endif /* __HAVE_UCAS_MP && MULTIPROCESSOR */
	return _ucas_64(uaddr, old, new, ret);
}
#endif /* _LP64 */

__strong_alias(ucas_int,ucas_32);
#ifdef _LP64
__strong_alias(ucas_ptr,ucas_64);
#else
__strong_alias(ucas_ptr,ucas_32);
#endif /* _LP64 */

int
ufetch_8(const uint8_t *uaddr, uint8_t *valp)
{

	ASSERT_SLEEPABLE();
	CHECK_ALIGNMENT();
	return _ufetch_8(uaddr, valp);
}

int
ufetch_16(const uint16_t *uaddr, uint16_t *valp)
{

	ASSERT_SLEEPABLE();
	CHECK_ALIGNMENT();
	return _ufetch_16(uaddr, valp);
}

int
ufetch_32(const uint32_t *uaddr, uint32_t *valp)
{

	ASSERT_SLEEPABLE();
	CHECK_ALIGNMENT();
	return _ufetch_32(uaddr, valp);
}

#ifdef _LP64
int
ufetch_64(const uint64_t *uaddr, uint64_t *valp)
{

	ASSERT_SLEEPABLE();
	CHECK_ALIGNMENT();
	return _ufetch_64(uaddr, valp);
}
#endif /* _LP64 */

__strong_alias(ufetch_char,ufetch_8);
__strong_alias(ufetch_short,ufetch_16);
__strong_alias(ufetch_int,ufetch_32);
#ifdef _LP64
__strong_alias(ufetch_long,ufetch_64);
__strong_alias(ufetch_ptr,ufetch_64);
#else
__strong_alias(ufetch_long,ufetch_32);
__strong_alias(ufetch_ptr,ufetch_32);
#endif /* _LP64 */

int
ustore_8(uint8_t *uaddr, uint8_t val)
{

	ASSERT_SLEEPABLE();
	CHECK_ALIGNMENT();
	return _ustore_8(uaddr, val);
}

int
ustore_16(uint16_t *uaddr, uint16_t val)
{

	ASSERT_SLEEPABLE();
	CHECK_ALIGNMENT();
	return _ustore_16(uaddr, val);
}

int
ustore_32(uint32_t *uaddr, uint32_t val)
{

	ASSERT_SLEEPABLE();
	CHECK_ALIGNMENT();
	return _ustore_32(uaddr, val);
}

#ifdef _LP64
int
ustore_64(uint64_t *uaddr, uint64_t val)
{

	ASSERT_SLEEPABLE();
	CHECK_ALIGNMENT();
	return _ustore_64(uaddr, val);
}
#endif /* _LP64 */

__strong_alias(ustore_char,ustore_8);
__strong_alias(ustore_short,ustore_16);
__strong_alias(ustore_int,ustore_32);
#ifdef _LP64
__strong_alias(ustore_long,ustore_64);
__strong_alias(ustore_ptr,ustore_64);
#else
__strong_alias(ustore_long,ustore_32);
__strong_alias(ustore_ptr,ustore_32);
#endif /* _LP64 */
