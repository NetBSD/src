/*	$NetBSD: libnvmm.c,v 1.1 2018/11/10 09:28:56 maxv Exp $	*/

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "nvmm.h"

static int nvmm_fd = -1;
static size_t nvmm_page_size = 0;

/* -------------------------------------------------------------------------- */

static int
_nvmm_area_add(struct nvmm_machine *mach, gpaddr_t gpa, uintptr_t hva,
    size_t size)
{
	struct nvmm_area *area;
	void *ptr;
	size_t i;

	for (i = 0; i < mach->nareas; i++) {
		if (gpa >= mach->areas[i].gpa &&
		    gpa < mach->areas[i].gpa + mach->areas[i].size) {
			goto error;
		}
		if (gpa + size >= mach->areas[i].gpa &&
		    gpa + size < mach->areas[i].gpa + mach->areas[i].size) {
			goto error;
		}
		if (gpa < mach->areas[i].gpa &&
		    gpa + size >= mach->areas[i].gpa + mach->areas[i].size) {
			goto error;
		}
	}

	mach->nareas++;
	ptr = realloc(mach->areas, mach->nareas * sizeof(struct nvmm_area));
	if (ptr == NULL)
		return -1;
	mach->areas = ptr;

	area = &mach->areas[mach->nareas-1];
	area->gpa = gpa;
	area->hva = hva;
	area->size = size;

	return 0;

error:
	errno = EEXIST;
	return -1;
}

static int
_nvmm_area_delete(struct nvmm_machine *mach, gpaddr_t gpa, uintptr_t hva,
    size_t size)
{
	size_t i;

	for (i = 0; i < mach->nareas; i++) {
		if (gpa == mach->areas[i].gpa &&
		    hva == mach->areas[i].hva &&
		    size == mach->areas[i].size) {
			break;
		}
	}
	if (i == mach->nareas) {
		errno = ENOENT;
		return -1;
	}

	memcpy(&mach->areas[i], &mach->areas[i+1],
	    (mach->nareas - i - 1) * sizeof(struct nvmm_area));
	mach->nareas--;

	return 0;
}

/* -------------------------------------------------------------------------- */

static int
nvmm_init(void)
{
	if (nvmm_fd != -1)
		return 0;
	nvmm_fd = open("/dev/nvmm", O_RDWR);
	if (nvmm_fd == -1)
		return -1;
	nvmm_page_size = sysconf(_SC_PAGESIZE);
	return 0;
}

int
nvmm_capability(struct nvmm_capability *cap)
{
	struct nvmm_ioc_capability args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	ret = ioctl(nvmm_fd, NVMM_IOC_CAPABILITY, &args);
	if (ret == -1)
		return -1;

	memcpy(cap, &args.cap, sizeof(args.cap));

	return 0;
}

int
nvmm_machine_create(struct nvmm_machine *mach)
{
	struct nvmm_ioc_machine_create args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	ret = ioctl(nvmm_fd, NVMM_IOC_MACHINE_CREATE, &args);
	if (ret == -1)
		return -1;

	memset(mach, 0, sizeof(*mach));
	mach->machid = args.machid;

	return 0;
}

int
nvmm_machine_destroy(struct nvmm_machine *mach)
{
	struct nvmm_ioc_machine_destroy args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	args.machid = mach->machid;

	ret = ioctl(nvmm_fd, NVMM_IOC_MACHINE_DESTROY, &args);
	if (ret == -1)
		return -1;

	free(mach->areas);

	return 0;
}

int
nvmm_machine_configure(struct nvmm_machine *mach, uint64_t op, void *conf)
{
	struct nvmm_ioc_machine_configure args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	args.machid = mach->machid;
	args.op = op;
	args.conf = conf;

	ret = ioctl(nvmm_fd, NVMM_IOC_MACHINE_CONFIGURE, &args);
	if (ret == -1)
		return -1;

	return 0;
}

int
nvmm_vcpu_create(struct nvmm_machine *mach, nvmm_cpuid_t cpuid)
{
	struct nvmm_ioc_vcpu_create args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	args.machid = mach->machid;
	args.cpuid = cpuid;

	ret = ioctl(nvmm_fd, NVMM_IOC_VCPU_CREATE, &args);
	if (ret == -1)
		return -1;

	return 0;
}

int
nvmm_vcpu_destroy(struct nvmm_machine *mach, nvmm_cpuid_t cpuid)
{
	struct nvmm_ioc_vcpu_destroy args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	args.machid = mach->machid;
	args.cpuid = cpuid;

	ret = ioctl(nvmm_fd, NVMM_IOC_VCPU_DESTROY, &args);
	if (ret == -1)
		return -1;

	return 0;
}

int
nvmm_vcpu_setstate(struct nvmm_machine *mach, nvmm_cpuid_t cpuid,
    void *state, uint64_t flags)
{
	struct nvmm_ioc_vcpu_setstate args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	args.machid = mach->machid;
	args.cpuid = cpuid;
	args.state = state;
	args.flags = flags;

	ret = ioctl(nvmm_fd, NVMM_IOC_VCPU_SETSTATE, &args);
	if (ret == -1)
		return -1;

	return 0;
}

int
nvmm_vcpu_getstate(struct nvmm_machine *mach, nvmm_cpuid_t cpuid,
    void *state, uint64_t flags)
{
	struct nvmm_ioc_vcpu_getstate args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	args.machid = mach->machid;
	args.cpuid = cpuid;
	args.state = state;
	args.flags = flags;

	ret = ioctl(nvmm_fd, NVMM_IOC_VCPU_GETSTATE, &args);
	if (ret == -1)
		return -1;

	return 0;
}

int
nvmm_vcpu_inject(struct nvmm_machine *mach, nvmm_cpuid_t cpuid,
    struct nvmm_event *event)
{
	struct nvmm_ioc_vcpu_inject args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	args.machid = mach->machid;
	args.cpuid = cpuid;
	memcpy(&args.event, event, sizeof(args.event));

	ret = ioctl(nvmm_fd, NVMM_IOC_VCPU_INJECT, &args);
	if (ret == -1)
		return -1;

	return 0;
}

int
nvmm_vcpu_run(struct nvmm_machine *mach, nvmm_cpuid_t cpuid,
    struct nvmm_exit *exit)
{
	struct nvmm_ioc_vcpu_run args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	args.machid = mach->machid;
	args.cpuid = cpuid;
	memset(&args.exit, 0, sizeof(args.exit));

	ret = ioctl(nvmm_fd, NVMM_IOC_VCPU_RUN, &args);
	if (ret == -1)
		return -1;

	memcpy(exit, &args.exit, sizeof(args.exit));

	return 0;
}

int
nvmm_gpa_map(struct nvmm_machine *mach, uintptr_t hva, gpaddr_t gpa,
    size_t size, int flags)
{
	struct nvmm_ioc_gpa_map args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	args.machid = mach->machid;
	args.hva = hva;
	args.gpa = gpa;
	args.size = size;
	args.flags = flags;

	ret = ioctl(nvmm_fd, NVMM_IOC_GPA_MAP, &args);
	if (ret == -1)
		return -1;

	ret = _nvmm_area_add(mach, gpa, hva, size);
	if (ret == -1) {
		nvmm_gpa_unmap(mach, hva, gpa, size);
		return -1;
	}

	return 0;
}

int
nvmm_gpa_unmap(struct nvmm_machine *mach, uintptr_t hva, gpaddr_t gpa,
    size_t size)
{
	struct nvmm_ioc_gpa_unmap args;
	int ret;

	if (nvmm_init() == -1) {
		return -1;
	}

	ret = _nvmm_area_delete(mach, gpa, hva, size);
	if (ret == -1)
		return -1;

	args.machid = mach->machid;
	args.gpa = gpa;
	args.size = size;

	ret = ioctl(nvmm_fd, NVMM_IOC_GPA_UNMAP, &args);
	if (ret == -1)
		return -1;

	ret = munmap((void *)hva, size);

	return ret;
}

/*
 * nvmm_gva_to_gpa(): architecture-specific.
 */

int
nvmm_gpa_to_hva(struct nvmm_machine *mach, gpaddr_t gpa, uintptr_t *hva)
{
	size_t i;

	if (gpa % nvmm_page_size != 0) {
		errno = EINVAL;
		return -1;
	}

	for (i = 0; i < mach->nareas; i++) {
		if (gpa < mach->areas[i].gpa) {
			continue;
		}
		if (gpa >= mach->areas[i].gpa + mach->areas[i].size) {
			continue;
		}

		*hva = mach->areas[i].hva + (gpa - mach->areas[i].gpa);
		return 0;
	}

	errno = ENOENT;
	return -1;
}

/*
 * nvmm_assist_io(): architecture-specific.
 */
