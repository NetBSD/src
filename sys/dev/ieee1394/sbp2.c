/*	$NetBSD: sbp2.c,v 1.1.2.2 2002/02/28 04:13:36 nathanw Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by James Chacon.
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

static int sbp2_print_data(struct configrom_data *);
static int sbp2_print_dir(u_int8_t);
static void sbp2_init(struct fwnode_softc *, struct fwnode_device_cap *);
static void sbp2_login(struct ieee1394_abuf *, int);
static void sbp2_login_resp(struct ieee1394_abuf *, int);

#ifdef SBP2_DEBUG
#define DPRINTF(x)      if (sbp2debug) printf x
#define DPRINTFN(n,x)   if (sbp2debug>(n)) printf x
int     sbp2debug = 1;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

/* XXX: All the sbp2 routines are a complete hack simply to probe a device for
   the moment to make sure all the other code is good. This should all be gutted
   out to it's own file and properly abstracted.
*/

static int
sbp2_print_data(struct configrom_data *data)
{
	switch (data->key_value) {
	case SBP2_KEYVALUE_Command_Set:
		printf("SBP2 Command Set: ");
		if (data->val == 0x104d8)
			printf("SCSI 2\n");
		else
			printf("0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Unit_Characteristics:
		printf("SBP2 Unit Characteristics: 0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Command_Set_Revision:
		printf("SBP2 Command Set Revision: 0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Command_Set_Spec_Id:
		printf("SBP2 Command Set Spec Id: 0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Firmware_Revision:
		printf("SBP2 Firmware Revision: 0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Reconnect_Timeout:
		printf("SBP2 Reconnect Timeout: 0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Unit_Unique_Id:
		printf("SBP2 Unit Unique Id: 0x%08x\n", data->val);
		break;
	case P1212_KEYVALUE_Unit_Dependent_Info:
		if (data->key_type == P1212_KEYTYPE_Immediate) 
			printf("SBP2 Logical Unit Number: 0x%08x\n", data->val);
		else if (data->key_type == P1212_KEYTYPE_Offset) 
			printf("SBP2 Management Agent: 0x%08x\n", data->val);
		break;
	default:
		return 0;
	}
	return 1;
}

static int
sbp2_print_dir(u_int8_t dir_type)
{
	switch(dir_type) {
	case SBP2_KEYVALUE_Logical_Unit_Directory:
		printf("Logical Unit ");
		break;
	default:
		return 0;
	}
	return 1;
}

static void
sbp2_init(struct fwnode_softc *sc, struct fwnode_device_cap *devcap)
{
	struct ieee1394_abuf *ab, *ab2;
	u_int32_t loc = ((u_int32_t *)devcap->dev_data)[0];
	
	devcap->dev_valid = 0;
	ab = malloc(sizeof(struct ieee1394_abuf), M_1394DATA, M_WAITOK);
	memset(ab, 0, sizeof(struct ieee1394_abuf));
	ab->ab_data = malloc(8, M_1394DATA, M_WAITOK);
	memset(ab->ab_data, 0, 8);
	ab2 = malloc(sizeof(struct ieee1394_abuf), M_1394DATA, M_WAITOK);
	memset(ab2, 0, sizeof(struct ieee1394_abuf));
	
	loc *= 4;
	ab->ab_length = 8;
	ab->ab_req = (struct ieee1394_softc *)sc;
	ab->ab_retlen = 0;
	ab->ab_cb = NULL;
	ab->ab_cbarg = NULL;
	ab->ab_csr = CSR_BASE + loc;
	ab->ab_data[0] = htonl((u_int32_t)(SBP2_LOGIN_ORB >> 32));
	ab->ab_data[1] = (u_int32_t)(SBP2_LOGIN_ORB & 0xffffffff);
	ab->ab_tcode = IEEE1394_TCODE_WRITE_REQ_BLOCK;
	
	ab2->ab_length = 32;
	ab2->ab_tcode = IEEE1394_TCODE_READ_REQ_BLOCK;
	ab2->ab_retlen = 0;
	ab2->ab_data = NULL;
	ab2->ab_csr = SBP2_LOGIN_ORB;
	ab2->ab_cb = sbp2_login;
	ab2->ab_cbarg = devcap;
	ab2->ab_req = (struct ieee1394_softc *)sc;
	
	sc->sc1394_inreg(ab2, FALSE);
	sc->sc1394_write(ab);
	return;
}

static void
sbp2_login(struct ieee1394_abuf *ab, int rcode)
{
	struct fwnode_softc *sc = (struct fwnode_softc *)ab->ab_req;
	/*    struct fwnode_device_cap *devcap = ab->ab_cbarg;*/
	struct ieee1394_abuf *statab, *respab;
	
	/* Got a read so allocate the buffer and write out the response. */
	
	if (rcode) {
#ifdef FW_DEBUG
		printf ("sbp2_login: Bad return code: %d\n", rcode);
#endif
		if (ab->ab_data)
			free (ab->ab_data, M_1394DATA);
		free(ab, M_1394DATA);
	}
	
	sc->sc1394_unreg(ab, FALSE);

	ab->ab_data = malloc(32, M_1394DATA, M_WAITOK);
	respab = malloc(sizeof(struct ieee1394_abuf), M_1394DATA, M_WAITOK);
	statab = malloc(sizeof(struct ieee1394_abuf), M_1394DATA, M_WAITOK);
	memset(respab, 0, sizeof(struct ieee1394_abuf));
	memset(statab, 0, sizeof(struct ieee1394_abuf));
	
	statab->ab_length = 32;
	statab->ab_tcode = IEEE1394_TCODE_WRITE_REQ_BLOCK;
	statab->ab_retlen = 0;
	statab->ab_data = NULL;
	statab->ab_csr = SBP2_LOGIN_STATUS;
	statab->ab_cb = sbp2_login_resp;
	statab->ab_cbarg = ab->ab_cbarg;
	statab->ab_req = ab->ab_req;
	
	sc->sc1394_inreg(statab, TRUE);
	
	respab->ab_length = 16;
	respab->ab_tcode = IEEE1394_TCODE_WRITE_REQ_BLOCK;
	respab->ab_retlen = 0;
	respab->ab_data = NULL;
	respab->ab_csr = SBP2_LOGIN_RESP;
	respab->ab_cb = sbp2_login_resp;
	respab->ab_cbarg = ab->ab_cbarg;
	respab->ab_req = ab->ab_req;
	
	sc->sc1394_inreg(respab, TRUE);
	
	memset(ab->ab_data, 0, 32);

	/* Fill in a login packet. First 2 quads are 0 for password. */

	/* Addr for response. */
	ab->ab_data[2] = htonl(SBP2_LOGIN_RESP >> 32);
	ab->ab_data[3] = htonl(SBP2_LOGIN_RESP & 0xffffffff);

	/* Set notify and exclusive use bits. Login to lun 0 (XXX) */
	ab->ab_data[4] = htonl(0x90000000);

	/* Password length (0) and login response length (16) */
	ab->ab_data[5] = htonl(0x00000010);

	/* Addr for status packet. */
	ab->ab_data[6] = htonl(SBP2_LOGIN_STATUS >> 32);
	ab->ab_data[7] = htonl(SBP2_LOGIN_STATUS & 0xffffffff);

	ab->ab_retlen = 0;
	ab->ab_cb = NULL;
	ab->ab_cbarg = NULL;
	ab->ab_tcode = IEEE1394_TCODE_READ_RESP_BLOCK;
	ab->ab_length = 32;
	
	sc->sc1394_write(ab);
}

static void
sbp2_login_resp(struct ieee1394_abuf *ab, int rcode)
{
	struct fwnode_softc *sc = (struct fwnode_softc *)ab->ab_req;
	struct fwnode_device_cap *devcap = ab->ab_cbarg;
	u_int64_t csr;
	int resp, src, status, len, dead;
#ifdef FW_DEBUG
	int i;
#endif
	
	if (rcode) {
		DPRINTF(("Bad return code: %d\n", rcode));
		if (ab->ab_data)
			free(ab->ab_data, M_1394DATA);
		free(ab, M_1394DATA);
	}
	
	DPRINTF(("csr: 0x%016qx\n", (quad_t)ab->ab_csr));
	for (i = 0; i < (ab->ab_retlen / 4); i++) 
		DPRINTF(("%d: 0x%08x\n", i, ntohl(ab->ab_data[i])));
	
	if (ab->ab_csr == SBP2_LOGIN_RESP) {
		devcap->dev_valid |= 0x01;
		devcap->dev_spec = ntohl(ab->ab_data[0]) & 0xffff;
		devcap->dev_cmdptr =
		    (((u_int64_t)(ntohl(ab->ab_data[1]) & 0xfff)) << 32) +
		    ntohl(ab->ab_data[2]);
	}
	if (ab->ab_csr == SBP2_LOGIN_STATUS) {
		status = ntohl(ab->ab_data[0]);
		src = (status >> 30) & 0x3;
		resp = (status >> 28) & 0x3;
		dead = (status >> 27) & 0x1;
		len = (status >> 24) & 0x7;
		csr = ((u_int64_t)(status & 0xffff) << 32) |
		    ntohl(ab->ab_data[1]);
		status = (status >> 16) & 0xff;
		DPRINTF(("status -- src: %d, resp: %d, dead: %d, len: %d, "
		    "status: %d\nstatus -- csr: 0x%016qx\n", src, resp, dead,
		    (len + 1) * 4, status, (quad_t)csr));
		if ((src == 0) && (resp == 0) && (dead == 0) && (status == 0) &&
		    (csr == SBP2_LOGIN_ORB)) {
			devcap->dev_valid |= 0x02;
			DPRINTF(("Got a valid status\n"));
		}
	}
	if (devcap->dev_valid == 0x3) {
		devcap->name = "fwscsi";
		sc->sc_child = config_found(&sc->sc_sc1394.sc1394_dev, devcap,
		    fwnode_print);
	}

	/*
	 * Leave the handler for status since unsolicited status will get sent
	 * to the addr specified in the login packet.
	 */
	
	if (ab->ab_csr == SBP2_LOGIN_RESP) {
		sc->sc1394_unreg(ab, TRUE);
		free(ab->ab_data, M_1394DATA);
		free(ab, M_1394DATA);
	}
	
}
