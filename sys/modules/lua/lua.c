/*	$NetBSD: lua.c,v 1.13.2.7 2018/02/25 22:59:28 snj Exp $ */

/*
 * Copyright (c) 2011 - 2017 by Marc Balmer <mbalmer@NetBSD.org>.
 * Copyright (c) 2014 by Lourival Vieira Neto <lneto@NetBSD.org>.
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
 * 3. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Lua device driver */

#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/conf.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>
#include <sys/lock.h>
#include <sys/lua.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/namei.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>
#include <sys/cpu.h>

#include <lauxlib.h>

#include "luavar.h"

struct lua_softc {
	device_t		 sc_dev;

	kmutex_t		 sc_lock;
	kcondvar_t		 sc_inuse_cv;
	bool			 sc_inuse;

	/* Locking access to state queues */
	kmutex_t		 sc_state_lock;
	kcondvar_t		 sc_state_cv;
	bool			 sc_state;

	struct sysctllog	*sc_log;
};

static device_t	sc_self;
static bool	lua_autoload_on = true;
static bool	lua_require_on = true;
static bool	lua_bytecode_on = false;
static int	lua_verbose;
static int	lua_max_instr;

static LIST_HEAD(, lua_state)	lua_states;
static LIST_HEAD(, lua_module)	lua_modules;

static int lua_match(device_t, cfdata_t, void *);
static void lua_attach(device_t, device_t, void *);
static int lua_detach(device_t, int);
static klua_State *klua_find(const char *);
static const char *lua_reader(lua_State *, void *, size_t *);
static void lua_maxcount(lua_State *, lua_Debug *);

static int lua_require(lua_State *);

CFATTACH_DECL_NEW(lua, sizeof(struct lua_softc),
	lua_match, lua_attach, lua_detach, NULL);

dev_type_open(luaopen);
dev_type_close(luaclose);
dev_type_ioctl(luaioctl);

const struct cdevsw lua_cdevsw = {
	.d_open = luaopen,
	.d_close = luaclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = luaioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

struct lua_loadstate {
	struct vnode	*vp;
	size_t		 size;
	off_t		 off;
};

extern struct cfdriver lua_cd;

static int
lua_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
lua_attach(device_t parent, device_t self, void *aux)
{
	struct lua_softc *sc;
	const struct sysctlnode *node;

	if (sc_self)
		return;

	sc = device_private(self);
	sc->sc_dev = self;
	sc_self = self;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_inuse_cv, "luactl");

	mutex_init(&sc->sc_state_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_state_cv, "luastate");

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* Sysctl to provide some control over behaviour */
        sysctl_createv(&sc->sc_log, 0, NULL, &node,
            CTLFLAG_OWNDESC,
            CTLTYPE_NODE, "lua",
            SYSCTL_DESCR("Lua options"),
            NULL, 0, NULL, 0,
            CTL_KERN, CTL_CREATE, CTL_EOL);

        if (node == NULL) {
		aprint_error(": can't create sysctl node\n");
                return;
	}

	/*
	 * XXX Some of the sysctl values must not be changed after the
	 * securelevel has been raised.
	 */
        sysctl_createv(&sc->sc_log, 0, &node, NULL,
            CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
            CTLTYPE_BOOL, "require",
            SYSCTL_DESCR("Enable the require command"),
            NULL, 0, &lua_require_on, 0,
	    CTL_CREATE, CTL_EOL);

        sysctl_createv(&sc->sc_log, 0, &node, NULL,
            CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
            CTLTYPE_BOOL, "autoload",
            SYSCTL_DESCR("Enable automatic load of modules"),
            NULL, 0, &lua_autoload_on, 0,
	    CTL_CREATE, CTL_EOL);

        sysctl_createv(&sc->sc_log, 0, &node, NULL,
            CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
            CTLTYPE_BOOL, "bytecode",
            SYSCTL_DESCR("Enable loading of bytecode"),
            NULL, 0, &lua_bytecode_on, 0,
	    CTL_CREATE, CTL_EOL);

        sysctl_createv(&sc->sc_log, 0, &node, NULL,
            CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
            CTLTYPE_INT, "verbose",
            SYSCTL_DESCR("Enable verbose output"),
            NULL, 0, &lua_verbose, 0,
	    CTL_CREATE, CTL_EOL);

        sysctl_createv(&sc->sc_log, 0, &node, NULL,
            CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
            CTLTYPE_INT, "maxcount",
            SYSCTL_DESCR("Limit maximum instruction count"),
            NULL, 0, &lua_max_instr, 0,
	    CTL_CREATE, CTL_EOL);

	aprint_normal_dev(self, "%s\n", LUA_COPYRIGHT);
}

static int
lua_detach(device_t self, int flags)
{
	struct lua_softc *sc;
	struct lua_state *s;

	sc = device_private(self);
	pmf_device_deregister(self);

	if (sc->sc_log != NULL) {
		sysctl_teardown(&sc->sc_log);
		sc->sc_log = NULL;
	}

	/* Traverse the list of states and close them */
	while ((s = LIST_FIRST(&lua_states)) != NULL) {
		LIST_REMOVE(s, lua_next);
		klua_close(s->K);
		if (lua_verbose)
			device_printf(self, "state %s destroyed\n",
			    s->lua_name);
		kmem_free(s, sizeof(struct lua_state));
	}
	mutex_destroy(&sc->sc_lock);
	cv_destroy(&sc->sc_inuse_cv);
	mutex_destroy(&sc->sc_state_lock);
	cv_destroy(&sc->sc_state_cv);
	sc_self = NULL;
	return 0;
}

int
luaopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct lua_softc *sc;
	int error = 0;

	if (minor(dev) > 0)
		return ENXIO;

	sc = device_lookup_private(&lua_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	mutex_enter(&sc->sc_lock);
	while (sc->sc_inuse == true) {
		error = cv_wait_sig(&sc->sc_inuse_cv, &sc->sc_lock);
		if (error)
			break;
	}
	if (!error)
		sc->sc_inuse = true;
	mutex_exit(&sc->sc_lock);

	if (error)
		return error;
	return 0;
}

int
luaclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct lua_softc *sc;

	if (minor(dev) > 0)
		return ENXIO;
	sc = device_lookup_private(&lua_cd, minor(dev));
	mutex_enter(&sc->sc_lock);
	sc->sc_inuse = false;
	cv_signal(&sc->sc_inuse_cv);
	mutex_exit(&sc->sc_lock);
	return 0;
}

int
luaioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct lua_softc *sc;
	struct lua_info *info;
	struct lua_create *create;
	struct lua_require *require;
	struct lua_load *load;
	struct lua_state *s;
	struct lua_module *m;
	kauth_cred_t cred;
	struct nameidata nd;
	struct pathbuf *pb;
	struct vattr va;
	struct lua_loadstate ls;
	struct lua_state_info *states;
	int error, n;
	klua_State *K;

	sc = device_lookup_private(&lua_cd, minor(dev));
	if (!device_is_active(sc->sc_dev))
		return EBUSY;

	switch (cmd) {
	case LUAINFO:
		info = data;
		if (info->states == NULL) {
			info->num_states = 0;
			LIST_FOREACH(s, &lua_states, lua_next)
				info->num_states++;
		} else {
			n = 0;
			LIST_FOREACH(s, &lua_states, lua_next) {
				if (n > info->num_states)
					break;
				n++;
			}
			info->num_states = n;
			states = kmem_alloc(sizeof(*states) * n, KM_SLEEP);
			if (copyin(info->states, states, sizeof(*states) * n)
			    == 0) {
				n = 0;
				LIST_FOREACH(s, &lua_states, lua_next) {
					if (n > info->num_states)
						break;
					strcpy(states[n].name, s->lua_name);
					strcpy(states[n].desc, s->lua_desc);
					states[n].user = s->K->ks_user;
					n++;
				}
				copyout(states, info->states,
				    sizeof(*states) * n);
				kmem_free(states, sizeof(*states) * n);
			}
		}
		break;
	case LUACREATE:
		create = data;

		if (*create->name == '_') {
			if (lua_verbose)
				device_printf(sc->sc_dev, "names of user "
				    "created states must not begin with '_'");
				return ENXIO;
		}
		LIST_FOREACH(s, &lua_states, lua_next)
			if (!strcmp(s->lua_name, create->name)) {
				if (lua_verbose)
					device_printf(sc->sc_dev,
					    "state %s exists\n", create->name);
				return EBUSY;
			}

		K = kluaL_newstate(create->name, create->desc, IPL_NONE);

		if (K == NULL)
			return ENOMEM;

		K->ks_user = true;

		if (lua_verbose)
			device_printf(sc->sc_dev, "state %s created\n",
			    create->name);
		break;
	case LUADESTROY:
		create = data;

		K = klua_find(create->name);

		if (K != NULL && (K->ks_user == true)) {
			klua_close(K);
			return 0;
		}
		return EBUSY;
	case LUAREQUIRE:	/* 'require' a module in a State */
		require = data;
		LIST_FOREACH(s, &lua_states, lua_next)
			if (!strcmp(s->lua_name, require->state)) {
				LIST_FOREACH(m, &s->lua_modules, mod_next)
					if (!strcmp(m->mod_name, require->module))
						return ENXIO;
				LIST_FOREACH(m, &lua_modules, mod_next)
					if (!strcmp(m->mod_name,
					    require->module)) {
					    	if (lua_verbose)
						    	device_printf(
						    	    sc->sc_dev,
					    		    "requiring module "
					    		    "%s to state %s\n",
					    		    m->mod_name,
					    		    s->lua_name);
						klua_lock(s->K);
						luaL_requiref(
							s->K->L,
							m->mod_name,
							m->open,
							1);
						klua_unlock(s->K);
					    	m->refcount++;
					    	LIST_INSERT_HEAD(
					    	    &s->lua_modules, m,
					    	    mod_next);
					    	return 0;
					}
			}
		return ENXIO;
	case LUALOAD:
		load = data;
		if (strrchr(load->path, '/') == NULL)
			return ENXIO;

		LIST_FOREACH(s, &lua_states, lua_next)
			if (!strcmp(s->lua_name, load->state)) {
				if (lua_verbose)
					device_printf(sc->sc_dev,
					    "loading %s into state %s\n",
					    load->path, s->lua_name);
				cred = kauth_cred_get();
				pb = pathbuf_create(load->path);
				if (pb == NULL)
					return ENOMEM;
				NDINIT(&nd, LOOKUP, FOLLOW | NOCHROOT, pb);
				error = vn_open(&nd, FREAD, 0);
				pathbuf_destroy(pb);
				if (error) {
					if (lua_verbose)
						device_printf(sc->sc_dev,
						    "error vn_open %d\n",
						    error);
					return error;
				}
				error = VOP_GETATTR(nd.ni_vp, &va,
				    kauth_cred_get());
				if (error) {
					VOP_UNLOCK(nd.ni_vp);
					vn_close(nd.ni_vp, FREAD,
					    kauth_cred_get());
					if (lua_verbose)
						device_printf(sc->sc_dev,
						    "erro VOP_GETATTR %d\n",
						    error);
					return error;
				}
				if (va.va_type != VREG) {
					VOP_UNLOCK(nd.ni_vp);
					vn_close(nd.ni_vp, FREAD,
					    kauth_cred_get());
					return EINVAL;
				}
				ls.vp = nd.ni_vp;
				ls.off = 0L;
				ls.size = va.va_size;
				VOP_UNLOCK(nd.ni_vp);
				klua_lock(s->K);
				error = lua_load(s->K->L, lua_reader, &ls,
				    strrchr(load->path, '/') + 1, "bt");
				vn_close(nd.ni_vp, FREAD, cred);
				switch (error) {
				case 0:	/* no error */
					break;
				case LUA_ERRSYNTAX:
					if (lua_verbose)
						device_printf(sc->sc_dev,
						    "syntax error\n");
					klua_unlock(s->K);
					return EINVAL;
				case LUA_ERRMEM:
					if (lua_verbose)
						device_printf(sc->sc_dev,
						    "memory error\n");
					klua_unlock(s->K);
					return ENOMEM;
				default:
					if (lua_verbose)
						device_printf(sc->sc_dev,
						    "load error %d: %s\n",
						    error,
						    lua_tostring(s->K->L, -1));
					klua_unlock(s->K);
					return EINVAL;
				}
				if (lua_max_instr > 0)
					lua_sethook(s->K->L, lua_maxcount,
					    LUA_MASKCOUNT, lua_max_instr);
				error = lua_pcall(s->K->L, 0, LUA_MULTRET, 0);
				if (error) {
					if (lua_verbose) {
						device_printf(sc->sc_dev,
						    "execution error: %s\n",
						    lua_tostring(s->K->L, -1));
					}
					klua_unlock(s->K);
					return EINVAL;
				}
				klua_unlock(s->K);
				return 0;
			}
		return ENXIO;
	}
	return 0;
}

static int
lua_require(lua_State *L)
{
	struct lua_state *s;
	struct lua_module *m, *md;
	const char *module;
	char name[MAXPATHLEN];

	module = lua_tostring(L, -1);
	md = NULL;
	LIST_FOREACH(m, &lua_modules, mod_next)
		if (!strcmp(m->mod_name, module)) {
			md = m;
			break;
		}

	if (md == NULL && lua_autoload_on && strchr(module, '/') == NULL) {
		snprintf(name, sizeof name, "lua%s", module);
		if (lua_verbose)
			device_printf(sc_self, "autoload %s\n", name);
		module_autoload(name, MODULE_CLASS_MISC);
		LIST_FOREACH(m, &lua_modules, mod_next)
			if (!strcmp(m->mod_name, module)) {
				md = m;
				break;
			}
	}

	if (md != NULL)
		LIST_FOREACH(s, &lua_states, lua_next)
			if (s->K->L == L) {
				if (lua_verbose)
					device_printf(sc_self,
					    "require module %s\n",
					    md->mod_name);
				luaL_requiref(L, md->mod_name, md->open, 0);

				LIST_FOREACH(m, &s->lua_modules, mod_next)
					if (m == md)
						return 1;

				md->refcount++;
				LIST_INSERT_HEAD(&s->lua_modules, md, mod_next);
				return 1;
			}

	lua_pushstring(L, "module not found");
	return lua_error(L);
}

typedef struct {
	size_t size;
} __packed alloc_header_t;

static void *
lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	void *nptr = NULL;

	const size_t hdr_size = sizeof(alloc_header_t);
	alloc_header_t *hdr = (alloc_header_t *) ((char *) ptr - hdr_size);

	if (nsize == 0) { /* freeing */
		if (ptr != NULL)
			kmem_intr_free(hdr, hdr->size);
	} else if (ptr != NULL && nsize <= hdr->size - hdr_size) /* shrinking */
		return ptr; /* don't need to reallocate */
	else { /* creating or expanding */
		km_flag_t sleep = cpu_intr_p() || cpu_softintr_p() ?
			KM_NOSLEEP : KM_SLEEP;

		size_t alloc_size = nsize + hdr_size;
		alloc_header_t *nhdr = kmem_intr_alloc(alloc_size, sleep);
		if (nhdr == NULL) /* failed to allocate */
			return NULL;

		nhdr->size = alloc_size;
		nptr = (void *) ((char *) nhdr + hdr_size);

		if (ptr != NULL) { /* expanding */
			memcpy(nptr, ptr, osize);
			kmem_intr_free(hdr, hdr->size);
		}
	}
	return nptr;
}

static const char *
lua_reader(lua_State *L, void *data, size_t *size)
{
	struct lua_loadstate *ls;
	static char buf[1024];
	size_t rsiz;

	ls = data;
	if (ls->size < sizeof(buf))
		rsiz = ls->size;
	else
		rsiz = sizeof(buf);
	vn_rdwr(UIO_READ, ls->vp, buf, rsiz, ls->off, UIO_SYSSPACE,
	    0, curlwp->l_cred, NULL, curlwp);
	if (ls->off == 0L && lua_bytecode_on == false && buf[0] == 0x1b) {
		*size = 0L;
		lua_pushstring(L, "loading of bytecode is not allowed");
		lua_error(L);
		return NULL;
	} else {
		*size = rsiz;
		ls->off += *size;
		ls->size -= *size;
	}
	return buf;
}

static void
lua_maxcount(lua_State *L, lua_Debug *d)
{
	lua_pushstring(L, "maximum instruction count exceeded");
	lua_error(L);
}

int
klua_mod_register(const char *name, lua_CFunction open)
{
	struct lua_module *m;

	LIST_FOREACH(m, &lua_modules, mod_next)
		if (!strcmp(m->mod_name, name))
			return EBUSY;
	m = kmem_zalloc(sizeof(struct lua_module), KM_SLEEP);
	strlcpy(m->mod_name, name, LUA_MAX_MODNAME);
	m->open = open;
	m->refcount = 0;
	LIST_INSERT_HEAD(&lua_modules, m, mod_next);
	if (lua_verbose)
		device_printf(sc_self, "registered lua module %s\n", name);
	return 0;
}

int
klua_mod_unregister(const char *name)
{
	struct lua_module *m;

	LIST_FOREACH(m, &lua_modules, mod_next)
		if (!strcmp(m->mod_name, name)) {
			if (m->refcount == 0) {
				LIST_REMOVE(m, mod_next);
				kmem_free(m, sizeof(struct lua_module));
				if (lua_verbose)
					device_printf(sc_self,
					    "unregistered lua module %s\n",
					    name);
				return 0;
			} else
				return EBUSY;
		}
	return 0;
}

klua_State *
klua_newstate(lua_Alloc f, void *ud, const char *name, const char *desc,
    int ipl)
{
	klua_State *K;
	struct lua_state *s;
	struct lua_softc *sc;
	int error = 0;

	s = kmem_zalloc(sizeof(struct lua_state), KM_SLEEP);
	sc = device_private(sc_self);
	mutex_enter(&sc->sc_state_lock);
	while (sc->sc_state == true) {
		error = cv_wait_sig(&sc->sc_state_cv, &sc->sc_state_lock);
		if (error)
			break;
	}
	if (!error)
		sc->sc_state = true;
	mutex_exit(&sc->sc_state_lock);

	if (error) {
		kmem_free(s, sizeof(struct lua_state));
		return NULL;
	}

	K = kmem_zalloc(sizeof(klua_State), KM_SLEEP);
	K->L = lua_newstate(f, ud);
	K->ks_user = false;
	if (K->L == NULL) {
		kmem_free(K, sizeof(klua_State));
		K = NULL;
		goto finish;
	}

	strlcpy(s->lua_name, name, MAX_LUA_NAME);
	strlcpy(s->lua_desc, desc, MAX_LUA_DESC);
	s->K = K;

	if (lua_require_on || lua_autoload_on) {
		lua_pushcfunction(K->L, lua_require);
		lua_setglobal(K->L, "require");
	}
	LIST_INSERT_HEAD(&lua_states, s, lua_next);

	mutex_init(&K->ks_lock, MUTEX_DEFAULT, ipl);

finish:
	mutex_enter(&sc->sc_state_lock);
	sc->sc_state = false;
	cv_signal(&sc->sc_state_cv);
	mutex_exit(&sc->sc_state_lock);
	return K;
}

inline klua_State *
kluaL_newstate(const char *name, const char *desc, int ipl)
{
	return klua_newstate(lua_alloc, NULL, name, desc, ipl);
}

void
klua_close(klua_State *K)
{
	struct lua_state *s;
	struct lua_softc *sc;
	struct lua_module *m;
	int error = 0;

	/* XXX consider registering a handler instead of a fixed name. */
	lua_getglobal(K->L, "onClose");
	if (lua_isfunction(K->L, -1))
		lua_pcall(K->L, -1, 0, 0);

	sc = device_private(sc_self);
	mutex_enter(&sc->sc_state_lock);
	while (sc->sc_state == true) {
		error = cv_wait_sig(&sc->sc_state_cv, &sc->sc_state_lock);
		if (error)
			break;
	}
	if (!error)
		sc->sc_state = true;
	mutex_exit(&sc->sc_state_lock);

	if (error)
		return;		/* Nothing we can do... */

	LIST_FOREACH(s, &lua_states, lua_next)
		if (s->K == K) {
			LIST_REMOVE(s, lua_next);
			LIST_FOREACH(m, &s->lua_modules, mod_next)
				m->refcount--;
			kmem_free(s, sizeof(struct lua_state));
		}

	lua_close(K->L);
	mutex_destroy(&K->ks_lock);
	kmem_free(K, sizeof(klua_State));

	mutex_enter(&sc->sc_state_lock);
	sc->sc_state = false;
	cv_signal(&sc->sc_state_cv);
	mutex_exit(&sc->sc_state_lock);
}

static klua_State *
klua_find(const char *name)
{
	struct lua_state *s;
	struct lua_softc *sc;
	klua_State *K;
	int error = 0;

	K = NULL;
	sc = device_private(sc_self);
	mutex_enter(&sc->sc_state_lock);
	while (sc->sc_state == true) {
		error = cv_wait_sig(&sc->sc_state_cv, &sc->sc_state_lock);
		if (error)
			break;
	}
	if (!error)
		sc->sc_state = true;
	mutex_exit(&sc->sc_state_lock);

	if (error)
		return NULL;

	LIST_FOREACH(s, &lua_states, lua_next)
		if (!strcmp(s->lua_name, name)) {
			K = s->K;
			break;
		}

	mutex_enter(&sc->sc_state_lock);
	sc->sc_state = false;
	cv_signal(&sc->sc_state_cv);
	mutex_exit(&sc->sc_state_lock);
	return K;
}

inline void
klua_lock(klua_State *K)
{
	mutex_enter(&K->ks_lock);
}

inline void
klua_unlock(klua_State *K)
{
	mutex_exit(&K->ks_lock);
}

MODULE(MODULE_CLASS_MISC, lua, NULL);

#ifdef _MODULE
static const struct cfiattrdata luabus_iattrdata = {
	"luabus", 0, { { NULL, NULL, 0 },}
};

static const struct cfiattrdata *const lua_attrs[] = {
	&luabus_iattrdata, NULL
};

CFDRIVER_DECL(lua, DV_DULL, lua_attrs);
extern struct cfattach lua_ca;
static int lualoc[] = {
	-1,
	-1,
	-1
};

static struct cfdata lua_cfdata[] = {
	{
		.cf_name = "lua",
		.cf_atname = "lua",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = lualoc,
		.cf_flags = 0,
		.cf_pspec = NULL,
	},
	{ NULL, NULL, 0, FSTATE_NOTFOUND, NULL, 0, NULL }
};
#endif

static int
lua_modcmd(modcmd_t cmd, void *opaque)
{
#ifdef _MODULE
	devmajor_t cmajor, bmajor;
	int error = 0;

	cmajor = bmajor = NODEVMAJOR;
#endif
	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_cfdriver_attach(&lua_cd);
		if (error)
			return error;

		error = config_cfattach_attach(lua_cd.cd_name,
		    &lua_ca);
		if (error) {
			config_cfdriver_detach(&lua_cd);
			aprint_error("%s: unable to register cfattach\n",
			    lua_cd.cd_name);
			return error;
		}
		error = config_cfdata_attach(lua_cfdata, 1);
		if (error) {
			config_cfattach_detach(lua_cd.cd_name,
			    &lua_ca);
			config_cfdriver_detach(&lua_cd);
			aprint_error("%s: unable to register cfdata\n",
			    lua_cd.cd_name);
			return error;
		}
		error = devsw_attach(lua_cd.cd_name, NULL, &bmajor,
		    &lua_cdevsw, &cmajor);
		if (error) {
			aprint_error("%s: unable to register devsw\n",
			    lua_cd.cd_name);
			config_cfattach_detach(lua_cd.cd_name, &lua_ca);
			config_cfdriver_detach(&lua_cd);
			return error;
		}
		config_attach_pseudo(lua_cfdata);
#endif
		return 0;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_cfdata_detach(lua_cfdata);
		if (error)
			return error;

		config_cfattach_detach(lua_cd.cd_name, &lua_ca);
		config_cfdriver_detach(&lua_cd);
		devsw_detach(NULL, &lua_cdevsw);
#endif
		return 0;
	case MODULE_CMD_AUTOUNLOAD:
		/* no auto-unload */
		return EBUSY;
	default:
		return ENOTTY;
	}
}
