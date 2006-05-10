/* $NetBSD: sigsegv.c,v 1.5 2006/05/10 19:07:22 mrg Exp $ */

#include <sys/ucontext.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void
sigsegv(int signo, siginfo_t *info, void *ptr)
{
#ifdef DEBUG
	printf("%d %p %p\n", signo, info, ptr);
	if (info != NULL) {
		printf("si_signo=%d\n", info->si_signo);
		printf("si_errno=%d\n", info->si_errno);
		printf("si_code=%d\n", info->si_code);
		printf("si_trap=%d\n", info->si_trap);
		printf("si_addr=%p\n", info->si_addr);
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
	assert(info->si_signo == SIGSEGV);
	assert(info->si_errno == 0);
	assert(info->si_code == SEGV_MAPERR);
	assert(info->si_addr == (void *)0);
	exit(0);
}

int
main(void)
{
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = sigsegv;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGSEGV, &sa, NULL);
	*(long *)0 = 0;
	return 0;
}
