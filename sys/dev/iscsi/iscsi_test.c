/*	$NetBSD: iscsi_test.c,v 1.3 2012/12/29 11:05:30 mlelstv Exp $	*/

/*-
 * Copyright (c) 2006,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include "iscsi_globals.h"

#ifdef ISCSI_TEST_MODE

STATIC test_pars_list_t test_list = TAILQ_HEAD_INITIALIZER(test_list);

/* ------------------- Local Stuff for PDU modifications -------------------- */

/*
 * find_pdu_offset:
 *    Return pointer to PDU location to be modified, based on io vector.
 *    Also return maximum size of field.
 *
 *    Parameter:
 *       pdu      the PDU to modify
 *       offset   the offset into the PDU
 *       ptr      OUT: The pointer to the data
 *       maxlen   OUT: Maximum length of field
 *
 *    Returns 0 on success, nonzero if offset exceeds PDU size.
 */

STATIC int
find_pdu_offset(pdu_t *pdu, int offset, uint8_t **ptr, int *maxlen)
{
	uint8_t *pt = NULL;
	int i, maxl = 0;

	for (i = 0; i < pdu->uio.uio_iovcnt; i++) {
		pt = pdu->io_vec[i].iov_base;
		maxl = pdu->io_vec[i].iov_len;

		if (offset >= maxl) {
			offset -= maxl;
		} else {
			maxl -= offset;
			pt += offset;
			offset = -1;
			break;
		}
	}
	DEB(1, ("Find pdu offset: offs=%d, ptr=%x, maxl=%d\n",
			offset, (int) pt, maxl));

	if (offset >= 0 || !maxl)
		return 1;

	*ptr = pt;
	*maxlen = maxl;
	return 0;
}


/*
 * mod_pdu:
 *    Modify given PDU according to modification descriptor.
 *
 *    Parameter:
 *       pdu      the PDU to modify
 *       mp       the modification descriptor
 */

STATIC void
mod_pdu(pdu_t *pdu, iscsi_pdu_mod_t *mp)
{
	int add = mp->flags & ISCSITEST_MOD_FLAG_ADD_VAL;
	int64_t val = *((int64_t *) mp->value);
	uint8_t *ptr;
	int len;

	switch (mp->offset) {
	case ISCSITEST_OFFSET_DATADIGEST:
		ptr = (uint8_t *) &pdu->data_digest;
		len = sizeof(pdu->data_digest);
		break;

	case ISCSITEST_OFFSET_HEADERDIGEST:
		ptr = (uint8_t *) &pdu->pdu.HeaderDigest;
		len = sizeof(pdu->pdu.HeaderDigest);
		break;

	case ISCSITEST_OFFSET_DATA:
		if ((len = pdu->io_vec[1].iov_len) == 0)
			return;
		ptr = pdu->io_vec[1].iov_base;
		break;

	case ISCSITEST_OFFSET_DRV_CMDSN:
		ptr = (uint8_t *) &pdu->connection->session->CmdSN;
		len = sizeof(pdu->connection->session->CmdSN);
		break;

	default:
		if (find_pdu_offset(pdu, mp->offset, &ptr, &len))
			return;
		break;
	}

	len = min(len, mp->size);

	DEB(1, ("mod_pdu: mpoff=%d, size=%d, len=%d, val=%qx, *ptr=%qx\n",
			mp->offset, mp->size, len, val, *((uint64_t *) ptr)));

	if (!add) {
		if (mp->flags & ISCSITEST_MOD_FLAG_REORDER)
			val = htonq(val);
		memcpy(ptr, &val, len);
	} else if (len == mp->size) {
		switch (len) {
		case 1:
			*ptr += *mp->value;
			break;
		case 2:
			val += (int64_t) (ntohs(*((uint16_t *) ptr)));
			*((uint16_t *) ptr) = htons((uint16_t) val);
			break;
		case 3:
			val += (int64_t) (ntoh3(ptr));
			hton3((uint32_t) val, ptr);
			break;
		case 4:
			val += (int64_t) (ntohl(*((uint32_t *) ptr)));
			*((uint32_t *) ptr) = htonl((uint32_t) val);
			break;
		case 8:
			val += ntohq(*((uint64_t *) ptr));
			*((uint64_t *) ptr) = htonq(val);
			break;

		default:
			break;
		}
	}
	DEB(1, ("mod_pdu: *ptr=%qx\n", *((uint64_t *) ptr)));
}


/*
 * update_options:
 *    Update test options from current mod record.
 *
 *    Parameter:
 *       tp       the test parameters
 *       mod      the mod descriptor
 */

STATIC void
update_options(test_pars_t *tp, mod_desc_t *mod)
{
	if ((tp->options & ISCSITEST_OPT_DISABLE_CCB_TIMEOUT) &&
		(mod->pars.options & ISCSITEST_OPT_ENABLE_CCB_TIMEOUT))
		tp->options &= ~ISCSITEST_OPT_DISABLE_CCB_TIMEOUT;
	else if (!(tp->options & ISCSITEST_OPT_DISABLE_CCB_TIMEOUT) &&
			 (mod->pars.options & ISCSITEST_OPT_DISABLE_CCB_TIMEOUT))
		tp->options |= ISCSITEST_OPT_DISABLE_CCB_TIMEOUT;

	if ((tp->options & ISCSITEST_OPT_DISABLE_CONN_TIMEOUT) &&
		(mod->pars.options & ISCSITEST_OPT_ENABLE_CONN_TIMEOUT))
		tp->options &= ~ISCSITEST_OPT_DISABLE_CONN_TIMEOUT;
	else if (!(tp->options & ISCSITEST_OPT_DISABLE_CONN_TIMEOUT) &&
			 (mod->pars.options & ISCSITEST_OPT_DISABLE_CONN_TIMEOUT))
		tp->options |= ISCSITEST_OPT_DISABLE_CONN_TIMEOUT;

	if (mod->pars.options & ISCSITEST_OPT_USE_RANDOM_TX)
		tp->lose_random[CNT_TX] = mod->pars.lose_random_tx;
	if (mod->pars.options & ISCSITEST_OPT_USE_RANDOM_RX)
		tp->lose_random[CNT_RX] = mod->pars.lose_random_rx;
}


/*
 * test_get:
 *    Copy PDU to app
 *
 *    Parameter:
 *       pdu      the PDU to modify
 *       mod      the modification descriptor
 *       error    the PDU error indication
 */

STATIC void
test_get(pdu_t *pdu, mod_desc_t *mod, int error)
{
	uint32_t size, cpy, dlen;
	uint8_t *pptr;

	pptr = mod->pdu_ptr;
	size = mod->pars.pdu_size;

	cpy = min(size, BHS_SIZE);
	memcpy(pptr, &pdu->pdu, cpy);
	size -= cpy;
	pptr += cpy;
	dlen = pdu->save_uio.uio_resid;

	if (size && dlen) {
		cpy = min(size, dlen);
		memcpy(pptr, (char *) pdu->save_uio.uio_iov, cpy);
		size -= cpy;
	}
	mod->pars.pdu_actual_size = BHS_SIZE + dlen;

	switch (error) {
	case 0:
		mod->pars.status = ISCSI_STATUS_SUCCESS;
		break;

	case TEST_INVALID_HEADER_CRC:
		mod->pars.status = ISCSI_STATUS_TEST_HEADER_CRC_ERROR;
		break;

	case TEST_INVALID_DATA_CRC:
		mod->pars.status = ISCSI_STATUS_TEST_DATA_CRC_ERROR;
		break;

	case TEST_READ_ERROR:
		mod->pars.status = ISCSI_STATUS_TEST_DATA_READ_ERROR;
		break;

	default:
		mod->pars.status = ISCSI_STATUS_GENERAL_ERROR;
		break;
	}
}


/*
 * check_loss:
 *    Check whether PDU is to be discarded randomly.
 *
 *    Parameter:
 *       tp       the test parameters
 *       rxtx     whether this is a recieve or send PDU (CNT_TX or CNT_RX)
 *
 *    Returns nonzero if the PDU is to be discarded, 0 for normal processing.
 */

STATIC int
check_loss(test_pars_t *tp, int rxtx)
{
	return (tp->lose_random[rxtx]) ?
		(cprng_fast32() % tp->lose_random[rxtx]) : 0;
}


/*
 * test_mod:
 *    Check whether PDU is to be modified - calculate offset, modify PDU
 *    on match.
 *
 *    Parameter:
 *       tp       the test parameters
 *       pdu      the PDU to modify
 *       kind     the PDU kind
 *       rxtx     whether this is a recieve or send PDU (CNT_TX or CNT_RX)
 *       err      the PDU error indicator (set on digest/read errors, rx only)
 *
 *    Returns nonzero if the PDU is to be discarded, 0 for normal processing.
 */

STATIC int
test_mod(test_pars_t *tp, pdu_t *pdu, iscsi_pdu_kind_t kind, int rxtx, int err)
{
	mod_desc_t *mod;
	uint32_t mpoff, off;
	int i, rc = 0, s;

	tp->pdu_count[kind][rxtx]++;
	tp->pdu_count[ANY_PDU][rxtx]++;

	do {
		if ((mod = TAILQ_FIRST(&tp->mods)) == NULL) {
			return check_loss(tp, rxtx);
		}
		if (mod->pars.which_pdu != ANY_PDU &&
		    mod->pars.which_pdu != kind) {
			return check_loss(tp, rxtx);
		}
		mpoff = mod->pars.pdu_offset;

		switch (mod->pars.which_offset) {
		case ABSOLUTE_ANY:
			off = tp->pdu_count[ANY_PDU][CNT_TX] +
				  tp->pdu_count[ANY_PDU][CNT_RX];
			break;
		case RELATIVE_ANY:
			off = (tp->pdu_count[ANY_PDU][CNT_TX] +
				   tp->pdu_count[ANY_PDU][CNT_RX]) -
				(tp->pdu_last[ANY_PDU][CNT_TX] + tp->pdu_last[ANY_PDU][CNT_RX]);
			break;

		case ABSOLUTE_PDUKIND:
			off = tp->pdu_count[kind][rxtx];
			break;
		case RELATIVE_PDUKIND:
			off = tp->pdu_count[kind][rxtx] - tp->pdu_last[kind][rxtx];
			break;

		case ABSOLUTE_TX:
			if (rxtx != CNT_TX)
				return check_loss(tp, rxtx);
			off = tp->pdu_count[ANY_PDU][CNT_TX];
			break;
		case RELATIVE_TX:
			if (rxtx != CNT_TX)
				return check_loss(tp, rxtx);
			off = tp->pdu_count[ANY_PDU][CNT_TX] -
				  tp->pdu_last[ANY_PDU][CNT_TX];
			break;

		case ABSOLUTE_RX:
			if (rxtx != CNT_RX)
				return check_loss(tp, rxtx);
			off = tp->pdu_count[ANY_PDU][CNT_RX];
			break;
		case RELATIVE_RX:
			if (rxtx != CNT_RX)
				return check_loss(tp, rxtx);
			off = tp->pdu_count[ANY_PDU][CNT_RX] -
				  tp->pdu_last[ANY_PDU][CNT_RX];
			break;

		default:
			/* bad offset - skip this entry */
			mpoff = off = 0;
			break;
		}

		DEB(1, ("test_mod: kind=%d, rxtx=%d, pdukind=%d, mpoff=%d, "
				"whichoff=%d, off=%d\n", kind, rxtx, mod->pars.which_pdu,
				mpoff, mod->pars.which_offset, off));

		if (!off || (mpoff != 0 && mpoff < off)) {
			/* This might happen in some cases. Just discard the modification. */
			s = splbio();
			TAILQ_REMOVE(&tp->mods, mod, link);
			splx(s);

			update_options(tp, mod);

			if (mod->pars.options & ISCSITEST_OPT_WAIT_FOR_COMPLETION) {
				mod->pars.status = ISCSI_STATUS_TEST_MODIFICATION_SKIPPED;
				wakeup(mod);
			}
			free(mod, M_TEMP);
		}
	} while (mpoff && mpoff < off);

	if (mpoff > off)
		return check_loss(tp, rxtx);

	DEB(1, ("test_mod: opt=%x, pdu_ptr=%x, num_mods=%d\n", mod->pars.options,
			(int) mod->pdu_ptr, mod->pars.num_pdu_mods));

	if (mod->pdu_ptr)
		test_get(pdu, mod, err);

	if (mod->pars.options & ISCSITEST_OPT_DISCARD_PDU)
		rc = 1;
	else if (check_loss(tp, rxtx))
		rc = 1;
	else if (mod->pars.num_pdu_mods) {
		if (!(mod->pars.options & ISCSITEST_OPT_MOD_PERMANENT)) {
			/*
             * Note: if the PDU is later resent, the unmodified one will be
             * used as resend_pdu restores the original io vector.
             */
			pdu->mod_pdu = pdu->pdu;
			pdu->io_vec[0].iov_base = &pdu->mod_pdu;
		}
		for (i = 0; i < mod->pars.num_pdu_mods; i++) {
			mod_pdu(pdu, &mod->mods[i]);
		}
	}

	if (rxtx == CNT_TX) {
		if (mod->pars.options & ISCSITEST_OPT_NO_RESPONSE_PDU) {
			ccb_t *ccb = pdu->owner;

			DEB(1, ("test_mod: No response expected, completing CCB %x\n",
					(int)ccb));

			if (ccb != NULL &&
				(ccb->disp == CCBDISP_WAIT || ccb->disp == CCBDISP_SCSIPI)) {
				/* simulate timeout */
				wake_ccb(ccb, ISCSI_STATUS_TIMEOUT);
			}
		}

		if ((mod->pars.options & ISCSITEST_SFLAG_UPDATE_FIELDS) &&
			mod->pars.num_pdu_mods) {
			connection_t *conn = pdu->connection;

			if (conn->HeaderDigest &&
				!(mod->pars.options & ISCSITEST_SFLAG_NO_HEADER_DIGEST))
				pdu->pdu.HeaderDigest = gen_digest(&pdu->pdu, BHS_SIZE);

			if (pdu->uio.uio_iovcnt > 1 && conn->DataDigest &&
				!(mod->pars.options & ISCSITEST_SFLAG_NO_DATA_DIGEST))
				pdu->data_digest = gen_digest_2(
						pdu->io_vec[1].iov_base,
						pdu->io_vec[1].iov_len,
						pdu->io_vec[2].iov_base,
						pdu->io_vec[2].iov_len);
		}
	}

	s = splbio();
	TAILQ_REMOVE(&tp->mods, mod, link);
	update_options(tp, mod);
	/* we've modified a PDU - copy current count into last count */
	memcpy(tp->pdu_last, tp->pdu_count, sizeof(tp->pdu_last));
	splx(s);

	if (mod->pars.options & ISCSITEST_OPT_WAIT_FOR_COMPLETION) {
		wakeup(mod);
	}
	if (mod->pars.options & ISCSITEST_KILL_CONNECTION) {
		kill_connection(tp->connection,
				ISCSI_STATUS_TEST_CONNECTION_CLOSED,
				NO_LOGOUT, TRUE);
	}
	free(mod, M_TEMP);

	return rc;
}


/*
 * free_negs:
 *    Free all negotiation elements attached to test parameter.
 *
 *    Parameter:
 *       tp       The test parameter
 */

STATIC void
free_negs(test_pars_t *tp)
{
	neg_desc_t *np;

	while ((np = TAILQ_FIRST(&tp->negs)) != NULL) {
		TAILQ_REMOVE(&tp->negs, np, link);
		free(np, M_TEMP);
	}
}


/*
 * free_mods:
 *    Free all modification elements attached to test parameter.
 *
 *    Parameter:
 *       tp       The test parameter
 *       status   The status code to return for clients waiting on completion
 */

STATIC void
free_mods(test_pars_t *tp, uint32_t status)
{
	mod_desc_t *mp;

	while ((mp = TAILQ_FIRST(&tp->mods)) != NULL) {
		TAILQ_REMOVE(&tp->mods, mp, link);
		if (mp->pars.options & ISCSITEST_OPT_WAIT_FOR_COMPLETION) {
			mp->pars.status = status;
			wakeup(mp);
		}
		free(mp, M_TEMP);
	}
}


/* ---------------- Global functions for PDU modifications ------------------ */

/*
 * test_mode_rx:
 *    Check whether received PDU is to be modified - determine PDU kind,
 *    then call test_mod for further processing.
 *
 *    Parameter:
 *       conn     the connection
 *       pdu      the PDU to modify
 *       error    invalid frame if nonzero: don't do mods, frame will be
 *                discarded anyway.
 *
 *    Returns nonzero if the PDU is to be discarded, 0 for normal processing.
 */

int
test_mode_rx(connection_t *conn, pdu_t *pdu, int error)
{
	test_pars_t *tp;
	iscsi_pdu_kind_t kind;

	if ((tp = conn->test_pars) == NULL) {
		return 0;
	}
	switch (pdu->pdu.Opcode & OPCODE_MASK) {
	case TOP_NOP_In:
		kind = NOP_IN_PDU;
		break;

	case TOP_SCSI_Response:
		kind = RESPONSE_PDU;
		break;

	case TOP_SCSI_Task_Management:
		kind = TASK_RSP_PDU;
		break;

	case TOP_Login_Response:
		kind = LOGIN_RSP_PDU;
		break;

	case TOP_Text_Response:
		kind = TEXT_RSP_PDU;
		break;

	case TOP_SCSI_Data_in:
		kind = DATA_IN_PDU;
		break;

	case TOP_Logout_Response:
		kind = LOGOUT_RSP_PDU;
		break;

	case TOP_R2T:
		kind = R2T_PDU;
		break;

	case TOP_Asynchronous_Message:
		kind = ASYNCH_PDU;
		break;

	case TOP_Reject:
		kind = REJECT_PDU;
		break;

	default:
		kind = INVALID_PDU;
		break;
	}

	return test_mod(tp, pdu, kind, CNT_RX, error);
}


/*
 * test_mode_tx:
 *    Check whether PDU to be sent is to be modified - determine PDU kind,
 *    then call test_mod for further processing.
 *
 *    Parameter:
 *       conn     the connection
 *       pdu      the PDU to modify
 *
 *    Returns nonzero if the PDU is to be discarded, 0 for normal processing.
 */

int
test_mode_tx(connection_t *conn, pdu_t *pdu)
{
	test_pars_t *tp;
	iscsi_pdu_kind_t kind;

	if ((tp = conn->test_pars) == NULL)
		return 0;

	switch (pdu->pdu.Opcode & OPCODE_MASK) {
	case IOP_NOP_Out:
		kind = NOP_OUT_PDU;
		break;

	case IOP_SCSI_Command:
		kind = COMMAND_PDU;
		break;

	case IOP_SCSI_Task_Management:
		kind = TASK_REQ_PDU;
		break;

	case IOP_Login_Request:
		kind = LOGIN_REQ_PDU;
		break;

	case IOP_Text_Request:
		kind = TEXT_REQ_PDU;
		break;

	case IOP_SCSI_Data_out:
		kind = DATA_OUT_PDU;
		break;

	case IOP_Logout_Request:
		kind = LOGOUT_REQ_PDU;
		break;

	case IOP_SNACK_Request:
		kind = SNACK_PDU;
		break;

	default:
		kind = INVALID_PDU;
		break;
	}

	return test_mod(tp, pdu, kind, CNT_TX, 0);
}


/*
 * test_assign_connection:
 *    Check whether there is an unassigned test parameter, and assign it to the
 *    given connection.
 *    Note: The number of concurrent tests at any time is assumed to be
 *          very small (usually 1-2). The algorithm used therefore can be very
 *          very simple minded.
 *
 *    Parameter:
 *       conn     the connection
 */

void
test_assign_connection(connection_t *conn)
{
	test_pars_t *tp;

	for (tp = TAILQ_FIRST(&test_list); tp != NULL; tp = TAILQ_NEXT(tp, link)) {
		if (tp->connection == NULL) {
			tp->connection = conn;
			conn->test_pars = tp;
			DEB(1, ("Assigning session %d, connection %d to test %d\n",
					conn->session->id, conn->id, tp->test_id));
			return;
		}
	}
}

/*
 * test_remove_connection:
 *    If the connection has test parameters assigned, remove and free those
 *    parameters.
 *
 *    Parameter:
 *       conn     the connection
 */

void
test_remove_connection(connection_t *conn)
{
	test_pars_t *tp;
	int s;

	s = splbio();
	if ((tp = conn->test_pars) != NULL) {
		conn->test_pars = NULL;
		TAILQ_REMOVE(&test_list, tp, link);
		splx(s);
		DEB(9, ("Remove test %d from connection %d\n", tp->test_id, conn->id));
		free_negs(tp);
		free_mods(tp, ISCSI_STATUS_TEST_CONNECTION_CLOSED);
		free(tp, M_TEMP);
	} else {
		splx(s);
	}
}


/* ----------------------- Local Stuff for IOCTLs --------------------------- */

/*
 * find_test_id:
 *    Search the given test ID in the test list.
 *
 *    Parameter:
 *       id    The test ID
 *
 *    Returns a pointer to the test pars is found, else NULL.
 */

STATIC test_pars_t *
find_test_id(uint32_t id)
{
	test_pars_t *curr;

	for (curr = TAILQ_FIRST(&test_list);
	     curr && curr->test_id != id;
	     curr = TAILQ_NEXT(curr, link)) {
	}
	return curr;
}


/*
 * add_neg:
 *    Add negotiation element to test parameter.
 *    Helper for test_define and test_add_neg.
 *
 *    Parameter:
 *       tp       The test parameter
 *       addr     The pointer to the descriptor
 *       len      The descriptor size
 *
 *    Returns 0 on success, else an ISCSI status code.
 */

STATIC uint32_t
add_neg(test_pars_t *tp, void *addr, uint32_t len)
{
	neg_desc_t *negp;
	uint32_t size;

	/* len already includes the size of iscsi_test_negotiation_descriptor_t */
	size = len + sizeof(neg_desc_t) -
		sizeof(iscsi_test_negotiation_descriptor_t);
	if ((negp = malloc(size, M_TEMP, M_WAITOK)) == NULL) {
		return ISCSI_STATUS_NO_RESOURCES;
	}
	copyin(addr, &negp->entry, len);
	TAILQ_INSERT_TAIL(&tp->negs, negp, link);
	DEB(1, ("add_neg: size %d, phase %d, flags %x, data %02x...\n",
			negp->entry.size, negp->entry.state, negp->entry.flags,
			negp->entry.value[0]));
	return 0;
}



/* ----------------- Global functions for IOCTLs --------------------------- */

/*
 * test_define:
 *    Implements ISCSI_TEST_DEFINE ioctl.
 *
 *    Parameter:
 *       par      The IOCTL parameter.
 */

void
test_define(iscsi_test_define_parameters_t *par)
{
	test_pars_t *tp;
	session_t *sess = NULL;
	connection_t *conn = NULL;

	if (!par->test_id) {
		par->status = ISCSI_STATUS_INVALID_ID;
		return;
	}
	if (find_test_id(par->test_id) != NULL) {
		par->status = ISCSI_STATUS_DUPLICATE_ID;
		return;
	}
	if (par->session_id &&
	    (sess = find_session(par->session_id)) == NULL) {
		par->status = ISCSI_STATUS_INVALID_SESSION_ID;
		return;
	}
	if (sess != NULL) {
		if (par->connection_id &&
			(conn = find_connection(sess, par->connection_id)) == NULL) {
			par->status = ISCSI_STATUS_INVALID_CONNECTION_ID;
			return;
		} else if (!par->connection_id) {
			conn = TAILQ_FIRST(&sess->conn_list);
		}
		if (conn->test_pars != NULL) {
			par->status = ISCSI_STATUS_TEST_ALREADY_ASSIGNED;
			return;
		}
	}

	if ((tp = malloc(sizeof(*tp), M_TEMP, M_WAITOK | M_ZERO)) == NULL) {
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return;
	}
	TAILQ_INIT(&tp->negs);
	TAILQ_INIT(&tp->mods);

	if (par->neg_descriptor_size &&
	    (par->status = add_neg(tp, par->neg_descriptor_ptr,
				par->neg_descriptor_size)) != 0) {
		free(tp, M_TEMP);
		return;
	}

	tp->test_id = par->test_id;
	tp->options = par->options;
	tp->lose_random[CNT_RX] = par->lose_random_rx;
	tp->lose_random[CNT_TX] = par->lose_random_tx;
	tp->connection = conn;
	tp->firstburst_val = par->firstburst_val;
	tp->maxburst_val = par->maxburst_val;
	tp->r2t_val = par->r2t_val;

	DEB(1, ("TestDefine: id=%d, opt=%x, negsize=%d, conn=%x\n",
		tp->test_id, tp->options, par->neg_descriptor_size, (int)conn));

	TAILQ_INSERT_TAIL(&test_list, tp, link);

	if (conn != NULL) {
		tp->connection = conn;
		conn->test_pars = tp;
		DEB(1, ("Assigning session %d, connection %d to test %d\n",
				conn->session->id, conn->id, tp->test_id));
	}

	par->status = ISCSI_STATUS_SUCCESS;
}


/*
 * test_add_neg:
 *    Implements ISCSI_TEST_ADD_NEGOTIATION ioctl.
 *
 *    Parameter:
 *       par      The IOCTL parameter.
 */

void
test_add_neg(iscsi_test_add_negotiation_parameters_t *par)
{
	test_pars_t *tp;

	if ((tp = find_test_id(par->test_id)) == NULL) {
		par->status = ISCSI_STATUS_INVALID_ID;
		return;
	}
	if (par->neg_descriptor_size <
		sizeof(iscsi_test_negotiation_descriptor_t)) {
		par->status = ISCSI_STATUS_PARAMETER_INVALID;
		return;
	}
	par->status =
		add_neg(tp, par->neg_descriptor_ptr, par->neg_descriptor_size);
}


/*
 * test_add_mod:
 *    Implements ISCSI_TEST_ADD_MODIFICATION ioctl.
 *
 *    Parameter:
 *       par      The IOCTL parameter.
 */

void
test_add_mod(struct proc *p, iscsi_test_add_modification_parameters_t *par)
{
	test_pars_t *tp;
	mod_desc_t *modp;
	uint32_t len;
	uint32_t psize = par->pdu_size;
	void *pdu_ptr = par->pdu_ptr;

	if ((tp = find_test_id(par->test_id)) == NULL) {
		par->status = ISCSI_STATUS_INVALID_ID;
		return;
	}

	len = par->num_pdu_mods * sizeof(iscsi_pdu_mod_t);

	if ((modp = malloc(len + sizeof(mod_desc_t),
		M_TEMP, M_WAITOK | M_ZERO)) == NULL) {
		par->status = ISCSI_STATUS_NO_RESOURCES;
		return;
	}

	par->status = ISCSI_STATUS_SUCCESS;

	modp->pars = *par;
	if (len)
		copyin(par->mod_ptr, modp->mods, len);

	if (psize) {
		if (pdu_ptr == NULL) {
			free(modp, M_TEMP);
			par->status = ISCSI_STATUS_PARAMETER_INVALID;
			return;
		}

		if (pdu_ptr == NULL ||
			(par->status = map_databuf(p, &pdu_ptr, psize)) != 0) {
			free(modp, M_TEMP);
			return;
		}

		modp->pdu_ptr = pdu_ptr;
		/* force wait */
		modp->pars.options |= ISCSITEST_OPT_WAIT_FOR_COMPLETION;
	}
	TAILQ_INSERT_TAIL(&tp->mods, modp, link);

	if (modp->pars.options & ISCSITEST_OPT_WAIT_FOR_COMPLETION)
		tsleep(modp, PWAIT, "test_completion", 0);
}


/*
 * test_cancel:
 *    Implements ISCSI_TEST_CANCEL ioctl.
 *
 *    Parameter:
 *       par      The IOCTL parameter.
 */

void
test_cancel(iscsi_test_cancel_parameters_t *par)
{
	test_pars_t *tp;
	int s;

	if ((tp = find_test_id(par->test_id)) == NULL) {
		par->status = ISCSI_STATUS_INVALID_ID;
		return;
	}
	DEB(1, ("Test Cancel, id %d\n", par->test_id));

	s = splbio();
	if (tp->connection)
		tp->connection->test_pars = NULL;
	TAILQ_REMOVE(&test_list, tp, link);
	splx(s);

	free_negs(tp);
	free_mods(tp, ISCSI_STATUS_TEST_CANCELED);
	free(tp, M_TEMP);
	par->status = ISCSI_STATUS_SUCCESS;
}


/*
 * test_send_pdu:
 *    Implements ISCSI_TEST_SEND_PDU ioctl.
 *
 *    Parameter:
 *       p        The caller's context.
 *       par      The IOCTL parameter.
 */

void
test_send_pdu(struct proc *p, iscsi_test_send_pdu_parameters_t *par)
{
	static uint8_t pad_bytes[4] = { 0 };
	test_pars_t *tp;
	connection_t *conn;
	pdu_t *pdu;
	uint32_t psize = par->pdu_size;
	void *pdu_ptr = par->pdu_ptr;
	struct uio *uio;
	uint32_t i, pad, dsl, size;
	int s;

	if ((tp = find_test_id(par->test_id)) == NULL) {
		par->status = ISCSI_STATUS_INVALID_ID;
		return;
	}
	if (!psize || pdu_ptr == NULL ||
		((par->options & ISCSITEST_SFLAG_UPDATE_FIELDS) && psize < BHS_SIZE)) {
		par->status = ISCSI_STATUS_PARAMETER_INVALID;
		return;
	}
	if ((conn = tp->connection) == NULL) {
		par->status = ISCSI_STATUS_TEST_INACTIVE;
		return;
	}
	if ((pdu = get_pdu(conn, TRUE)) == NULL) {
		par->status = ISCSI_STATUS_TEST_CONNECTION_CLOSED;
		return;
	}
	DEB(1, ("Test Send PDU, id %d\n", par->test_id));

	if ((par->status = map_databuf(p, &pdu_ptr, psize)) != 0) {
		free_pdu(pdu);
		return;
	}

	i = 1;
	if (!par->options) {
		pdu->io_vec[0].iov_base = pdu_ptr;
		pdu->io_vec[0].iov_len = size = psize;
	} else {
		memcpy(&pdu->pdu, pdu_ptr, BHS_SIZE);

		if (!(pdu->pdu.Opcode & OP_IMMEDIATE))
			conn->session->CmdSN++;
		pdu->pdu.p.command.CmdSN = htonl(conn->session->CmdSN);

		dsl = psize - BHS_SIZE;
		size = BHS_SIZE;

		hton3(dsl, pdu->pdu.DataSegmentLength);

		if (conn->HeaderDigest &&
			!(par->options & ISCSITEST_SFLAG_NO_HEADER_DIGEST)) {
			pdu->pdu.HeaderDigest = gen_digest(&pdu->pdu, BHS_SIZE);
			size += 4;
		}

		pdu->io_vec[0].iov_base = &pdu->pdu;
		pdu->io_vec[0].iov_len = size;

		if (dsl) {
			pdu->io_vec[1].iov_base = &pdu_ptr[BHS_SIZE];
			pdu->io_vec[1].iov_len = dsl;
			i++;
			size += dsl;

			/* Pad to next multiple of 4 */
			pad = (par->options & ISCSITEST_SFLAG_NO_PADDING) ? 0 : size & 0x03;

			if (pad) {
				pad = 4 - pad;
				pdu->io_vec[i].iov_base = pad_bytes;
				pdu->io_vec[i].iov_len = pad;
				i++;
				size += pad;
			}

			if (conn->DataDigest &&
				!(par->options & ISCSITEST_SFLAG_NO_DATA_DIGEST)) {
				pdu->data_digest = gen_digest_2(&pdu_ptr[BHS_SIZE], dsl,
												pad_bytes, pad);
				pdu->io_vec[i].iov_base = &pdu->data_digest;
				pdu->io_vec[i].iov_len = 4;
				i++;
				size += 4;
			}
		}
	}
	uio = &pdu->uio;
	uio->uio_iov = pdu->io_vec;
	UIO_SETUP_SYSSPACE(uio);
	uio->uio_rw = UIO_WRITE;
	uio->uio_iovcnt = i;
	uio->uio_resid = size;

	pdu->disp = PDUDISP_SIGNAL;
	pdu->flags = PDUF_BUSY | PDUF_NOUPDATE;

	s = splbio();
	/* Enqueue for sending */
	if (pdu->pdu.Opcode & OP_IMMEDIATE)
		TAILQ_INSERT_HEAD(&conn->pdus_to_send, pdu, send_chain);
	else
		TAILQ_INSERT_TAIL(&conn->pdus_to_send, pdu, send_chain);

	wakeup(&conn->pdus_to_send);
	tsleep(pdu, PINOD, "test_send_pdu", 0);
	splx(s);

	unmap_databuf(p, pdu_ptr, psize);
	par->status = ISCSI_STATUS_SUCCESS;
	if (par->options & ISCSITEST_KILL_CONNECTION)
		kill_connection(conn, ISCSI_STATUS_TEST_CONNECTION_CLOSED, NO_LOGOUT,
						TRUE);
}

#endif
