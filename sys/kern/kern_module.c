/*	$NetBSD: kern_module.c,v 1.6 2008/01/18 16:41:46 rumble Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Kernel module support.
 *
 * XXX Deps for loadable modules don't work, because we must load the
 * module in order to find out which modules it requires.  Linking may
 * fail because of missing symbols.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_module.c,v 1.6 2008/01/18 16:41:46 rumble Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/kobj.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <machine/stdarg.h>

#ifndef LKM	/* XXX */
struct vm_map *lkm_map;
#endif

struct modlist	module_list = TAILQ_HEAD_INITIALIZER(module_list);
struct modlist	module_bootlist = TAILQ_HEAD_INITIALIZER(module_bootlist);
u_int		module_count;
kmutex_t	module_lock;

/* Ensure that the kernel's link set isn't empty. */
static modinfo_t module_dummy;
__link_set_add_rodata(modules, module_dummy);

static module_t	*module_lookup(const char *);
static int	module_do_load(const char *, bool, bool, module_t **);
static int	module_do_unload(const char *);
static void	module_error(const char *, ...);
static int	module_do_builtin(const char *, module_t **);

/*
 * module_error:
 *
 *	Utility function: log an error.
 */
static void
module_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	printf("WARNING: module error: ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

/*
 * module_init:
 *
 *	Initialize the module subsystem.
 */
void
module_init(void)
{
	extern struct vm_map *lkm_map;

	if (lkm_map == NULL)
		lkm_map = kernel_map;
	mutex_init(&module_lock, MUTEX_DEFAULT, IPL_NONE);
}

/*
 * module_init_class:
 *
 *	Initialize all built-in and pre-loaded modules of the
 *	specified class.
 */
void
module_init_class(modclass_t class)
{
	__link_set_decl(modules, modinfo_t);
	modinfo_t *const *mip, *mi;
	module_t *mod;

	mutex_enter(&module_lock);
	/*
	 * Builtins first.  These can't depend on pre-loaded modules.
	 */
	__link_set_foreach(mip, modules) {
		mi = *mip;
		if (mi == &module_dummy) {
			continue;
		}
		if (class != MODULE_CLASS_ANY && class != mi->mi_class) {
			continue;
		}
		(void)module_do_builtin(mi->mi_name, NULL);
	}
	/*
	 * Now preloaded modules.  These will be pulled off the
	 * list as we call module_do_load();
	 */
	while ((mod = TAILQ_FIRST(&module_bootlist)) != NULL) {
		module_do_load(mod->mod_info->mi_name, false, false, NULL);
	}
	mutex_exit(&module_lock);
}

/*
 * module_jettison:
 *
 *	Return memory used by pre-loaded modules to the freelist.
 */
void
module_jettison(void)
{

	/* XXX nothing yet */
}

/*
 * module_load:
 *
 *	Load a single module from the file system.  If force is set,
 *	bypass the version check.
 */
int
module_load(const char *filename, bool force)
{
	int error;

	mutex_enter(&module_lock);
	error = module_do_load(filename, false, force, NULL);
	mutex_exit(&module_lock);

	return error;
}

/*
 * module_unload:
 *
 *	Find and unload a module by name.
 */
int
module_unload(const char *name)
{
	int error;

	mutex_enter(&module_lock);
	error = module_do_unload(name);
	mutex_exit(&module_lock);

	return error;
}

/*
 * module_lookup:
 *
 *	Look up a module by name.
 */
module_t *
module_lookup(const char *name)
{
	module_t *mod;

	KASSERT(mutex_owned(&module_lock));

	TAILQ_FOREACH(mod, &module_list, mod_chain) {
		if (strcmp(mod->mod_info->mi_name, name) == 0) {
			break;
		}
	}

	return mod;
}

/*
 * module_hold:
 *
 *	Add a single reference to a module.  It's the caller's
 *	responsibility to ensure that the reference is dropped
 *	later.
 */
int
module_hold(const char *name)
{
	module_t *mod;

	mutex_enter(&module_lock);
	mod = module_lookup(name);
	if (mod == NULL) {
		mutex_exit(&module_lock);
		return ENOENT;
	}
	mod->mod_refcnt++;
	mutex_exit(&module_lock);

	return 0;
}

/*
 * module_rele:
 *
 *	Release a reference acquired with module_hold().
 */
void
module_rele(const char *name)
{
	module_t *mod;

	mutex_enter(&module_lock);
	mod = module_lookup(name);
	if (mod == NULL) {
		mutex_exit(&module_lock);
		panic("module_rele: gone");
	}
	mod->mod_refcnt--;
	mutex_exit(&module_lock);
}

/*
 * module_do_builtin:
 *
 *	Initialize a single module from the list of modules that are
 *	built into the kernel (linked into the kernel image).
 */
static int
module_do_builtin(const char *name, module_t **modp)
{
	__link_set_decl(modules, modinfo_t);
	modinfo_t *const *mip;
	const char *p, *s;
	char buf[MAXMODNAME];
	modinfo_t *mi;
	module_t *mod, *mod2;
	size_t len;
	int error, i;

	KASSERT(mutex_owned(&module_lock));

	/*
	 * Check to see if already loaded.
	 */
	if ((mod = module_lookup(name)) != NULL) {
		if (modp != NULL) {
			*modp = mod;
		}
		return 0;
	}

	/*
	 * Search the list to see if we have a module by this name.
	 */
	error = ENOENT;
	__link_set_foreach(mip, modules) {
		mi = *mip;
		if (mi == &module_dummy) {
			continue;
		}
		if (strcmp(mi->mi_name, name) == 0) {
			error = 0;
			break;
		}
	}
	if (error != 0) {
		return error;
	}

	/*
	 * Initialize pre-requisites.
	 */
	mod = kmem_zalloc(sizeof(*mod), KM_SLEEP);
	if (mod == NULL) {
		return ENOMEM;
	}
	if (modp != NULL) {
		*modp = mod;
	}
	if (mi->mi_required != NULL) {
		for (s = mi->mi_required; *s != '\0'; s = p) {
			if (*s == ',')
				s++;
			p = s;
			while (*p != '\0' && *p != ',')
				p++;
			len = min(p - s + 1, sizeof(buf));
			strlcpy(buf, s, len);
			if (buf[0] == '\0')
				break;
			if (mod->mod_nrequired == MAXMODDEPS - 1) {
				module_error("too many required modules");
				kmem_free(mod, sizeof(*mod));
				return EINVAL;
			}
			error = module_do_builtin(buf, &mod2);
			if (error != 0) {
				kmem_free(mod, sizeof(*mod));
				return error;
			}
			mod->mod_required[mod->mod_nrequired++] = mod2;
		}
	}

	/*
	 * Try to initialize the module.
	 */
	error = (*mi->mi_modcmd)(MODULE_CMD_INIT, NULL);
	if (error != 0) {
		module_error("builtin module `%s' "
		    "failed to init", mi->mi_name);
		kmem_free(mod, sizeof(*mod));
		return error;
	}
	mod->mod_info = mi;
	mod->mod_source = MODULE_SOURCE_KERNEL;
	module_count++;
	TAILQ_INSERT_TAIL(&module_list, mod, mod_chain);

	/*
	 * If that worked, count dependencies.
	 */
	for (i = 0; i < mod->mod_nrequired; i++) {
		mod->mod_required[i]->mod_refcnt++;
	}

	return 0;
}

/*
 * module_do_load:
 *
 *	Helper routine: load a module from the file system, or one
 *	pushed by the boot loader.
 */
static int
module_do_load(const char *filename, bool isdep, bool force, module_t **modp)
{
	static TAILQ_HEAD(,module) pending = TAILQ_HEAD_INITIALIZER(pending);
	static int depth;
	const int maxdepth = 6;
	modinfo_t *mi;
	module_t *mod, *mod2;
	char buf[MAXMODNAME];
	const char *s, *p;
	void *addr;
	size_t size;
	int error;
	size_t len;
	u_int i;

	KASSERT(mutex_owned(&module_lock));

	error = 0;

	/*
	 * Avoid recursing too far.
	 */
	if (++depth > maxdepth) {
		module_error("too many required modules");
		depth--;
		return EMLINK;
	}

	/*
	 * Load the module and link.  Before going to the file system,
	 * scan the list of modules loaded by the boot loader.  Just
	 * before init is started the list of modules loaded at boot
	 * will be purged.  Before init is started we can assume that
	 * `filename' is a module name and not a path name.
	 */
	TAILQ_FOREACH(mod, &module_bootlist, mod_chain) {
		if (strcmp(mod->mod_info->mi_name, filename) == 0) {
			TAILQ_REMOVE(&module_bootlist, mod, mod_chain);
			break;
		}
	}
	if (mod == NULL) {
		mod = kmem_zalloc(sizeof(*mod), KM_SLEEP);
		if (mod == NULL) {
			depth--;
			return ENOMEM;
		}
		error = kobj_open_file(&mod->mod_kobj, filename);
		if (error != 0) {
			kmem_free(mod, sizeof(*mod));
			depth--;
			module_error("unable to open object file");
			return error;
		}
		error = kobj_load(mod->mod_kobj);
		if (error != 0)
			module_error("unable to load kernel object");
		mod->mod_source = MODULE_SOURCE_FILESYS;
	}
	TAILQ_INSERT_TAIL(&pending, mod, mod_chain);
	if (error != 0) {
		goto fail;
	}

	/*
	 * Find module info record and check compatibility.
	 */
	error = kobj_find_section(mod->mod_kobj, "link_set_modules",
	    &addr, &size);
	if (error != 0) {
		module_error("`link_set_modules' section not present");
		goto fail;
	}
	if (size != sizeof(modinfo_t **)) {
		module_error("`link_set_modules' section wrong size");
		goto fail;
	}
	mod->mod_info = *(modinfo_t **)addr;
	mi = mod->mod_info;

	if (strlen(mi->mi_name) >= MAXMODNAME) {
		error = EINVAL;
		module_error("module name too long");
		goto fail;
	}

	/*
	 * If loading a dependency, `filename' is a plain module name.
	 * The name must match.
	 */
	if (isdep && strcmp(mi->mi_name, filename) != 0) {
		error = ENOENT;
		goto fail;
	}

	/*
	 * Check to see if the module is already loaded.  If so, we may
	 * have been recursively called to handle a dependency, so be sure
	 * to set modp.
	 */
	if ((mod2 = module_lookup(mi->mi_name)) != NULL) {
		if (modp != NULL)
			*modp = mod2;
		error = EEXIST;
		goto fail;
	}

	/*
	 * Block circular dependencies.
	 */
	TAILQ_FOREACH(mod2, &pending, mod_chain) {
		if (mod == mod2) {
			continue;
		}
		if (strcmp(mod2->mod_info->mi_name, mi->mi_name) == 0) {
		    	error = EDEADLK;
			module_error("circular dependency detected");
		    	goto fail;
		}
	}

	/*
	 * Pass proper name to kobj.  This will register the module
	 * with the ksyms framework.
	 */
	error = kobj_set_name(mod->mod_kobj, mi->mi_name);
	if (error != 0) {
		module_error("unable to set name");
		goto fail;
	}

	/*
	 * Now try to load any requisite modules.
	 */
	if (mi->mi_required != NULL) {
		for (s = mi->mi_required; *s != '\0'; s = p) {
			if (*s == ',')
				s++;
			p = s;
			while (*p != '\0' && *p != ',')
				p++;
			len = p - s + 1;
			if (len >= MAXMODNAME) {
				error = EINVAL;
				module_error("required module name too long");
				goto fail;
			}
			strlcpy(buf, s, len);
			if (buf[0] == '\0')
				break;
			if (mod->mod_nrequired == MAXMODDEPS - 1) {
				error = EINVAL;
				module_error("too many required modules");
				goto fail;
			}
			if (strcmp(buf, mi->mi_name) == 0) {
				error = EDEADLK;
				module_error("self-dependency detected");
				goto fail;
			}
			error = module_do_load(buf, true, force,
			    &mod->mod_required[mod->mod_nrequired++]);
			if (error != 0 && error != EEXIST)
				goto fail;
		}
	}

	/*
	 * We loaded all needed modules successfully: initialize.
	 */
	error = (*mi->mi_modcmd)(MODULE_CMD_INIT, NULL);
	if (error != 0) {
		module_error("modctl function returned error %d", error);
		goto fail;
	}

	/*
	 * Good, the module loaded successfully.  Put it onto the
	 * list and add references to its requisite modules.
	 */
	module_count++;
	kobj_close(mod->mod_kobj);
	TAILQ_REMOVE(&pending, mod, mod_chain);
	TAILQ_INSERT_TAIL(&module_list, mod, mod_chain);
	for (i = 0; i < mod->mod_nrequired; i++) {
		KASSERT(mod->mod_required[i] != NULL);
		mod->mod_required[i]->mod_refcnt++;
	}
	if (modp != NULL) {
		*modp = mod;
	}
	depth--;
	return 0;
 fail:
	kobj_close(mod->mod_kobj);
	TAILQ_REMOVE(&pending, mod, mod_chain);
	kobj_unload(mod->mod_kobj);
	kmem_free(mod, sizeof(*mod));
	depth--;
	return error;
}

/*
 * module_do_unload:
 *
 *	Helper routine: do the dirty work of unloading a module.
 */
static int
module_do_unload(const char *name)
{
	module_t *mod;
	int error;
	u_int i;

	KASSERT(mutex_owned(&module_lock));

	mod = module_lookup(name);
	if (mod == NULL) {
		return ENOENT;
	}
	if (mod->mod_refcnt != 0 || mod->mod_source == MODULE_SOURCE_KERNEL) {
		return EBUSY;
	}
	error = (*mod->mod_info->mi_modcmd)(MODULE_CMD_FINI, NULL);
	if (error != 0) {
		return error;
	}
	module_count--;
	TAILQ_REMOVE(&module_list, mod, mod_chain);
	for (i = 0; i < mod->mod_nrequired; i++) {
		mod->mod_required[i]->mod_refcnt--;
	}
	if (mod->mod_kobj != NULL) {
		kobj_unload(mod->mod_kobj);
	}
	kmem_free(mod, sizeof(*mod));

	return 0;
}

/*
 * module_prime:
 *
 *	Push a module loaded by the bootloader onto our internal
 *	list.
 */
int
module_prime(void *base, size_t size)
{
	module_t *mod;
	int error;

	mod = kmem_zalloc(sizeof(*mod), KM_SLEEP);
	if (mod == NULL) {
		return ENOMEM;
	}
	mod->mod_source = MODULE_SOURCE_BOOT;

	error = kobj_open_mem(&mod->mod_kobj, base, size);
	if (error == 0) {
		kmem_free(mod, sizeof(*mod));
		module_error("unable to open object pushed by boot loader");
		return error;
	}
	error = kobj_load(mod->mod_kobj);
	if (error != 0) {
		kmem_free(mod, sizeof(*mod));
		module_error("unable to push object pushed by boot loader");
		return error;
	}
	mod->mod_source = MODULE_SOURCE_FILESYS;
	TAILQ_INSERT_TAIL(&module_bootlist, mod, mod_chain);

	return 0;
}
