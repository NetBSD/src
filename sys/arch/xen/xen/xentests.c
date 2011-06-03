/* $NetBSD: xentests.c,v 1.1.2.1 2011/06/03 13:27:42 cherry Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cherry G. Mathew <cherry@NetBSD.org>
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: xentests.c,v 1.1.2.1 2011/06/03 13:27:42 cherry Exp $");

#include <sys/types.h>

#include <sys/cpu.h>
#include <sys/callout.h>
#include <sys/condvar.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/xcall.h>

#include <uvm/uvm.h>
#include <machine/pmap.h>

enum {
	TNAMEMAX = 100 /* Totally arbitrary */
};

struct xen_ipi_test_payload {
	const union {
		int id;
	} in;
	struct {
		bool testresult;
		char testname[TNAMEMAX];
	} out;
};

struct xen_ipi_test {
	bool (*tfunc)(void); /* Pointer to test function */
	const char *tname;   /* Test name */
};

static volatile uint32_t xc_sentinel = 0;

static void
xc_func(void *citarg, void *dummy2)
{
	struct cpu_info *ci = curcpu();
	
	printf("xc recieved on %s for %s\n",
	       cpu_name(ci),
	       cpu_name(citarg));

	if (ci != citarg) {
		printf("xcall recieved on wrong CPU!\n");
		return;
	}

	atomic_or_32(&xc_sentinel, (1 << ci->ci_cpuid));
}

static bool
xc_send(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci, *self = curcpu();
	uint64_t xc_tkt;

	printf("entered \n");
	/* Do not send to self and defunct CPUs */
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == NULL) {
			continue;
		}
		if(ci->ci_data.cpu_idlelwp == NULL ||
		   (ci->ci_flags & CPUF_PRESENT) == 0 ||
		   ci == self) {
			continue;
		}

		/* Reset sentinel*/
		xc_sentinel = 0;

		/* Normal priority xcall. */
		xc_tkt = xc_unicast(0, xc_func, ci, NULL, ci);
		xc_wait(xc_tkt); /* wait for ack and return true. */
		if (!(xc_sentinel & (1 << ci->ci_cpuid))) {
			return false;
		}

	}

	return true;
}

static bool
xc_send_high(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci, *self = curcpu();
	uint64_t xc_tkt;

	printf("high entered \n");
	/* Do not send to self and defunct CPUs */
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == NULL) {
			continue;
		}
		if(ci->ci_data.cpu_idlelwp == NULL ||
		   (ci->ci_flags & CPUF_PRESENT) == 0 ||
		   ci == self) {
			continue;
		}

		/* Reset sentinel*/
		xc_sentinel = 0;

		/* High priority first. */

		xc_tkt = xc_unicast(XC_HIGHPRI, xc_func, ci, NULL, ci);

		xc_wait(xc_tkt); /* wait for ack and return true. */

		if ((!xc_sentinel & (1 << ci->ci_cpuid))) {
			return false;
		}
		
	}

	return true;
}


/* callout(9) sanity check */

struct cb_struct {
	volatile bool bolt;
};
		

static void
cb_func(void *arg)
{
	struct cb_struct *cbs = arg;

	cbs->bolt = true;

	printf("callback ran successfully\n");
}

static bool
callout(void)
{
	int i;
	struct cb_struct cbs;
	callout_t *cp;


	cp = kmem_alloc(sizeof *cp, KM_SLEEP);
	if (!cp) {
		printf("kmem_alloc() failed\n");
		return false;
	}

	bool result = false;

	cbs.bolt = false;

	callout_init(cp, CALLOUT_MPSAFE);
	callout_setfunc(cp, cb_func, &cbs);

	/* Test for various states */
	callout_schedule(cp, 1);

	/* PENDING: XXX: Assumes we are run before the timer expires. */
	if (!callout_pending(cp)) {
		printf("callout not in pending state !\n");
		result = false;
		goto bailout;
	}

	i = 0;

	while(cbs.bolt == false) {

		if (i++ > hz * 1000) {  /* XXX: Not sure what a good timeout is */
			printf("number of yield()s allowed exceeded\n");
			result = false;
			goto bailout;
		}

		yield();
	}

	if (!callout_active(cp)) {
		printf("callout not active \n");
		result = false;
		goto bailout;
	}

	/* INVOKING */
	if (!callout_invoking(cp)) {
		printf("callout not invoking \n");
		result = false;
		goto bailout;
	}

	if (!callout_expired(cp)) {
		printf("callout not expired!\n");
		result = false;
		goto bailout;
	}

	result = true;
bailout:
	if (!callout_active(cp)) {
		callout_destroy(cp);
	}

	return result;
}

static struct xen_ipi_test tests[] = {
	{ xc_send, "cross call at normal priority" },
	{ xc_send_high, "cross call at high priority" },
	{ callout, "test the callout infrastructure" },
};

const size_t ntests = sizeof tests / sizeof tests[1];

static void
runtest(struct xen_ipi_test_payload *tp)
{
	int n;

	KASSERT(tp->in.id < ntests);

	printf("TEST description:: %s\n", tests[tp->in.id].tname);
	tp->out.testresult = tests[tp->in.id].tfunc();
	printf("%s: %s\n", tests[tp->in.id].tname,
	       tp->out.testresult ? "SUCCESS" :  "FAILURE");
	printf("DONE: %s\n", tests[tp->in.id].tname);

	n = snprintf(tp->out.testname, TNAMEMAX, "%s", tests[tp->in.id].tname);

	if (n >= 0) {
		memset(&tp->out.testname[n + 1], 0, TNAMEMAX - n - 1);
	}
	else {
		memset(tp->out.testname, 0, TNAMEMAX);
	}
}

/* 
 * sysctl(9) stuff 
 */

/* sysctl helper routine */

static int
sysctl_kern_xen_ipi_test(SYSCTLFN_ARGS)
{
	int error;
	struct xen_ipi_test_payload test;

	KASSERT(rnode != NULL);

	/* XXX: permission checks */

	if (strcmp(rnode->sysctl_name, "xen_ipi") == 0) {

		/* Tests don't want just lookups */
		if (newp == NULL || newlen != sizeof test ||
		    oldp == NULL || oldlenp == NULL) {
			return EINVAL;
		}

		error = sysctl_copyin(l, newp, &test, sizeof test);
		if (error) {
			return error;
		}

		if (test.in.id >= ntests) {
			aprint_debug("unknown test id %d requested\n", test.in.id);			
			return EINVAL;
		}

		runtest(&test);

		error = sysctl_copyout(l, &test, oldp, sizeof test);
		if (error) {
			return error;
		}

		*oldlenp = sizeof test;
		return 0;
	}

	return EINVAL;
}

/* Setup nodes. */
SYSCTL_SETUP(sysctl_kern_xen_ipi_test_setup, "sysctl machdep.tests.xen_ipi setup")
{
	const struct sysctlnode *node = NULL;
	struct xen_ipi_test_payload test_payload;

	memset(&test_payload, 0, sizeof test_payload);

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);

	if (node == NULL) {
		printf("sysctl create failed\n");
	}

	sysctl_createv(clog, 0, &node, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "tests",
	    SYSCTL_DESCR("kernel based tests"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_READWRITE | CTLFLAG_PRIVATE |
	    CTLFLAG_OWNDATA,
	    CTLTYPE_STRUCT, "xen_ipi",
	    SYSCTL_DESCR("Xen ipi kernel tests control interface "),
		sysctl_kern_xen_ipi_test, 0, &test_payload, sizeof(test_payload),
		CTL_CREATE, CTL_EOL);
}

/* 
 * Thread wrappers - we can't use kthread(9) because trap.c ignores
 * pcb_onfault on proc0
 */

/* 
 * Sets the page fault handler to fault_routine. To restore the
 * default handler, use prep_fault(NULL);
 * Returns the fault handler that has been set.
 */
static void *
prep_fault(void *fault_routine)
{
	struct lwp *l;
	struct pcb *pcb;
	static volatile bool once = false;
	static void *onfault_orig = NULL;
	void *prevfault = NULL;

        /* 
	 * onfault is a copy of the original handler. It is restored
	 * when fault_routine == NULL 
	 */

	l = curlwp;

	KASSERT(l != &lwp0);

	/* Setup pcb_onfault */
	pcb = lwp_getpcb(l);
	KASSERT(pcb != NULL);

	if (!once) {	/* Save original */
		onfault_orig = pcb->pcb_onfault;
		once = true;
	}

	prevfault = pcb->pcb_onfault;
	pcb->pcb_onfault = fault_routine ? fault_routine : onfault_orig;

	return prevfault;
}

static label_t sequel[MAXCPUS];

/* Fault handler, to test for legitimate page faults. */
static void
thread_pagefault(void)
{
	longjmp(&sequel[curcpu()->ci_cpuid]);
	panic("pagefault handler did not longjmp() !");
}

struct thread_arg {
	void (*func)(void *);
	void (*abort)(void *);
	void *arg;
};

struct thread_ctx {
	struct lwp *lwp;
	struct thread_arg ctx;
	bool landed; /* For setjmp */
};

/* Can only be called from own thread */

static void
thread_exit(struct thread_ctx *t)
{
	KASSERT(t != NULL);
	KASSERT(t->lwp == curlwp);

	lwp_exit(curlwp);

}

/* 
 * Same as thread_exit, but calls abort callback, if registered,
 * before exiting
 */
static void
thread_abort(struct thread_ctx *t)
{
	KASSERT(t != NULL);
	KASSERT(t->lwp == curlwp);

	if (t->ctx.abort != NULL) {
		t->ctx.abort(t->ctx.arg);
	}

	lwp_exit(curlwp);
}


static void
setjmp_tramp(void *arg)
{
	KASSERT(arg != NULL);

	struct lwp *l;
	struct thread_ctx *t = arg;

	l = curlwp;
	KASSERT(l == t->lwp);

	setjmp(&sequel[curcpu()->ci_cpuid]);
	if (t->landed == true) { /* 
			    * got here via longjmp() from fault
			    * routine.
			    */

		printf("caught exception\n");
		thread_abort(t);
	}
	t->landed = true;
	t->ctx.func(t->ctx.arg);
	thread_exit(t);
}

static struct thread_ctx *
thread_spawn(struct cpu_info *ci, 
	     void (*func)(void *),
	     void *arg,
	     void (*abort)(void *))
{
	struct lwp *l1;
	struct thread_ctx *t;

	vaddr_t uarea;

	KASSERT(ci != NULL);
	KASSERT(func != NULL);

	t = (struct thread_ctx *) uvm_km_alloc(kernel_map, 
					       sizeof *t, 0, UVM_KMF_WIRED);
	if (t == NULL) {
		return NULL;
	}

	t->ctx.func = func;
	t->ctx.arg = arg;
	if (abort != NULL) {
		t->ctx.abort = abort;
	}
	t->landed = false;

	uarea = uvm_uarea_system_alloc(); /* XXX: why system ? */

	if (uarea == 0) {
		return NULL;
	}

	if (lwp_create(curlwp, curlwp->l_proc, uarea,
		       0/*flags*/, NULL/*stack*/, 0/*stacksize*/,
		       setjmp_tramp, t, &t->lwp, SCHED_RR)) {
		uvm_uarea_system_free(uarea);
		return NULL;
	}

	l1 = t->lwp;
	KASSERT(l1 != NULL);

	mutex_enter(l1->l_proc->p_lock);
	lwp_lock(l1);
	l1->l_priority = PRI_KTHREAD;
	l1->l_stat = LSRUN;
	l1->l_flag = LW_AFFINITY;
	lwp_migrate(l1, ci); /* unlocks l1 */

	lwp_lock(l1);
	sched_enqueue(l1, false);
	lwp_unlock(l1);
	mutex_exit(l1->l_proc->p_lock);

	return t;
}

/* 
 * This function reaps the context memory, not thread_wait(); 
 * This makes it compulsory to use this function from the controlling
 * thread, to make sure memory is not leaked.
 *
 * This is slightly lame, but we're a testing framework, not a
 * threading library.
 */

static int
thread_wait(struct thread_ctx *ctx)
{

	int error;
	struct lwp *l;

	KASSERT(ctx != NULL);

	l = ctx->lwp;
	mutex_enter(l->l_proc->p_lock);
	error = lwp_wait1(curlwp, l->l_lid, NULL, LWPWAIT_EXITCONTROL);
	mutex_exit(l->l_proc->p_lock);

	if (!error) {
		uvm_km_free(kernel_map, (vaddr_t) ctx, sizeof *ctx,
			    UVM_KMF_WIRED);
	}
	return error;
}

/* Barrier stuff */

struct barrier {
	struct simplelock lock;
	size_t stile;
};

static void
vm_barrier_init(struct barrier *bt, size_t ncpus)
{
	KASSERT(bt != NULL);

	simple_lock_init(&bt->lock);
	bt->stile = ncpus;
}

static void
vm_barrier_destroy(struct barrier *bt)
{
	KASSERT(bt != NULL);

	bt->stile = 0;


	simple_lock_init(&bt->lock); /* reset */
}

static void
vm_barrier_reset(struct barrier *bt)
{
	bt->stile = 0;

	simple_lock_init(&bt->lock); /* reset */
}

static void
vm_barrier_hold(struct barrier *bt)
{
	size_t sem;

	KASSERT(bt != NULL);

	if (bt->stile == 0) { /* uninitialised/reset */
		printf("%s: bt->stile == 0\n", __func__);
		return;
	}

	simple_lock(&bt->lock);
	bt->stile--;
	simple_unlock(&bt->lock);

	do {
		simple_lock(&bt->lock);
		sem = bt->stile;
		simple_unlock(&bt->lock);
	} while (sem);
}

/* Pattern helpers. */
static void
pattern_write(vaddr_t addr, size_t len)
{
	vaddr_t p;
	for (p = addr; p < (addr + len); p++) {
		memset((void *)p, p % 255, 1);
	}
}

static bool
pattern_verify(vaddr_t addr, size_t len)
{
	vaddr_t p;
	int c;
	for (p = addr; p < (addr + len); p++) {
		c = 0;
		memcpy(&c, (void *)p, 1);
		if (c != (p % 255)) {
			return false;
		}
	}

	return true;
}


struct test_args {
	union {
		struct {
			vaddr_t addr;
			vaddr_t len;
		} mv;

		struct {
			pmap_t pmap;
			vaddr_t va;
			paddr_t pa;
		} pm;
	} p;

	struct barrier ready_bar; /* tests are all ready */
	struct barrier go_bar; /* Fire! */
	struct barrier done_bar;

	bool result[MAXCPUS];
};

/*
 * Kickoff spawns off multiple threads, one with "master" status, and
 * all the other with "slave" status. All threads fireup, and
 * synchronise at three barriers, namely, "ready", "go", and "done".
 * Each thread runs on a unique CPU, and is bound to it for the life
 * of the test.
 */

static size_t
system_ncpus(void)
{
	size_t ncpus = 0;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	for (CPU_INFO_FOREACH(cii, ci)) ncpus++;
	return ncpus;
}

typedef void (*callback_t)(void *);

static bool
test_kickoff(struct test_args *ta, 
	     callback_t setup,
	     callback_t master, 
	     callback_t slave,
	     callback_t cleanup)
{
	struct cpu_info *ci_writer, *ci_reader;
	CPU_INFO_ITERATOR cii_writer, cii_reader;
	cpuid_t cpuid_w, cpuid_r;
	size_t ncpus = 0;
	bool result = true;

	KASSERT(ta != NULL);
	KASSERT(master != NULL);
	KASSERT(slave != NULL);

	struct thread_ctx *lw[MAXCPUS], *lr[MAXCPUS];

	ncpus = system_ncpus();

	if (ncpus < 2) {
		return false;
	}

	/* Detect number of cpus. */
	for (CPU_INFO_FOREACH(cii_writer, ci_writer)) {
		/* spawn write thread */
		printf("writer -> %s\n", cpu_name(ci_writer));

		cpuid_w = ci_writer->ci_cpuid;

		/* reset payloads */
		ta->result[cpuid_w] = false;

		/* reset barriers */
		vm_barrier_init(&ta->ready_bar, ncpus);
		vm_barrier_init(&ta->go_bar, ncpus);
		vm_barrier_init(&ta->done_bar, ncpus);

		/* Test specific setup, if available */
		if (setup) {
			setup(ta);
		}

		lw[cpuid_w] = thread_spawn(ci_writer, master, ta, cleanup);

		if (lw[cpuid_w] == NULL) {
			panic("thread_spawn() failed\n");
		}

		for (CPU_INFO_FOREACH(cii_reader, ci_reader)) {
			if (ci_reader == ci_writer) {
				continue;
			}
			cpuid_r = ci_reader->ci_cpuid;

			/* spawn reader thread */
			printf("reader -> %s\n", cpu_name(ci_reader));
			lr[cpuid_r] = thread_spawn(ci_reader, slave, ta, cleanup);
			if (lr[cpuid_r] == NULL) {
				panic("thread_spawn() failed\n");
			}

		}

		/* All threads should have fired up by now */

		/* Wait for threads */
		printf("waiting for cpuid_w %lu\n", cpuid_w);

		thread_wait(lw[cpuid_w]); /* Wait for reader */

		for (CPU_INFO_FOREACH(cii_reader, ci_reader)) {
			if (ci_reader == ci_writer) {
				continue;
			}
			cpuid_r = ci_reader->ci_cpuid;
			
			thread_wait(lr[cpuid_r]); /* Wait for reader */

			if (ta->result[cpuid_r] == false) {
				printf("Thread #%lu failed\n", cpuid_r);
				result = false;
			}
			else printf("Thread #%lu succeeded!\n", cpuid_r);
		}

		if (ta->result[cpuid_w] == false) {
			printf("Thread #%lu failed\n", cpuid_w);
			result = false;
		}
		else printf("Thread #%lu succeeded!\n", cpuid_w);

		/* 
		 * Cleanup with test specific routine, if provided.
		 */
		if (cleanup) {
			cleanup(ta); 
		}

		/* Destroy barriers */
		vm_barrier_destroy(&ta->done_bar);
		vm_barrier_destroy(&ta->go_bar);
		vm_barrier_destroy(&ta->ready_bar);

		printf("threads done\n");

	}

	return result;
}

/* Test #1: thread functions. */

static void
setup_page(void *arg)
{
	KASSERT(arg != NULL);
	struct test_args *ta = arg;

	printf("%s called\n", __func__);
	ta->p.mv.addr = uvm_km_alloc(kernel_map, PAGE_SIZE, 
				     PAGE_SIZE, UVM_KMF_WIRED);

	ta->p.mv.len = PAGE_SIZE;

}

static void
write_to_page(void *arg)
{
	KASSERT(arg != NULL);
	struct test_args *ta = arg;
	cpuid_t cpuid = curcpu()->ci_cpuid;

	ta->result[cpuid] = ta->p.mv.addr ? true : false;

	if (ta->result[cpuid] == false) {
		return;
	}

	vm_barrier_hold(&ta->ready_bar);

	pattern_write(ta->p.mv.addr, PAGE_SIZE);

	vm_barrier_hold(&ta->go_bar); /* Tell the readers to go */
	printf("%s: write_to_page done\n", cpu_name(curcpu()));

	vm_barrier_hold(&ta->done_bar);
	printf("%s: All reads done\n", cpu_name(curcpu()));
}

static void
verify_page(void *arg)
{
	KASSERT(arg != NULL);

	struct test_args *ta = arg;

	cpuid_t cpuid = curcpu()->ci_cpuid;

	vm_barrier_hold(&ta->ready_bar);
	KASSERT(ta->p.mv.addr != 0 && ta->p.mv.len == PAGE_SIZE);

	vm_barrier_hold(&ta->go_bar);

	ta->result[cpuid] = pattern_verify(ta->p.mv.addr, ta->p.mv.len);
	vm_barrier_hold(&ta->done_bar);
}

static void
cleanup_page(void *arg)
{
	struct test_args *ta = arg;

	KASSERT(ta != NULL);

	if (ta->p.mv.addr == 0 || ta->p.mv.len != PAGE_SIZE) {
		printf("%s: ta->p.mv unset\n", __func__);
		return;
	}

	/* Free up test specific resources */
	uvm_km_free(kernel_map, ta->p.mv.addr, ta->p.mv.len, UVM_KMF_WIRED);

	ta->p.mv.addr = 0;
	ta->p.mv.len = 0;

	/* Reseting barriers is really for the thread_abort() path */

	/* reset barriers */
	vm_barrier_reset(&ta->done_bar);
	vm_barrier_reset(&ta->go_bar);
	vm_barrier_reset(&ta->ready_bar);

}

/* Test #1 */
/* 
 * This test writes a known pattern to a page of kernel memory, from
 * one CPU, and iteratively verifies the content of that memory from
 * other CPUS. 
 * Note: Not useful beyond a basic sanity check. If this test does not
 * pass, the VM system is basically useless for MP.
 */

static bool
test_writeread(void)
{

	struct test_args ta;
	memset(&ta, 0, sizeof ta);

	return 	test_kickoff(&ta, setup_page, write_to_page, verify_page, cleanup_page);
}

/* TLB shootdown test */
static void
tlb_setup(void *arg)
{
	KASSERT(arg != NULL);
	setup_page(arg);

	prep_fault(thread_pagefault);
}

static void
tlb_master(void *arg)
{
	KASSERT(arg != NULL);

	struct test_args *ta = arg;
	KASSERT(ta->p.mv.addr != 0 && ta->p.mv.len == PAGE_SIZE);

	cpuid_t cpuid = curcpu()->ci_cpuid;

	ta->result[cpuid] = true;

	vm_barrier_hold(&ta->ready_bar);

	pattern_write(ta->p.mv.addr, PAGE_SIZE);
	pmap_kremove(ta->p.mv.addr, PAGE_SIZE);
	vm_barrier_hold(&ta->go_bar); /* Tell the readers to go */
	printf("%s: write_to_page done\n", cpu_name(curcpu()));

	vm_barrier_hold(&ta->done_bar);
	printf("%s: All reads done\n", cpu_name(curcpu()));
}

static void
tlb_slave(void *arg)
{
	KASSERT(arg != NULL);

	struct test_args *ta = arg;
	KASSERT(ta->p.mv.addr != 0 && ta->p.mv.len == PAGE_SIZE);

	cpuid_t cpuid = curcpu()->ci_cpuid;
	ta->result[cpuid] = true;

	vm_barrier_hold(&ta->ready_bar);
	vm_barrier_hold(&ta->go_bar);

	/* If we get past pattern_verify(), it's a stale TLB. FAIL! */
	pattern_verify(ta->p.mv.addr, ta->p.mv.len);
	ta->result[cpuid] = false;
	vm_barrier_hold(&ta->done_bar);
}

static void
tlb_cleanup(void *arg)
{
	KASSERT(arg != NULL);

	prep_fault(NULL);
	cleanup_page(arg);
}

/* Test #2 */
/* 
 * This test repeats test #1 (see Test#1 for details). Followed by
 * this, the master thread removes the underlying mapping of the
 * shared page in question, and signals the slaves to access them. If
 * the page is accessible without error, this means that the TLB entry
 * on the slave CPU is stale.
 */

static bool
test_tlbstale(void)
{

	bool result = false;
	struct test_args ta;
	memset(&ta, 0, sizeof ta);

	result = test_kickoff(&ta, tlb_setup, tlb_master, tlb_slave, tlb_cleanup);

	return result;
}


/* pmap enter and extract */

static void
pmap_setupentry(void *arg)
{
	KASSERT(arg != NULL);
	struct test_args *ta = arg;

	vaddr_t va = 0xfeedface & ~PAGE_MASK; /* PAGE_SIZE aligned */
	paddr_t pa;
	struct vm_page *pg;

	/* Get a free page */
	pg = uvm_pagealloc(NULL, 0, NULL, 0);

	if (pg == NULL) {
		printf("Couldn't fetch free vm_page\n");
		return;
	}

	pg->flags &= ~PG_BUSY;
	UVM_PAGE_OWN(pg, NULL);

	pa = VM_PAGE_TO_PHYS(pg);

	ta->p.pm.va = va;
	ta->p.pm.pa = pa;

}

static void
pmap_newentry(void *arg)
{
	KASSERT(arg != NULL);
	struct test_args *ta = arg;
	cpuid_t cpuid = curcpu()->ci_cpuid;
	pmap_t pmap = ta->p.pm.pmap;

	ta->result[cpuid] = false;

	pmap_enter(pmap, ta->p.pm.va, ta->p.pm.pa, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE, PMAP_CANFAIL);


	vm_barrier_hold(&ta->ready_bar);
	ta->result[cpuid] = true;
	vm_barrier_hold(&ta->go_bar);
	vm_barrier_hold(&ta->done_bar);

	return;
}

static void
pmap_extractentry(void *arg)
{
	KASSERT(arg != NULL);
	struct test_args *ta = arg;
	pmap_t pmap = ta->p.pm.pmap;

	vaddr_t va = ta->p.pm.va;
	paddr_t pa = 0;

	cpuid_t cpuid = curcpu()->ci_cpuid;

	ta->result[cpuid] = false;

	vm_barrier_hold(&ta->ready_bar);

	pmap_extract(pmap, va, &pa);

	vm_barrier_hold(&ta->go_bar);

	/* verify */
	ta->result[cpuid] = (pa == ta->p.pm.pa);

	vm_barrier_hold(&ta->done_bar);

	return;
}

static void
pmap_cleanuptest(void *arg)
{
	KASSERT(arg != NULL);
	struct test_args *ta = arg;
	pmap_t pmap = ta->p.pm.pmap;

	vaddr_t va = ta->p.pm.va;
	paddr_t pa = ta->p.pm.pa;

	if (va == 0 || pa == 0) {
		printf("%s: called before\n", __func__);
		return;
	}

	struct vm_page *pg = PHYS_TO_VM_PAGE(pa);

	KASSERT(pg != NULL);

	pmap_remove(pmap, va, va + PAGE_SIZE);
	uvm_pagefree(pg);

	//pmap_destroy(pmap); //XXX: This seems to interfere with the
	//currently loaded pmap. va may need tweaking.

	ta->p.pm.va = 0;
	ta->p.pm.pa = 0;

	/* Reseting barriers is really for the thread_abort() path */

	/* reset barriers */
	vm_barrier_reset(&ta->done_bar);
	vm_barrier_reset(&ta->go_bar);
	vm_barrier_reset(&ta->ready_bar);

	return;
}


static bool
test_pmapenterextract(void)
{
	bool result = false;

	struct test_args ta;
	memset(&ta, 0, sizeof ta);

	/* Create new pmap */
	ta.p.pm.pmap = pmap_create();

	result = test_kickoff(&ta, pmap_setupentry, pmap_newentry, pmap_extractentry, pmap_cleanuptest);

	return result;
}

/*
 * The xentest_proc() runs in its own process context obtained via
 * fork1() very early in the boot process, and runs in place of the
 * init(1) process by forking out of lwp0. The init(1) may not
 * co-exist with xentest_proc()
 */

/*
 * This is the in-kernel test process. It has nothing to do with the
 * above sysctl(9) based framework, although it may use the individual
 * tests as it deems fit.
 */

struct tests {
	bool (*testfunc)(void);
	const char *testname;
};
	

static struct tests xentests[] = {
	{ test_writeread, "Read Write test" },
	{ test_tlbstale, "TLB stale test" },
	{ test_pmapenterextract, "insert and extract a single pmap entry" }
};

extern void (*cont_init)(void *);
extern bool xen_tests_done;

void xentest_proc(void *arg);
void xentest_proc(void *arg)
{
	int i;

	/* tests */
	for (i = 0; i < sizeof xentests / sizeof xentests[0]; i++) {
		printf("Running Test: %s\n", xentests[i].testname);
		if (xentests[i].testfunc() == true) {
			printf("PASSED\n");
		}
		else {
			printf("FAILED\n");
		}
	}

	printf("tests done\n");

#ifdef XENTESTKERNEL
	xen_tests_done = true;

	cont_init(curlwp); /* continue with boot */
#endif  /* XENTESTKERNEL */

}
