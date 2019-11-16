/*	$NetBSD: md.c,v 1.2 2019/11/16 21:25:14 martin Exp $ */

/* md.c -- Machine specific code for amd64 */

#include "../i386/md.c"

void	amd64_md_boot_cfg_finalize(const char *path);

void
amd64_md_boot_cfg_finalize(const char *path)
{
	char buf[MAXPATHLEN];

	if (get_kernel_set() != SET_KERNEL_2)
		return;

	run_program(RUN_CHROOT|RUN_FATAL,
	    "sh -c 'sed -e \"s:;boot:;pkboot:\" "
	    "< %s > %s.1", path, path);
	snprintf(buf, sizeof buf, "%s.1", path);
	mv_within_target_or_die(buf, path);
}
