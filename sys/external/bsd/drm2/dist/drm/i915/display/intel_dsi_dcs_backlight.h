/*	$NetBSD: intel_dsi_dcs_backlight.h,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __INTEL_DSI_DCS_BACKLIGHT_H__
#define __INTEL_DSI_DCS_BACKLIGHT_H__

struct intel_connector;

int intel_dsi_dcs_init_backlight_funcs(struct intel_connector *intel_connector);

#endif /* __INTEL_DSI_DCS_BACKLIGHT_H__ */
