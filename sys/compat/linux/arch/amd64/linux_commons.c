/*	$NetBSD: linux_commons.c,v 1.3.2.2 2005/11/10 14:00:59 skrll Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: linux_commons.c,v 1.3.2.2 2005/11/10 14:00:59 skrll Exp $");

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

#include "../../common/linux_pipe.c"
#include "../../common/linux_file64.c"
#include "../../common/linux_misc_notalpha.c"
#include "../../common/linux_sig_notalpha.c"
#include "../../common/linux_sigaction.c"
#include "../../common/linux_futex.c"
