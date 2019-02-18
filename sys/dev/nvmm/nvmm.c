/*	$NetBSD: nvmm.c,v 1.8 2019/02/18 12:17:45 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
__KERNEL_RCSID(0, "$NetBSD: nvmm.c,v 1.8 2019/02/18 12:17:45 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <sys/cpu.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/proc.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>

#include "ioconf.h"

#include <dev/nvmm/nvmm.h>
#include <dev/nvmm/nvmm_internal.h>
#include <dev/nvmm/nvmm_ioctl.h>

static struct nvmm_machine machines[NVMM_MAX_MACHINES];

static const struct nvmm_impl *nvmm_impl_list[] = {
	&nvmm_x86_svm,	/* x86 AMD SVM */
	&nvmm_x86_vmx	/* x86 Intel VMX */
};

static const struct nvmm_impl *nvmm_impl = NULL;

/* -------------------------------------------------------------------------- */

static int
nvmm_machine_alloc(struct nvmm_machine **ret)
{
	struct nvmm_machine *mach;
	size_t i;

	for (i = 0; i < NVMM_MAX_MACHINES; i++) {
		mach = &machines[i];

		rw_enter(&mach->lock, RW_WRITER);
		if (mach->present) {
			rw_exit(&mach->lock);
			continue;
		}

		mach->present = true;
		*ret = mach;
		return 0;
	}

	return ENOBUFS;
}

static void
nvmm_machine_free(struct nvmm_machine *mach)
{
	KASSERT(rw_write_held(&mach->lock));
	KASSERT(mach->present);
	mach->present = false;
}

static int
nvmm_machine_get(nvmm_machid_t machid, struct nvmm_machine **ret, bool writer)
{
	struct nvmm_machine *mach;
	krw_t op = writer ? RW_WRITER : RW_READER;

	if (machid >= NVMM_MAX_MACHINES) {
		return EINVAL;
	}
	mach = &machines[machid];

	rw_enter(&mach->lock, op);
	if (!mach->present) {
		rw_exit(&mach->lock);
		return ENOENT;
	}
	if (mach->procid != curproc->p_pid) {
		rw_exit(&mach->lock);
		return EPERM;
	}
	*ret = mach;

	return 0;
}

static void
nvmm_machine_put(struct nvmm_machine *mach)
{
	rw_exit(&mach->lock);
}

/* -------------------------------------------------------------------------- */

static int
nvmm_vcpu_alloc(struct nvmm_machine *mach, struct nvmm_cpu **ret)
{
	struct nvmm_cpu *vcpu;
	size_t i;

	for (i = 0; i < NVMM_MAX_VCPUS; i++) {
		vcpu = &mach->cpus[i];

		mutex_enter(&vcpu->lock);
		if (vcpu->present) {
			mutex_exit(&vcpu->lock);
			continue;
		}

		vcpu->present = true;
		vcpu->cpuid = i;
		vcpu->state = kmem_zalloc(nvmm_impl->state_size, KM_SLEEP);
		*ret = vcpu;
		return 0;
	}

	return ENOBUFS;
}

static void
nvmm_vcpu_free(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	KASSERT(mutex_owned(&vcpu->lock));
	vcpu->present = false;
	kmem_free(vcpu->state, nvmm_impl->state_size);
	vcpu->hcpu_last = -1;
}

int
nvmm_vcpu_get(struct nvmm_machine *mach, nvmm_cpuid_t cpuid,
    struct nvmm_cpu **ret)
{
	struct nvmm_cpu *vcpu;

	if (cpuid >= NVMM_MAX_VCPUS) {
		return EINVAL;
	}
	vcpu = &mach->cpus[cpuid];

	mutex_enter(&vcpu->lock);
	if (!vcpu->present) {
		mutex_exit(&vcpu->lock);
		return ENOENT;
	}
	*ret = vcpu;

	return 0;
}

void
nvmm_vcpu_put(struct nvmm_cpu *vcpu)
{
	mutex_exit(&vcpu->lock);
}

/* -------------------------------------------------------------------------- */

static void
nvmm_kill_machines(pid_t pid)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	size_t i, j;
	int error;

	for (i = 0; i < NVMM_MAX_MACHINES; i++) {
		mach = &machines[i];

		rw_enter(&mach->lock, RW_WRITER);
		if (!mach->present || mach->procid != pid) {
			rw_exit(&mach->lock);
			continue;
		}

		/* Kill it. */
		for (j = 0; j < NVMM_MAX_VCPUS; j++) {
			error = nvmm_vcpu_get(mach, j, &vcpu);
			if (error)
				continue;
			(*nvmm_impl->vcpu_destroy)(mach, vcpu);
			nvmm_vcpu_free(mach, vcpu);
			nvmm_vcpu_put(vcpu);
		}
		uvmspace_free(mach->vm);

		/* Drop the kernel UOBJ refs. */
		for (j = 0; j < NVMM_MAX_SEGS; j++) {
			if (!mach->segs[j].present)
				continue;
			uao_detach(mach->segs[j].uobj);
		}

		nvmm_machine_free(mach);

		rw_exit(&mach->lock);
	}
}

/* -------------------------------------------------------------------------- */

static int
nvmm_capability(struct nvmm_ioc_capability *args)
{
	args->cap.version = NVMM_CAPABILITY_VERSION;
	args->cap.state_size = nvmm_impl->state_size;
	args->cap.max_machines = NVMM_MAX_MACHINES;
	args->cap.max_vcpus = NVMM_MAX_VCPUS;
	args->cap.max_ram = NVMM_MAX_RAM;

	(*nvmm_impl->capability)(&args->cap);

	return 0;
}

static int
nvmm_machine_create(struct nvmm_ioc_machine_create *args)
{
	struct nvmm_machine *mach;
	int error;

	error = nvmm_machine_alloc(&mach);
	if (error)
		return error;

	/* Curproc owns the machine. */
	mach->procid = curproc->p_pid;

	/* Zero out the segments. */
	memset(&mach->segs, 0, sizeof(mach->segs));

	/* Create the machine vmspace. */
	mach->gpa_begin = 0;
	mach->gpa_end = NVMM_MAX_RAM;
	mach->vm = uvmspace_alloc(0, mach->gpa_end - mach->gpa_begin, false);

	(*nvmm_impl->machine_create)(mach);

	args->machid = mach->machid;
	nvmm_machine_put(mach);

	return 0;
}

static int
nvmm_machine_destroy(struct nvmm_ioc_machine_destroy *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;
	size_t i;

	error = nvmm_machine_get(args->machid, &mach, true);
	if (error)
		return error;

	for (i = 0; i < NVMM_MAX_VCPUS; i++) {
		error = nvmm_vcpu_get(mach, i, &vcpu);
		if (error)
			continue;

		(*nvmm_impl->vcpu_destroy)(mach, vcpu);
		nvmm_vcpu_free(mach, vcpu);
		nvmm_vcpu_put(vcpu);
	}

	(*nvmm_impl->machine_destroy)(mach);

	/* Free the machine vmspace. */
	uvmspace_free(mach->vm);

	/* Drop the kernel UOBJ refs. */
	for (i = 0; i < NVMM_MAX_SEGS; i++) {
		if (!mach->segs[i].present)
			continue;
		uao_detach(mach->segs[i].uobj);
	}

	nvmm_machine_free(mach);
	nvmm_machine_put(mach);

	return 0;
}

static int
nvmm_machine_configure(struct nvmm_ioc_machine_configure *args)
{
	struct nvmm_machine *mach;
	size_t allocsz;
	void *data;
	int error;

	if (__predict_false(args->op >= nvmm_impl->conf_max)) {
		return EINVAL;
	}

	allocsz = nvmm_impl->conf_sizes[args->op];
	data = kmem_alloc(allocsz, KM_SLEEP);

	error = nvmm_machine_get(args->machid, &mach, true);
	if (error) {
		kmem_free(data, allocsz);
		return error;
	}

	error = copyin(args->conf, data, allocsz);
	if (error) {
		goto out;
	}

	error = (*nvmm_impl->machine_configure)(mach, args->op, data);

out:
	nvmm_machine_put(mach);
	kmem_free(data, allocsz);
	return error;
}

static int
nvmm_vcpu_create(struct nvmm_ioc_vcpu_create *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_alloc(mach, &vcpu);
	if (error)
		goto out;

	error = (*nvmm_impl->vcpu_create)(mach, vcpu);
	if (error) {
		nvmm_vcpu_free(mach, vcpu);
		nvmm_vcpu_put(vcpu);
		goto out;
	}

	nvmm_vcpu_put(vcpu);

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_vcpu_destroy(struct nvmm_ioc_vcpu_destroy *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	(*nvmm_impl->vcpu_destroy)(mach, vcpu);
	nvmm_vcpu_free(mach, vcpu);
	nvmm_vcpu_put(vcpu);

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_vcpu_setstate(struct nvmm_ioc_vcpu_setstate *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	error = copyin(args->state, vcpu->state, nvmm_impl->state_size);
	if (error) {
		nvmm_vcpu_put(vcpu);
		goto out;
	}

	(*nvmm_impl->vcpu_setstate)(vcpu, vcpu->state, args->flags);
	nvmm_vcpu_put(vcpu);

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_vcpu_getstate(struct nvmm_ioc_vcpu_getstate *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	(*nvmm_impl->vcpu_getstate)(vcpu, vcpu->state, args->flags);
	nvmm_vcpu_put(vcpu);
	error = copyout(vcpu->state, args->state, nvmm_impl->state_size);

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_vcpu_inject(struct nvmm_ioc_vcpu_inject *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	error = (*nvmm_impl->vcpu_inject)(mach, vcpu, &args->event);
	nvmm_vcpu_put(vcpu);

out:
	nvmm_machine_put(mach);
	return error;
}

static void
nvmm_do_vcpu_run(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct vmspace *vm = mach->vm;

	while (1) {
		(*nvmm_impl->vcpu_run)(mach, vcpu, exit);

		if (__predict_true(exit->reason != NVMM_EXIT_MEMORY)) {
			break;
		}
		if (uvm_fault(&vm->vm_map, exit->u.mem.gpa, VM_PROT_ALL)) {
			break;
		}
	}
}

static int
nvmm_vcpu_run(struct nvmm_ioc_vcpu_run *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	nvmm_do_vcpu_run(mach, vcpu, &args->exit);
	nvmm_vcpu_put(vcpu);

out:
	nvmm_machine_put(mach);
	return error;
}

/* -------------------------------------------------------------------------- */

static struct uvm_object *
nvmm_seg_getuobj(struct nvmm_machine *mach, uintptr_t hva, size_t size,
   size_t *off)
{
	struct nvmm_seg *seg;
	size_t i;

	for (i = 0; i < NVMM_MAX_SEGS; i++) {
		seg = &mach->segs[i];
		if (!seg->present) {
			continue;
		}
		if (hva >= seg->hva && hva + size <= seg->hva + seg->size) {
			*off = hva - seg->hva;
			return seg->uobj;
		}
	}

	return NULL;
}

static struct nvmm_seg *
nvmm_seg_find(struct nvmm_machine *mach, uintptr_t hva, size_t size)
{
	struct nvmm_seg *seg;
	size_t i;

	for (i = 0; i < NVMM_MAX_SEGS; i++) {
		seg = &mach->segs[i];
		if (seg->present && seg->hva == hva && seg->size == size) {
			return seg;
		}
	}

	return NULL;
}

static int
nvmm_seg_validate(struct nvmm_machine *mach, uintptr_t hva, size_t size)
{
	struct nvmm_seg *seg;
	size_t i;

	if ((hva % PAGE_SIZE) != 0 || (size % PAGE_SIZE) != 0) {
		return EINVAL;
	}
	if (hva == 0) {
		return EINVAL;
	}

	for (i = 0; i < NVMM_MAX_SEGS; i++) {
		seg = &mach->segs[i];
		if (!seg->present) {
			continue;
		}

		if (hva >= seg->hva && hva + size <= seg->hva + seg->size) {
			break;
		}

		if (hva >= seg->hva && hva < seg->hva + seg->size) {
			return EEXIST;
		}
		if (hva + size > seg->hva &&
		    hva + size <= seg->hva + seg->size) {
			return EEXIST;
		}
		if (hva <= seg->hva && hva + size >= seg->hva + seg->size) {
			return EEXIST;
		}
	}

	return 0;
}

static struct nvmm_seg *
nvmm_seg_alloc(struct nvmm_machine *mach)
{
	struct nvmm_seg *seg;
	size_t i;

	for (i = 0; i < NVMM_MAX_SEGS; i++) {
		seg = &mach->segs[i];
		if (!seg->present) {
			seg->present = true;
			return seg;
		}
	}

	return NULL;
}

static void
nvmm_seg_free(struct nvmm_seg *seg)
{
	struct vmspace *vmspace = curproc->p_vmspace;

	uvm_unmap(&vmspace->vm_map, seg->hva, seg->hva + seg->size);
	uao_detach(seg->uobj);

	seg->uobj = NULL;
	seg->present = false;
}

static int
nvmm_hva_map(struct nvmm_ioc_hva_map *args)
{
	struct vmspace *vmspace = curproc->p_vmspace;
	struct nvmm_machine *mach;
	struct nvmm_seg *seg;
	vaddr_t uva;
	int error;

	error = nvmm_machine_get(args->machid, &mach, true);
	if (error)
		return error;

	error = nvmm_seg_validate(mach, args->hva, args->size);
	if (error)
		goto out;

	seg = nvmm_seg_alloc(mach);
	if (seg == NULL) {
		error = ENOBUFS;
		goto out;
	}

	seg->hva = args->hva;
	seg->size = args->size;
	seg->uobj = uao_create(seg->size, 0);
	uva = seg->hva;

	/* Take a reference for the user. */
	uao_reference(seg->uobj);

	/* Map the uobj into the user address space, as pageable. */
	error = uvm_map(&vmspace->vm_map, &uva, seg->size, seg->uobj, 0, 0,
	    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW, UVM_INH_SHARE,
	    UVM_ADV_RANDOM, UVM_FLAG_FIXED|UVM_FLAG_UNMAP));
	if (error) {
		uao_detach(seg->uobj);
	}

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_hva_unmap(struct nvmm_ioc_hva_unmap *args)
{
	struct nvmm_machine *mach;
	struct nvmm_seg *seg;
	int error;

	error = nvmm_machine_get(args->machid, &mach, true);
	if (error)
		return error;

	seg = nvmm_seg_find(mach, args->hva, args->size);
	if (seg == NULL)
		return ENOENT;

	nvmm_seg_free(seg);

	nvmm_machine_put(mach);
	return 0;
}

/* -------------------------------------------------------------------------- */

static int
nvmm_gpa_map(struct nvmm_ioc_gpa_map *args)
{
	struct nvmm_machine *mach;
	struct uvm_object *uobj;
	gpaddr_t gpa;
	size_t off;
	int error;

	error = nvmm_machine_get(args->machid, &mach, false);
	if (error)
		return error;

	if ((args->gpa % PAGE_SIZE) != 0 || (args->size % PAGE_SIZE) != 0 ||
	    (args->hva % PAGE_SIZE) != 0) {
		error = EINVAL;
		goto out;
	}
	if (args->hva == 0) {
		error = EINVAL;
		goto out;
	}
	if (args->gpa < mach->gpa_begin || args->gpa >= mach->gpa_end) {
		error = EINVAL;
		goto out;
	}
	if (args->gpa + args->size <= args->gpa) {
		error = EINVAL;
		goto out;
	}
	if (args->gpa + args->size > mach->gpa_end) {
		error = EINVAL;
		goto out;
	}
	gpa = args->gpa;

	uobj = nvmm_seg_getuobj(mach, args->hva, args->size, &off);
	if (uobj == NULL) {
		error = EINVAL;
		goto out;
	}

	/* Take a reference for the machine. */
	uao_reference(uobj);

	/* Map the uobj into the machine address space, as pageable. */
	error = uvm_map(&mach->vm->vm_map, &gpa, args->size, uobj, off, 0,
	    UVM_MAPFLAG(UVM_PROT_RWX, UVM_PROT_RWX, UVM_INH_NONE,
	    UVM_ADV_RANDOM, UVM_FLAG_FIXED|UVM_FLAG_UNMAP));
	if (error) {
		uao_detach(uobj);
		goto out;
	}
	if (gpa != args->gpa) {
		uao_detach(uobj);
		printf("[!] uvm_map problem\n");
		error = EINVAL;
		goto out;
	}

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_gpa_unmap(struct nvmm_ioc_gpa_unmap *args)
{
	struct nvmm_machine *mach;
	gpaddr_t gpa;
	int error;

	error = nvmm_machine_get(args->machid, &mach, false);
	if (error)
		return error;

	if ((args->gpa % PAGE_SIZE) != 0 || (args->size % PAGE_SIZE) != 0) {
		error = EINVAL;
		goto out;
	}
	if (args->gpa < mach->gpa_begin || args->gpa >= mach->gpa_end) {
		error = EINVAL;
		goto out;
	}
	if (args->gpa + args->size <= args->gpa) {
		error = EINVAL;
		goto out;
	}
	if (args->gpa + args->size >= mach->gpa_end) {
		error = EINVAL;
		goto out;
	}
	gpa = args->gpa;

	/* Unmap the memory from the machine. */
	uvm_unmap(&mach->vm->vm_map, gpa, gpa + args->size);

out:
	nvmm_machine_put(mach);
	return error;
}

/* -------------------------------------------------------------------------- */

static int
nvmm_init(void)
{
	size_t i, n;

	for (i = 0; i < __arraycount(nvmm_impl_list); i++) {
		if (!(*nvmm_impl_list[i]->ident)()) {
			continue;
		}
		nvmm_impl = nvmm_impl_list[i];
		break;
	}
	if (nvmm_impl == NULL) {
		printf("[!] No implementation found\n");
		return ENOTSUP;
	}

	for (i = 0; i < NVMM_MAX_MACHINES; i++) {
		machines[i].machid = i;
		rw_init(&machines[i].lock);
		for (n = 0; n < NVMM_MAX_VCPUS; n++) {
			mutex_init(&machines[i].cpus[n].lock, MUTEX_DEFAULT,
			    IPL_NONE);
			machines[i].cpus[n].hcpu_last = -1;
		}
	}

	(*nvmm_impl->init)();

	return 0;
}

static void
nvmm_fini(void)
{
	size_t i, n;

	for (i = 0; i < NVMM_MAX_MACHINES; i++) {
		rw_destroy(&machines[i].lock);
		for (n = 0; n < NVMM_MAX_VCPUS; n++) {
			mutex_destroy(&machines[i].cpus[n].lock);
		}
		/* TODO need to free stuff, etc */
	}

	(*nvmm_impl->fini)();
}

/* -------------------------------------------------------------------------- */

static int
nvmm_open(dev_t dev, int flags, int type, struct lwp *l)
{
	if (minor(dev) != 0) {
		return EXDEV;
	}

	return 0;
}

static int
nvmm_close(dev_t dev, int flags, int type, struct lwp *l)
{
	KASSERT(minor(dev) == 0);

	nvmm_kill_machines(l->l_proc->p_pid);

	return 0;
}

static int
nvmm_ioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	KASSERT(minor(dev) == 0);

	switch (cmd) {
	case NVMM_IOC_CAPABILITY:
		return nvmm_capability(data);
	case NVMM_IOC_MACHINE_CREATE:
		return nvmm_machine_create(data);
	case NVMM_IOC_MACHINE_DESTROY:
		return nvmm_machine_destroy(data);
	case NVMM_IOC_MACHINE_CONFIGURE:
		return nvmm_machine_configure(data);
	case NVMM_IOC_VCPU_CREATE:
		return nvmm_vcpu_create(data);
	case NVMM_IOC_VCPU_DESTROY:
		return nvmm_vcpu_destroy(data);
	case NVMM_IOC_VCPU_SETSTATE:
		return nvmm_vcpu_setstate(data);
	case NVMM_IOC_VCPU_GETSTATE:
		return nvmm_vcpu_getstate(data);
	case NVMM_IOC_VCPU_INJECT:
		return nvmm_vcpu_inject(data);
	case NVMM_IOC_VCPU_RUN:
		return nvmm_vcpu_run(data);
	case NVMM_IOC_GPA_MAP:
		return nvmm_gpa_map(data);
	case NVMM_IOC_GPA_UNMAP:
		return nvmm_gpa_unmap(data);
	case NVMM_IOC_HVA_MAP:
		return nvmm_hva_map(data);
	case NVMM_IOC_HVA_UNMAP:
		return nvmm_hva_unmap(data);
	default:
		return EINVAL;
	}
}

const struct cdevsw nvmm_cdevsw = {
	.d_open = nvmm_open,
	.d_close = nvmm_close,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = nvmm_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

void
nvmmattach(int nunits)
{
	/* nothing */
}

MODULE(MODULE_CLASS_DRIVER, nvmm, NULL);

static int
nvmm_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = nvmm_init();
		if (error)
			return error;

#if defined(_MODULE)
		{
			devmajor_t bmajor = NODEVMAJOR;
			devmajor_t cmajor = 345;

			/* mknod /dev/nvmm c 345 0 */
			error = devsw_attach("nvmm", NULL, &bmajor,
			    &nvmm_cdevsw, &cmajor);
			if (error) {
				nvmm_fini();
				return error;
			}
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#if defined(_MODULE)
		{
			error = devsw_detach(NULL, &nvmm_cdevsw);
			if (error) {
				return error;
			}
		}
#endif
		nvmm_fini();
		return 0;

	default:
		return ENOTTY;
	}
}
