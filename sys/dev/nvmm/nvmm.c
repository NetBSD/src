/*	$NetBSD: nvmm.c,v 1.35 2020/08/18 17:04:37 maxv Exp $	*/

/*
 * Copyright (c) 2018-2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: nvmm.c,v 1.35 2020/08/18 17:04:37 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <sys/cpu.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/device.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>

#include "ioconf.h"

#include <dev/nvmm/nvmm.h>
#include <dev/nvmm/nvmm_internal.h>
#include <dev/nvmm/nvmm_ioctl.h>

static struct nvmm_machine machines[NVMM_MAX_MACHINES];
static volatile unsigned int nmachines __cacheline_aligned;

static const struct nvmm_impl *nvmm_impl_list[] = {
#if defined(__x86_64__)
	&nvmm_x86_svm,	/* x86 AMD SVM */
	&nvmm_x86_vmx	/* x86 Intel VMX */
#endif
};

static const struct nvmm_impl *nvmm_impl = NULL;

static struct nvmm_owner root_owner;

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
		mach->time = time_second;
		*ret = mach;
		atomic_inc_uint(&nmachines);
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
	atomic_dec_uint(&nmachines);
}

static int
nvmm_machine_get(struct nvmm_owner *owner, nvmm_machid_t machid,
    struct nvmm_machine **ret, bool writer)
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
	if (owner != &root_owner && mach->owner != owner) {
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
nvmm_vcpu_alloc(struct nvmm_machine *mach, nvmm_cpuid_t cpuid,
    struct nvmm_cpu **ret)
{
	struct nvmm_cpu *vcpu;

	if (cpuid >= NVMM_MAX_VCPUS) {
		return EINVAL;
	}
	vcpu = &mach->cpus[cpuid];

	mutex_enter(&vcpu->lock);
	if (vcpu->present) {
		mutex_exit(&vcpu->lock);
		return EBUSY;
	}

	vcpu->present = true;
	vcpu->comm = NULL;
	vcpu->hcpu_last = -1;
	*ret = vcpu;
	return 0;
}

static void
nvmm_vcpu_free(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	KASSERT(mutex_owned(&vcpu->lock));
	vcpu->present = false;
	if (vcpu->comm != NULL) {
		uvm_deallocate(kernel_map, (vaddr_t)vcpu->comm, PAGE_SIZE);
	}
}

static int
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

static void
nvmm_vcpu_put(struct nvmm_cpu *vcpu)
{
	mutex_exit(&vcpu->lock);
}

/* -------------------------------------------------------------------------- */

static void
nvmm_kill_machines(struct nvmm_owner *owner)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	size_t i, j;
	int error;

	for (i = 0; i < NVMM_MAX_MACHINES; i++) {
		mach = &machines[i];

		rw_enter(&mach->lock, RW_WRITER);
		if (!mach->present || mach->owner != owner) {
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
		(*nvmm_impl->machine_destroy)(mach);
		uvmspace_free(mach->vm);

		/* Drop the kernel UOBJ refs. */
		for (j = 0; j < NVMM_MAX_HMAPPINGS; j++) {
			if (!mach->hmap[j].present)
				continue;
			uao_detach(mach->hmap[j].uobj);
		}

		nvmm_machine_free(mach);

		rw_exit(&mach->lock);
	}
}

/* -------------------------------------------------------------------------- */

static int
nvmm_capability(struct nvmm_owner *owner, struct nvmm_ioc_capability *args)
{
	args->cap.version = NVMM_KERN_VERSION;
	args->cap.state_size = nvmm_impl->state_size;
	args->cap.max_machines = NVMM_MAX_MACHINES;
	args->cap.max_vcpus = NVMM_MAX_VCPUS;
	args->cap.max_ram = NVMM_MAX_RAM;

	(*nvmm_impl->capability)(&args->cap);

	return 0;
}

static int
nvmm_machine_create(struct nvmm_owner *owner,
    struct nvmm_ioc_machine_create *args)
{
	struct nvmm_machine *mach;
	int error;

	error = nvmm_machine_alloc(&mach);
	if (error)
		return error;

	/* Curproc owns the machine. */
	mach->owner = owner;

	/* Zero out the host mappings. */
	memset(&mach->hmap, 0, sizeof(mach->hmap));

	/* Create the machine vmspace. */
	mach->gpa_begin = 0;
	mach->gpa_end = NVMM_MAX_RAM;
	mach->vm = uvmspace_alloc(0, mach->gpa_end - mach->gpa_begin, false);

	/* Create the comm uobj. */
	mach->commuobj = uao_create(NVMM_MAX_VCPUS * PAGE_SIZE, 0);

	(*nvmm_impl->machine_create)(mach);

	args->machid = mach->machid;
	nvmm_machine_put(mach);

	return 0;
}

static int
nvmm_machine_destroy(struct nvmm_owner *owner,
    struct nvmm_ioc_machine_destroy *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;
	size_t i;

	error = nvmm_machine_get(owner, args->machid, &mach, true);
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
	for (i = 0; i < NVMM_MAX_HMAPPINGS; i++) {
		if (!mach->hmap[i].present)
			continue;
		uao_detach(mach->hmap[i].uobj);
	}

	nvmm_machine_free(mach);
	nvmm_machine_put(mach);

	return 0;
}

static int
nvmm_machine_configure(struct nvmm_owner *owner,
    struct nvmm_ioc_machine_configure *args)
{
	struct nvmm_machine *mach;
	size_t allocsz;
	uint64_t op;
	void *data;
	int error;

	op = NVMM_MACH_CONF_MD(args->op);
	if (__predict_false(op >= nvmm_impl->mach_conf_max)) {
		return EINVAL;
	}

	allocsz = nvmm_impl->mach_conf_sizes[op];
	data = kmem_alloc(allocsz, KM_SLEEP);

	error = nvmm_machine_get(owner, args->machid, &mach, true);
	if (error) {
		kmem_free(data, allocsz);
		return error;
	}

	error = copyin(args->conf, data, allocsz);
	if (error) {
		goto out;
	}

	error = (*nvmm_impl->machine_configure)(mach, op, data);

out:
	nvmm_machine_put(mach);
	kmem_free(data, allocsz);
	return error;
}

static int
nvmm_vcpu_create(struct nvmm_owner *owner, struct nvmm_ioc_vcpu_create *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(owner, args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_alloc(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	/* Allocate the comm page. */
	uao_reference(mach->commuobj);
	error = uvm_map(kernel_map, (vaddr_t *)&vcpu->comm, PAGE_SIZE,
	    mach->commuobj, args->cpuid * PAGE_SIZE, 0, UVM_MAPFLAG(UVM_PROT_RW,
	    UVM_PROT_RW, UVM_INH_SHARE, UVM_ADV_RANDOM, 0));
	if (error) {
		uao_detach(mach->commuobj);
		nvmm_vcpu_free(mach, vcpu);
		nvmm_vcpu_put(vcpu);
		goto out;
	}
	error = uvm_map_pageable(kernel_map, (vaddr_t)vcpu->comm,
	    (vaddr_t)vcpu->comm + PAGE_SIZE, false, 0);
	if (error) {
		nvmm_vcpu_free(mach, vcpu);
		nvmm_vcpu_put(vcpu);
		goto out;
	}
	memset(vcpu->comm, 0, PAGE_SIZE);

	error = (*nvmm_impl->vcpu_create)(mach, vcpu);
	if (error) {
		nvmm_vcpu_free(mach, vcpu);
		nvmm_vcpu_put(vcpu);
		goto out;
	}

	nvmm_vcpu_put(vcpu);

	atomic_inc_uint(&mach->ncpus);

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_vcpu_destroy(struct nvmm_owner *owner, struct nvmm_ioc_vcpu_destroy *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(owner, args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	(*nvmm_impl->vcpu_destroy)(mach, vcpu);
	nvmm_vcpu_free(mach, vcpu);
	nvmm_vcpu_put(vcpu);

	atomic_dec_uint(&mach->ncpus);

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_vcpu_configure(struct nvmm_owner *owner,
    struct nvmm_ioc_vcpu_configure *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	size_t allocsz;
	uint64_t op;
	void *data;
	int error;

	op = NVMM_VCPU_CONF_MD(args->op);
	if (__predict_false(op >= nvmm_impl->vcpu_conf_max))
		return EINVAL;

	allocsz = nvmm_impl->vcpu_conf_sizes[op];
	data = kmem_alloc(allocsz, KM_SLEEP);

	error = nvmm_machine_get(owner, args->machid, &mach, false);
	if (error) {
		kmem_free(data, allocsz);
		return error;
	}

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error) {
		nvmm_machine_put(mach);
		kmem_free(data, allocsz);
		return error;
	}

	error = copyin(args->conf, data, allocsz);
	if (error) {
		goto out;
	}

	error = (*nvmm_impl->vcpu_configure)(vcpu, op, data);

out:
	nvmm_vcpu_put(vcpu);
	nvmm_machine_put(mach);
	kmem_free(data, allocsz);
	return error;
}

static int
nvmm_vcpu_setstate(struct nvmm_owner *owner,
    struct nvmm_ioc_vcpu_setstate *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(owner, args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	(*nvmm_impl->vcpu_setstate)(vcpu);
	nvmm_vcpu_put(vcpu);

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_vcpu_getstate(struct nvmm_owner *owner,
    struct nvmm_ioc_vcpu_getstate *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(owner, args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	(*nvmm_impl->vcpu_getstate)(vcpu);
	nvmm_vcpu_put(vcpu);

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_vcpu_inject(struct nvmm_owner *owner, struct nvmm_ioc_vcpu_inject *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(owner, args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	error = (*nvmm_impl->vcpu_inject)(vcpu);
	nvmm_vcpu_put(vcpu);

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_do_vcpu_run(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_vcpu_exit *exit)
{
	struct vmspace *vm = mach->vm;
	int ret;

	while (1) {
		/* Got a signal? Or pending resched? Leave. */
		if (__predict_false(nvmm_return_needed())) {
			exit->reason = NVMM_VCPU_EXIT_NONE;
			return 0;
		}

		/* Run the VCPU. */
		ret = (*nvmm_impl->vcpu_run)(mach, vcpu, exit);
		if (__predict_false(ret != 0)) {
			return ret;
		}

		/* Process nested page faults. */
		if (__predict_true(exit->reason != NVMM_VCPU_EXIT_MEMORY)) {
			break;
		}
		if (exit->u.mem.gpa >= mach->gpa_end) {
			break;
		}
		if (uvm_fault(&vm->vm_map, exit->u.mem.gpa, exit->u.mem.prot)) {
			break;
		}
	}

	return 0;
}

static int
nvmm_vcpu_run(struct nvmm_owner *owner, struct nvmm_ioc_vcpu_run *args)
{
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;

	error = nvmm_machine_get(owner, args->machid, &mach, false);
	if (error)
		return error;

	error = nvmm_vcpu_get(mach, args->cpuid, &vcpu);
	if (error)
		goto out;

	error = nvmm_do_vcpu_run(mach, vcpu, &args->exit);
	nvmm_vcpu_put(vcpu);

out:
	nvmm_machine_put(mach);
	return error;
}

/* -------------------------------------------------------------------------- */

static struct uvm_object *
nvmm_hmapping_getuobj(struct nvmm_machine *mach, uintptr_t hva, size_t size,
   size_t *off)
{
	struct nvmm_hmapping *hmapping;
	size_t i;

	for (i = 0; i < NVMM_MAX_HMAPPINGS; i++) {
		hmapping = &mach->hmap[i];
		if (!hmapping->present) {
			continue;
		}
		if (hva >= hmapping->hva &&
		    hva + size <= hmapping->hva + hmapping->size) {
			*off = hva - hmapping->hva;
			return hmapping->uobj;
		}
	}

	return NULL;
}

static int
nvmm_hmapping_validate(struct nvmm_machine *mach, uintptr_t hva, size_t size)
{
	struct nvmm_hmapping *hmapping;
	size_t i;

	if ((hva % PAGE_SIZE) != 0 || (size % PAGE_SIZE) != 0) {
		return EINVAL;
	}
	if (hva == 0) {
		return EINVAL;
	}

	for (i = 0; i < NVMM_MAX_HMAPPINGS; i++) {
		hmapping = &mach->hmap[i];
		if (!hmapping->present) {
			continue;
		}

		if (hva >= hmapping->hva &&
		    hva + size <= hmapping->hva + hmapping->size) {
			break;
		}

		if (hva >= hmapping->hva &&
		    hva < hmapping->hva + hmapping->size) {
			return EEXIST;
		}
		if (hva + size > hmapping->hva &&
		    hva + size <= hmapping->hva + hmapping->size) {
			return EEXIST;
		}
		if (hva <= hmapping->hva &&
		    hva + size >= hmapping->hva + hmapping->size) {
			return EEXIST;
		}
	}

	return 0;
}

static struct nvmm_hmapping *
nvmm_hmapping_alloc(struct nvmm_machine *mach)
{
	struct nvmm_hmapping *hmapping;
	size_t i;

	for (i = 0; i < NVMM_MAX_HMAPPINGS; i++) {
		hmapping = &mach->hmap[i];
		if (!hmapping->present) {
			hmapping->present = true;
			return hmapping;
		}
	}

	return NULL;
}

static int
nvmm_hmapping_free(struct nvmm_machine *mach, uintptr_t hva, size_t size)
{
	struct vmspace *vmspace = curproc->p_vmspace;
	struct nvmm_hmapping *hmapping;
	size_t i;

	for (i = 0; i < NVMM_MAX_HMAPPINGS; i++) {
		hmapping = &mach->hmap[i];
		if (!hmapping->present || hmapping->hva != hva ||
		    hmapping->size != size) {
			continue;
		}

		uvm_unmap(&vmspace->vm_map, hmapping->hva,
		    hmapping->hva + hmapping->size);
		uao_detach(hmapping->uobj);

		hmapping->uobj = NULL;
		hmapping->present = false;

		return 0;
	}

	return ENOENT;
}

static int
nvmm_hva_map(struct nvmm_owner *owner, struct nvmm_ioc_hva_map *args)
{
	struct vmspace *vmspace = curproc->p_vmspace;
	struct nvmm_machine *mach;
	struct nvmm_hmapping *hmapping;
	vaddr_t uva;
	int error;

	error = nvmm_machine_get(owner, args->machid, &mach, true);
	if (error)
		return error;

	error = nvmm_hmapping_validate(mach, args->hva, args->size);
	if (error)
		goto out;

	hmapping = nvmm_hmapping_alloc(mach);
	if (hmapping == NULL) {
		error = ENOBUFS;
		goto out;
	}

	hmapping->hva = args->hva;
	hmapping->size = args->size;
	hmapping->uobj = uao_create(hmapping->size, 0);
	uva = hmapping->hva;

	/* Take a reference for the user. */
	uao_reference(hmapping->uobj);

	/* Map the uobj into the user address space, as pageable. */
	error = uvm_map(&vmspace->vm_map, &uva, hmapping->size, hmapping->uobj,
	    0, 0, UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW, UVM_INH_SHARE,
	    UVM_ADV_RANDOM, UVM_FLAG_FIXED|UVM_FLAG_UNMAP));
	if (error) {
		uao_detach(hmapping->uobj);
	}

out:
	nvmm_machine_put(mach);
	return error;
}

static int
nvmm_hva_unmap(struct nvmm_owner *owner, struct nvmm_ioc_hva_unmap *args)
{
	struct nvmm_machine *mach;
	int error;

	error = nvmm_machine_get(owner, args->machid, &mach, true);
	if (error)
		return error;

	error = nvmm_hmapping_free(mach, args->hva, args->size);

	nvmm_machine_put(mach);
	return error;
}

/* -------------------------------------------------------------------------- */

static int
nvmm_gpa_map(struct nvmm_owner *owner, struct nvmm_ioc_gpa_map *args)
{
	struct nvmm_machine *mach;
	struct uvm_object *uobj;
	gpaddr_t gpa;
	size_t off;
	int error;

	error = nvmm_machine_get(owner, args->machid, &mach, false);
	if (error)
		return error;

	if ((args->prot & ~(PROT_READ|PROT_WRITE|PROT_EXEC)) != 0) {
		error = EINVAL;
		goto out;
	}

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

	uobj = nvmm_hmapping_getuobj(mach, args->hva, args->size, &off);
	if (uobj == NULL) {
		error = EINVAL;
		goto out;
	}

	/* Take a reference for the machine. */
	uao_reference(uobj);

	/* Map the uobj into the machine address space, as pageable. */
	error = uvm_map(&mach->vm->vm_map, &gpa, args->size, uobj, off, 0,
	    UVM_MAPFLAG(args->prot, UVM_PROT_RWX, UVM_INH_NONE,
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
nvmm_gpa_unmap(struct nvmm_owner *owner, struct nvmm_ioc_gpa_unmap *args)
{
	struct nvmm_machine *mach;
	gpaddr_t gpa;
	int error;

	error = nvmm_machine_get(owner, args->machid, &mach, false);
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
nvmm_ctl_mach_info(struct nvmm_owner *owner, struct nvmm_ioc_ctl *args)
{
	struct nvmm_ctl_mach_info ctl;
	struct nvmm_machine *mach;
	struct nvmm_cpu *vcpu;
	int error;
	size_t i;

	if (args->size != sizeof(ctl))
		return EINVAL;
	error = copyin(args->data, &ctl, sizeof(ctl));
	if (error)
		return error;

	error = nvmm_machine_get(owner, ctl.machid, &mach, true);
	if (error)
		return error;

	ctl.nvcpus = 0;
	for (i = 0; i < NVMM_MAX_VCPUS; i++) {
		error = nvmm_vcpu_get(mach, i, &vcpu);
		if (error)
			continue;
		ctl.nvcpus++;
		nvmm_vcpu_put(vcpu);
	}

	ctl.nram = 0;
	for (i = 0; i < NVMM_MAX_HMAPPINGS; i++) {
		if (!mach->hmap[i].present)
			continue;
		ctl.nram += mach->hmap[i].size;
	}

	ctl.pid = mach->owner->pid;
	ctl.time = mach->time;

	nvmm_machine_put(mach);

	error = copyout(&ctl, args->data, sizeof(ctl));
	if (error)
		return error;

	return 0;
}

static int
nvmm_ctl(struct nvmm_owner *owner, struct nvmm_ioc_ctl *args)
{
	switch (args->op) {
	case NVMM_CTL_MACH_INFO:
		return nvmm_ctl_mach_info(owner, args);
	default:
		return EINVAL;
	}
}

/* -------------------------------------------------------------------------- */

static const struct nvmm_impl *
nvmm_ident(void)
{
	size_t i;

	for (i = 0; i < __arraycount(nvmm_impl_list); i++) {
		if ((*nvmm_impl_list[i]->ident)())
			return nvmm_impl_list[i];
	}

	return NULL;
}

static int
nvmm_init(void)
{
	size_t i, n;

	nvmm_impl = nvmm_ident();
	if (nvmm_impl == NULL)
		return ENOTSUP;

	for (i = 0; i < NVMM_MAX_MACHINES; i++) {
		machines[i].machid = i;
		rw_init(&machines[i].lock);
		for (n = 0; n < NVMM_MAX_VCPUS; n++) {
			machines[i].cpus[n].present = false;
			machines[i].cpus[n].cpuid = n;
			mutex_init(&machines[i].cpus[n].lock, MUTEX_DEFAULT,
			    IPL_NONE);
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
	}

	(*nvmm_impl->fini)();
	nvmm_impl = NULL;
}

/* -------------------------------------------------------------------------- */

static dev_type_open(nvmm_open);

const struct cdevsw nvmm_cdevsw = {
	.d_open = nvmm_open,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

static int nvmm_ioctl(file_t *, u_long, void *);
static int nvmm_close(file_t *);
static int nvmm_mmap(file_t *, off_t *, size_t, int, int *, int *,
    struct uvm_object **, int *);

static const struct fileops nvmm_fileops = {
	.fo_read = fbadop_read,
	.fo_write = fbadop_write,
	.fo_ioctl = nvmm_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = fnullop_poll,
	.fo_stat = fbadop_stat,
	.fo_close = nvmm_close,
	.fo_kqfilter = fnullop_kqfilter,
	.fo_restart = fnullop_restart,
	.fo_mmap = nvmm_mmap,
};

static int
nvmm_open(dev_t dev, int flags, int type, struct lwp *l)
{
	struct nvmm_owner *owner;
	struct file *fp;
	int error, fd;

	if (__predict_false(nvmm_impl == NULL))
		return ENXIO;
	if (minor(dev) != 0)
		return EXDEV;
	if (!(flags & O_CLOEXEC))
		return EINVAL;
	error = fd_allocfile(&fp, &fd);
	if (error)
		return error;

	if (OFLAGS(flags) & O_WRONLY) {
		owner = &root_owner;
	} else {
		owner = kmem_alloc(sizeof(*owner), KM_SLEEP);
		owner->pid = l->l_proc->p_pid;
	}

	return fd_clone(fp, fd, flags, &nvmm_fileops, owner);
}

static int
nvmm_close(file_t *fp)
{
	struct nvmm_owner *owner = fp->f_data;

	KASSERT(owner != NULL);
	nvmm_kill_machines(owner);
	if (owner != &root_owner) {
		kmem_free(owner, sizeof(*owner));
	}
	fp->f_data = NULL;

   	return 0;
}

static int
nvmm_mmap(file_t *fp, off_t *offp, size_t size, int prot, int *flagsp,
    int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	struct nvmm_owner *owner = fp->f_data;
	struct nvmm_machine *mach;
	nvmm_machid_t machid;
	nvmm_cpuid_t cpuid;
	int error;

	if (prot & PROT_EXEC)
		return EACCES;
	if (size != PAGE_SIZE)
		return EINVAL;

	cpuid = NVMM_COMM_CPUID(*offp);
	if (__predict_false(cpuid >= NVMM_MAX_VCPUS))
		return EINVAL;

	machid = NVMM_COMM_MACHID(*offp);
	error = nvmm_machine_get(owner, machid, &mach, false);
	if (error)
		return error;

	uao_reference(mach->commuobj);
	*uobjp = mach->commuobj;
	*offp = cpuid * PAGE_SIZE;
	*maxprotp = prot;
	*advicep = UVM_ADV_RANDOM;

	nvmm_machine_put(mach);
	return 0;
}

static int
nvmm_ioctl(file_t *fp, u_long cmd, void *data)
{
	struct nvmm_owner *owner = fp->f_data;

	KASSERT(owner != NULL);

	switch (cmd) {
	case NVMM_IOC_CAPABILITY:
		return nvmm_capability(owner, data);
	case NVMM_IOC_MACHINE_CREATE:
		return nvmm_machine_create(owner, data);
	case NVMM_IOC_MACHINE_DESTROY:
		return nvmm_machine_destroy(owner, data);
	case NVMM_IOC_MACHINE_CONFIGURE:
		return nvmm_machine_configure(owner, data);
	case NVMM_IOC_VCPU_CREATE:
		return nvmm_vcpu_create(owner, data);
	case NVMM_IOC_VCPU_DESTROY:
		return nvmm_vcpu_destroy(owner, data);
	case NVMM_IOC_VCPU_CONFIGURE:
		return nvmm_vcpu_configure(owner, data);
	case NVMM_IOC_VCPU_SETSTATE:
		return nvmm_vcpu_setstate(owner, data);
	case NVMM_IOC_VCPU_GETSTATE:
		return nvmm_vcpu_getstate(owner, data);
	case NVMM_IOC_VCPU_INJECT:
		return nvmm_vcpu_inject(owner, data);
	case NVMM_IOC_VCPU_RUN:
		return nvmm_vcpu_run(owner, data);
	case NVMM_IOC_GPA_MAP:
		return nvmm_gpa_map(owner, data);
	case NVMM_IOC_GPA_UNMAP:
		return nvmm_gpa_unmap(owner, data);
	case NVMM_IOC_HVA_MAP:
		return nvmm_hva_map(owner, data);
	case NVMM_IOC_HVA_UNMAP:
		return nvmm_hva_unmap(owner, data);
	case NVMM_IOC_CTL:
		return nvmm_ctl(owner, data);
	default:
		return EINVAL;
	}
}

/* -------------------------------------------------------------------------- */

static int nvmm_match(device_t, cfdata_t, void *);
static void nvmm_attach(device_t, device_t, void *);
static int nvmm_detach(device_t, int);

extern struct cfdriver nvmm_cd;

CFATTACH_DECL_NEW(nvmm, 0, nvmm_match, nvmm_attach, nvmm_detach, NULL);

static struct cfdata nvmm_cfdata[] = {
	{
		.cf_name = "nvmm",
		.cf_atname = "nvmm",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = NULL,
		.cf_flags = 0,
		.cf_pspec = NULL,
	},
	{ NULL, NULL, 0, FSTATE_NOTFOUND, NULL, 0, NULL }
};

static int
nvmm_match(device_t self, cfdata_t cfdata, void *arg)
{
	return 1;
}

static void
nvmm_attach(device_t parent, device_t self, void *aux)
{
	int error;

	error = nvmm_init();
	if (error)
		panic("%s: impossible", __func__);
	aprint_normal_dev(self, "attached, using backend %s\n",
	    nvmm_impl->name);
}

static int
nvmm_detach(device_t self, int flags)
{
	if (atomic_load_relaxed(&nmachines) > 0)
		return EBUSY;
	nvmm_fini();
	return 0;
}

void
nvmmattach(int nunits)
{
	/* nothing */
}

MODULE(MODULE_CLASS_MISC, nvmm, NULL);

#if defined(_MODULE)
CFDRIVER_DECL(nvmm, DV_VIRTUAL, NULL);
#endif

static int
nvmm_modcmd(modcmd_t cmd, void *arg)
{
#if defined(_MODULE)
	devmajor_t bmajor = NODEVMAJOR;
	devmajor_t cmajor = 345;
#endif
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if (nvmm_ident() == NULL) {
			aprint_error("%s: cpu not supported\n",
			    nvmm_cd.cd_name);
			return ENOTSUP;
		}
#if defined(_MODULE)
		error = config_cfdriver_attach(&nvmm_cd);
		if (error)
			return error;
#endif
		error = config_cfattach_attach(nvmm_cd.cd_name, &nvmm_ca);
		if (error) {
			config_cfdriver_detach(&nvmm_cd);
			aprint_error("%s: config_cfattach_attach failed\n",
			    nvmm_cd.cd_name);
			return error;
		}

		error = config_cfdata_attach(nvmm_cfdata, 1);
		if (error) {
			config_cfattach_detach(nvmm_cd.cd_name, &nvmm_ca);
			config_cfdriver_detach(&nvmm_cd);
			aprint_error("%s: unable to register cfdata\n",
			    nvmm_cd.cd_name);
			return error;
		}

		if (config_attach_pseudo(nvmm_cfdata) == NULL) {
			aprint_error("%s: config_attach_pseudo failed\n",
			    nvmm_cd.cd_name);
			config_cfattach_detach(nvmm_cd.cd_name, &nvmm_ca);
			config_cfdriver_detach(&nvmm_cd);
			return ENXIO;
		}

#if defined(_MODULE)
		/* mknod /dev/nvmm c 345 0 */
		error = devsw_attach(nvmm_cd.cd_name, NULL, &bmajor,
			&nvmm_cdevsw, &cmajor);
		if (error) {
			aprint_error("%s: unable to register devsw\n",
			    nvmm_cd.cd_name);
			config_cfattach_detach(nvmm_cd.cd_name, &nvmm_ca);
			config_cfdriver_detach(&nvmm_cd);
			return error;
		}
#endif
		return 0;
	case MODULE_CMD_FINI:
		error = config_cfdata_detach(nvmm_cfdata);
		if (error)
			return error;
		error = config_cfattach_detach(nvmm_cd.cd_name, &nvmm_ca);
		if (error)
			return error;
#if defined(_MODULE)
		config_cfdriver_detach(&nvmm_cd);
		devsw_detach(NULL, &nvmm_cdevsw);
#endif
		return 0;
	case MODULE_CMD_AUTOUNLOAD:
		return EBUSY;
	default:
		return ENOTTY;
	}
}
