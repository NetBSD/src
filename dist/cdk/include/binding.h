#ifndef CDKBINDING_H
#define CDKBINDING_H	1

#include <cdk/cdk.h>

/*
 * Copyright 1999, Mike Glover
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
 *    must display the following acknowledgment:
 * 	This product includes software developed by Mike Glover
 * 	and contributors.
 * 4. Neither the name of Mike Glover, nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MIKE GLOVER AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL MIKE GLOVER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Create definitions for the key bindings.
 */

/*
 * This is the key binding prototype, typed for use with Perl.
 */
#define BINDFN_PROTO(func)  \
	void (func) ( \
		EObjectType	/* cdktype */, \
		void *		/* object */, \
		void *		/* clientData */, \
		chtype		/* input */)

typedef BINDFN_PROTO(*BINDFN);

/*
 * This is the prototype for the process callback functions.
 */
typedef int (*PROCESSFN) (
		EObjectType	/* cdktype */,
		void *		/* object */,
		void *		/* clientData */,
		chtype 		/* input */);

/*
 * This binds the key to the event.
 */
void bindCDKObject (
		EObjectType	/* cdktype */,
		void *		/* object */,
		chtype		/* key */,
		BINDFN		/* function */,
		void *		/* data */);

/*
 * This unbinds the key from the event.
 */
void unbindCDKObject (
		EObjectType	/* cdktype */,
		void *		/* object */,
		chtype		/* key */);

/*
 * This checks if the given key has an event 'attached' to it.
 */
int checkCDKObjectBind (
		EObjectType	/* cdktype */,
		void *		/* object */,
		chtype		/* key */);

/*
 * This cleans out all of the key bindings.
 */
void cleanCDKObjectBindings (
		EObjectType	/* cdktype */,
		void *		/* object */);

#endif /* CDKBINDING_H */
