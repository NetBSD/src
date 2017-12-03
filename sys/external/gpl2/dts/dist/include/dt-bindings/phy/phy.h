/*	$NetBSD: phy.h,v 1.1.1.2.2.2 2017/12/03 11:38:39 jdolecek Exp $	*/

/*
 *
 * This header provides constants for the phy framework
 *
 * Copyright (C) 2014 STMicroelectronics
 * Author: Gabriel Fernandez <gabriel.fernandez@st.com>
 * License terms:  GNU General Public License (GPL), version 2
 */

#ifndef _DT_BINDINGS_PHY
#define _DT_BINDINGS_PHY

#define PHY_NONE		0
#define PHY_TYPE_SATA		1
#define PHY_TYPE_PCIE		2
#define PHY_TYPE_USB2		3
#define PHY_TYPE_USB3		4
#define PHY_TYPE_UFS		5

#endif /* _DT_BINDINGS_PHY */
