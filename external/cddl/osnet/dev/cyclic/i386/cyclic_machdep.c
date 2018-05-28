/*	$NetBSD: cyclic_machdep.c,v 1.5 2018/05/28 21:05:02 chs Exp $	*/

/*-
 * Copyright 2006-2008 John Birrell <jb@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/cddl/dev/cyclic/i386/cyclic_machdep.c 222813 2011-06-07 08:46:13Z attilio $
 *
 */

static void enable(cyb_arg_t);
static void disable(cyb_arg_t);
static void reprogram(cyb_arg_t, hrtime_t);
static void xcall(cyb_arg_t, cpu_t *, cyc_func_t, void *);

static cyc_backend_t	be	= {
	NULL,		/* cyb_configure */
	NULL,		/* cyb_unconfigure */
	enable,
	disable,
	reprogram,
	xcall,
	NULL		/* cyb_arg_t cyb_arg */
};

static void
cyclic_ap_start(void *dummy)
{
	/* Initialise the rest of the CPUs. */
	cyclic_mp_init();
}

SYSINIT(cyclic_ap_start, SI_SUB_SMP, SI_ORDER_ANY, cyclic_ap_start, NULL);

/*
 *  Machine dependent cyclic subsystem initialisation.
 */
static void
cyclic_machdep_init(void)
{
	/* Register the cyclic backend. */
	cyclic_init(&be);
#ifdef __NetBSD__
	cyclic_ap_start(NULL);
#endif
}

static void
cyclic_machdep_uninit(void)
{
	int i;

	for (i = 0; i <= mp_maxid; i++)
		/* Reset the cyclic clock callback hook. */
		cyclic_clock_func[i] = NULL;

	/* De-register the cyclic backend. */
	cyclic_uninit();
}

static hrtime_t exp_due[MAXCPU];

/*
 * This function is the one registered by the machine dependent
 * initialiser as the callback for high speed timer events.
 */
static void
cyclic_clock(struct clockframe *frame)
{
	cpu_t *c = &solaris_cpu[cpu_number()];

	if (c->cpu_cyclic != NULL && gethrtime() >= exp_due[cpu_number()]) {
		if (TRAPF_USERMODE(frame)) {
			c->cpu_profile_pc = 0;
			c->cpu_profile_upc = TRAPF_PC(frame);
		} else {
			c->cpu_profile_pc = TRAPF_PC(frame);
			c->cpu_profile_upc = 0;
		}

		c->cpu_intr_actv = 1;

		/* Fire any timers that are due. */
		cyclic_fire(c);

		c->cpu_intr_actv = 0;
	}
}

static void enable(cyb_arg_t arg)
{
	/* Register the cyclic clock callback function. */
	cyclic_clock_func[cpu_number()] = cyclic_clock;
}

static void disable(cyb_arg_t arg)
{
	/* Reset the cyclic clock callback function. */
	cyclic_clock_func[cpu_number()] = NULL;
}

static void reprogram(cyb_arg_t arg, hrtime_t exp)
{
	exp_due[cpu_number()] = exp;
}

#ifdef __NetBSD__
static void xcall_func(void *arg0, void *arg1)
{
	cyc_func_t func;

	func = arg0;
	(*func)(arg1);
}
#endif

static void xcall(cyb_arg_t arg, cpu_t *c, cyc_func_t func, void *param)
{
#ifdef __NetBSD__
	uint64_t xc;

	xc = xc_unicast(XC_HIGHPRI, xcall_func, func, param, cpu_lookup(c->cpuid));
	xc_wait(xc);
#else
	smp_rendezvous_cpus((cpumask_t) (1 << c->cpuid),
	    smp_no_rendevous_barrier, func, smp_no_rendevous_barrier, param);
#endif
}
