/*	$NetBSD: npf_test_subr.c,v 1.1 2012/05/30 21:38:04 rmind Exp $	*/

/*
 * NPF initialisation and handler routines.
 *
 * Public Domain.
 */

#include <sys/types.h>
#include <net/if.h>
#include <net/if_types.h>

#include "npf_impl.h"
#include "npf_test.h"

/* State of the current stream. */
static npf_state_t	cstream_state;
static void *		cstream_ptr;
static bool		cstream_retval;

int
npf_test_load(const void *xml)
{
	prop_dictionary_t npf_dict = prop_dictionary_internalize(xml);
	return npfctl_reload(0, npf_dict);
}

/*
 * State sampler - this routine is called from inside of NPF state engine.
 */
void
npf_state_sample(npf_state_t *nst, bool retval)
{
	/* Pointer will serve as an ID. */
	cstream_ptr = nst;
	memcpy(&cstream_state, nst, sizeof(npf_state_t));
	cstream_retval = retval;
}

int
npf_test_handlepkt(const void *data, size_t len, unsigned idx,
    bool forw, int64_t *result)
{
	ifnet_t ifp = { .if_index = idx };
	struct mbuf *m;
	int i = 0, error;

	m = mbuf_getwithdata(data, len);
	error = npf_packet_handler(NULL, &m, &ifp, forw ? PFIL_OUT : PFIL_IN);
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

	result[i++] = tstate->nst_end;
	result[i++] = tstate->nst_maxend;
	result[i++] = tstate->nst_maxwin;

	return 0;
}
