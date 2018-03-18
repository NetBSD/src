/* $NetBSD: cpuio.h,v 1.7 2018/03/18 00:51:45 christos Exp $ */

#include <sys/ioccom.h>

struct compat6_cpu_ucode {
	uint64_t version;
	char fwname[PATH_MAX];
};

extern int (*compat_cpuctl_ioctl)(struct lwp *, u_long, void *);

#define OIOC_CPU_UCODE_GET_VERSION      _IOR('c', 4, struct compat6_cpu_ucode)
#define OIOC_CPU_UCODE_APPLY            _IOW('c', 5, struct compat6_cpu_ucode)

void kern_cpu_60_init(void);
void kern_cpu_60_fini(void);
