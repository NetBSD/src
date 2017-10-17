/* $NetBSD: vmtreg.h,v 1.2 2017/10/17 06:50:00 maya Exp $ */
/* NetBSD: vmt.c,v 1.15 2016/11/10 03:32:04 ozaki-r Exp */
/* $OpenBSD: vmt.c,v 1.11 2011/01/27 21:29:25 dtucker Exp $ */

/*
 * Copyright (c) 2007 David Crawshaw <david@zentus.com>
 * Copyright (c) 2008 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* OS name to report to host */
#ifdef __i386__
#define VM_OS_NAME	"other"
#else
#define VM_OS_NAME	"other-64"
#endif

/* "The" magic number, always occupies the EAX register. */
#define VM_MAGIC			0x564D5868

/* Port numbers, passed on EDX.LOW . */
#define VM_PORT_CMD			0x5658
#define VM_PORT_RPC			0x5659

/* Commands, passed on ECX.LOW. */
#define VM_CMD_GET_SPEED		0x01
#define VM_CMD_APM			0x02
#define VM_CMD_GET_MOUSEPOS		0x04
#define VM_CMD_SET_MOUSEPOS		0x05
#define VM_CMD_GET_CLIPBOARD_LEN	0x06
#define VM_CMD_GET_CLIPBOARD		0x07
#define VM_CMD_SET_CLIPBOARD_LEN	0x08
#define VM_CMD_SET_CLIPBOARD		0x09
#define VM_CMD_GET_VERSION		0x0a
#define  VM_VERSION_UNMANAGED			0x7fffffff
#define VM_CMD_GET_DEVINFO		0x0b
#define VM_CMD_DEV_ADDREMOVE		0x0c
#define VM_CMD_GET_GUI_OPTIONS		0x0d
#define VM_CMD_SET_GUI_OPTIONS		0x0e
#define VM_CMD_GET_SCREEN_SIZE		0x0f
#define VM_CMD_GET_HWVER		0x11
#define VM_CMD_POPUP_OSNOTFOUND		0x12
#define VM_CMD_GET_BIOS_UUID		0x13
#define VM_CMD_GET_MEM_SIZE		0x14
/*#define VM_CMD_GET_TIME		0x17 */	/* deprecated */
#define VM_CMD_RPC			0x1e
#define VM_CMD_GET_HZ			0x2d
#define VM_CMD_GET_TIME_FULL		0x2e
#define VM_CMD_GET_VCPU_INFO		0x44
#define  VCPUINFO_LEGACY_X2APIC			__BIT(3)
#define  VCPUINFO_VCPU_RESERVED			__BIT(31)

/* RPC sub-commands, passed on ECX.HIGH. */
#define VM_RPC_OPEN			0x00
#define VM_RPC_SET_LENGTH		0x01
#define VM_RPC_SET_DATA			0x02
#define VM_RPC_GET_LENGTH		0x03
#define VM_RPC_GET_DATA			0x04
#define VM_RPC_GET_END			0x05
#define VM_RPC_CLOSE			0x06

/* RPC magic numbers, passed on EBX. */
#define VM_RPC_OPEN_RPCI	0x49435052UL /* with VM_RPC_OPEN. */
#define VM_RPC_OPEN_TCLO	0x4F4C4354UL /* with VP_RPC_OPEN. */
#define VM_RPC_ENH_DATA		0x00010000UL /* with enhanced RPC data calls. */

#define VM_RPC_FLAG_COOKIE	0x80000000UL

/* RPC reply flags */
#define VM_RPC_REPLY_SUCCESS	0x0001
#define VM_RPC_REPLY_DORECV	0x0002		/* incoming message available */
#define VM_RPC_REPLY_CLOSED	0x0004		/* RPC channel is closed */
#define VM_RPC_REPLY_UNSENT	0x0008		/* incoming message was removed? */
#define VM_RPC_REPLY_CHECKPOINT	0x0010		/* checkpoint occurred -> retry */
#define VM_RPC_REPLY_POWEROFF	0x0020		/* underlying device is powering off */
#define VM_RPC_REPLY_TIMEOUT	0x0040
#define VM_RPC_REPLY_HB		0x0080		/* high-bandwidth tx/rx available */

/* VM state change IDs */
#define VM_STATE_CHANGE_HALT	1
#define VM_STATE_CHANGE_REBOOT	2
#define VM_STATE_CHANGE_POWERON 3
#define VM_STATE_CHANGE_RESUME  4
#define VM_STATE_CHANGE_SUSPEND 5

/* VM guest info keys */
#define VM_GUEST_INFO_DNS_NAME		1
#define VM_GUEST_INFO_IP_ADDRESS	2
#define VM_GUEST_INFO_DISK_FREE_SPACE	3
#define VM_GUEST_INFO_BUILD_NUMBER	4
#define VM_GUEST_INFO_OS_NAME_FULL	5
#define VM_GUEST_INFO_OS_NAME		6
#define VM_GUEST_INFO_UPTIME		7
#define VM_GUEST_INFO_MEMORY		8
#define VM_GUEST_INFO_IP_ADDRESS_V2	9

/* RPC responses */
#define VM_RPC_REPLY_OK			"OK "
#define VM_RPC_RESET_REPLY		"OK ATR toolbox"
#define VM_RPC_REPLY_ERROR		"ERROR Unknown command"
#define VM_RPC_REPLY_ERROR_IP_ADDR	"ERROR Unable to find guest IP address"
