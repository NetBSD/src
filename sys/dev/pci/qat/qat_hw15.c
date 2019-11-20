/*	$NetBSD: qat_hw15.c,v 1.1 2019/11/20 09:37:46 hikaru Exp $	*/

/*
 * Copyright (c) 2019 Internet Initiative Japan, Inc.
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
 *   Copyright(c) 2007-2013 Intel Corporation. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: qat_hw15.c,v 1.1 2019/11/20 09:37:46 hikaru Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <opencrypto/xform.h>

/* XXX same as sys/arch/x86/x86/via_padlock.c */
#include <opencrypto/cryptosoft_xform.c>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include "qatreg.h"
#include "qat_hw15reg.h"
#include "qatvar.h"
#include "qat_hw15var.h"

int		qat_adm_ring_init_ring_table(struct qat_softc *);
void		qat_adm_ring_build_slice_mask(uint16_t *, uint32_t, uint32_t);
void		qat_adm_ring_build_shram_mask(uint64_t *, uint32_t, uint32_t);
int		qat_adm_ring_build_ring_table(struct qat_softc *, uint32_t);
int		qat_adm_ring_build_init_msg(struct qat_softc *,
		    struct fw_init_req *, enum fw_init_cmd_id, uint32_t,
		    struct qat_accel_init_cb *);
int		qat_adm_ring_send_init_msg_sync(struct qat_softc *,
		    enum fw_init_cmd_id, uint32_t);
int		qat_adm_ring_send_init_msg(struct qat_softc *,
		    enum fw_init_cmd_id);
int		qat_adm_ring_intr(struct qat_softc *, void *, void *);

uint32_t	qat_crypto_setup_cipher_desc(struct qat_session *,
		    struct qat_crypto_desc *desc, struct cryptoini *,
		    struct fw_cipher_hdr *, uint8_t *, uint32_t, enum fw_slice);
uint32_t	qat_crypto_setup_auth_desc(struct qat_session *,
		    struct qat_crypto_desc *, struct cryptoini *,
		    struct fw_auth_hdr *, uint8_t *, uint32_t, enum fw_slice);

void
qat_msg_req_type_populate(struct arch_if_req_hdr *msg, enum arch_if_req type,
    uint32_t rxring)
{

	memset(msg, 0, sizeof(struct arch_if_req_hdr));
	msg->flags = ARCH_IF_FLAGS_VALID_FLAG |
	    ARCH_IF_FLAGS_RESP_RING_TYPE_ET | ARCH_IF_FLAGS_RESP_TYPE_S;
	msg->req_type = type;
	msg->resp_pipe_id = rxring;
}

void
qat_msg_cmn_hdr_populate(struct fw_la_bulk_req *msg, bus_addr_t desc_paddr,
    uint8_t hdrsz, uint8_t hwblksz, uint16_t comn_req_flags, uint32_t flow_id)
{
	struct fw_comn_req_hdr *hdr = &msg->comn_hdr;

	hdr->comn_req_flags = comn_req_flags;
	hdr->content_desc_params_sz = hwblksz;
	hdr->content_desc_hdr_sz = hdrsz;
	hdr->content_desc_addr = desc_paddr;
	msg->flow_id = flow_id;
}

void
qat_msg_service_cmd_populate(struct fw_la_bulk_req *msg, enum fw_la_cmd_id cmdid,
    uint16_t cmd_flags)
{
	msg->comn_la_req.la_cmd_id = cmdid;
	msg->comn_la_req.u.la_flags = cmd_flags;
}

void
qat_msg_cmn_mid_populate(struct fw_comn_req_mid *msg, void *cookie,
    uint64_t src, uint64_t dst)
{

	msg->opaque_data = (uint64_t)(uintptr_t)cookie;
	msg->src_data_addr = src;
	if (dst == 0)
		msg->dest_data_addr = src;
	else
		msg->dest_data_addr = dst;
}

void
qat_msg_req_params_populate(struct fw_la_bulk_req *msg,
    bus_addr_t req_params_paddr, uint8_t req_params_sz)
{
	msg->req_params_addr = req_params_paddr;
	msg->comn_la_req.u1.req_params_blk_sz = req_params_sz / 8;
}


void
qat_msg_cmn_footer_populate(union fw_comn_req_ftr *msg, uint64_t next_addr)
{
	msg->next_request_addr = next_addr;
}

void
qat_msg_params_populate(struct fw_la_bulk_req *msg,
    struct qat_crypto_desc *desc, uint8_t req_params_sz,
    uint16_t service_cmd_flags, uint16_t comn_req_flags)
{
	qat_msg_cmn_hdr_populate(msg, desc->qcd_desc_paddr,
	    desc->qcd_hdr_sz, desc->qcd_hw_blk_sz, comn_req_flags, 0);
	qat_msg_service_cmd_populate(msg, desc->qcd_cmd_id, service_cmd_flags);
	qat_msg_cmn_mid_populate(&msg->comn_mid, NULL, 0, 0);
	qat_msg_req_params_populate(msg, 0, req_params_sz);
	qat_msg_cmn_footer_populate(&msg->comn_ftr, 0);
}

int
qat_adm_ring_init_ring_table(struct qat_softc *sc)
{
	struct qat_admin_rings *qadr = &sc->sc_admin_rings;

	if (sc->sc_ae_num == 1) {
		qadr->qadr_cya_ring_tbl =
		    &qadr->qadr_master_ring_tbl[0];
		qadr->qadr_srv_mask[0] = QAT_SERVICE_CRYPTO_A;
	} else if (sc->sc_ae_num == 2 ||
	    sc->sc_ae_num == 4) {
		qadr->qadr_cya_ring_tbl =
		    &qadr->qadr_master_ring_tbl[0];
		qadr->qadr_srv_mask[0] = QAT_SERVICE_CRYPTO_A;
		qadr->qadr_cyb_ring_tbl =
		    &qadr->qadr_master_ring_tbl[1];
		qadr->qadr_srv_mask[1] = QAT_SERVICE_CRYPTO_B;
	}

	return 0;
}

int
qat_adm_ring_init(struct qat_softc *sc)
{
	struct qat_admin_rings *qadr = &sc->sc_admin_rings;
	int error, i, j;

	error = qat_alloc_dmamem(sc, &qadr->qadr_dma,
	    PAGE_SIZE, PAGE_SIZE);
	if (error)
		return error;

	qadr->qadr_master_ring_tbl = qadr->qadr_dma.qdm_dma_vaddr;

	KASSERT(sc->sc_ae_num *
	    sizeof(struct fw_init_ring_table) <= PAGE_SIZE);

	/* Initialize the Master Ring Table */
	for (i = 0; i < sc->sc_ae_num; i++) {
		struct fw_init_ring_table *firt =
		    &qadr->qadr_master_ring_tbl[i];

		for (j = 0; j < INIT_RING_TABLE_SZ; j++) {
			struct fw_init_ring_params *firp = 
			    &firt->firt_bulk_rings[j];

			firp->firp_reserved = 0;
			firp->firp_curr_weight = QAT_DEFAULT_RING_WEIGHT;
			firp->firp_init_weight = QAT_DEFAULT_RING_WEIGHT;
			firp->firp_ring_pvl = QAT_DEFAULT_PVL;
		}
		memset(firt->firt_ring_mask, 0, sizeof(firt->firt_ring_mask));
	}

	error = qat_etr_setup_ring(sc, 0, RING_NUM_ADMIN_TX,
	    ADMIN_RING_SIZE, sc->sc_hw.qhw_fw_req_size,
	    NULL, NULL, "admin_tx", &qadr->qadr_admin_tx);
	if (error)
		return error;

	error = qat_etr_setup_ring(sc, 0, RING_NUM_ADMIN_RX,
	    ADMIN_RING_SIZE, sc->sc_hw.qhw_fw_resp_size,
	    qat_adm_ring_intr, qadr, "admin_rx", &qadr->qadr_admin_rx);
	if (error)
		return error;

	/*
	 * Finally set up the service indices into the Master Ring Table
	 * and convenient ring table pointers for each service enabled.
	 * Only the Admin rings are initialized.
	 */
	error = qat_adm_ring_init_ring_table(sc);
	if (error)
		return error;

	/*
	 * Calculate the number of active AEs per QAT
	 * needed for Shram partitioning.
	 */
	for (i = 0; i < sc->sc_ae_num; i++) {
		if (qadr->qadr_srv_mask[i])
			qadr->qadr_active_aes_per_accel++;
	}

	return 0;
}

void
qat_adm_ring_build_slice_mask(uint16_t *slice_mask, uint32_t srv_mask,
   uint32_t init_shram)
{
	uint16_t shram = 0, comn_req = 0;

	if (init_shram)
		shram = COMN_REQ_SHRAM_INIT_REQUIRED;

	if (srv_mask & QAT_SERVICE_CRYPTO_A)
		comn_req |= COMN_REQ_CY0_ONLY(shram);
	if (srv_mask & QAT_SERVICE_CRYPTO_B)
		comn_req |= COMN_REQ_CY1_ONLY(shram);

	*slice_mask = comn_req;
}

void
qat_adm_ring_build_shram_mask(uint64_t *shram_mask, uint32_t active_aes,
    uint32_t ae)
{
	*shram_mask = 0;

	if (active_aes == 1) {
		*shram_mask = ~(*shram_mask);
	} else if (active_aes == 2) {
		if (ae == 1)
			*shram_mask = ((~(*shram_mask)) & 0xffffffff);
		else
			*shram_mask = ((~(*shram_mask)) & 0xffffffff00000000ull);
	} else if (active_aes == 3) {
		if (ae == 0)
			*shram_mask = ((~(*shram_mask)) & 0x7fffff);
		else if (ae == 1)
			*shram_mask = ((~(*shram_mask)) & 0x3fffff800000ull);
		else
			*shram_mask = ((~(*shram_mask)) & 0xffffc00000000000ull);
	} else {
		panic("Only three services are supported in current version");
	}
}

int
qat_adm_ring_build_ring_table(struct qat_softc *sc, uint32_t ae)
{
	struct qat_admin_rings *qadr = &sc->sc_admin_rings;
	struct fw_init_ring_table *tbl;
	struct fw_init_ring_params *param;
	uint8_t srv_mask = sc->sc_admin_rings.qadr_srv_mask[ae];

	if ((srv_mask & QAT_SERVICE_CRYPTO_A)) {
		tbl = qadr->qadr_cya_ring_tbl;
	} else if ((srv_mask & QAT_SERVICE_CRYPTO_B)) {
		tbl = qadr->qadr_cyb_ring_tbl;
	} else {
		aprint_error_dev(sc->sc_dev,
		    "Invalid execution engine %d\n", ae);
		return EINVAL;
	}

	param = &tbl->firt_bulk_rings[sc->sc_hw.qhw_ring_sym_tx];
	param->firp_curr_weight = QAT_HI_PRIO_RING_WEIGHT;
	param->firp_init_weight = QAT_HI_PRIO_RING_WEIGHT;
	FW_INIT_RING_MASK_SET(tbl, sc->sc_hw.qhw_ring_sym_tx);

	return 0;
}

int
qat_adm_ring_build_init_msg(struct qat_softc *sc,
    struct fw_init_req *initmsg, enum fw_init_cmd_id cmd, uint32_t ae,
    struct qat_accel_init_cb *cb)
{
	struct fw_init_set_ae_info_hdr *aehdr;
	struct fw_init_set_ae_info *aeinfo;
	struct fw_init_set_ring_info_hdr *ringhdr;
	struct fw_init_set_ring_info *ringinfo;
	int init_shram = 0, tgt_id, cluster_id;
	uint32_t srv_mask;

	srv_mask = sc->sc_admin_rings.qadr_srv_mask[
	    ae % sc->sc_ae_num];
	
	memset(initmsg, 0, sizeof(struct fw_init_req));

	qat_msg_req_type_populate(&initmsg->comn_hdr.arch_if,
	    ARCH_IF_REQ_QAT_FW_INIT,
	    sc->sc_admin_rings.qadr_admin_rx->qr_ring_id);

	qat_msg_cmn_mid_populate(&initmsg->comn_mid, cb, 0, 0);

	switch (cmd) {
	case FW_INIT_CMD_SET_AE_INFO:
		if (ae % sc->sc_ae_num == 0)
			init_shram = 1;
		if (ae >= sc->sc_ae_num) {
			tgt_id = 1;
			cluster_id = 1;
		} else {
			cluster_id = 0;
			if (sc->sc_ae_mask)
				tgt_id = 0;
			else
				tgt_id = 1;
		}
		aehdr = &initmsg->u.set_ae_info;
		aeinfo = &initmsg->u1.set_ae_info;

		aehdr->init_cmd_id = cmd;
		/* XXX that does not support sparse ae_mask */
		aehdr->init_trgt_id = ae;
		aehdr->init_ring_cluster_id = cluster_id;
		aehdr->init_qat_id = tgt_id;

		qat_adm_ring_build_slice_mask(&aehdr->init_slice_mask, srv_mask,
		    init_shram);

		qat_adm_ring_build_shram_mask(&aeinfo->init_shram_mask,
		    sc->sc_admin_rings.qadr_active_aes_per_accel,
		    ae % sc->sc_ae_num);

		break;
	case FW_INIT_CMD_SET_RING_INFO:
		ringhdr = &initmsg->u.set_ring_info;
		ringinfo = &initmsg->u1.set_ring_info;

		ringhdr->init_cmd_id = cmd;
		/* XXX that does not support sparse ae_mask */
		ringhdr->init_trgt_id = ae;

		/* XXX */
		qat_adm_ring_build_ring_table(sc,
		    ae % sc->sc_ae_num);

		ringhdr->init_ring_tbl_sz = sizeof(struct fw_init_ring_table);

		ringinfo->init_ring_table_ptr =
		    sc->sc_admin_rings.qadr_dma.qdm_dma_seg.ds_addr +
		    ((ae % sc->sc_ae_num) *
		    sizeof(struct fw_init_ring_table));

		break;
	default:
		return ENOTSUP;
	}

	return 0;
}

int
qat_adm_ring_send_init_msg_sync(struct qat_softc *sc,
    enum fw_init_cmd_id cmd, uint32_t ae)
{
	struct fw_init_req initmsg;
	struct qat_accel_init_cb cb;
	int error;

	error = qat_adm_ring_build_init_msg(sc, &initmsg, cmd, ae, &cb);
	if (error)
		return error;

	error = qat_etr_put_msg(sc, sc->sc_admin_rings.qadr_admin_tx,
	    (uint32_t *)&initmsg);
	if (error)
		return error;

	error = tsleep(&cb, PZERO, "qat_init", hz * 3 / 2);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Timed out initialization firmware: %d\n", error);
		return error;
	}
	if (cb.qaic_status) {
		aprint_error_dev(sc->sc_dev, "Failed to initialize firmware\n");
		return EIO;
	}

	return error;
}

int
qat_adm_ring_send_init_msg(struct qat_softc *sc,
    enum fw_init_cmd_id cmd)
{
	struct qat_admin_rings *qadr = &sc->sc_admin_rings;
	uint32_t error, ae;

	for (ae = 0; ae < sc->sc_ae_num; ae++) {
		uint8_t srv_mask = qadr->qadr_srv_mask[ae];
		switch (cmd) {
		case FW_INIT_CMD_SET_AE_INFO:
		case FW_INIT_CMD_SET_RING_INFO:
			if (!srv_mask)
				continue;
			break;
		case FW_INIT_CMD_TRNG_ENABLE:
		case FW_INIT_CMD_TRNG_DISABLE:
			if (!(srv_mask & QAT_SERVICE_CRYPTO_A))
				continue;
			break;
		default:
			return ENOTSUP;
		}

		error = qat_adm_ring_send_init_msg_sync(sc, cmd, ae);
		if (error)
			return error;
	}

	return 0;
}

int
qat_adm_ring_send_init(struct qat_softc *sc)
{
	int error;

	error = qat_adm_ring_send_init_msg(sc, FW_INIT_CMD_SET_AE_INFO);
	if (error)
		return error;

	error = qat_adm_ring_send_init_msg(sc, FW_INIT_CMD_SET_RING_INFO);
	if (error)
		return error;

	aprint_verbose_dev(sc->sc_dev, "Initialization completed\n");

	return 0;
}

int
qat_adm_ring_intr(struct qat_softc *sc, void *arg, void *msg)
{
	struct arch_if_resp_hdr *resp;
	struct fw_init_resp *init_resp;
	struct qat_accel_init_cb *init_cb;
	int handled = 0;

	resp = (struct arch_if_resp_hdr *)msg;

	switch (resp->resp_type) {
	case ARCH_IF_REQ_QAT_FW_INIT:
		init_resp = (struct fw_init_resp *)msg;
		init_cb = (struct qat_accel_init_cb *)
		    (uintptr_t)init_resp->comn_resp.opaque_data;
		init_cb->qaic_status =
		    __SHIFTOUT(init_resp->comn_resp.comn_status,
		    COMN_RESP_INIT_ADMIN_STATUS);
		wakeup(init_cb);
		break;
	default:
		aprint_error_dev(sc->sc_dev,
		    "unknown resp type %d\n", resp->resp_type);
		break;
	}

	return handled;
}

static inline uint16_t
qat_hw15_get_comn_req_flags(uint8_t ae)
{
	if (ae == 0) {
		return COMN_REQ_ORD_STRICT | COMN_REQ_PTR_TYPE_SGL |
		    COMN_REQ_AUTH0_SLICE_REQUIRED |
		    COMN_REQ_CIPHER0_SLICE_REQUIRED;
	} else {
		return COMN_REQ_ORD_STRICT | COMN_REQ_PTR_TYPE_SGL |
		    COMN_REQ_AUTH1_SLICE_REQUIRED |
		    COMN_REQ_CIPHER1_SLICE_REQUIRED;
	}
}

uint32_t
qat_crypto_setup_cipher_desc(struct qat_session *qs,
    struct qat_crypto_desc *desc, struct cryptoini *crie,
    struct fw_cipher_hdr *cipher_hdr, uint8_t *hw_blk_ptr,
    uint32_t hw_blk_offset, enum fw_slice next_slice)
{
	struct hw_cipher_config *cipher_config = (struct hw_cipher_config *)
	    (hw_blk_ptr + hw_blk_offset);
	uint32_t hw_blk_size;
	uint8_t *cipher_key = (uint8_t *)(cipher_config + 1);

	cipher_config->val = qat_crypto_load_cipher_cryptoini(desc, crie);
	cipher_config->reserved = 0;

	cipher_hdr->state_padding_sz = 0;
	cipher_hdr->key_sz = crie->cri_klen / 64; /* bits to quad words */

	cipher_hdr->state_sz = desc->qcd_cipher_blk_sz / 8;

	cipher_hdr->next_id = next_slice;
	cipher_hdr->curr_id = FW_SLICE_CIPHER;
	cipher_hdr->offset = hw_blk_offset / 8;
	cipher_hdr->resrvd = 0;

	hw_blk_size = sizeof(struct hw_cipher_config);

	memcpy(cipher_key, crie->cri_key, crie->cri_klen / 8);
	hw_blk_size += crie->cri_klen / 8;

	return hw_blk_size;
}

uint32_t
qat_crypto_setup_auth_desc(struct qat_session *qs, struct qat_crypto_desc *desc,
    struct cryptoini *cria, struct fw_auth_hdr *auth_hdr, uint8_t *hw_blk_ptr,
    uint32_t hw_blk_offset, enum fw_slice next_slice)
{
	struct qat_sym_hash_def const *hash_def;
	const struct swcr_auth_hash *sah;
	struct hw_auth_setup *auth_setup;
	uint32_t hw_blk_size;
	uint8_t *state1, *state2;
	uint32_t state_size;

	auth_setup = (struct hw_auth_setup *)(hw_blk_ptr + hw_blk_offset);

	auth_setup->auth_config.config =
	    qat_crypto_load_auth_cryptoini(desc, cria, &hash_def);
	sah = hash_def->qshd_alg->qshai_sah;
	auth_setup->auth_config.reserved = 0;

	/* for HMAC in mode 1 authCounter is the block size
	 * else the authCounter is 0. The firmware expects the counter to be
	 * big endian */
	auth_setup->auth_counter.counter =
	    htonl(hash_def->qshd_qat->qshqi_auth_counter);
	auth_setup->auth_counter.reserved = 0;

	auth_hdr->next_id = next_slice;
	auth_hdr->curr_id = FW_SLICE_AUTH;
	auth_hdr->offset = hw_blk_offset / 8;
	auth_hdr->resrvd = 0;

	auth_hdr->hash_flags = FW_AUTH_HDR_FLAG_NO_NESTED;
	auth_hdr->u.inner_prefix_sz = 0;
	auth_hdr->outer_prefix_sz = 0;
	auth_hdr->final_sz = sah->auth_hash->authsize;
	auth_hdr->inner_state1_sz =
	    roundup(hash_def->qshd_qat->qshqi_state1_len, 8);
	auth_hdr->inner_res_sz = hash_def->qshd_alg->qshai_digest_len;
	auth_hdr->inner_state2_sz =
	    roundup(hash_def->qshd_qat->qshqi_state2_len, 8);
	auth_hdr->inner_state2_off = auth_hdr->offset +
	    ((sizeof(struct hw_auth_setup) + auth_hdr->inner_state1_sz) / 8);

	hw_blk_size = sizeof(struct hw_auth_setup) + auth_hdr->inner_state1_sz +
	    auth_hdr->inner_state2_sz;

	auth_hdr->outer_config_off = 0;
	auth_hdr->outer_state1_sz = 0;
	auth_hdr->outer_res_sz = 0;
	auth_hdr->outer_prefix_off = 0;

	state1 = (uint8_t *)(auth_setup + 1);
	state2 = state1 + auth_hdr->inner_state1_sz;

	state_size = hash_def->qshd_alg->qshai_state_size;
	if (hash_def->qshd_qat->qshqi_algo_enc == HW_AUTH_ALGO_SHA1) {
		uint32_t state1_pad_len = auth_hdr->inner_state1_sz -
		    state_size;
		uint32_t state2_pad_len = auth_hdr->inner_state2_sz -
		    state_size;
		if (state1_pad_len > 0)
			memset(state1 + state_size, 0, state1_pad_len);
		if (state2_pad_len > 0)
			memset(state2 + state_size, 0, state2_pad_len);
	}

	desc->qcd_state_storage_sz = (sizeof(struct hw_auth_counter) +
	    roundup(state_size, 8)) / 8;

	qat_crypto_hmac_precompute(desc, cria, hash_def, state1, state2);

	return hw_blk_size;
}


void
qat_hw15_crypto_setup_desc(struct qat_crypto *qcy, struct qat_session *qs,
    struct qat_crypto_desc *desc,
    struct cryptoini *crie, struct cryptoini *cria)
{
	struct fw_cipher_hdr *cipher_hdr;
	struct fw_auth_hdr *auth_hdr;
	struct fw_la_bulk_req *req_cache;
	uint32_t ctrl_blk_size = 0, ctrl_blk_offset = 0, hw_blk_offset = 0;
	int i;
	uint16_t la_cmd_flags = 0;
	uint8_t req_params_sz = 0;
	uint8_t *ctrl_blk_ptr;
	uint8_t *hw_blk_ptr;

	if (crie != NULL)
		ctrl_blk_size += sizeof(struct fw_cipher_hdr);
	if (cria != NULL)
		ctrl_blk_size += sizeof(struct fw_auth_hdr);

	ctrl_blk_ptr = desc->qcd_content_desc;
	hw_blk_ptr = ctrl_blk_ptr + ctrl_blk_size;

	for (i = 0; i < MAX_FW_SLICE; i++) {
		switch (desc->qcd_slices[i]) {
		case FW_SLICE_CIPHER:
			cipher_hdr = (struct fw_cipher_hdr *)(ctrl_blk_ptr +
			    ctrl_blk_offset);
			ctrl_blk_offset += sizeof(struct fw_cipher_hdr);
			hw_blk_offset += qat_crypto_setup_cipher_desc(qs, desc,
			    crie, cipher_hdr, hw_blk_ptr, hw_blk_offset,
			    desc->qcd_slices[i + 1]);
			req_params_sz += sizeof(struct fw_la_cipher_req_params);
			break;
		case FW_SLICE_AUTH:
			auth_hdr = (struct fw_auth_hdr *)(ctrl_blk_ptr +
			    ctrl_blk_offset);
			ctrl_blk_offset += sizeof(struct fw_auth_hdr);
			hw_blk_offset += qat_crypto_setup_auth_desc(qs, desc,
			    cria, auth_hdr, hw_blk_ptr, hw_blk_offset,
			    desc->qcd_slices[i + 1]);
			req_params_sz += sizeof(struct fw_la_auth_req_params);
			la_cmd_flags |= LA_FLAGS_RET_AUTH_RES;
			/* no digest verify */
			break;
		case FW_SLICE_DRAM_WR:
			i = MAX_FW_SLICE; /* end of chain */
			break;
		default:
			KASSERT(0);
			break;
		}
	}

	desc->qcd_hdr_sz = ctrl_blk_offset / 8;
	desc->qcd_hw_blk_sz = hw_blk_offset / 8;

	req_cache = (struct fw_la_bulk_req *)desc->qcd_req_cache;
	qat_msg_req_type_populate(
	    &req_cache->comn_hdr.arch_if,
	    ARCH_IF_REQ_QAT_FW_LA, 0);

	la_cmd_flags |= LA_FLAGS_PROTO_NO;

	qat_msg_params_populate(req_cache,
	    desc, req_params_sz, la_cmd_flags, 0);

#ifdef QAT_DUMP
	qat_dump_raw(QAT_DUMP_DESC, "qcd_content_desc",
	    desc->qcd_content_desc, sizeof(desc->qcd_content_desc));
	qat_dump_raw(QAT_DUMP_DESC, "qcd_req_cache",
	    &desc->qcd_req_cache, sizeof(desc->qcd_req_cache));
#endif

	bus_dmamap_sync(qcy->qcy_sc->sc_dmat,
	    qcy->qcy_session_dmamems[qs->qs_lid].qdm_dma_map, 0,
	    sizeof(struct qat_session),
	    BUS_DMASYNC_PREWRITE);
}

void
qat_hw15_crypto_setup_req_params(struct qat_crypto_bank *qcb, struct qat_session *qs,
    struct qat_crypto_desc const *desc, struct qat_sym_cookie *qsc,
    struct cryptodesc *crde, struct cryptodesc *crda, bus_addr_t icv_paddr)
{
	struct qat_sym_bulk_cookie *qsbc;
	struct fw_la_bulk_req *bulk_req;
	struct fw_la_cipher_req_params *cipher_req;
	struct fw_la_auth_req_params *auth_req;
	uint32_t req_params_offset = 0;
	uint8_t *req_params_ptr;
	enum fw_la_cmd_id cmd_id = desc->qcd_cmd_id;
	enum fw_slice next_slice;

	qsbc = &qsc->u.qsc_bulk_cookie;

	bulk_req = (struct fw_la_bulk_req *)qsbc->qsbc_msg;
	memcpy(bulk_req, &desc->qcd_req_cache, QAT_HW15_SESSION_REQ_CACHE_SIZE);
	bulk_req->comn_hdr.arch_if.resp_pipe_id = qcb->qcb_sym_rx->qr_ring_id;
	bulk_req->comn_hdr.comn_req_flags =
	    qat_hw15_get_comn_req_flags(qcb->qcb_bank % 2);
	bulk_req->comn_mid.src_data_addr = qsc->qsc_buffer_list_desc_paddr;
	bulk_req->comn_mid.dest_data_addr = qsc->qsc_buffer_list_desc_paddr;
	bulk_req->req_params_addr = qsc->qsc_bulk_req_params_buf_paddr;
	bulk_req->comn_ftr.next_request_addr = 0;
	bulk_req->comn_mid.opaque_data = (uint64_t)(uintptr_t)qsc;

	if (icv_paddr != 0)
		bulk_req->comn_la_req.u.la_flags |= LA_FLAGS_DIGEST_IN_BUFFER;

	req_params_ptr = qsbc->qsbc_req_params_buf;

	if (cmd_id != FW_LA_CMD_AUTH) {
		cipher_req = (struct fw_la_cipher_req_params *)
		    (req_params_ptr + req_params_offset);
		req_params_offset += sizeof(struct fw_la_cipher_req_params);

		if (cmd_id == FW_LA_CMD_CIPHER || cmd_id == FW_LA_CMD_HASH_CIPHER)
			next_slice = FW_SLICE_DRAM_WR;
		else
			next_slice = FW_SLICE_AUTH;

		cipher_req->resrvd = 0;

		cipher_req->cipher_state_sz = desc->qcd_cipher_blk_sz / 8;

		cipher_req->curr_id = FW_SLICE_CIPHER;
		cipher_req->next_id = next_slice;

		cipher_req->resrvd1 = 0;

		cipher_req->cipher_off = crde->crd_skip;
		cipher_req->cipher_len = crde->crd_len;
		cipher_req->state_address = qsc->qsc_iv_buf_paddr;

	}
	if (cmd_id != FW_LA_CMD_CIPHER) {
		auth_req = (struct fw_la_auth_req_params *)
		    (req_params_ptr + req_params_offset);
		req_params_offset += sizeof(struct fw_la_auth_req_params);

		if (cmd_id == FW_LA_CMD_HASH_CIPHER)
			next_slice = FW_SLICE_CIPHER;
		else
			next_slice = FW_SLICE_DRAM_WR;

		auth_req->next_id = next_slice;
		auth_req->curr_id = FW_SLICE_AUTH;

		auth_req->auth_res_address = icv_paddr;
		auth_req->auth_res_sz = 0; /* no digest verify */

		auth_req->auth_len = crda->crd_len;
		auth_req->auth_off = crda->crd_skip;

		auth_req->hash_state_sz = 0;
		auth_req->u1.prefix_addr = desc->qcd_hash_state_paddr +
		    desc->qcd_state_storage_sz;

		auth_req->u.resrvd = 0;
	}

#ifdef QAT_DUMP
	qat_dump_raw(QAT_DUMP_DESC, "req_params", req_params_ptr, req_params_offset);
#endif
}

