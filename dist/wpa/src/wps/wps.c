/*
 * Wi-Fi Protected Setup
 * Copyright (c) 2007, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include "includes.h"

#include "common.h"
#include "wps.h"


struct wps_data {
	int authenticator;
	int registrar;
	int msg_num;
};


struct wps_data * wps_init(int authenticator, int registrar)
{
	struct wps_data *data = os_zalloc(sizeof(*data));
	if (data == NULL)
		return NULL;
	data->authenticator = authenticator;
	data->registrar = registrar;
	data->msg_num = 1;
	return data;
}


void wps_deinit(struct wps_data *data)
{
	os_free(data);
}


enum wps_process_res wps_process_msg(struct wps_data *wps, u8 op_code,
				     const struct wpabuf *msg)
{
	/* TODO: proper processing and/or sending to an external process */

	wpa_hexdump_buf(MSG_MSGDUMP, "WPS: Received message", msg);
	if ((wps->registrar && (wps->msg_num & 1) == 0) ||
	    (!wps->registrar && (wps->msg_num & 1) == 1)) {
		wpa_printf(MSG_DEBUG, "WPS: Unexpected message number %d",
			   wps->msg_num);
		return WPS_FAILURE;
	}

	if (wps->msg_num <= 8 && op_code == WSC_MSG) {
		wpa_printf(MSG_DEBUG, "WPS: Process M%d", wps->msg_num);
	} else if (wps->registrar && wps->msg_num == 3 &&
		   op_code == WSC_ACK) {
		wpa_printf(MSG_DEBUG, "WPS: Process ACK to M2/M2D");
		/* could send out next M2/M2D */
		return WPS_DONE;
	} else if (wps->registrar && wps->msg_num == 3 &&
		   op_code == WSC_Done) {
		wpa_printf(MSG_DEBUG, "WPS: Process Done to M2/M2D");
		return WPS_DONE;
	} else if (wps->msg_num <= 8 && op_code == WSC_Done) {
		wpa_printf(MSG_DEBUG, "WPS: Process Done prior to completion");
		return WPS_DONE;
	} else if (wps->msg_num <= 8 && op_code == WSC_ACK) {
		wpa_printf(MSG_DEBUG, "WPS: Process ACK prior to completion");
		return WPS_DONE;
	} else if (wps->msg_num <= 8 && op_code == WSC_NACK) {
		wpa_printf(MSG_DEBUG, "WPS: Process NACK prior to completion");
		return WPS_DONE;
	} else if (wps->registrar && wps->msg_num == 9 &&
		   op_code == WSC_Done) {
		wpa_printf(MSG_DEBUG, "WPS: Process Done");
		if (wps->authenticator)
			return WPS_DONE;
	} else if (!wps->registrar && wps->msg_num == 10 &&
		   op_code == WSC_ACK) {
		wpa_printf(MSG_DEBUG, "WPS: Process ACK");
		return WPS_DONE;
	} else {
		wpa_printf(MSG_DEBUG, "WPS: Unexpected Op-Code %d "
			   "(msg_num=%d)", op_code, wps->msg_num);
		return WPS_FAILURE;
	}

	wps->msg_num++;
	return WPS_CONTINUE;
}


struct wpabuf * wps_get_msg(struct wps_data *wps, u8 *op_code)
{
	struct wpabuf *msg;

	/* TODO: proper processing and/or query from an external process */

	if ((wps->registrar && (wps->msg_num & 1) == 1) ||
	    (!wps->registrar && (wps->msg_num & 1) == 0)) {
		wpa_printf(MSG_DEBUG, "WPS: Unexpected request for message "
			   "number %d", wps->msg_num);
		return NULL;
	}

	if (wps->msg_num == 7 || wps->msg_num == 8) {
		msg = wpabuf_alloc(2000);
		if (msg == NULL)
			return NULL;
		*op_code = WSC_MSG;
		wpabuf_put_u8(msg, WSC_MSG);
		wpabuf_put(msg, 1999);
		wpa_printf(MSG_DEBUG, "WPS: Send M%d", wps->msg_num);
	} else if (wps->msg_num <= 6) {
		msg = wpabuf_alloc(1);
		if (msg == NULL)
			return NULL;
		*op_code = WSC_MSG;
		wpabuf_put_u8(msg, WSC_MSG);
		wpa_printf(MSG_DEBUG, "WPS: Send M%d", wps->msg_num);
	} else if (!wps->registrar && wps->msg_num == 9) {
		msg = wpabuf_alloc(1);
		if (msg == NULL)
			return NULL;
		*op_code = WSC_Done;
		wpabuf_put_u8(msg, WSC_Done);
		wpa_printf(MSG_DEBUG, "WPS: Send Done");
	} else if (wps->registrar && wps->msg_num == 10) {
		msg = wpabuf_alloc(1);
		if (msg == NULL)
			return NULL;
		*op_code = WSC_ACK;
		wpabuf_put_u8(msg, WSC_ACK);
		wpa_printf(MSG_DEBUG, "WPS: Send ACK");
	} else
		return NULL;

	wps->msg_num++;
	return msg;
}
