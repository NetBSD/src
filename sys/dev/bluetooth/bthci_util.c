/*	$NetBSD: bthci_util.c,v 1.1.2.2 2003/01/15 18:44:15 thorpej Exp $	*/

/*
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Sainty <David.Sainty@dtsp.co.nz>
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bthci_util.c,v 1.1.2.2 2003/01/15 18:44:15 thorpej Exp $");

#include <dev/bluetooth/bt_hci.h>
#include <dev/bluetooth/bthci_util.h>

static char const * const hci_eventnames[] = {
	"NULL",
	"INQUIRY_COMPL",
	"INQUIRY_RESULT",
	"CON_COMPL",
	"CON_REQ",
	"DISCON_COMPL",
	"AUTH_COMPL",
	"REMOTE_NAME_REQ_COMPL",
	"ENCRYPTION_CHANGE",
	"CHANGE_CON_LINK_KEY_COMPL",
	"MASTER_LINK_KEY_COMPL",
	"READ_REMOTE_FEATURES_COMPL",
	"READ_REMOTE_VER_INFO_COMPL",
	"QOS_SETUP_COMPL",
	"COMMAND_COMPL",
	"COMMAND_STATUS",
	"HARDWARE_ERROR",
	"FLUSH_OCCUR",
	"ROLE_CHANGE",
	"NUM_COMPL_PKTS",
	"MODE_CHANGE",
	"RETURN_LINK_KEYS",
	"PIN_CODE_REQ",
	"LINK_KEY_REQ",
	"LINK_KEY_NOTIFICATION",
	"LOOPBACK_COMMAND",
	"DATA_BUFFER_OVERFLOW",
	"MAX_SLOT_CHANGE",
	"READ_CLOCK_OFFSET_COMPL",
	"CON_PKT_TYPE_CHANGED",
	"QOS_VIOLATION",
	"PAGE_SCAN_MODE_CHANGE",
	"PAGE_SCAN_REP_MODE_CHANGE"
};

char const*
bthci_eventstr(unsigned int evtcode)
{
	if (evtcode < (sizeof(hci_eventnames) / sizeof(*hci_eventnames)))
		return hci_eventnames[evtcode];

	if (evtcode == BT_HCI_EVENT_BT_LOGO)
		return "BT_LOGO";

	if (evtcode == BT_HCI_EVENT_VENDOR)
		return "VENDOR";

	return "UNRECOGNISED";
}
