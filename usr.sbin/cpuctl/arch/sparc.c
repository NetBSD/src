/*	$NetBSD: sparc.c,v 1.1 2018/01/16 08:23:18 mrg Exp $	*/

/*
 * Copyright (c) 2018 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: sparc.c,v 1.1 2018/01/16 08:23:18 mrg Exp $");
#endif

#include <sys/types.h>
#include <sys/cpuio.h>
#include <sys/sysctl.h>

#include <machine/cpu.h>

#include <stdio.h>
#include <err.h>

#include "../cpuctl.h"
#include "../../../sys/arch/sparc/sparc/cache_print.h"

void
identifycpu(int fd, const char *cpuname)
{
	char path[128];
	char *name;
	char *fpuname;
	int cpu_arch, mid, hz;
	struct cacheinfo cacheinfo;
	size_t len;

	len = sizeof(cpu_arch);
	if (sysctlbyname("machdep.cpu_arch", &cpu_arch, &len, 0, 0) == -1)
		err(1, "couldn't get machdep.cpu_arch");

	snprintf(path, sizeof path, "hw.%s.cacheinfo", cpuname);
	len = sizeof(cacheinfo);
	if (sysctlbyname(path, &cacheinfo, &len, 0, 0) == -1)
		err(1, "couldn't get %s", path);

	snprintf(path, sizeof path, "hw.%s.mid", cpuname);
	len = sizeof(mid);
	if (sysctlbyname(path, &mid, &len, 0, 0) == -1)
		err(1, "couldn't get %s", path);

	snprintf(path, sizeof path, "hw.%s.clock_frequency", cpuname);
	len = sizeof(hz);
	if (sysctlbyname(path, &hz, &len, 0, 0) == -1)
		err(1, "couldn't get %s", path);
	snprintf(path, sizeof path, "hw.%s.name", cpuname);
	name = asysctlbyname(path, &len);

	snprintf(path, sizeof path, "hw.%s.fpuname", cpuname);
	fpuname = asysctlbyname(path, &len);
	
	printf("%s: mid %d: %s @ %d MHz, %s FPU\n", cpuname, mid, name, hz / 1000000, fpuname);
	cache_printf_backend(&cacheinfo, cpuname);

	printf("%s: SPARC v%d\n", cpuname, cpu_arch);
}

bool
identifycpu_bind(void)
{

	return false;
}

int
ucodeupdate_check(int fd, struct cpu_ucode *uc)
{

	return 0;
}
