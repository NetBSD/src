/*	$NetBSD: rtld.c,v 1.24 1999/10/25 13:57:12 kleink Exp $	 */

/*
 * Copyright 1996 John D. Polstra.
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
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
 *      This product includes software developed by John Polstra.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * Dynamic linker for ELF.
 *
 * John Polstra <jdp@polstra.com>.
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <dirent.h>

#include <ctype.h>

#include <dlfcn.h>
#include "debug.h"
#include "rtld.h"

#if !defined(lint)
#include "sysident.h"
#endif

/*
 * Debugging support.
 */

typedef void    (*funcptr) __P((void));

/*
 * Function declarations.
 */
static void     _rtld_init __P((caddr_t));
static void     _rtld_exit __P((void));

Elf_Addr        _rtld __P((Elf_Word *));


/*
 * Data declarations.
 */
static char    *error_message;	/* Message for dlopen(), or NULL */

struct r_debug  _rtld_debug;	/* for GDB; */
bool            _rtld_trust;	/* False for setuid and setgid programs */
Obj_Entry      *_rtld_objlist;	/* Head of linked list of shared objects */
Obj_Entry     **_rtld_objtail;	/* Link field of last object in list */
Obj_Entry      *_rtld_objmain;	/* The main program shared object */
Obj_Entry       _rtld_objself;	/* The dynamic linker shared object */
char            _rtld_path[] = _PATH_RTLD;
#ifdef	VARPSZ
int		_rtld_pagesz;	/* Page size, as provided by kernel */
#endif

Search_Path    *_rtld_default_paths;
Search_Path    *_rtld_paths;
/*
 * Global declarations normally provided by crt0.
 */
char           *__progname;
char          **environ;

#ifdef OLD_GOT
extern Elf_Addr _GLOBAL_OFFSET_TABLE_[];
#else
extern Elf_Addr _GLOBAL_OFFSET_TABLE_[];
extern Elf_Dyn  _DYNAMIC;
#endif

static void _rtld_call_fini_functions __P((Obj_Entry *));
static void _rtld_call_init_functions __P((Obj_Entry *));
static Obj_Entry *_rtld_dlcheck __P((void *));
static void _rtld_unref_object_dag __P((Obj_Entry *));

static void
_rtld_call_fini_functions(first)
	Obj_Entry *first;
{
	Obj_Entry *obj;

	for (obj = first; obj != NULL; obj = obj->next)
		if (obj->fini != NULL)
			(*obj->fini)();
}

static void
_rtld_call_init_functions(first)
	Obj_Entry *first;
{
	if (first != NULL) {
		_rtld_call_init_functions(first->next);
		if (first->init != NULL)
			(*first->init)();
	}
}

/*
 * Initialize the dynamic linker.  The argument is the address at which
 * the dynamic linker has been mapped into memory.  The primary task of
 * this function is to relocate the dynamic linker.
 */
static void
_rtld_init(mapbase)
	caddr_t mapbase;
{
	Obj_Entry objself;/* The dynamic linker shared object */
#ifdef RTLD_RELOCATE_SELF
	int dodebug = false;
#else
	int dodebug = true;
#endif

	memset(&objself, 0, sizeof objself);

	/* Conjure up an Obj_Entry structure for the dynamic linker. */
	objself.path = NULL;
	objself.rtld = true;
	objself.mapbase = mapbase;

#if defined(__mips__)
	/*
	* mips and ld.so currently linked at load address,
	* so no relocation needed
	*/
	objself.relocbase = 0;
#else
	objself.relocbase = mapbase;
#endif

	objself.pltgot = NULL;

#ifdef OLD_GOT
	objself.dynamic = (Elf_Dyn *) _GLOBAL_OFFSET_TABLE_[0];
#else
	objself.dynamic = (Elf_Dyn *) & _DYNAMIC;
#endif

#ifdef RTLD_RELOCATE_SELF
	/* We have not been relocated yet, so fix the dynamic address */
	objself.dynamic = (Elf_Dyn *)
		((u_long) mapbase + (char *) objself.dynamic);
#endif				/* RTLD_RELOCATE_SELF */

	_rtld_digest_dynamic(&objself);

#ifdef __alpha__
	/* XXX XXX XXX */
	objself.pltgot = NULL;
#endif
	assert(objself.needed == NULL);

#if !defined(__mips__) && !defined(__i386__)
	/* no relocation for mips/i386 */
	assert(!objself.textrel);
#endif

	_rtld_relocate_objects(&objself, true, dodebug);

	/*
	 * Now that we relocated ourselves, we can use globals.
	 */
	_rtld_objself = objself;

	_rtld_objself.path = _rtld_path;
	_rtld_add_paths(&_rtld_default_paths, RTLD_DEFAULT_LIBRARY_PATH, true);

	/*
	 * Set up the _rtld_objlist pointer, so that rtld symbols can be found.
	 */
	_rtld_objlist = &_rtld_objself;

	/* Make the object list empty again. */
	_rtld_objlist = NULL;
	_rtld_objtail = &_rtld_objlist;

	_rtld_debug.r_brk = _rtld_debug_state;
	_rtld_debug.r_state = RT_CONSISTENT;
}

/*
 * Cleanup procedure.  It will be called (by the atexit() mechanism) just
 * before the process exits.
 */
static void
_rtld_exit()
{
	dbg(("rtld_exit()"));

	_rtld_call_fini_functions(_rtld_objlist->next);
}

/*
 * Main entry point for dynamic linking.  The argument is the stack
 * pointer.  The stack is expected to be laid out as described in the
 * SVR4 ABI specification, Intel 386 Processor Supplement.  Specifically,
 * the stack pointer points to a word containing ARGC.  Following that
 * in the stack is a null-terminated sequence of pointers to argument
 * strings.  Then comes a null-terminated sequence of pointers to
 * environment strings.  Finally, there is a sequence of "auxiliary
 * vector" entries.
 *
 * This function returns the entry point for the main program, the dynamic
 * linker's exit procedure in sp[0], and a pointer to the main object in
 * sp[1].
 */
Elf_Addr
_rtld(sp)
	Elf_Word *sp;
{
	const AuxInfo  *pAUX_base, *pAUX_entry, *pAUX_execfd, *pAUX_phdr,
	               *pAUX_phent, *pAUX_phnum;
#ifdef	VARPSZ
	const AuxInfo  *pAUX_pagesz;
#endif
	char          **env;
	const AuxInfo  *aux;
	const AuxInfo  *auxp;
	Elf_Word       *const osp = sp;
	bool            bind_now = 0;
	const char     *ld_bind_now;
	const char    **argv;
#if defined(RTLD_DEBUG) && !defined(RTLD_RELOCATE_SELF)
	int             i = 0;
#endif

	/*
         * On entry, the dynamic linker itself has not been relocated yet.
         * Be very careful not to reference any global data until after
         * _rtld_init has returned.  It is OK to reference file-scope statics
         * and string constants, and to call static and global functions.
         */
	/* Find the auxiliary vector on the stack. */
	/* first Elf_Word reserved to address of exit routine */
#if defined(RTLD_DEBUG) && !defined(RTLD_RELOCATE_SELF)
	dbg(("sp = %p, argc = %ld, argv = %p <%s>\n", sp, (long)sp[2],
	     &sp[3], (char *) sp[3]));
	dbg(("got is at %p, dynamic is at %p\n",
	    _GLOBAL_OFFSET_TABLE_, &_DYNAMIC));
	debug = 1;
	dbg(("_ctype_ is %p\n", _ctype_));
#endif

	sp += 2;		/* skip over return argument space */
	argv = (const char **) &sp[1];
	sp += sp[0] + 2;	/* Skip over argc, arguments, and NULL
				 * terminator */
	env = (char **) sp;
	while (*sp++ != 0) {	/* Skip over environment, and NULL terminator */
#if defined(RTLD_DEBUG) && !defined(RTLD_RELOCATE_SELF)
		dbg(("env[%d] = %p %s\n", i++, (void *)sp[-1], (char *)sp[-1]));
#endif
	}
	aux = (const AuxInfo *) sp;

	/* Digest the auxiliary vector. */
	pAUX_base = pAUX_entry = pAUX_execfd = NULL;
	pAUX_phdr = pAUX_phent = pAUX_phnum = NULL;
#ifdef	VARPSZ
	pAUX_pagesz = NULL;
#endif
	for (auxp = aux; auxp->a_type != AT_NULL; ++auxp) {
		switch (auxp->a_type) {
		case AT_BASE:
			pAUX_base = auxp;
			break;
		case AT_ENTRY:
			pAUX_entry = auxp;
			break;
		case AT_EXECFD:
			pAUX_execfd = auxp;
			break;
		case AT_PHDR:
			pAUX_phdr = auxp;
			break;
		case AT_PHENT:
			pAUX_phent = auxp;
			break;
		case AT_PHNUM:
			pAUX_phnum = auxp;
			break;
#ifdef	VARPSZ
		case AT_PAGESZ:
			pAUX_pagesz = auxp;
			break;
#endif
		}
	}

	/* Initialize and relocate ourselves. */
	assert(pAUX_base != NULL);
	_rtld_init((caddr_t) pAUX_base->a_v);

#ifdef	VARPSZ
	assert(pAUX_pagesz != NULL);
	_rtld_pagesz = (int)pAUX_pagesz->a_v;
#endif

#ifdef RTLD_DEBUG
	dbg(("_ctype_ is %p\n", _ctype_));
#endif

	__progname = _rtld_objself.path;
	environ = env;

	_rtld_trust = geteuid() == getuid() && getegid() == getgid();

	ld_bind_now = getenv("LD_BIND_NOW");
	if (ld_bind_now != NULL && *ld_bind_now != '\0')
		bind_now = true;
	if (_rtld_trust) {
#ifdef DEBUG
		const char     *ld_debug = getenv("LD_DEBUG");
		if (ld_debug != NULL && *ld_debug != '\0')
			debug = 1;
#endif
		_rtld_add_paths(&_rtld_paths, getenv("LD_LIBRARY_PATH"), true);
	}
	_rtld_process_hints(&_rtld_paths, _PATH_LD_HINTS, true);
	dbg(("%s is initialized, base address = %p", __progname,
	     (void *) pAUX_base->a_v));

	/*
         * Load the main program, or process its program header if it is
         * already loaded.
         */
	if (pAUX_execfd != NULL) {	/* Load the main program. */
		int             fd = pAUX_execfd->a_v;
		dbg(("loading main program"));
		_rtld_objmain = _rtld_map_object(argv[0], fd);
		close(fd);
		if (_rtld_objmain == NULL)
			_rtld_die();
	} else {		/* Main program already loaded. */
		const Elf_Phdr *phdr;
		int             phnum;
		caddr_t         entry;

		dbg(("processing main program's program header"));
		assert(pAUX_phdr != NULL);
		phdr = (const Elf_Phdr *) pAUX_phdr->a_v;
		assert(pAUX_phnum != NULL);
		phnum = pAUX_phnum->a_v;
		assert(pAUX_phent != NULL);
		assert(pAUX_phent->a_v == sizeof(Elf_Phdr));
		assert(pAUX_entry != NULL);
		entry = (caddr_t) pAUX_entry->a_v;
		_rtld_objmain = _rtld_digest_phdr(phdr, phnum, entry);
	}

	_rtld_objmain->path = xstrdup("main program");
	_rtld_objmain->mainprog = true;
	_rtld_digest_dynamic(_rtld_objmain);

	_rtld_linkmap_add(_rtld_objmain);
	_rtld_linkmap_add(&_rtld_objself);

	/* Link the main program into the list of objects. */
	*_rtld_objtail = _rtld_objmain;
	_rtld_objtail = &_rtld_objmain->next;
	++_rtld_objmain->refcount;

	/*
	 * Pre-load user-specified objects after the main program but before
	 * any shared object dependencies.
	 */
	dbg(("preloading objects"));
	if (_rtld_trust && _rtld_preload(getenv("LD_PRELOAD"), true) == -1)
		_rtld_die();

	dbg(("loading needed objects"));
	if (_rtld_load_needed_objects(_rtld_objmain) == -1)
		_rtld_die();

	dbg(("relocating objects"));
	if (_rtld_relocate_objects(_rtld_objmain, bind_now, true) == -1)
		_rtld_die();

	dbg(("doing copy relocations"));
	if (_rtld_do_copy_relocations(_rtld_objmain, true) == -1)
		_rtld_die();

	dbg(("calling _init functions"));
	_rtld_call_init_functions(_rtld_objmain->next);

	dbg(("control at program entry point = %p, obj = %p, exit = %p",
	     _rtld_objmain->entry, _rtld_objmain, _rtld_exit));

	/*
	 * Return with the entry point and the exit procedure in at the top
	 * of stack.
	 */

	_rtld_debug_state();	/* say hello to gdb! */

	((void **) osp)[0] = _rtld_exit;
	((void **) osp)[1] = _rtld_objmain;
	return (Elf_Addr) _rtld_objmain->entry;
}

void
_rtld_die()
{
	const char *msg = _rtld_dlerror();

	if (msg == NULL)
		msg = "Fatal error";
	xerrx(1, "%s\n", msg);
}

static Obj_Entry *
_rtld_dlcheck(handle)
	void *handle;
{
	Obj_Entry *obj;

	for (obj = _rtld_objlist; obj != NULL; obj = obj->next)
		if (obj == (Obj_Entry *) handle)
			break;

	if (obj == NULL || obj->dl_refcount == 0) {
		xwarnx("Invalid shared object handle %p", handle);
		return NULL;
	}
	return obj;
}

static void
_rtld_unref_object_dag(root)
	Obj_Entry *root;
{
	assert(root->refcount != 0);
	--root->refcount;
	if (root->refcount == 0) {
		const Needed_Entry *needed;

		for (needed = root->needed; needed != NULL;
		    needed = needed->next)
			_rtld_unref_object_dag(needed->obj);
	}
}

int
_rtld_dlclose(handle)
	void *handle;
{
	Obj_Entry *root = _rtld_dlcheck(handle);

	if (root == NULL)
		return -1;

	_rtld_debug.r_state = RT_DELETE;
	_rtld_debug_state();

	--root->dl_refcount;
	_rtld_unref_object_dag(root);
	if (root->refcount == 0) {	/* We are finished with some objects. */
		Obj_Entry      *obj;
		Obj_Entry     **linkp;

		/* Finalize objects that are about to be unmapped. */
		for (obj = _rtld_objlist->next; obj != NULL; obj = obj->next)
			if (obj->refcount == 0 && obj->fini != NULL)
				(*obj->fini) ();

		/* Unmap all objects that are no longer referenced. */
		linkp = &_rtld_objlist->next;
		while ((obj = *linkp) != NULL) {
			if (obj->refcount == 0) {
				munmap(obj->mapbase, obj->mapsize);
				free(obj->path);
				while (obj->needed != NULL) {
					Needed_Entry   *needed = obj->needed;
					obj->needed = needed->next;
					free(needed);
				}
				_rtld_linkmap_delete(obj);
				*linkp = obj->next;
				if (obj->next == NULL)
					_rtld_objtail = linkp;
				free(obj);
			} else
				linkp = &obj->next;
		}
	}
	_rtld_debug.r_state = RT_CONSISTENT;
	_rtld_debug_state();

	return 0;
}

char *
_rtld_dlerror()
{
	char *msg = error_message;
	error_message = NULL;
	return msg;
}

void *
_rtld_dlopen(name, mode)
	const char *name;
	int mode;
{
	Obj_Entry **old_obj_tail = _rtld_objtail;
	Obj_Entry *obj = NULL;

	_rtld_debug.r_state = RT_ADD;
	_rtld_debug_state();

	if (name == NULL) {
		obj = _rtld_objmain;
	} else {
		char *path = _rtld_find_library(name, _rtld_objmain);
		if (path != NULL)
			obj = _rtld_load_object(path, true);
	}

	if (obj != NULL) {
		++obj->dl_refcount;
		if (*old_obj_tail != NULL) {	/* We loaded something new. */
			assert(*old_obj_tail == obj);

			/* FIXME - Clean up properly after an error. */
			if (_rtld_load_needed_objects(obj) == -1) {
				--obj->dl_refcount;
				obj = NULL;
			} else if (_rtld_relocate_objects(obj,
			    (mode & 3) == RTLD_NOW, true) == -1) {
				--obj->dl_refcount;
				obj = NULL;
			} else {
				_rtld_call_init_functions(obj);
			}
		}
	}
	_rtld_debug.r_state = RT_CONSISTENT;
	_rtld_debug_state();

	return obj;
}

void *
_rtld_dlsym(handle, name)
	void *handle;
	const char *name;
{
	const Obj_Entry *obj = _rtld_dlcheck(handle);
	const Elf_Sym  *def;
	const Obj_Entry *defobj;

	if (obj == NULL)
		return NULL;

	/*
         * FIXME - This isn't correct.  The search should include the whole
         * DAG rooted at the given object.
         */
	def = _rtld_find_symdef(_rtld_objlist, 0, name, obj, &defobj, false);
	if (def != NULL)
		return defobj->relocbase + def->st_value;

	_rtld_error("Undefined symbol \"%s\"", name);
	return NULL;
}

/*
 * Error reporting function.  Use it like printf.  If formats the message
 * into a buffer, and sets things up so that the next call to dlerror()
 * will return the message.
 */
void
#ifdef __STDC__
_rtld_error(const char *fmt,...)
#else
_rtld_error(va_alist)
	va_dcl
#endif
{
	static char     buf[512];
	va_list         ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	const char *fmt;

	va_start(ap);
	fmt = va_arg(ap, const char *);
#endif
	xvsnprintf(buf, sizeof buf, fmt, ap);
	error_message = buf;
	va_end(ap);
}

void
_rtld_debug_state()
{
	/* do nothing */
}

void
_rtld_linkmap_add(obj)
	Obj_Entry *obj;
{
	struct link_map *l = &obj->linkmap;
	struct link_map *prev;

	obj->linkmap.l_name = obj->path;
	obj->linkmap.l_addr = obj->mapbase;
	obj->linkmap.l_ld = obj->dynamic;
#ifdef __mips__
	/* GDB needs load offset on MIPS to use the symbols */
	obj->linkmap.l_offs = obj->relocbase;
#endif

	if (_rtld_debug.r_map == NULL) {
		_rtld_debug.r_map = l;
		return;
	}
	for (prev = _rtld_debug.r_map; prev->l_next != NULL; prev = prev->l_next);
	l->l_prev = prev;
	prev->l_next = l;
	l->l_next = NULL;
}

void
_rtld_linkmap_delete(obj)
	Obj_Entry *obj;
{
	struct link_map *l = &obj->linkmap;

	if (l->l_prev == NULL) {
		if ((_rtld_debug.r_map = l->l_next) != NULL)
			l->l_next->l_prev = NULL;
		return;
	}
	if ((l->l_prev->l_next = l->l_next) != NULL)
		l->l_next->l_prev = l->l_prev;
}
