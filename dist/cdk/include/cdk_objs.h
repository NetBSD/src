#ifndef CDK_OBJS_H
#define CDK_OBJS_H

/*
 * Copyright 1999, Thomas Dickey
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
 * THIS SOFTWARE IS PROVIDED BY THOMAS DICKEY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THOMAS DICKEY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

typedef struct CDKBINDING {
   BINDFN	bindFunction;
   void *	bindData;
   PROCESSFN	callbackfn;
} CDKBINDING;

/*
 * Methods common to all widgets.
 */
typedef struct CDKFUNCS {
   void		(*drawObj)(struct CDKOBJS *, boolean);
   void		(*eraseObj)(struct CDKOBJS *);
   void		(*moveObj)(struct CDKOBJS *, int, int, boolean, boolean);
} CDKFUNCS;

/*
 * Data common to all objects (widget instances).  This appears first in
 * each widget's struct to allow us to use generic functions in binding.c
 * and cdkscreen.c
 */
typedef struct CDKOBJS {
   int		screenIndex;
   CDKSCREEN *	screen;
   CDKFUNCS	*fn;
   boolean	box;
   int		bindingCount;
   CDKBINDING *	bindingList;
} CDKOBJS;

#define ObjOf(ptr)    (&(ptr)->obj)
#define MethodOf(ptr) (ObjOf(ptr)->fn)
#define ScreenOf(ptr) (ObjOf(ptr)->screen)
#define WindowOf(ptr) (ScreenOf(ptr)->window)

void *	_newCDKObject(unsigned, CDKFUNCS *);
#define newCDKObject(type,funcs) (type *)_newCDKObject(sizeof(type),funcs)

#define drawCDKObject(o,box)		MethodOf(o)->drawObj(ObjOf(o),box)
#define eraseCDKObject(o)		MethodOf(o)->eraseObj(ObjOf(o))
#define moveCDKObject(o,x,y,rel,ref)  	MethodOf(o)->moveObj(ObjOf(o),x,y,rel,ref)

#define DeclareCDKObjects(table,module) \
static void _drawCDK ## module (struct CDKOBJS *, boolean); \
static void _eraseCDK ## module (struct CDKOBJS *); \
static void _moveCDK ## module (struct CDKOBJS *, int, int, boolean, boolean); \
static CDKFUNCS table = { \
    _drawCDK ## module, \
    _eraseCDK ## module, \
    _moveCDK ## module, \
}

void positionCDKObject (CDKOBJS *, WINDOW *);

#endif /* CDK_OBJS_H */
