/* $Id: pthread_md.h,v 1.1.2.1 2001/03/05 23:52:05 nathanw Exp $ */

/* Copyright */

#ifndef _LIB_PTHREAD_I386_MD_H
#define _LIB_PTHREAD_I386_MD_H


static __inline long
pthread__sp(void)
{
	long ret;
	__asm("movl %%esp, %0" : "=g" (ret));

	return ret;
}

#define pthread__uc_sp(ucp) ((ucp)->uc_mcontext.__gregs[_REG_UESP])

#endif /* _LIB_PTHREAD_I386_MD_H */
