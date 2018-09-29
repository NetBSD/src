/*	$NetBSD: npf_test_subr.c,v 1.13 2018/09/29 14:41:36 rmind Exp $	*/

/*
 * NPF initialisation and handler routines.
 *
 * Public Domain.
 */

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/cprng.h>
#include <sys/kmem.h>
#include <net/if.h>
#include <net/if_types.h>
#endif

#include "npf_impl.h"
#include "npf_test.h"

/* State of the current stream. */
static npf_state_t	cstream_state;
static void *		cstream_ptr;
static bool		cstream_retval;

static long		(*_random_func)(void);
static int		(*_pton_func)(int, const char *, void *);
static const char *	(*_ntop_func)(int, const void *, char *, socklen_t);

static void		npf_state_sample(npf_state_t *, bool);

static void		load_npf_config_ifs(nvlist_t *, bool);

#ifndef __NetBSD__
/*
 * Standalone NPF: we define the same struct ifnet members
 * to reduce the npf_ifops_t implementation differences.
 */
struct ifnet {
	char		if_xname[32];
	void *		if_softc;
	TAILQ_ENTRY(ifnet) if_list;
};
#endif

static TAILQ_HEAD(, ifnet) npftest_ifnet_list =
    TAILQ_HEAD_INITIALIZER(npftest_ifnet_list);

static const char *	npftest_ifop_getname(ifnet_t *);
static void		npftest_ifop_flush(void *);
static void *		npftest_ifop_getmeta(const ifnet_t *);
static void		npftest_ifop_setmeta(ifnet_t *, void *);

static const npf_ifops_t npftest_ifops = {
	.getname	= npftest_ifop_getname,
	.lookup		= npf_test_getif,
	.flush		= npftest_ifop_flush,
	.getmeta	= npftest_ifop_getmeta,
	.setmeta	= npftest_ifop_setmeta,
};

void
npf_test_init(int (*pton_func)(int, const char *, void *),
    const char *(*ntop_func)(int, const void *, char *, socklen_t),
    long (*rndfunc)(void))
{
	npf_t *npf;

	npf_sysinit(1);
	npf = npf_create(0, &npftest_mbufops, &npftest_ifops);
	npf_thread_register(npf);
	npf_setkernctx(npf);

	npf_state_setsampler(npf_state_sample);
	_pton_func = pton_func;
	_ntop_func = ntop_func;
	_random_func = rndfunc;
}

void
npf_test_fini(void)
{
	npf_t *npf = npf_getkernctx();
	npf_destroy(npf);
	npf_sysfini();
}

int
npf_test_load(const void *buf, size_t len, bool verbose)
{
	nvlist_t *npf_dict;
	npf_error_t error;

	npf_dict = nvlist_unpack(buf, len, 0);
	if (!npf_dict) {
		printf("%s: could not unpack the nvlist\n", __func__);
		return EINVAL;
	}
	load_npf_config_ifs(npf_dict, verbose);

	// Note: npf_dict will be consumed by npf_load().
	return npf_load(npf_getkernctx(), npf_dict, &error);
}

ifnet_t *
npf_test_addif(const char *ifname, bool reg, bool verbose)
{
	npf_t *npf = npf_getkernctx();
	ifnet_t *ifp = kmem_zalloc(sizeof(*ifp), KM_SLEEP);

	/*
	 * This is a "fake" interface with explicitly set index.
	 * Note: test modules may not setup pfil(9) hooks and if_attach()
	 * may not trigger npf_ifmap_attach(), so we call it manually.
	 */
	strlcpy(ifp->if_xname, ifname, sizeof(ifp->if_xname));
	TAILQ_INSERT_TAIL(&npftest_ifnet_list, ifp, if_list);

	npf_ifmap_attach(npf, ifp);
	if (reg) {
		npf_ifmap_register(npf, ifname);
	}

	if (verbose) {
		printf("+ Interface %s\n", ifname);
	}
	return ifp;
}

static void
load_npf_config_ifs(nvlist_t *npf_dict, bool verbose)
{
	const nvlist_t * const *iflist;
	const nvlist_t *dbg_dict;
	size_t nitems;

	dbg_dict = dnvlist_get_nvlist(npf_dict, "debug", NULL);
	if (!dbg_dict) {
		return;
	}
	if (!nvlist_exists_nvlist_array(dbg_dict, "interfaces")) {
		return;
	}
	iflist = nvlist_get_nvlist_array(dbg_dict, "interfaces", &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *ifdict = iflist[i];
		const char *ifname;

		if ((ifname = nvlist_get_string(ifdict, "name")) != NULL) {
			(void)npf_test_addif(ifname, true, verbose);
		}
	}
}

static const char *
npftest_ifop_getname(ifnet_t *ifp)
{
	return ifp->if_xname;
}

ifnet_t *
npf_test_getif(const char *ifname)
{
	ifnet_t *ifp;

	TAILQ_FOREACH(ifp, &npftest_ifnet_list, if_list) {
		if (!strcmp(ifp->if_xname, ifname))
			return ifp;
	}
	return NULL;
}

static void
npftest_ifop_flush(void *arg)
{
	ifnet_t *ifp;

	TAILQ_FOREACH(ifp, &npftest_ifnet_list, if_list)
		ifp->if_softc = arg;
}

static void *
npftest_ifop_getmeta(const ifnet_t *ifp)
{
	return ifp->if_softc;
}

static void
npftest_ifop_setmeta(ifnet_t *ifp, void *arg)
{
	ifp->if_softc = arg;
}

/*
 * State sampler - this routine is called from inside of NPF state engine.
 */
static void
npf_state_sample(npf_state_t *nst, bool retval)
{
	/* Pointer will serve as an ID. */
	cstream_ptr = nst;
	memcpy(&cstream_state, nst, sizeof(npf_state_t));
	cstream_retval = retval;
}

int
npf_test_statetrack(const void *data, size_t len, ifnet_t *ifp,
    bool forw, int64_t *result)
{
	npf_t *npf = npf_getkernctx();
	struct mbuf *m;
	int i = 0, error;

	m = mbuf_getwithdata(data, len);
	error = npf_packet_handler(npf, &m, ifp, forw ? PFIL_OUT : PFIL_IN);
	if (error) {
		assert(m == NULL);
		return error;
	}
	assert(m != NULL);
	m_freem(m);

	const int di = forw ? NPF_FLOW_FORW : NPF_FLOW_BACK;
	npf_tcpstate_t *fstate = &cstream_state.nst_tcpst[di];
	npf_tcpstate_t *tstate = &cstream_state.nst_tcpst[!di];

	result[i++] = (intptr_t)cstream_ptr;
	result[i++] = cstream_retval;
	result[i++] = cstream_state.nst_state;

	result[i++] = fstate->nst_end;
	result[i++] = fstate->nst_maxend;
	result[i++] = fstate->nst_maxwin;
	result[i++] = fstate->nst_wscale;

	result[i++] = tstate->nst_end;
	result[i++] = tstate->nst_maxend;
	result[i++] = tstate->nst_maxwin;
	result[i++] = tstate->nst_wscale;

	return 0;
}

int
npf_inet_pton(int af, const char *src, void *dst)
{
	return _pton_func(af, src, dst);
}

const char *
npf_inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	return _ntop_func(af, src, dst, size);
}

#ifdef _KERNEL
/*
 * Need to override cprng_fast32() -- we need deterministic PRNG.
 */
uint32_t
cprng_fast32(void)
{
	return (uint32_t)(_random_func ? _random_func() : random());
}
#endif
