/*	$NetBSD: linux32_time.c,v 1.3 2006/05/14 21:24:50 elad Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: linux32_time.c,v 1.3 2006/05/14 21:24:50 elad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fstypes.h>
#include <sys/signal.h>
#include <sys/dirent.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/sa.h>
#include <sys/proc.h>
#include <sys/ucred.h>
#include <sys/swap.h>

#include <machine/types.h>

#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_oldolduname.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/common/linux32_socketcall.h>
#include <compat/linux32/linux32_syscallargs.h>

extern struct timezone linux_sys_tz;
int
linux32_sys_gettimeofday(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_gettimeofday_args /* {
		syscallarg(netbsd32_timevalp_t) tp;
		syscallarg(netbsd32_timezonep_t) tzp;
	} */ *uap = v;
	struct timeval tv;
	struct netbsd32_timeval tv32;
	int error;

	if (NETBSD32PTR64(SCARG(uap, tp)) != NULL) {
		microtime(&tv);
		netbsd32_from_timeval(&tv, &tv32);
		if ((error = copyout(&tv32, 
		    (caddr_t)NETBSD32PTR64(SCARG(uap, tp)), 
		    sizeof(tv32))) != 0)
			return error;
	}

	/* timezone size does not change */
	if (NETBSD32PTR64(SCARG(uap, tzp)) != NULL) {
		if ((error = copyout(&linux_sys_tz,
		    (caddr_t)NETBSD32PTR64(SCARG(uap, tzp)), 
		    sizeof(linux_sys_tz))) != 0)
			return error;
	}

	return 0;
}

int
linux32_sys_settimeofday(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_settimeofday_args /* {
		syscallarg(netbsd32_timevalp_t) tp;
		syscallarg(netbsd32_timezonep_t) tzp;
	} */ *uap = v;
	struct linux_sys_settimeofday_args ua;

	NETBSD32TOP_UAP(tp, struct timeval);
	NETBSD32TOP_UAP(tzp, struct timezone);

	return linux_sys_settimeofday(l, &ua, retval);
}

int
linux32_sys_time(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_time_args /* {
		syscallcarg(linux32_timep_t) t;
	} */ *uap = v;
        struct timeval atv;
        linux32_time_t tt;
        int error;
 
        microtime(&atv);

        tt = (linux32_time_t)atv.tv_sec;

        if (SCARG(uap, t) && (error = copyout(&tt, 
	    NETBSD32PTR64(SCARG(uap, t)), sizeof(tt))))
                return error;

        retval[0] = tt;

        return 0;
}


int
linux32_sys_times(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_times_args /* {
		syscallarg(linux32_tmsp_t) tms;
	} */ *uap = v;
	struct linux32_tms ltms32;
	struct linux_tms ltms;
	struct linux_tms *ltmsp;
	struct linux_sys_times_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);
	int error;

	ltmsp = stackgap_alloc(l->l_proc, &sg, sizeof(*ltmsp));
	SCARG(&ua, tms) = (struct times *)ltmsp;

	if ((error = linux_sys_times(l, &ua, retval)) != 0)
		return error;

	if ((error = copyin(ltmsp, &ltms, sizeof(ltms))) != 0)
		return error;

	ltms32.ltms32_utime = (linux32_clock_t)ltms.ltms_utime;
	ltms32.ltms32_stime = (linux32_clock_t)ltms.ltms_stime;
	ltms32.ltms32_cutime = (linux32_clock_t)ltms.ltms_cutime;
	ltms32.ltms32_cstime = (linux32_clock_t)ltms.ltms_cstime;

	if ((error = copyout(&ltms32, 
	    NETBSD32PTR64(SCARG(uap, tms)), sizeof(ltms32))) != 0)
		return error;

	return 0;
}

int
linux32_sys_stime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_stime_args /* {
		syscallarg(linux32_timep_t) t;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct timespec ts;
	linux32_time_t tt32;
	int error;
	
	if ((error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)) != 0)
		return error;

	if ((error = copyin(&tt32, 
	    NETBSD32PTR64(SCARG(uap, t)), 
	    sizeof tt32)) != 0)

	ts.tv_sec = (long)tt32;
	ts.tv_nsec = 0;

	return settime(p, &ts);
}

int
linux32_sys_utime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_utime_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(linux32_utimbufp_t) times;
	} */ *uap = v;
	struct proc *p = l->l_proc;
        caddr_t sg = stackgap_init(p, 0);
        struct sys_utimes_args ua;
        struct timeval tv[2], *tvp;
        struct linux32_utimbuf lut;
        int error;

	NETBSD32TOP_UAP(path, const char);
        CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));


        if (NETBSD32PTR64(SCARG(uap, times)) != NULL) {
                if ((error = copyin(NETBSD32PTR64(SCARG(uap, times)), 
		    &lut, sizeof lut)))
                        return error;

                tv[0].tv_sec = (long)lut.l_actime;
                tv[0].tv_usec = 0;
                tv[1].tv_sec = (long)lut.l_modtime;
		tv[1].tv_usec = 0;

	        tvp = (struct timeval *) stackgap_alloc(p, &sg, sizeof(tv));

                if ((error = copyout(tv, tvp, sizeof(tv))))
                        return error;
                SCARG(&ua, tptr) = tvp;
        } else {
               SCARG(&ua, tptr) = NULL;
	}
                     
        return sys_utimes(l, &ua, retval);
} 
