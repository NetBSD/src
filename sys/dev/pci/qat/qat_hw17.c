/*	$NetBSD: qat_hw17.c,v 1.1 2019/11/20 09:37:46 hikaru Exp $	*/

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
 *   Copyright(c) 2014 Intel Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: qat_hw17.c,v 1.1 2019/11/20 09:37:46 hikaru Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <opencrypto/xform.h>

/* XXX same as sys/arch/x86/x86/via_padlock.c */
#include <opencrypto/cryptosoft_xform.c>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include "qatreg.h"
#include "qat_hw17reg.h"
#include "qatvar.h"
#include "qat_hw17var.h"

int		qat_adm_mailbox_put_msg_sync(struct qat_softc *, uint32_t,
		    void *, void *);
int		qat_adm_mailbox_send(struct qat_softc *,
		    struct fw_init_admin_req *, struct fw_init_admin_resp *);
int		qat_adm_mailbox_send_init_me(struct qat_softc *);
int		qat_adm_mailbox_send_hb_timer(struct qat_softc *);
int		qat_adm_mailbox_send_fw_status(struct qat_softc *);
int		qat_adm_mailbox_send_constants(struct qat_softc *);

uint32_t	qat_hw17_crypto_setup_cipher_desc(struct qat_session *,
		    struct qat_crypto_desc *, struct cryptoini *,
		    union hw_cipher_algo_blk *, uint32_t, struct fw_la_bulk_req *,
		    enum fw_slice);
uint32_t	qat_hw17_crypto_setup_auth_desc(struct qat_session *,
		    struct qat_crypto_desc *, struct cryptoini *,
		    union hw_auth_algo_blk *, uint32_t, struct fw_la_bulk_req *,
		    enum fw_slice);
void		qat_hw17_init_comn_req_hdr(struct qat_crypto_desc *,
		    struct fw_la_bulk_req *);

int
qat_adm_mailbox_init(struct qat_softc *sc)
{
	uint64_t addr;
	int error;
	struct qat_dmamem *qdm;

	error = qat_alloc_dmamem(sc, &sc->sc_admin_comms.qadc_dma,
	    PAGE_SIZE, PAGE_SIZE);
	if (error)
		return error;

	qdm = &sc->sc_admin_comms.qadc_const_tbl_dma;
	error = qat_alloc_dmamem(sc, qdm, PAGE_SIZE, PAGE_SIZE);
	if (error)
		return error;

	memcpy(qdm->qdm_dma_vaddr,
	    mailbox_const_tab, sizeof(mailbox_const_tab));

	bus_dmamap_sync(sc->sc_dmat, qdm->qdm_dma_map, 0,
	    qdm->qdm_dma_map->dm_mapsize, BUS_DMASYNC_PREWRITE);

	error = qat_alloc_dmamem(sc, &sc->sc_admin_comms.qadc_hb_dma,
	    PAGE_SIZE, PAGE_SIZE);
	if (error)
		return error;

	addr = (uint64_t)sc->sc_admin_comms.qadc_dma.qdm_dma_seg.ds_addr;
	qat_misc_write_4(sc, ADMINMSGUR, addr >> 32);
	qat_misc_write_4(sc, ADMINMSGLR, addr);

	return 0;
}

int
qat_adm_mailbox_put_msg_sync(struct qat_softc *sc, uint32_t ae,
    void *in, void *out)
{
	uint32_t mailbox;
	bus_size_t mb_offset = MAILBOX_BASE + (ae * MAILBOX_STRIDE);
	int offset = ae * ADMINMSG_LEN * 2;
	int times, received;
	uint8_t *buf = (uint8_t *)sc->sc_admin_comms.qadc_dma.qdm_dma_vaddr + offset;

	mailbox = qat_misc_read_4(sc, mb_offset);
	if (mailbox == 1)
		return EAGAIN;

	memcpy(buf, in, ADMINMSG_LEN);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_admin_comms.qadc_dma.qdm_dma_map, 0,
	    sc->sc_admin_comms.qadc_dma.qdm_dma_map->dm_mapsize,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	qat_misc_write_4(sc, mb_offset, 1);

	received = 0;
	for (times = 0; times < 50; times++) {
		delay(20000);
		if (qat_misc_read_4(sc, mb_offset) == 0) {
			received = 1;
			break;
		}
	}
	if (received) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_admin_comms.qadc_dma.qdm_dma_map, 0,
		    sc->sc_admin_comms.qadc_dma.qdm_dma_map->dm_mapsize,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		memcpy(out, buf + ADMINMSG_LEN, ADMINMSG_LEN);
	} else {
		aprint_error_dev(sc->sc_dev,
		    "Failed to send admin msg to accelerator\n");
	}

	return received ? 0 : EFAULT;
}

int
qat_adm_mailbox_send(struct qat_softc *sc,
    struct fw_init_admin_req *req, struct fw_init_admin_resp *resp)
{
	int error;
	uint32_t mask;
	uint8_t ae;

	for (ae = 0, mask = sc->sc_ae_mask; mask; ae++, mask >>= 1) {
		if (!(mask & 1))
			continue;

		error = qat_adm_mailbox_put_msg_sync(sc, ae, req, resp);
		if (error)
			return error;
		if (resp->init_resp_hdr.status) {
			aprint_error_dev(sc->sc_dev,
			    "Failed to send admin msg: cmd %d\n",
			    req->init_admin_cmd_id);
			return EFAULT;
		}
	}

	return 0;
}

int
qat_adm_mailbox_send_init_me(struct qat_softc *sc)
{
	struct fw_init_admin_req req;
	struct fw_init_admin_resp resp;

	memset(&req, 0, sizeof(req));
	req.init_admin_cmd_id = FW_INIT_ME;

	return qat_adm_mailbox_send(sc, &req, &resp);
}

int
qat_adm_mailbox_send_hb_timer(struct qat_softc *sc)
{
	struct fw_init_admin_req req;
	struct fw_init_admin_resp resp;

	memset(&req, 0, sizeof(req));
	req.init_admin_cmd_id = FW_HEARTBEAT_TIMER_SET;

	req.init_cfg_ptr = sc->sc_admin_comms.qadc_hb_dma.qdm_dma_seg.ds_addr;
	req.heartbeat_ticks =
	    sc->sc_hw.qhw_clock_per_sec / 1000 * QAT_HB_INTERVAL;

	return qat_adm_mailbox_send(sc, &req, &resp);
}

int
qat_adm_mailbox_send_fw_status(struct qat_softc *sc)
{
	int error;
	struct fw_init_admin_req req;
	struct fw_init_admin_resp resp;

	memset(&req, 0, sizeof(req));
	req.init_admin_cmd_id = FW_STATUS_GET;

	error = qat_adm_mailbox_send(sc, &req, &resp);
	if (error)
		return error;

	aprint_normal_dev(sc->sc_dev,
	    "loaded firmware: version %d.%d.%d\n",
	    resp.u.s.version_major_num,
	    resp.u.s.version_minor_num,
	    resp.init_resp_pars.u.s1.version_patch_num);

	return 0;
}

int
qat_adm_mailbox_send_constants(struct qat_softc *sc)
{
	struct fw_init_admin_req req;
	struct fw_init_admin_resp resp;

	memset(&req, 0, sizeof(req));
	req.init_admin_cmd_id = FW_CONSTANTS_CFG;

	req.init_cfg_sz = 1024;
	req.init_cfg_ptr =
	    sc->sc_admin_comms.qadc_const_tbl_dma.qdm_dma_seg.ds_addr;

	return qat_adm_mailbox_send(sc, &req, &resp);
}

int
qat_adm_mailbox_send_init(struct qat_softc *sc)
{
	int error;

	error = qat_adm_mailbox_send_init_me(sc);
	if (error)
		return error;

	error = qat_adm_mailbox_send_hb_timer(sc);
	if (error)
		return error;

	error = qat_adm_mailbox_send_fw_status(sc);
	if (error)
		return error;

	return qat_adm_mailbox_send_constants(sc);
}

int
qat_arb_init(struct qat_softc *sc)
{
	uint32_t arb_cfg = 0x1 << 31 | 0x4 << 4 | 0x1;
	uint32_t arb, i;
	const uint32_t *thd_2_arb_cfg;

	/* Service arb configured for 32 bytes responses and
	 * ring flow control check enabled. */
	for (arb = 0; arb < MAX_ARB; arb++)
		qat_arb_sarconfig_write_4(sc, arb, arb_cfg);

	/* Map worker threads to service arbiters */
	sc->sc_hw.qhw_get_arb_mapping(sc, &thd_2_arb_cfg);

	if (!thd_2_arb_cfg)
		return EINVAL;

	for (i = 0; i < sc->sc_hw.qhw_num_engines; i++)
		qat_arb_wrk_2_ser_map_write_4(sc, i, *(thd_2_arb_cfg + i));

	return 0;
}

int
qat_set_ssm_wdtimer(struct qat_softc *sc)
{
	uint32_t timer;
	u_int mask;
	int i;

	timer = sc->sc_hw.qhw_clock_per_sec / 1000 * QAT_SSM_WDT;
	for (i = 0, mask = sc->sc_accel_mask; mask; i++, mask >>= 1) {
		if (!(mask & 1))
			continue;
		qat_misc_write_4(sc, SSMWDT(i), timer);
		qat_misc_write_4(sc, SSMWDTPKE(i), timer);
	}

	return 0;
}

int
qat_check_slice_hang(struct qat_softc *sc)
{
	int handled = 0;

	return handled;
}

uint32_t
qat_hw17_crypto_setup_cipher_desc(struct qat_session *qs,
    struct qat_crypto_desc *desc, struct cryptoini *crie,
    union hw_cipher_algo_blk *cipher, uint32_t cd_blk_offset,
    struct fw_la_bulk_req *req_tmpl, enum fw_slice next_slice)
{
	struct fw_cipher_cd_ctrl_hdr *cipher_cd_ctrl =
	    (struct fw_cipher_cd_ctrl_hdr *)&req_tmpl->cd_ctrl;
	int keylen = crie->cri_klen / 8;

	cipher->max.cipher_config.val =
	    qat_crypto_load_cipher_cryptoini(desc, crie);
	memcpy(cipher->max.key, crie->cri_key, keylen);

	cipher_cd_ctrl->cipher_state_sz = desc->qcd_cipher_blk_sz >> 3;
	cipher_cd_ctrl->cipher_key_sz = keylen >> 3;
	cipher_cd_ctrl->cipher_cfg_offset = cd_blk_offset >> 3;
	FW_COMN_CURR_ID_SET(cipher_cd_ctrl, FW_SLICE_CIPHER);
	FW_COMN_NEXT_ID_SET(cipher_cd_ctrl, next_slice);

	return roundup(sizeof(struct hw_cipher_config) + keylen, 8);
}

uint32_t
qat_hw17_crypto_setup_auth_desc(struct qat_session *qs,
    struct qat_crypto_desc *desc, struct cryptoini *cria,
    union hw_auth_algo_blk *auth, uint32_t cd_blk_offset,
    struct fw_la_bulk_req *req_tmpl, enum fw_slice next_slice)
{
	struct fw_auth_cd_ctrl_hdr *auth_cd_ctrl =
	    (struct fw_auth_cd_ctrl_hdr *)&req_tmpl->cd_ctrl;
	struct qat_sym_hash_def const *hash_def;
	uint8_t *state1, *state2;

	auth->max.inner_setup.auth_config.config =
	    qat_crypto_load_auth_cryptoini(desc, cria, &hash_def);
	auth->max.inner_setup.auth_counter.counter =
	    htonl(hash_def->qshd_qat->qshqi_auth_counter);

	auth_cd_ctrl->hash_cfg_offset = cd_blk_offset >> 3;
	auth_cd_ctrl->hash_flags = FW_AUTH_HDR_FLAG_NO_NESTED;
	auth_cd_ctrl->inner_res_sz = hash_def->qshd_alg->qshai_digest_len;
	auth_cd_ctrl->final_sz = desc->qcd_auth_sz;

	auth_cd_ctrl->inner_state1_sz =
	    roundup(hash_def->qshd_qat->qshqi_state1_len, 8);
	auth_cd_ctrl->inner_state2_sz =
	    roundup(hash_def->qshd_qat->qshqi_state2_len, 8);
	auth_cd_ctrl->inner_state2_offset =
	    auth_cd_ctrl->hash_cfg_offset +
	    ((sizeof(struct hw_auth_setup) +
	    auth_cd_ctrl->inner_state1_sz) >> 3);

	state1 = auth->max.state1;
	state2 = auth->max.state1 + auth_cd_ctrl->inner_state1_sz;
	qat_crypto_hmac_precompute(desc, cria, hash_def, state1, state2);

	FW_COMN_CURR_ID_SET(auth_cd_ctrl, FW_SLICE_AUTH);
	FW_COMN_NEXT_ID_SET(auth_cd_ctrl, next_slice);

	return roundup(auth_cd_ctrl->inner_state1_sz +
	    auth_cd_ctrl->inner_state2_sz +
	    sizeof(struct hw_auth_setup), 8);
}

void
qat_hw17_init_comn_req_hdr(struct qat_crypto_desc *desc,
    struct fw_la_bulk_req *req)
{
	union fw_comn_req_hdr_cd_pars *cd_pars = &req->cd_pars;
	struct fw_comn_req_hdr *req_hdr = &req->comn_hdr;

	req_hdr->service_cmd_id = desc->qcd_cmd_id;
	req_hdr->hdr_flags = FW_COMN_VALID;
	req_hdr->service_type = FW_COMN_REQ_CPM_FW_LA;
	req_hdr->comn_req_flags = FW_COMN_FLAGS_BUILD(
	    COMN_CD_FLD_TYPE_64BIT_ADR, COMN_PTR_TYPE_SGL);
	req_hdr->serv_specif_flags = 0;
	cd_pars->s.content_desc_addr = desc->qcd_desc_paddr;
}

void
qat_hw17_crypto_setup_desc(struct qat_crypto *qcy, struct qat_session *qs,
    struct qat_crypto_desc *desc,
    struct cryptoini *crie, struct cryptoini *cria)
{
	union hw_cipher_algo_blk *cipher;
	union hw_auth_algo_blk *auth;
	struct fw_la_bulk_req *req_tmpl;
	struct fw_comn_req_hdr *req_hdr;
	union fw_comn_req_hdr_cd_pars *cd_pars;
	uint32_t cd_blk_offset = 0;
	int i;
	uint8_t *cd_blk_ptr;

	req_tmpl = (struct fw_la_bulk_req *)desc->qcd_req_cache;
	req_hdr = &req_tmpl->comn_hdr;
	cd_pars = &req_tmpl->cd_pars;
	cd_blk_ptr = desc->qcd_content_desc;

	memset(req_tmpl, 0, sizeof(struct fw_la_bulk_req));
	qat_hw17_init_comn_req_hdr(desc, req_tmpl);

	for (i = 0; i < MAX_FW_SLICE; i++) {
		switch (desc->qcd_slices[i]) {
		case FW_SLICE_CIPHER:
			cipher = (union hw_cipher_algo_blk *)(cd_blk_ptr +
			    cd_blk_offset);
			cd_blk_offset += qat_hw17_crypto_setup_cipher_desc(
			    qs, desc, crie, cipher, cd_blk_offset, req_tmpl,
			    desc->qcd_slices[i + 1]);
			break;
		case FW_SLICE_AUTH:
			auth = (union hw_auth_algo_blk *)(cd_blk_ptr +
			    cd_blk_offset);
			cd_blk_offset += qat_hw17_crypto_setup_auth_desc(
			    qs, desc, cria, auth, cd_blk_offset, req_tmpl,
			    desc->qcd_slices[i + 1]);
			req_hdr->serv_specif_flags |= FW_LA_RET_AUTH_RES;
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

	cd_pars->s.content_desc_params_sz =
	    roundup(cd_blk_offset, QAT_OPTIMAL_ALIGN) >> 3;

#ifdef QAT_DUMP
	qat_dump_raw(QAT_DUMP_DESC, "qcd_content_desc",
	    desc->qcd_content_desc, cd_pars->s.content_desc_params_sz << 3);
	qat_dump_raw(QAT_DUMP_DESC, "qcd_req_cache",
	    &desc->qcd_req_cache, sizeof(desc->qcd_req_cache));
#endif

	bus_dmamap_sync(qcy->qcy_sc->sc_dmat,
	    qcy->qcy_session_dmamems[qs->qs_lid].qdm_dma_map, 0,
	    sizeof(struct qat_session),
	    BUS_DMASYNC_PREWRITE);
}

void
qat_hw17_crypto_setup_req_params(struct qat_crypto_bank *qcb, struct qat_session *qs,
    struct qat_crypto_desc const *desc, struct qat_sym_cookie *qsc,
    struct cryptodesc *crde, struct cryptodesc *crda, bus_addr_t icv_paddr)
{
	struct qat_sym_bulk_cookie *qsbc;
	struct fw_la_bulk_req *bulk_req;
	struct fw_la_cipher_req_params *cipher_param;
	struct fw_la_auth_req_params *auth_param;
	uint32_t req_params_offset = 0;
	uint8_t *req_params_ptr;
	enum fw_la_cmd_id cmd_id = desc->qcd_cmd_id;

	qsbc = &qsc->u.qsc_bulk_cookie;
	bulk_req = (struct fw_la_bulk_req *)qsbc->qsbc_msg;

	memcpy(bulk_req, desc->qcd_req_cache, sizeof(struct fw_la_bulk_req));
	bulk_req->comn_mid.opaque_data = (uint64_t)(uintptr_t)qsc;
	bulk_req->comn_mid.src_data_addr = qsc->qsc_buffer_list_desc_paddr;
	bulk_req->comn_mid.dest_data_addr = qsc->qsc_buffer_list_desc_paddr;

	if (icv_paddr != 0)
		bulk_req->comn_hdr.serv_specif_flags |= FW_LA_DIGEST_IN_BUFFER;

	req_params_ptr = (uint8_t *)&bulk_req->serv_specif_rqpars;

	if (cmd_id != FW_LA_CMD_AUTH) {
		cipher_param = (struct fw_la_cipher_req_params *)
		    (req_params_ptr + req_params_offset);
		req_params_offset += sizeof(struct fw_la_cipher_req_params);

		cipher_param->u.s.cipher_IV_ptr = qsc->qsc_iv_buf_paddr;
		cipher_param->cipher_offset = crde->crd_skip;
		cipher_param->cipher_length = crde->crd_len;
	}

	if (cmd_id != FW_LA_CMD_CIPHER) {
		auth_param = (struct fw_la_auth_req_params *)
		    (req_params_ptr + req_params_offset);
		req_params_offset += sizeof(struct fw_la_auth_req_params);

		auth_param->auth_off = crda->crd_skip;
		auth_param->auth_len = crda->crd_len;
		auth_param->auth_res_addr = icv_paddr;
		auth_param->auth_res_sz = 0; /* XXX no digest verify */
		auth_param->hash_state_sz = 0;
		auth_param->u1.auth_partial_st_prefix = 0;
		auth_param->u2.aad_sz = 0;
	}

#ifdef QAT_DUMP
	qat_dump_raw(QAT_DUMP_DESC, "req_params", req_params_ptr, req_params_offset);
#endif
}

