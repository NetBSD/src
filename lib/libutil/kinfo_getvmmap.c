#include <sys/cdefs.h>
#ifdef __FBSDID
__FBSDID("$FreeBSD: head/lib/libutil/kinfo_getvmmap.c 186512 2008-12-27 11:12:23Z rwatson $");
#endif
__RCSID("$NetBSD: kinfo_getvmmap.c,v 1.4 2017/06/14 12:24:51 kamil Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <uvm/uvm_param.h>

struct kinfo_vmentry *
kinfo_getvmmap(pid_t pid, size_t *cntp)
{
	int mib[5];
	int error;
	size_t len;
	struct kinfo_vmentry *kiv;

	*cntp = 0;
	len = 0;
	mib[0] = CTL_VM;
	mib[1] = VM_PROC;
	mib[2] = VM_PROC_MAP;
	mib[3] = pid;
	mib[4] = sizeof(*kiv);

	error = sysctl(mib, (u_int)__arraycount(mib), NULL, &len, NULL, 0);
	if (error)
		return NULL;

	len = len * 4 / 3;

	kiv = malloc(len);
	if (kiv == NULL)
		return NULL;

	error = sysctl(mib, (u_int)__arraycount(mib), kiv, &len, NULL, 0);
	if (error) {
		free(kiv);
		return NULL;
	}

	*cntp = len / sizeof(*kiv);
	return kiv;	/* Caller must free() return value */
}
