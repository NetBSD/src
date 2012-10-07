/*
 * wpa_supplicant - Off-channel Action frame TX/RX
 * Copyright (c) 2009-2010, Atheros Communications
 * Copyright (c) 2011, Qualcomm Atheros
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

#ifndef OFFCHANNEL_H
#define OFFCHANNEL_H

int offchannel_send_action(struct wpa_supplicant *wpa_s, unsigned int freq,
			   const u8 *dst, const u8 *src, const u8 *bssid,
			   const u8 *buf, size_t len, unsigned int wait_time,
			   void (*tx_cb)(struct wpa_supplicant *wpa_s,
					 unsigned int freq, const u8 *dst,
					 const u8 *src, const u8 *bssid,
					 const u8 *data, size_t data_len,
					 enum offchannel_send_action_result
					 result),
			   int no_cck);
void offchannel_send_action_done(struct wpa_supplicant *wpa_s);
void offchannel_remain_on_channel_cb(struct wpa_supplicant *wpa_s,
				     unsigned int freq, unsigned int duration);
void offchannel_cancel_remain_on_channel_cb(struct wpa_supplicant *wpa_s,
					    unsigned int freq);
void offchannel_deinit(struct wpa_supplicant *wpa_s);
void offchannel_send_action_tx_status(
	struct wpa_supplicant *wpa_s, const u8 *dst, const u8 *data,
	size_t data_len, enum offchannel_send_action_result result);

#endif /* OFFCHANNEL_H */
