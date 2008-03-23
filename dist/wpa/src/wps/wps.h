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

#ifndef WPS_H
#define WPS_H

enum wsc_op_code {
	WSC_Start = 0x01,
	WSC_ACK = 0x02,
	WSC_NACK = 0x03,
	WSC_MSG = 0x04,
	WSC_Done = 0x05,
	WSC_FRAG_ACK = 0x06
};


struct wps_data * wps_init(int authenticator, int registrar);

void wps_deinit(struct wps_data *data);

enum wps_process_res {
	WPS_DONE, WPS_CONTINUE, WPS_FAILURE, WPS_PENDING
};
enum wps_process_res wps_process_msg(struct wps_data *wps, u8 op_code,
				     const struct wpabuf *msg);

struct wpabuf * wps_get_msg(struct wps_data *wps, u8 *op_code);


#endif /* WPS_H */
