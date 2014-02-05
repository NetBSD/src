/*	$NetBSD: npf_test_subr.c,v 1.8 2014/02/05 03:49:48 rmind Exp $	*/

/*
 * NPF initialisation and handler routines.
 *
 * Public Domain.
 */

#include <sys/types.h>
#include <sys/cprng.h>
#include <net/if.h>
#include <net/if_types.h>

#include "npf_impl.h"
#include "npf_test.h"

/* State of the current stream. */
static npf_state_t	cstream_state;
static void *		cstream_ptr;
static bool		cstream_retval;

static void		npf_state_sample(npf_state_t *, bool);
static long		(*npf_random_func)(void) = NULL;

void
npf_test_init(long (*rndfunc)(void))
{
	npf_state_setsampler(npf_state_sample);
	npf_random_func = rndfunc;
}

int
npf_test_load(const void *xml)
{
	prop_dictionary_t npf_dict = prop_dictionary_internalize(xml);
	return npfctl_reload(0, npf_dict);
}

ifnet_t *
npf_test_addif(const char *ifname, bool reg, bool verbose)
{
	ifnet_t *ifp = if_alloc(IFT_OTHER);

	/*
	 * This is a "fake" interface with explicitly set index.
	 * Note: test modules may not setup pfil(9) hooks and if_attach()
	 * may not trigger npf_ifmap_attach(), so we call it manually.
	 */
	strlcpy(ifp->if_xname, ifname, sizeof(ifp->if_xname));
	ifp->if_dlt = DLT_NULL;
	ifp->if_index = 0;
	if_attach(ifp);
	if_alloc_sadl(ifp);

	npf_ifmap_attach(ifp);
	if (reg) {
		npf_ifmap_register(ifname);
	}

	if (verbose) {
		printf("+ Interface %s\n", ifname);
	}
	return ifp;
}

ifnet_t *
npf_test_getif(const char *ifname)
{
	return ifunit(ifname);
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
	struct mbuf *m;
	int i = 0, error;

	m = mbuf_getwithdata(data, len);
	error = npf_packet_handler(NULL, &m, ifp, forw ? PFIL_OUT : PFIL_IN);
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

/*
 * Need to override for cprng_fast32() -- we need deterministic PRNG.
 */
uint32_t
_arc4random(void)
{
	return (uint32_t)(npf_random_func ? npf_random_func() : random());
}
