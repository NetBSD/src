/*	$NetBSD: common.c,v 1.3 1999/05/19 14:50:50 kleink Exp $	*/

/*
 * Copyright (c) 1995 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NOT A STANDALONE FILE!
 */

static char *
_strrchr(p, ch)
char *p, ch;
{
	char *save;

	for (save = NULL;; ++p) {
		if (*p == ch)
			save = (char *)p;
		if (!*p)
			return(save);
	}
/* NOTREACHED */
}

#ifdef MCRT0
asm ("  .text");
asm ("_eprol:");
#endif

#ifdef DYNAMIC
void
_rtld_setup(cleanup, obj)
	void (*cleanup) __P((void));
	const Obj_Entry *obj;
{

	if ((obj == NULL) || (obj->magic != RTLD_MAGIC))
		_FATAL("Corrupt Obj_Entry pointer in GOT");
	if (obj->version != RTLD_VERSION)
		_FATAL("Dynamic linker version mismatch");

	__mainprog_obj = obj;
	atexit(cleanup);
}

void *
dlopen(name, mode)
	const char *name;
	int mode;
{

	if (__mainprog_obj == NULL)
		return NULL;
	return (__mainprog_obj->dlopen)(name, mode);
}

int
dlclose(fd)
	void *fd;
{

	if (__mainprog_obj == NULL)
		return -1;
	return (__mainprog_obj->dlclose)(fd);
}

void *
dlsym(fd, name)
	void *fd;
	const char *name;
{

	if (__mainprog_obj == NULL)
		return NULL;
	return (__mainprog_obj->dlsym)(fd, name);
}

#if 0 /* not supported for ELF shlibs, apparently */
int
dlctl(fd, cmd, arg)
	void *fd, *arg;
	int cmd;
{

	if (__mainprog_obj == NULL)
		return -1;
	return (__mainprog_obj->dlctl)(fd, cmd, arg);
}
#endif

__aconst char *
dlerror()
{

	if (__mainprog_obj == NULL)
		return ("Dynamic linker interface not available");
	return (__mainprog_obj->dlerror)();
}
#endif /* DYNAMIC */
