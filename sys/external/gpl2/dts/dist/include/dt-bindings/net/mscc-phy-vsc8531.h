/*	$NetBSD: mscc-phy-vsc8531.h,v 1.1.1.1.6.2 2017/08/28 17:53:03 skrll Exp $	*/

/*
 * Device Tree constants for Microsemi VSC8531 PHY
 *
 * Author: Nagaraju Lakkaraju
 *
 * License: Dual MIT/GPL
 * Copyright (c) 2017 Microsemi Corporation
 */

#ifndef _DT_BINDINGS_MSCC_VSC8531_H
#define _DT_BINDINGS_MSCC_VSC8531_H

/* PHY LED Modes */
#define VSC8531_LINK_ACTIVITY           0
#define VSC8531_LINK_1000_ACTIVITY      1
#define VSC8531_LINK_100_ACTIVITY       2
#define VSC8531_LINK_10_ACTIVITY        3
#define VSC8531_LINK_100_1000_ACTIVITY  4
#define VSC8531_LINK_10_1000_ACTIVITY   5
#define VSC8531_LINK_10_100_ACTIVITY    6
#define VSC8531_DUPLEX_COLLISION        8
#define VSC8531_COLLISION               9
#define VSC8531_ACTIVITY                10
#define VSC8531_AUTONEG_FAULT           12
#define VSC8531_SERIAL_MODE             13
#define VSC8531_FORCE_LED_OFF           14
#define VSC8531_FORCE_LED_ON            15

#endif
