/* $NetBSD: cpuio.h,v 1.8 2018/03/18 04:10:39 christos Exp $ */

#ifndef _SYS_COMPAT_CPUIO_H_
#define _SYS_COMPAT_CPUIO_H_

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

#endif /* _SYS_COMPAT_CPUIO_H_ */
