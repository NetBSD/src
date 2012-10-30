/*
 * WPA Supplicant - Glue code to setup EAPOL and RSN modules
 * Copyright (c) 2003-2008, Jouni Malinen <j@w1.fi>
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

#ifndef WPAS_GLUE_H
#define WPAS_GLUE_H

enum wpa_ctrl_req_type;

int wpa_supplicant_init_eapol(struct wpa_supplicant *wpa_s);
int wpa_supplicant_init_wpa(struct wpa_supplicant *wpa_s);
void wpa_supplicant_rsn_supp_set_config(struct wpa_supplicant *wpa_s,
					struct wpa_ssid *ssid);

const char * wpa_supplicant_ctrl_req_to_string(enum wpa_ctrl_req_type field,
					       const char *default_txt,
					       const char **txt);

enum wpa_ctrl_req_type wpa_supplicant_ctrl_req_from_string(const char *field);

#endif /* WPAS_GLUE_H */
