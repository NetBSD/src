#include <ucontext.h>


void _resumecontext(void)
{
	ucontext_t uc;

	getcontext(&uc);
	setcontext(uc.uc_link);
}
