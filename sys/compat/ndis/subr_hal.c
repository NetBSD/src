/*-
 * Copyright (c) 2003
 *	Bill Paul <wpaul@windriver.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/sys/compat/ndis/subr_hal.c,v 1.13.2.3 2005/03/31 04:24:35 wpaul Exp $");
#endif
#ifdef __NetBSD__
__KERNEL_RCSID(0, "$NetBSD: subr_hal.c,v 1.7.12.1 2012/10/30 17:20:46 yamt Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>

#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#ifdef __FreeBSD__
#include <sys/mutex.h>
#endif
#include <sys/proc.h>
#include <sys/sched.h>
#ifdef __FreeBSD__
#include <sys/module.h>
#endif

#include <sys/systm.h>
#ifdef __FreeBSD__
#include <machine/clock.h>
#include <machine/bus_memio.h>
#include <machine/bus_pio.h>
#endif
#include <sys/bus.h>

#ifdef __FreeBSD__
#include <sys/bus.h>
#include <sys/rman.h>
#endif

#include <compat/ndis/pe_var.h>
#include <compat/ndis/ntoskrnl_var.h>
#include <compat/ndis/hal_var.h>

__stdcall static void KeStallExecutionProcessor(uint32_t);
__stdcall static void WRITE_PORT_BUFFER_ULONG(uint32_t *,
	uint32_t *, uint32_t);
__stdcall static void WRITE_PORT_BUFFER_USHORT(uint16_t *,
	uint16_t *, uint32_t);
__stdcall static void WRITE_PORT_BUFFER_UCHAR(uint8_t *,
	uint8_t *, uint32_t);
__stdcall static void WRITE_PORT_ULONG(uint32_t *, uint32_t);
__stdcall static void WRITE_PORT_USHORT(uint16_t *, uint16_t);
__stdcall static void WRITE_PORT_UCHAR(uint8_t *, uint8_t);
__stdcall static uint32_t READ_PORT_ULONG(uint32_t *);
__stdcall static uint16_t READ_PORT_USHORT(uint16_t *);
__stdcall static uint8_t READ_PORT_UCHAR(uint8_t *);
__stdcall static void READ_PORT_BUFFER_ULONG(uint32_t *,
	uint32_t *, uint32_t);
__stdcall static void READ_PORT_BUFFER_USHORT(uint16_t *,
	uint16_t *, uint32_t);
__stdcall static void READ_PORT_BUFFER_UCHAR(uint8_t *,
	uint8_t *, uint32_t);
__stdcall static uint64_t KeQueryPerformanceCounter(uint64_t *);
__stdcall static void dummy (void);

extern struct mtx_pool *ndis_mtxpool;

#ifdef __NetBSD__
int win_irql;
#endif

int
hal_libinit(void)
{
	image_patch_table	*patch;

	patch = hal_functbl;
	while (patch->ipt_func != NULL) {
		windrv_wrap((funcptr)patch->ipt_func,
		    (funcptr *)&patch->ipt_wrap);
		patch++;
	}

	return(0);
}

int
hal_libfini(void)
{
	image_patch_table	*patch;

	patch = hal_functbl;
	while (patch->ipt_func != NULL) {
		windrv_unwrap(patch->ipt_wrap);
		patch++;
	}

	return(0);
}

__stdcall static void
KeStallExecutionProcessor(uint32_t usecs)
{
	DELAY(usecs);
	return;
}

__stdcall static void
WRITE_PORT_ULONG(uint32_t *port, uint32_t val)
{
	bus_space_write_4(NDIS_BUS_SPACE_IO, 0x0, (bus_size_t)port, val);
	return;
}

__stdcall static void
WRITE_PORT_USHORT(uint16_t *port, uint16_t val)
{
	bus_space_write_2(NDIS_BUS_SPACE_IO, 0x0, (bus_size_t)port, val);
	return;
}

__stdcall static void
WRITE_PORT_UCHAR(uint8_t *port, uint8_t val)
{
	bus_space_write_1(NDIS_BUS_SPACE_IO, 0x0, (bus_size_t)port, val);
	return;
}

__stdcall static void
WRITE_PORT_BUFFER_ULONG(uint32_t *port, uint32_t *val, uint32_t cnt)
{
	bus_space_write_multi_4(NDIS_BUS_SPACE_IO, 0x0,
	    (bus_size_t)port, val, cnt);
	return;
}

__stdcall static void
WRITE_PORT_BUFFER_USHORT(uint16_t *port, uint16_t *val, uint32_t cnt)
{
	bus_space_write_multi_2(NDIS_BUS_SPACE_IO, 0x0,
	    (bus_size_t)port, val, cnt);
	return;
}

__stdcall static void
WRITE_PORT_BUFFER_UCHAR(uint8_t *port, uint8_t *val, uint32_t cnt)
{
	bus_space_write_multi_1(NDIS_BUS_SPACE_IO, 0x0,
	    (bus_size_t)port, val, cnt);
	return;
}

__stdcall static uint16_t
READ_PORT_USHORT(uint16_t *port)
{
	return(bus_space_read_2(NDIS_BUS_SPACE_IO, 0x0, (bus_size_t)port));
}

__stdcall static uint32_t
READ_PORT_ULONG(uint32_t *port)
{
	return(bus_space_read_4(NDIS_BUS_SPACE_IO, 0x0, (bus_size_t)port));
}

__stdcall static uint8_t
READ_PORT_UCHAR(uint8_t *port)
{
	return(bus_space_read_1(NDIS_BUS_SPACE_IO, 0x0, (bus_size_t)port));
}

__stdcall static void
READ_PORT_BUFFER_ULONG(uint32_t *port, uint32_t *val, uint32_t cnt)
{
	bus_space_read_multi_4(NDIS_BUS_SPACE_IO, 0x0,
	    (bus_size_t)port, val, cnt);
	return;
}

__stdcall static void
READ_PORT_BUFFER_USHORT(uint16_t *port, uint16_t *val, uint32_t cnt)
{
	bus_space_read_multi_2(NDIS_BUS_SPACE_IO, 0x0,
	    (bus_size_t)port, val, cnt);
	return;
}

__stdcall static void
READ_PORT_BUFFER_UCHAR(uint8_t *port, uint8_t *val, uint32_t cnt)
{
	bus_space_read_multi_1(NDIS_BUS_SPACE_IO, 0x0,
	    (bus_size_t)port, val, cnt);
	return;
}

/*
 * The spinlock implementation in Windows differs from that of FreeBSD.
 * The basic operation of spinlocks involves two steps: 1) spin in a
 * tight loop while trying to acquire a lock, 2) after obtaining the
 * lock, disable preemption. (Note that on uniprocessor systems, you're
 * allowed to skip the first step and just lock out pre-emption, since
 * it's not possible for you to be in contention with another running
 * thread.) Later, you release the lock then re-enable preemption.
 * The difference between Windows and FreeBSD lies in how preemption
 * is disabled. In FreeBSD, it's done using critical_enter(), which on
 * the x86 arch translates to a cli instruction. This masks off all
 * interrupts, and effectively stops the scheduler from ever running
 * so _nothing_ can execute except the current thread. In Windows,
 * preemption is disabled by raising the processor IRQL to DISPATCH_LEVEL.
 * This stops other threads from running, but does _not_ block device
 * interrupts. This means ISRs can still run, and they can make other
 * threads runable, but those other threads won't be able to execute
 * until the current thread lowers the IRQL to something less than
 * DISPATCH_LEVEL.
 *
 * There's another commonly used IRQL in Windows, which is APC_LEVEL.
 * An APC is an Asynchronous Procedure Call, which differs from a DPC
 * (Defered Procedure Call) in that a DPC is queued up to run in
 * another thread, while an APC runs in the thread that scheduled
 * it (similar to a signal handler in a UNIX process). We don't
 * actually support the notion of APCs in FreeBSD, so for now, the
 * only IRQLs we're interested in are DISPATCH_LEVEL and PASSIVE_LEVEL.
 *
 * To simulate DISPATCH_LEVEL, we raise the current thread's priority
 * to PI_REALTIME, which is the highest we can give it. This should,
 * if I understand things correctly, prevent anything except for an
 * interrupt thread from preempting us. PASSIVE_LEVEL is basically
 * everything else.
 *
 * Be aware that, at least on the x86 arch, the Windows spinlock
 * functions are divided up in peculiar ways. The actual spinlock
 * functions are KfAcquireSpinLock() and KfReleaseSpinLock(), and
 * they live in HAL.dll. Meanwhile, KeInitializeSpinLock(),
 * KefAcquireSpinLockAtDpcLevel() and KefReleaseSpinLockFromDpcLevel()
 * live in ntoskrnl.exe. Most Windows source code will call
 * KeAcquireSpinLock() and KeReleaseSpinLock(), but these are just
 * macros that call KfAcquireSpinLock() and KfReleaseSpinLock().
 * KefAcquireSpinLockAtDpcLevel() and KefReleaseSpinLockFromDpcLevel()
 * perform the lock aquisition/release functions without doing the
 * IRQL manipulation, and are used when one is already running at
 * DISPATCH_LEVEL. Make sense? Good.
 *
 * According to the Microsoft documentation, any thread that calls
 * KeAcquireSpinLock() must be running at IRQL <= DISPATCH_LEVEL. If
 * we detect someone trying to acquire a spinlock from DEVICE_LEVEL
 * or HIGH_LEVEL, we panic.
 */

__fastcall uint8_t
KfAcquireSpinLock(REGARGS1(kspin_lock *lock))
{
	uint8_t			oldirql;

	/* I am so going to hell for this. */
	if (KeGetCurrentIrql() > DISPATCH_LEVEL)
		panic("IRQL_NOT_LESS_THAN_OR_EQUAL");

	oldirql = KeRaiseIrql(DISPATCH_LEVEL);
	KeAcquireSpinLockAtDpcLevel(lock);

	return(oldirql);
}

__fastcall void
KfReleaseSpinLock(REGARGS2(kspin_lock *lock, uint8_t newirql))
{
	KeReleaseSpinLockFromDpcLevel(lock);
	KeLowerIrql(newirql);

	return;
}

__stdcall uint8_t
KeGetCurrentIrql(void)
{
	if (AT_DISPATCH_LEVEL(curthread))
		return(DISPATCH_LEVEL);
	return(PASSIVE_LEVEL);
}

__stdcall static uint64_t
KeQueryPerformanceCounter(uint64_t *freq)
{
	if (freq != NULL)
		*freq = hz;

	return((uint64_t)ticks);
}


static int old_ipl;
static int ipl_raised = FALSE;

__fastcall uint8_t
KfRaiseIrql(REGARGS1(uint8_t irql))
{
	uint8_t			oldirql = 0;
//#ifdef __NetBSD__
//	uint8_t			s;
//#endif

	if (irql < KeGetCurrentIrql())
		panic("IRQL_NOT_LESS_THAN");

	if (KeGetCurrentIrql() == DISPATCH_LEVEL)
		return(DISPATCH_LEVEL);
#ifdef __NetBSD__
	if(irql >= DISPATCH_LEVEL && !ipl_raised) {
		old_ipl = splsoftclock();
		ipl_raised = TRUE;
		oldirql = win_irql;
		win_irql = irql;
	}
#else /* __FreeBSD__ */
	mtx_lock_spin(&sched_lock);
	oldirql = curthread->td_base_pri;
	sched_prio(curthread, PI_REALTIME);
#if __FreeBSD_version < 600000
	curthread->td_base_pri = PI_REALTIME;
#endif
	mtx_unlock_spin(&sched_lock);
#endif /* __FreeBSD__ */

	return(oldirql);
}

__fastcall void 
KfLowerIrql(REGARGS1(uint8_t oldirql))
{
//#ifdef __NetBSD__
//	uint8_t		s;
//#endif

	if (oldirql == DISPATCH_LEVEL)
		return;

#ifdef __FreeBSD__
	if (KeGetCurrentIrql() != DISPATCH_LEVEL)
		panic("IRQL_NOT_GREATER_THAN");
#else /* __NetBSD__ */
	if (KeGetCurrentIrql() < oldirql)
		panic("IRQL_NOT_GREATER_THAN");	
#endif

#ifdef __NetBSD__
	if(oldirql < DISPATCH_LEVEL && ipl_raised) {
		splx(old_ipl);
		ipl_raised = FALSE;
		win_irql = oldirql;
	}
#else
	mtx_lock_spin(&sched_lock);
#if __FreeBSD_version < 600000
	curthread->td_base_pri = oldirql;
#endif
	sched_prio(curthread, oldirql);
	mtx_unlock_spin(&sched_lock);
#endif /* __NetBSD__ */

	return;
}

__stdcall
static void dummy(void)
{
	printf ("hal dummy called...\n");
	return;
}

image_patch_table hal_functbl[] = {
	IMPORT_FUNC(KeStallExecutionProcessor),
	IMPORT_FUNC(WRITE_PORT_ULONG),
	IMPORT_FUNC(WRITE_PORT_USHORT),
	IMPORT_FUNC(WRITE_PORT_UCHAR),
	IMPORT_FUNC(WRITE_PORT_BUFFER_ULONG),
	IMPORT_FUNC(WRITE_PORT_BUFFER_USHORT),
	IMPORT_FUNC(WRITE_PORT_BUFFER_UCHAR),
	IMPORT_FUNC(READ_PORT_ULONG),
	IMPORT_FUNC(READ_PORT_USHORT),
	IMPORT_FUNC(READ_PORT_UCHAR),
	IMPORT_FUNC(READ_PORT_BUFFER_ULONG),
	IMPORT_FUNC(READ_PORT_BUFFER_USHORT),
	IMPORT_FUNC(READ_PORT_BUFFER_UCHAR),
	IMPORT_FUNC(KfAcquireSpinLock),
	IMPORT_FUNC(KfReleaseSpinLock),
	IMPORT_FUNC(KeGetCurrentIrql),
	IMPORT_FUNC(KeQueryPerformanceCounter),
	IMPORT_FUNC(KfLowerIrql),
	IMPORT_FUNC(KfRaiseIrql),

	/*
	 * This last entry is a catch-all for any function we haven't
	 * implemented yet. The PE import list patching routine will
	 * use it for any function that doesn't have an explicit match
	 * in this table.
	 */

	{ NULL, (FUNC)dummy, NULL },

	/* End of list. */

	{ NULL, NULL, NULL }
};
