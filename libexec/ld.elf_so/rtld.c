/*	$NetBSD: rtld.c,v 1.94 2003/06/05 10:41:33 simonb Exp $	 */

/*
 * Copyright 1996 John D. Polstra.
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * Copyright 2002 Charles M. Hannum <root@ihack.net>
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
 * Function declarations.
 */
static void     _rtld_init __P((caddr_t, caddr_t));
static void     _rtld_exit __P((void));

Elf_Addr        _rtld __P((Elf_Addr *, Elf_Addr));


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
char           *_rtld_path = _PATH_RTLD;
Elf_Sym         _rtld_sym_zero;	/* For resolving undefined weak refs. */
int		_rtld_pagesz;	/* Page size, as provided by kernel */

Search_Path    *_rtld_default_paths;
Search_Path    *_rtld_paths;

Library_Xform  *_rtld_xforms;

/*
 * Global declarations normally provided by crt0.
 */
char           *__progname;
char          **environ;

extern Elf_Addr _GLOBAL_OFFSET_TABLE_[];
extern Elf_Dyn  _DYNAMIC;

static void _rtld_call_fini_functions __P((Obj_Entry *));
static void _rtld_call_init_functions __P((Obj_Entry *));
static Obj_Entry *_rtld_dlcheck __P((void *));
static void _rtld_init_dag __P((Obj_Entry *));
static void _rtld_init_dag1 __P((Obj_Entry *, Obj_Entry *));
static void _rtld_objlist_remove __P((Objlist *, Obj_Entry *));
static void _rtld_unload_object __P((Obj_Entry *, bool));
static void _rtld_unref_dag __P((Obj_Entry *));
static Obj_Entry *_rtld_obj_from_addr __P((const void *));

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
_rtld_init(mapbase, relocbase)
	caddr_t mapbase, relocbase;
{

	/* Conjure up an Obj_Entry structure for the dynamic linker. */
	_rtld_objself.path = _rtld_path;
	_rtld_objself.pathlen = strlen(_rtld_path);
	_rtld_objself.rtld = true;
	_rtld_objself.mapbase = mapbase;
	_rtld_objself.relocbase = relocbase;
	_rtld_objself.dynamic = (Elf_Dyn *) &_DYNAMIC;

#ifdef RTLD_RELOCATE_SELF
#error platform still uses RTLD_RELOCATE_SELF
#endif

	_rtld_digest_dynamic(&_rtld_objself);
	assert(!_rtld_objself.needed && !_rtld_objself.pltrel &&
	    !_rtld_objself.pltrela);
#ifndef __mips__
	/* MIPS has a bogus DT_TEXTREL. */
	assert(!_rtld_objself.pltgot && !_rtld_objself.textrel);
#endif

	_rtld_add_paths(&_rtld_default_paths, RTLD_DEFAULT_LIBRARY_PATH);

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
_rtld(sp, relocbase)
	Elf_Addr *sp;
	Elf_Addr relocbase;
{
	const AuxInfo  *pAUX_base, *pAUX_entry, *pAUX_execfd, *pAUX_phdr,
	               *pAUX_phent, *pAUX_phnum, *pAUX_euid, *pAUX_egid,
		       *pAUX_ruid, *pAUX_rgid;
	const AuxInfo  *pAUX_pagesz;
	char          **env;
	const AuxInfo  *aux;
	const AuxInfo  *auxp;
	Elf_Addr       *const osp = sp;
	bool            bind_now = 0;
	const char     *ld_bind_now;
	const char    **argv;
	long		argc;
	const char **real___progname;
	const Obj_Entry **real___mainprog_obj;
	char ***real_environ;
#if defined(RTLD_DEBUG)
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
#if defined(RTLD_DEBUG)
	debug = 1;
	dbg(("sp = %p, argc = %ld, argv = %p <%s> relocbase %p", sp,
	    (long)sp[2], &sp[3], (char *) sp[3], (void *)relocbase));
	dbg(("got is at %p, dynamic is at %p", _GLOBAL_OFFSET_TABLE_,
	    &_DYNAMIC));
	dbg(("_ctype_ is %p", _ctype_));
#endif

	sp += 2;		/* skip over return argument space */
	argv = (const char **) &sp[1];
	argc = *(long *)sp;
	sp += 2 + argc;		/* Skip over argc, arguments, and NULL
				 * terminator */
	env = (char **) sp;
	while (*sp++ != 0) {	/* Skip over environment, and NULL terminator */
#if defined(RTLD_DEBUG)
		dbg(("env[%d] = %p %s", i++, (void *)sp[-1], (char *)sp[-1]));
#endif
	}
	aux = (const AuxInfo *) sp;

	pAUX_base = pAUX_entry = pAUX_execfd = NULL;
	pAUX_phdr = pAUX_phent = pAUX_phnum = NULL;
	pAUX_euid = pAUX_ruid = pAUX_egid = pAUX_rgid = NULL;
	pAUX_pagesz = NULL;

	/* Digest the auxiliary vector. */
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
#ifdef AT_EUID
		case AT_EUID:
			pAUX_euid = auxp;
			break;
		case AT_RUID:
			pAUX_ruid = auxp;
			break;
		case AT_EGID:
			pAUX_egid = auxp;
			break;
		case AT_RGID:
			pAUX_rgid = auxp;
			break;
#endif
		case AT_PAGESZ:
			pAUX_pagesz = auxp;
			break;
		}
	}

	/* Initialize and relocate ourselves. */
	if (pAUX_base == NULL) {
		_rtld_error("Bad pAUX_base");
		_rtld_die();
	}
	assert(pAUX_pagesz != NULL);
	_rtld_pagesz = (int)pAUX_pagesz->a_v;
	_rtld_init((caddr_t)pAUX_base->a_v, (caddr_t)relocbase);

	__progname = _rtld_objself.path;
	environ = env;

	_rtld_trust = ((pAUX_euid ? (uid_t)pAUX_euid->a_v : geteuid()) ==
	    (pAUX_ruid ? (uid_t)pAUX_ruid->a_v : getuid())) &&
	    ((pAUX_egid ? (gid_t)pAUX_egid->a_v : getegid()) ==
	    (pAUX_rgid ? (gid_t)pAUX_rgid->a_v : getgid()));

	ld_bind_now = getenv("LD_BIND_NOW");
	if (ld_bind_now != NULL && *ld_bind_now != '\0')
		bind_now = true;
	if (_rtld_trust) {
#ifdef DEBUG
		const char     *ld_debug = getenv("LD_DEBUG");
#ifdef RTLD_DEBUG
		debug = 0;
#endif
		if (ld_debug != NULL && *ld_debug != '\0')
			debug = 1;
#endif
		_rtld_add_paths(&_rtld_paths, getenv("LD_LIBRARY_PATH"));
	}
	_rtld_process_hints(&_rtld_paths, &_rtld_xforms, _PATH_LD_HINTS);
	dbg(("dynamic linker is initialized, mapbase=%p, relocbase=%p",
	     _rtld_objself.mapbase, _rtld_objself.relocbase));

	/*
         * Load the main program, or process its program header if it is
         * already loaded.
         */
	if (pAUX_execfd != NULL) {	/* Load the main program. */
		int             fd = pAUX_execfd->a_v;
		dbg(("loading main program"));
		_rtld_objmain = _rtld_map_object(xstrdup(argv[0] ? argv[0] :
		    "main program"), fd, NULL);
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
		_rtld_objmain->path = xstrdup(argv[0] ? argv[0] :
		    "main program");
		_rtld_objmain->pathlen = strlen(_rtld_objmain->path);
	}

	_rtld_objmain->mainprog = true;
	
	/*
	 * Get the actual dynamic linker pathname from the executable if
	 * possible.  (It should always be possible.)  That ensures that
	 * gdb will find the right dynamic linker even if a non-standard
	 * one is being used.
	 */
	if (_rtld_objmain->interp != NULL &&
	    strcmp(_rtld_objmain->interp, _rtld_objself.path) != 0)
		_rtld_objself.path = xstrdup(_rtld_objmain->interp);
	dbg(("actual dynamic linker is %s", _rtld_objself.path));
	
	_rtld_digest_dynamic(_rtld_objmain);

	/* Link the main program into the list of objects. */
	*_rtld_objtail = _rtld_objmain;
	_rtld_objtail = &_rtld_objmain->next;

	_rtld_linkmap_add(_rtld_objmain);
	_rtld_linkmap_add(&_rtld_objself);

	++_rtld_objmain->refcount;
	_rtld_objmain->mainref = 1;
	_rtld_objlist_add(&_rtld_list_main, _rtld_objmain);

	/* Initialize a fake symbol for resolving undefined weak references. */
	_rtld_sym_zero.st_info = ELF_ST_INFO(STB_GLOBAL, STT_NOTYPE);
	_rtld_sym_zero.st_shndx = SHN_ABS;

	/*
	 * Pre-load user-specified objects after the main program but before
	 * any shared object dependencies.
	 */
	dbg(("preloading objects"));
	if (_rtld_trust && _rtld_preload(getenv("LD_PRELOAD")) == -1)
		_rtld_die();

	dbg(("loading needed objects"));
	if (_rtld_load_needed_objects(_rtld_objmain, RTLD_MAIN) == -1)
		_rtld_die();

	dbg(("relocating objects"));
	if (_rtld_relocate_objects(_rtld_objmain, bind_now) == -1)
		_rtld_die();

	dbg(("doing copy relocations"));
	if (_rtld_do_copy_relocations(_rtld_objmain) == -1)
		_rtld_die();

	/*
	 * Set the __progname,  environ and, __mainprog_obj before
	 * calling anything that might use them.
	 */
	real___progname = _rtld_objmain_sym("__progname");
	if (real___progname) {
		if (argv[0] != NULL) {
			if ((*real___progname = strrchr(argv[0], '/')) == NULL)
				(*real___progname) = argv[0];
			else
				(*real___progname)++;
		} else {
			(*real___progname) = NULL;
		}
	}
	real_environ = _rtld_objmain_sym("environ");
	if (real_environ)
		*real_environ = environ;
	real___mainprog_obj = _rtld_objmain_sym("__mainprog_obj");
	if (real___mainprog_obj)
		*real___mainprog_obj = _rtld_objmain;

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
	xerrx(1, "%s", msg);
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
_rtld_init_dag(root)
	Obj_Entry *root;
{

	_rtld_init_dag1(root, root);
}

static void
_rtld_init_dag1(root, obj)
	Obj_Entry *root;
	Obj_Entry *obj;
{
	const Needed_Entry *needed;

	if (!obj->mainref) {
		if (_rtld_objlist_find(&obj->dldags, root))
			return;
		rdbg(("add %p (%s) to %p (%s) DAG", obj, obj->path, root,
		    root->path));
		_rtld_objlist_add(&obj->dldags, root);
		_rtld_objlist_add(&root->dagmembers, obj);
	}
	for (needed = obj->needed; needed != NULL; needed = needed->next)
		if (needed->obj != NULL)
			_rtld_init_dag1(root, needed->obj);
}

/*
 * Note, this is called only for objects loaded by dlopen().
 */
static void
_rtld_unload_object(root, do_fini_funcs)
	Obj_Entry *root;
	bool do_fini_funcs;
{

	_rtld_unref_dag(root);
	if (root->refcount == 0) { /* We are finished with some objects. */
		Obj_Entry *obj;
		Obj_Entry **linkp;
		Objlist_Entry *elm;

		/* Finalize objects that are about to be unmapped. */
		if (do_fini_funcs)
			for (obj = _rtld_objlist->next;  obj != NULL;  obj = obj->next)
				if (obj->refcount == 0 && obj->fini != NULL)
					(*obj->fini)();

		/* Remove the DAG from all objects' DAG lists. */
		SIMPLEQ_FOREACH(elm, &root->dagmembers, link)
			_rtld_objlist_remove(&elm->obj->dldags, root);

		/* Remove the DAG from the RTLD_GLOBAL list. */
		if (root->globalref) {
			root->globalref = 0;
			_rtld_objlist_remove(&_rtld_list_global, root);
		}

		/* Unmap all objects that are no longer referenced. */
		linkp = &_rtld_objlist->next;
		while ((obj = *linkp) != NULL) {
			if (obj->refcount == 0) {
#ifdef RTLD_DEBUG
				dbg(("unloading \"%s\"", obj->path));
#endif
				munmap(obj->mapbase, obj->mapsize);
				_rtld_objlist_remove(&_rtld_list_global, obj);
				_rtld_linkmap_delete(obj);
				*linkp = obj->next;
				_rtld_obj_free(obj);
			} else
				linkp = &obj->next;
		}
		_rtld_objtail = linkp;
	}
}

static void
_rtld_unref_dag(root)
	Obj_Entry *root;
{

	assert(root);
	assert(root->refcount != 0);
	--root->refcount;
	if (root->refcount == 0) {
		const Needed_Entry *needed;

		for (needed = root->needed; needed != NULL;
		     needed = needed->next) {
			if (needed->obj != NULL)
				_rtld_unref_dag(needed->obj);
		}
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
	_rtld_unload_object(root, true);

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
		obj->refcount++;
	} else
		obj = _rtld_load_library(name, _rtld_objmain, mode);

	if (obj != NULL) {
		++obj->dl_refcount;
		if (*old_obj_tail != NULL) {	/* We loaded something new. */
			assert(*old_obj_tail == obj);

			if (_rtld_load_needed_objects(obj, mode) == -1 ||
			    (_rtld_init_dag(obj),
			    _rtld_relocate_objects(obj,
			    ((mode & 3) == RTLD_NOW))) == -1) {
				_rtld_unload_object(obj, false);
				obj->dl_refcount--;
				obj = NULL;
			} else
				_rtld_call_init_functions(obj);
		}
	}
	_rtld_debug.r_state = RT_CONSISTENT;
	_rtld_debug_state();

	return obj;
}

/*
 * Find a symbol in the main program.
 */
void *
_rtld_objmain_sym(name)
	const char *name;
{
	unsigned long hash;
	const Elf_Sym *def;
	const Obj_Entry *obj;

	hash = _rtld_elf_hash(name);
	obj = _rtld_objmain;

	def = _rtld_symlook_list(name, hash, &_rtld_list_main, &obj, false);

	if (def != NULL)
		return obj->relocbase + def->st_value;
	return(NULL);
}

void *
_rtld_dlsym(handle, name)
	void *handle;
	const char *name;
{
	const Obj_Entry *obj;
	unsigned long hash;
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	void *retaddr;
	
	hash = _rtld_elf_hash(name);
	def = NULL;
	defobj = NULL;
	
	switch ((intptr_t)handle) {
	case (intptr_t)NULL:
	case (intptr_t)RTLD_NEXT:
	case (intptr_t)RTLD_DEFAULT:
	case (intptr_t)RTLD_SELF:
		retaddr = __builtin_return_address(0); /* __GNUC__ only */
		if ((obj = _rtld_obj_from_addr(retaddr)) == NULL) {
			_rtld_error("Cannot determine caller's shared object");
			return NULL;
		}

		switch ((intptr_t)handle) {
		case (intptr_t)NULL:	 /* Just the caller's shared object. */
			def = _rtld_symlook_obj(name, hash, obj, false);
			defobj = obj;
			break;

		case (intptr_t)RTLD_NEXT:	/* Objects after callers */
			obj = obj->next;
			/*FALLTHROUGH*/

		case (intptr_t)RTLD_SELF:	/* Caller included */
			for (; obj; obj = obj->next) {
				if ((def = _rtld_symlook_obj(name, hash, obj,
				    false)) != NULL) {
					defobj = obj;
					break;
				}
			}
			break;

		case (intptr_t)RTLD_DEFAULT:
			def = _rtld_symlook_default(name, hash, obj, &defobj,
			    true);
			break;

		default:
			abort();
		}
		break;

	default:
		if ((obj = _rtld_dlcheck(handle)) == NULL)
			return NULL;
		
		if (obj->mainprog) {
			/* Search main program and all libraries loaded by it */
			def = _rtld_symlook_list(name, hash, &_rtld_list_main,
			    &defobj, false);
		} else {
			/*
			 * XXX - This isn't correct.  The search should include
			 * the whole DAG rooted at the given object.
			 */
			def = _rtld_symlook_obj(name, hash, obj, false);
			defobj = obj;
		}
		break;
	}
	
	if (def != NULL) {
#ifdef __HAVE_FUNCTION_DESCRIPTORS
		if (ELF_ST_TYPE(def->st_info) == STT_FUNC)
			return (void *)_rtld_function_descriptor_alloc(defobj, 
			    def, 0);
#endif /* __HAVE_FUNCTION_DESCRIPTORS */
		return defobj->relocbase + def->st_value;
	}
	
	_rtld_error("Undefined symbol \"%s\"", name);
	return NULL;
}

int
_rtld_dladdr(addr, info)
	const void *addr;
	Dl_info *info;
{
	const Obj_Entry *obj;
	const Elf_Sym *def, *best_def;
	void *symbol_addr;
	unsigned long symoffset;
	
#ifdef __HAVE_FUNCTION_DESCRIPTORS
	addr = _rtld_function_descriptor_function(addr);
#endif /* __HAVE_FUNCTION_DESCRIPTORS */

	obj = _rtld_obj_from_addr(addr);
	if (obj == NULL) {
		_rtld_error("No shared object contains address");
		return 0;
	}
	info->dli_fname = obj->path;
	info->dli_fbase = obj->mapbase;
	info->dli_saddr = (void *)0;
	info->dli_sname = NULL;
	
	/*
	 * Walk the symbol list looking for the symbol whose address is
	 * closest to the address sent in.
	 */
	best_def = NULL;
	for (symoffset = 0; symoffset < obj->nchains; symoffset++) {
		def = obj->symtab + symoffset;

		/*
		 * For skip the symbol if st_shndx is either SHN_UNDEF or
		 * SHN_COMMON.
		 */
		if (def->st_shndx == SHN_UNDEF || def->st_shndx == SHN_COMMON)
			continue;

		/*
		 * If the symbol is greater than the specified address, or if it
		 * is further away from addr than the current nearest symbol,
		 * then reject it.
		 */
		symbol_addr = obj->relocbase + def->st_value;
		if (symbol_addr > addr || symbol_addr < info->dli_saddr)
			continue;

		/* Update our idea of the nearest symbol. */
		info->dli_sname = obj->strtab + def->st_name;
		info->dli_saddr = symbol_addr;
		best_def = def;

		/* Exact match? */
		if (info->dli_saddr == addr)
			break;
	}

#ifdef __HAVE_FUNCTION_DESCRIPTORS
	if (best_def != NULL && ELF_ST_TYPE(best_def->st_info) == STT_FUNC)
		info->dli_saddr = (void *)_rtld_function_descriptor_alloc(obj, 
		    best_def, 0);
#endif /* __HAVE_FUNCTION_DESCRIPTORS */

	return 1;
}

/*
 * Error reporting function.  Use it like printf.  If formats the message
 * into a buffer, and sets things up so that the next call to dlerror()
 * will return the message.
 */
void
_rtld_error(const char *fmt,...)
{
	static char     buf[512];
	va_list         ap;

	va_start(ap, fmt);
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
	obj->linkmap.l_addr = obj->relocbase;
	obj->linkmap.l_ld = obj->dynamic;
#ifdef __mips__
	/* XXX This field is not standard and will be removed eventually. */
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

static Obj_Entry *
_rtld_obj_from_addr(const void *addr)
{
	Obj_Entry *obj;
	
	for (obj = _rtld_objlist;  obj != NULL;  obj = obj->next) {
		if (addr < (void *) obj->mapbase)
			continue;
		if (addr < (void *) (obj->mapbase + obj->mapsize))
			return obj;
	}
	return NULL;
}

static void
_rtld_objlist_remove(list, obj)
	Objlist *list;
	Obj_Entry *obj;
{
	Objlist_Entry *elm;
	
	if ((elm = _rtld_objlist_find(list, obj)) != NULL) {
		SIMPLEQ_REMOVE(list, elm, Struct_Objlist_Entry, link);
		free(elm);
	}
}
