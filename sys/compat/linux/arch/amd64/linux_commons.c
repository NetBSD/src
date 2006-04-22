/*	$NetBSD: linux_commons.c,v 1.4.6.1 2006/04/22 11:38:13 simonb Exp $ */

/*
 * This file includes C files from the common
 * area to decrese the number of files to compile
 * in order to make building a kernel go faster.
 *
 * Option headers and headers which depend on
 * certain options being set need to be included
 * here.  This ensures that a header file sees
 * the options it needs even if one of included
 * C files doesn't use it.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: linux_commons.c,v 1.4.6.1 2006/04/22 11:38:13 simonb Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#include "opt_ktrace.h"
#include "opt_nfsserver.h"
#include "fs_nfs.h"
#include "fs_lfs.h"
#endif

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>

#include "../../common/linux_pipe.c"
#include "../../common/linux_file64.c"
#include "../../common/linux_misc_notalpha.c"
#include "../../common/linux_sig_notalpha.c"
#include "../../common/linux_sigaction.c"
#include "../../common/linux_futex.c"
