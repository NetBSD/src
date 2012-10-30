/* $NetBSD: cpuio.h,v 1.3.10.1 2012/10/30 17:20:48 yamt Exp $ */

struct compat6_cpu_ucode {
	uint64_t version;
	char fwname[PATH_MAX];
};

#define OIOC_CPU_UCODE_GET_VERSION      _IOR('c', 4, struct compat6_cpu_ucode)
#define OIOC_CPU_UCODE_APPLY            _IOW('c', 5, struct compat6_cpu_ucode)
