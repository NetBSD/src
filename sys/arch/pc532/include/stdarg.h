/* 
 * Copyright (c) 1993 Philip A. Nelson.
 * All rights reserved.
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
 *	This product includes software developed by Philip A. Nelson.
 * 4. The name of Philip A. Nelson may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP NELSON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *   Note that the type used in va_arg is supposed to match the
 *   actual type **after default promotions**.
 *   Thus, va_arg (..., short) is not valid.
 *
 *	machine/stdarg.h:
 *
 *	stdarg.h,v 1.1.1.1 1993/09/09 23:53:46 phil Exp
 */


#include <sys/types.h>   /* to make sure _VA_LIST_ gets defined if it
			    needs to. */

#ifndef _MACHINE_STDARG_H_
#define _MACHINE_STDARG_H_

/* The macro _VA_LIST_ is the same thing used by this file in Ultrix.  */
#ifdef _VA_LIST_
typedef _VA_LIST_ va_list;
/* #undef _VA_LIST_   This should be done, but stdio barffed if done. PAN */
#endif

/* Amount of space required in an argument list for an arg of type TYPE.
   TYPE may alternatively be an expression whose type is used.  */

#define __va_rounded_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

#define va_start(AP, LASTARG) 						\
 (AP = ((char *) __builtin_next_arg ()))

void va_end (va_list);
#define va_end(AP)

#define va_arg(AP, TYPE)						\
 (*((TYPE *) (AP += __va_rounded_size (TYPE),				\
	      AP - (sizeof (TYPE) < 4 ? sizeof (TYPE)			\
		    : __va_rounded_size (TYPE)))))

#endif
