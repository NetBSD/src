/* $NetBSD: sigfpe.c,v 1.2.2.2 2009/05/13 19:18:45 jym Exp $ */

#include <sys/time.h>
#include <sys/ucontext.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef __vax__
#include <ieeefp.h>
#endif

sig_atomic_t z = 0;

void
sigfpe(int signo, siginfo_t *info, void *ptr)
{
#ifdef DEBUG
	printf("%d %p %p\n", signo, info, ptr);
	if (info != NULL) {
		printf("si_signo=%d\n", info->si_signo);
		printf("si_errno=%d\n", info->si_errno);
		printf("si_code=%d\n", info->si_code);
		printf("si_value.sival_int=%d\n", info->si_value.sival_int);
	}
	if (ptr != NULL) {
		ucontext_t *ctx = ptr;
		int i;
		mcontext_t *mc = &ctx->uc_mcontext;
		printf("uc_flags 0x%x\n", ctx->uc_flags);
		printf("uc_link %p\n", ctx->uc_link);
		for (i = 0; i < sizeof(ctx->uc_sigmask.__bits) /
		    sizeof(ctx->uc_sigmask.__bits[0]); i++)
			printf("uc_sigmask[%d] 0x%x\n", i,
			    ctx->uc_sigmask.__bits[i]);
		printf("uc_stack %p %lu 0x%x\n", ctx->uc_stack.ss_sp, 
		    (unsigned long)ctx->uc_stack.ss_size,
		    ctx->uc_stack.ss_flags);
		for (i = 0; i < sizeof(mc->__gregs)/sizeof(mc->__gregs[0]); i++)
			printf("uc_mcontext.greg[%d] 0x%x\n", i,
			    mc->__gregs[i]);
	}
#endif
	assert(z++ == 0);
	assert(info->si_signo == SIGFPE);
	assert(info->si_code == FPE_FLTDIV);
	assert(info->si_errno == 0);
}

int
main(void)
{
	struct sigaction sa;
	double d = strtod("0", NULL);

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = sigfpe;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGFPE, &sa, NULL);
#ifndef __vax__
	fpsetmask(FP_X_INV|FP_X_DZ|FP_X_OFL|FP_X_UFL|FP_X_IMP);
#endif
	printf("%g\n", 1 / d);
	assert(z == 1);
	return 0;
}
