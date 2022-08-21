/*	$NetBSD: x86_pte_tester.c,v 1.3 2022/08/21 14:06:42 mlelstv Exp $	*/

/*
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#define __HAVE_DIRECT_MAP
#define __HAVE_PCPU_AREA
#define SVS

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <uvm/uvm.h>
#include <x86/pmap.h>

#if defined(__x86_64__)
# include <amd64/pmap.h>
# include <amd64/pmap_private.h>
# define NLEVEL 4
#else
# error "Unsupported configuration"
#endif

static struct {
	struct sysctllog *ctx_sysctllog;
	vaddr_t levels[NLEVEL];
	struct {
		size_t l4;
		size_t l3;
		size_t l2;
		size_t l1;
	} coord;
	struct {
		size_t n_rwx;
		size_t n_shstk;
		bool kernel_map_with_low_ptes;
		bool pte_is_user_accessible;
		size_t n_user_space_is_kernel;
		size_t n_kernel_space_is_user;
		size_t n_svs_g_bit_set;
	} results;
} tester_ctx;

typedef enum {
	WALK_NEXT, /* go to the next level */
	WALK_SKIP, /* skip the next level, but keep iterating on the current one */
	WALK_STOP  /* stop the iteration on the current level */
} walk_type;

/* -------------------------------------------------------------------------- */

#define is_flag(__ent, __flag)	(((__ent) & __flag) != 0)
#define is_valid(__ent)		is_flag(__ent, PTE_P)
#define get_pa(__pde)		(__pde & PTE_FRAME)

#define L4_MAX_NENTRIES (PAGE_SIZE / sizeof(pd_entry_t))
#define L3_MAX_NENTRIES (PAGE_SIZE / sizeof(pd_entry_t))
#define L2_MAX_NENTRIES (PAGE_SIZE / sizeof(pd_entry_t))
#define L1_MAX_NENTRIES (PAGE_SIZE / sizeof(pd_entry_t))

static void
scan_l1(paddr_t pa, walk_type (fn)(pd_entry_t pde, size_t slot, int lvl))
{
	pd_entry_t *pd = (pd_entry_t *)tester_ctx.levels[0];
	size_t i;

	pmap_kenter_pa(tester_ctx.levels[0], pa, VM_PROT_READ, 0);
	pmap_update(pmap_kernel());

	for (i = 0; i < L1_MAX_NENTRIES; i++) {
		tester_ctx.coord.l1 = i;
		if (is_valid(pd[i])) {
			fn(pd[i], i, 1);
		}
	}

	pmap_kremove(tester_ctx.levels[0], PAGE_SIZE);
	pmap_update(pmap_kernel());
}

static void
scan_l2(paddr_t pa, walk_type (fn)(pd_entry_t pde, size_t slot, int lvl))
{
	pd_entry_t *pd = (pd_entry_t *)tester_ctx.levels[1];
	walk_type ret;
	size_t i;

	pmap_kenter_pa(tester_ctx.levels[1], pa, VM_PROT_READ, 0);
	pmap_update(pmap_kernel());

	for (i = 0; i < L2_MAX_NENTRIES; i++) {
		tester_ctx.coord.l2 = i;
		if (!is_valid(pd[i]))
			continue;
		ret = fn(pd[i], i, 2);
		if (ret == WALK_STOP)
			break;
		if (is_flag(pd[i], PTE_PS))
			continue;
		if (ret == WALK_NEXT)
			scan_l1(get_pa(pd[i]), fn);
	}

	pmap_kremove(tester_ctx.levels[1], PAGE_SIZE);
	pmap_update(pmap_kernel());
}

static void
scan_l3(paddr_t pa, walk_type (fn)(pd_entry_t pde, size_t slot, int lvl))
{
	pd_entry_t *pd = (pd_entry_t *)tester_ctx.levels[2];
	walk_type ret;
	size_t i;

	pmap_kenter_pa(tester_ctx.levels[2], pa, VM_PROT_READ, 0);
	pmap_update(pmap_kernel());

	for (i = 0; i < L3_MAX_NENTRIES; i++) {
		tester_ctx.coord.l3 = i;
		if (!is_valid(pd[i]))
			continue;
		ret = fn(pd[i], i, 3);
		if (ret == WALK_STOP)
			break;
		if (is_flag(pd[i], PTE_PS))
			continue;
		if (ret == WALK_NEXT)
			scan_l2(get_pa(pd[i]), fn);
	}

	pmap_kremove(tester_ctx.levels[2], PAGE_SIZE);
	pmap_update(pmap_kernel());
}

static void
scan_l4(paddr_t pa, walk_type (fn)(pd_entry_t pde, size_t slot, int lvl))
{
	pd_entry_t *pd = (pd_entry_t *)tester_ctx.levels[3];
	walk_type ret;
	size_t i;

	pmap_kenter_pa(tester_ctx.levels[3], pa, VM_PROT_READ, 0);
	pmap_update(pmap_kernel());

	for (i = 0; i < L4_MAX_NENTRIES; i++) {
		tester_ctx.coord.l4 = i;
		if (!is_valid(pd[i]))
			continue;
		ret = fn(pd[i], i, 4);
		if (ret == WALK_STOP)
			break;
		if (is_flag(pd[i], PTE_PS))
			continue;
		if (ret == WALK_NEXT)
			scan_l3(get_pa(pd[i]), fn);
	}

	pmap_kremove(tester_ctx.levels[3], PAGE_SIZE);
	pmap_update(pmap_kernel());
}

static void
scan_tree(paddr_t pa, walk_type (fn)(pd_entry_t pde, size_t slot, int lvl))
{
	scan_l4(pa, fn);
}

/* -------------------------------------------------------------------------- */

/*
 * Rule: the number of kernel RWX pages should be zero.
 */
static walk_type
count_krwx(pd_entry_t pde, size_t slot, int lvl)
{
	if (lvl == NLEVEL && slot < 256) {
		return WALK_SKIP;
	}
	if (is_flag(pde, PTE_NX) || !is_flag(pde, PTE_W)) {
		return WALK_SKIP;
	}
	if (lvl != 1 && !is_flag(pde, PTE_PS)) {
		return WALK_NEXT;
	}

	if (lvl == 4) {
		tester_ctx.results.n_rwx += (NBPD_L4 / PAGE_SIZE);
	} else if (lvl == 3) {
		tester_ctx.results.n_rwx += (NBPD_L3 / PAGE_SIZE);
	} else if (lvl == 2) {
		tester_ctx.results.n_rwx += (NBPD_L2 / PAGE_SIZE);
	} else if (lvl == 1) {
		tester_ctx.results.n_rwx += (NBPD_L1 / PAGE_SIZE);
	}

	return WALK_NEXT;
}

/*
 * Rule: the number of kernel SHSTK pages should be zero.
 */
static walk_type
count_kshstk(pd_entry_t pde, size_t slot, int lvl)
{
	if (lvl == NLEVEL && slot < 256) {
		return WALK_SKIP;
	}

	if (is_flag(pde, PTE_PS) || lvl == 1) {
		if (!is_flag(pde, PTE_W) && is_flag(pde, PTE_D)) {
			if (lvl == 4) {
				tester_ctx.results.n_shstk += (NBPD_L4 / PAGE_SIZE);
			} else if (lvl == 3) {
				tester_ctx.results.n_shstk += (NBPD_L3 / PAGE_SIZE);
			} else if (lvl == 2) {
				tester_ctx.results.n_shstk += (NBPD_L2 / PAGE_SIZE);
			} else if (lvl == 1) {
				tester_ctx.results.n_shstk += (NBPD_L1 / PAGE_SIZE);
			}
		}
		return WALK_SKIP;
	}

	if (!is_flag(pde, PTE_W)) {
		return WALK_SKIP;
	}

	return WALK_NEXT;
}

/*
 * Rule: the lower half of the kernel map must be zero.
 */
static walk_type
check_kernel_map(pd_entry_t pde, size_t slot, int lvl)
{
	if (lvl != NLEVEL) {
		return WALK_STOP;
	}
	if (slot >= 256) {
		return WALK_SKIP;
	}
	if (pde != 0) {
		tester_ctx.results.kernel_map_with_low_ptes |= true;
	}
	return WALK_SKIP;
}

/*
 * Rule: the PTE space must not have user permissions.
 */
static walk_type
check_pte_space(pd_entry_t pde, size_t slot, int lvl)
{
	if (lvl != NLEVEL) {
		return WALK_STOP;
	}
	if (slot != PDIR_SLOT_PTE) {
		return WALK_SKIP;
	}
	if (is_flag(pde, PTE_U)) {
		tester_ctx.results.pte_is_user_accessible |= true;
	}
	return WALK_SKIP;
}

/*
 * Rule: each page in the lower half must have user permissions.
 */
static walk_type
check_user_space(pd_entry_t pde, size_t slot, int lvl)
{
	if (lvl == NLEVEL && slot >= 256) {
		return WALK_SKIP;
	}
	if (!is_flag(pde, PTE_U)) {
		tester_ctx.results.n_user_space_is_kernel += 1;
		return WALK_SKIP;
	}
	return WALK_NEXT;
}

/*
 * Rule: each page in the higher half must have kernel permissions.
 */
static walk_type
check_kernel_space(pd_entry_t pde, size_t slot, int lvl)
{
	if (lvl == NLEVEL && slot < 256) {
		return WALK_SKIP;
	}
	if (lvl == NLEVEL && slot == PDIR_SLOT_PTE) {
		return WALK_SKIP;
	}
	if (is_flag(pde, PTE_U)) {
		tester_ctx.results.n_kernel_space_is_user += 1;
		return WALK_SKIP;
	}
	return WALK_NEXT;
}

/*
 * Rule: the SVS map is allowed to use the G bit only on the PCPU area.
 */
static walk_type
check_svs_g_bit(pd_entry_t pde, size_t slot, int lvl)
{
	if (lvl == NLEVEL && slot == PDIR_SLOT_PCPU) {
		return WALK_SKIP;
	}
	if (is_flag(pde, PTE_G)) {
		tester_ctx.results.n_svs_g_bit_set += 1;
		return WALK_SKIP;
	}
	return WALK_NEXT;
}

/* -------------------------------------------------------------------------- */

static void
scan_svs(void)
{
	extern bool svs_enabled;
	paddr_t pa0;

	if (!svs_enabled) {
		tester_ctx.results.n_svs_g_bit_set = -1;
		return;
	}

	kpreempt_disable();
	pa0 = curcpu()->ci_svs_updirpa;
	scan_tree(pa0, &check_user_space);
	scan_tree(pa0, &check_kernel_space);
	scan_tree(pa0, &check_svs_g_bit);
	kpreempt_enable();
}

static void
scan_proc(struct proc *p)
{
	struct pmap *pmap = p->p_vmspace->vm_map.pmap;
	paddr_t pa0;

	mutex_enter(&pmap->pm_lock);

	kpreempt_disable();
	pa0 = (paddr_t)pmap->pm_pdirpa[0];
	scan_tree(pa0, &check_user_space);
	scan_tree(pa0, &check_kernel_space);
	scan_tree(pa0, &check_pte_space);
	kpreempt_enable();

	mutex_exit(&pmap->pm_lock);
}

static void
x86_pte_run_scans(void)
{
	struct pmap *kpm = pmap_kernel();
	paddr_t pa0;

	memset(&tester_ctx.results, 0, sizeof(tester_ctx.results));

	/* Scan the current user process. */
	scan_proc(curproc);

	/* Scan the SVS mapping. */
	scan_svs();

	/* Scan the kernel map. */
	pa0 = (paddr_t)kpm->pm_pdirpa[0];
	scan_tree(pa0, &count_krwx);
	scan_tree(pa0, &count_kshstk);
	scan_tree(pa0, &check_kernel_map);
}

static void
x86_pte_levels_init(void)
{
	size_t i;
	for (i = 0; i < NLEVEL; i++) {
		tester_ctx.levels[i] = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
		    UVM_KMF_VAONLY);
	}
}

static void
x86_pte_levels_destroy(void)
{
	size_t i;
	for (i = 0; i < NLEVEL; i++) {
		uvm_km_free(kernel_map, tester_ctx.levels[i], PAGE_SIZE,
		    UVM_KMF_VAONLY);
	}
}

/* -------------------------------------------------------------------------- */

static int
x86_pte_sysctl_run(SYSCTLFN_ARGS)
{
	if (oldlenp == NULL)
		return EINVAL;

	x86_pte_run_scans();

	if (oldp == NULL) {
		*oldlenp = sizeof(tester_ctx.results);
		return 0;
	}

	if (*oldlenp < sizeof(tester_ctx.results))
		return ENOMEM;

	return copyout(&tester_ctx.results, oldp, sizeof(tester_ctx.results));
}

static int
x86_pte_sysctl_init(void)
{
	struct sysctllog **log = &tester_ctx.ctx_sysctllog;
	const struct sysctlnode *rnode, *cnode;
	int error;

	error = sysctl_createv(log, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "x86_pte_test",
	    SYSCTL_DESCR("x86_pte testing interface"),
	    NULL, 0, NULL, 0, CTL_KERN, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, &cnode, CTLFLAG_PERMANENT,
	    CTLTYPE_STRUCT, "test",
	    SYSCTL_DESCR("execute a x86_pte test"),
	    x86_pte_sysctl_run, 0, NULL, 0, CTL_CREATE, CTL_EOL);

out:
 	if (error)
		sysctl_teardown(log);
	return error;
}

static void
x86_pte_sysctl_destroy(void)
{
	sysctl_teardown(&tester_ctx.ctx_sysctllog);
}

/* -------------------------------------------------------------------------- */

MODULE(MODULE_CLASS_MISC, x86_pte_tester, NULL);

static int
x86_pte_tester_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		x86_pte_levels_init();
		error = x86_pte_sysctl_init();
		break;
	case MODULE_CMD_FINI:
		x86_pte_sysctl_destroy();
		x86_pte_levels_destroy();
		break;
	default:
		error = ENOTTY;
		break;
	}

	return error;
}
