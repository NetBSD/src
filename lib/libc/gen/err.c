/*
 * Copyright (c) 1994 Christos Zoulas
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(__err, err);
__weak_reference(__verr, verr);
__weak_reference(__errx, errx);
__weak_reference(__verrx, verrx);
__weak_reference(__warn, warn);
__weak_reference(__vwarn, vwarn);
__weak_reference(__warnx, warnx);
__weak_reference(__vwarnx, vwarnx);
#else

/*
 * Without weak references, we would have to put one function per file
 * in order to be able to replace only a single function without picking
 * up all the others. Too bad; I am not going to create 100 little files.
 * Also it is not that simple to just create dummy stubs that call __func
 * because most of the functions are varyadic and we need to parse the
 * argument list anyway.
 */

#define	__err err
#define	__verr verr
#define	__errx errx
#define	__verrx verrx
#define	__warn warn
#define	__vwarn vwarn
#define	__warnx warnx
#define	__vwarnx vwarnx

#include "__err.c"

#endif
