/*	$NetBSD: load.c,v 1.16 2002/06/01 23:50:53 lukem Exp $	 */

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
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <dirent.h>

#include "debug.h"
#include "rtld.h"

static bool _rtld_load_by_name __P((const char *, Obj_Entry *, Needed_Entry **,
    int, bool));

Objlist _rtld_list_global =	/* Objects dlopened with RTLD_GLOBAL */
  SIMPLEQ_HEAD_INITIALIZER(_rtld_list_global);

void
_rtld_objlist_add(list, obj)
	Objlist *list;
	Obj_Entry *obj;
{
	Objlist_Entry *elm;

	elm = NEW(Objlist_Entry);
	elm->obj = obj;
	SIMPLEQ_INSERT_TAIL(list, elm, link);
}

Objlist_Entry *
_rtld_objlist_find(Objlist *list, const Obj_Entry *obj)
{
	Objlist_Entry *elm;

	SIMPLEQ_FOREACH(elm, list, link) {
		if (elm->obj == obj)
			return elm;
	}
	return NULL;
}

/*
 * Load a shared object into memory, if it is not already loaded.  The
 * argument must be a string allocated on the heap.  This function assumes
 * responsibility for freeing it when necessary.
 *
 * Returns a pointer to the Obj_Entry for the object.  Returns NULL
 * on failure.
 */
Obj_Entry *
_rtld_load_object(filepath, mode, dodebug)
	char *filepath;
	int mode;
	bool dodebug;
{
	Obj_Entry *obj;
	int fd = -1;
	struct stat sb;

	for (obj = _rtld_objlist->next; obj != NULL; obj = obj->next)
		if (strcmp(obj->path, filepath) == 0)
			break;

	/*
	 * If we didn't find a match by pathname, open the file and check
	 * again by device and inode.  This avoids false mismatches caused
	 * by multiple links or ".." in pathnames.
	 *
	 * To avoid a race, we open the file and use fstat() rather than
	 * using stat().
	 */
	if (obj == NULL) {
		if ((fd = open(filepath, O_RDONLY)) == -1) {
			_rtld_error("Cannot open \"%s\"", filepath);
			return NULL;
		}
		if (fstat(fd, &sb) == -1) {
			_rtld_error("Cannot fstat \"%s\"", filepath);
			close(fd);
			return NULL;
		}
		for (obj = _rtld_objlist->next; obj != NULL; obj = obj->next) {
			if (obj->ino == sb.st_ino && obj->dev == sb.st_dev) {
				close(fd);
				break;
			}
		}
	}

	if (obj == NULL) { /* First use of this object, so we must map it in */
		obj = _rtld_map_object(filepath, fd, &sb);
		(void)close(fd);
		if (obj == NULL) {
			free(filepath);
			return NULL;
		}
		obj->path = filepath;
		_rtld_digest_dynamic(obj);

		*_rtld_objtail = obj;
		_rtld_objtail = &obj->next;
#ifdef RTLD_LOADER
		_rtld_linkmap_add(obj);	/* for GDB */
#endif
		if (dodebug) {
			dbg(("  %p .. %p: %s", obj->mapbase,
			    obj->mapbase + obj->mapsize - 1, obj->path));
			if (obj->textrel)
				dbg(("  WARNING: %s has impure text",
				    obj->path));
		}
	} else
		free(filepath);

	++obj->refcount;
	if ((mode & RTLD_GLOBAL) &&
	    _rtld_objlist_find(&_rtld_list_global, obj) == NULL)
		_rtld_objlist_add(&_rtld_list_global, obj);
	return obj;
}

static bool
_rtld_load_by_name(name, obj, needed, mode, dodebug)
	const char *name;
	Obj_Entry *obj;
	Needed_Entry **needed;
	int mode;
	bool dodebug;
{
	Library_Xform *x = _rtld_xforms;
	Obj_Entry *o = NULL;
	size_t i, j;
	char *libpath;
	bool got = false;
	union {
		int i;
		char s[16];
	} val;

	if (dodebug)
		dbg(("load by name %s %p", name, x));
	for (; x; x = x->next) {
		if (strcmp(x->name, name) != 0)
			continue;

		i = sizeof(val);

		if (sysctl(x->ctl, x->ctlmax, &val, &i, NULL, 0) == -1) {
			xwarnx("sysctl");
			break;
		}

		switch (x->ctltype[x->ctlmax - 1]) {
		case CTLTYPE_INT:
			xsnprintf(val.s, sizeof(val.s), "%d", val.i);
			break;
		case CTLTYPE_STRING:
			break;
		default:
			xwarnx("unsupported sysctl type %d",
			    x->ctltype[x->ctlmax - 1]);
			break;
		}

		if (dodebug)
			dbg(("sysctl returns %s", val.s));

		for (i = 0; i < RTLD_MAX_ENTRY && x->entry[i].value != NULL;
		    i++) {
			if (dodebug)
				dbg(("entry %ld", (unsigned long)i));
			if (strcmp(x->entry[i].value, val.s) == 0)
				break;
		}

		if (i == RTLD_MAX_ENTRY) {
			xwarnx("sysctl value %s not found for lib%s",
			    val.s, name);
			break;
		}
		/* XXX: This can mess up debuggers, cause we lie about
		 * what we loaded in the needed objects */
		for (j = 0; j < RTLD_MAX_LIBRARY &&
		    x->entry[i].library[j] != NULL; j++) {
			libpath = _rtld_find_library(
			    x->entry[i].library[j], obj);
			if (libpath == NULL) {
				xwarnx("could not load %s for %s",
				    x->entry[i].library[j], name);
				continue;
			}
			o = _rtld_load_object(libpath, mode, true);
			if (o == NULL)
				continue;
			got = true;
			if (j == 0)
				(*needed)->obj = o;
			else {
				/* make a new one and put it in the chain */
				Needed_Entry *ne = xmalloc(sizeof(*ne));
				ne->name = (*needed)->name;
				ne->obj = o;
				ne->next = (*needed)->next;
				(*needed)->next = ne;
				*needed = ne;
			}
				
		}
		
	}

	if (got)
		return true;

	libpath = _rtld_find_library(name, obj);
	if (libpath == NULL)
		return false;
	return ((*needed)->obj = _rtld_load_object(libpath, mode, true)) != NULL;
}


/*
 * Given a shared object, traverse its list of needed objects, and load
 * each of them.  Returns 0 on success.  Generates an error message and
 * returns -1 on failure.
 */
int
_rtld_load_needed_objects(first, mode, dodebug)
	Obj_Entry *first;
	int mode;
	bool dodebug;
{
	Obj_Entry *obj;
	int status = 0;

	for (obj = first; obj != NULL; obj = obj->next) {
		Needed_Entry *needed;

		for (needed = obj->needed; needed != NULL;
		    needed = needed->next) {
			const char *name = obj->strtab + needed->name;
			if (!_rtld_load_by_name(name, obj, &needed, mode,
			    dodebug))
				status = -1;	/* FIXME - cleanup */
#ifdef RTLD_LOADER
			if (status == -1)
				return status;
#endif
		}
	}

	return status;
}

#ifdef RTLD_LOADER
int
_rtld_preload(preload_path, dodebug)
	const char *preload_path;
	bool dodebug;
{
	const char *path;
	char *cp, *buf;
	int status = 0;

	if (preload_path != NULL) {
		cp = buf = xstrdup(preload_path);
		while ((path = strsep(&cp, " :")) != NULL && status == 0) {
			if (_rtld_load_object(xstrdup(path), RTLD_GLOBAL,
			    dodebug) == NULL)
				status = -1;
			else if (dodebug)
				dbg((" preloaded \"%s\"", path));
		}
		free(buf);
	}

	return (status);
}
#endif
