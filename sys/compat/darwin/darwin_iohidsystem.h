/*	$NetBSD: darwin_iohidsystem.h,v 1.7 2003/09/11 23:16:19 manu Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_DARWIN_IOHIDSYSTEM_H_
#define	_DARWIN_IOHIDSYSTEM_H_

extern struct mach_iokit_devclass darwin_iohidsystem_devclass;

/* Events and event queue */
#define DARWIN_NX_NULLEVENT	0
#define DARWIN_NX_LMOUSEDOWN	1
#define DARWIN_NX_LMOUSEUP	2
#define DARWIN_NX_RMOUSEDOWN	3
#define DARWIN_NX_RMOUSEUP	4
#define DARWIN_NX_MOUSEMOVED	5
#define DARWIN_NX_LMOUSEDRAGGED	6
#define DARWIN_NX_RMOUSEDRAGGED	7
#define DARWIN_NX_MOUSEENTERED	8
#define DARWIN_NX_MOUSEEXITED	9
#define DARWIN_NX_KEYDOWN	10
#define DARWIN_NX_KEYUP		11
#define DARWIN_NX_FLAGSCHANGED	12

#define DARWIN_NX_LMOUSEDOWN_MASK	(1 << DARWIN_NX_LMOUSEDOWN)
#define DARWIN_NX_LMOUSEUP_MASK		(1 << DARWIN_NX_LMOUSEUP)
#define DARWIN_NX_RMOUSEDOWN_MASK	(1 << DARWIN_NX_RMOUSEDOWN)
#define DARWIN_NX_RMOUSEUP_MASK		(1 << DARWIN_NX_RMOUSEUP)
#define DARWIN_NX_MOUSEMOVED_MASK	(1 << DARWIN_NX_MOUSEMOVED)
#define DARWIN_NX_LMOUSEDRAGGED_MASK	(1 << DARWIN_NX_LMOUSEDRAGGED)
#define DARWIN_NX_RMOUSEDRAGGED_MASK	(1 << DARWIN_NX_RMOUSEDRAGGED)
#define DARWIN_NX_MOUSEENTERED_MASK	(1 << DARWIN_NX_MOUSEENTERED)
#define DARWIN_NX_MOUSEEXITED_MASK	(1 << DARWIN_NX_MOUSEEXITED)
#define DARWIN_NX_KEYDOWN_MASK		(1 << DARWIN_NX_KEYDOWN)
#define DARWIN_NX_KEYUP_MASK		(1 << DARWIN_NX_KEYUP)
#define DARWIN_NX_FLAGSCHANGED_MASK	(1 << DARWIN_NX_FLAGSCHANGED)

typedef struct {
	uint16_t tabletid;
	uint16_t pointerid;
	uint16_t deviceid;
	uint16_t system_tabletid;
	uint16_t vendor_pointertype;
	uint32_t pointer_serialnum;
	uint64_t uniqueid;
	uint32_t cap_mask;
	uint8_t ptrtype;
	uint8_t enter_proximity;
	int16_t reserved1;
} darwin_iohidsystem_tabletproxymity;

typedef struct {
	int32_t x;
	int32_t y;
	int32_t z;
	uint16_t buttons;
	uint16_t pressure;
	struct {
		int16_t x;
		int16_t y;
	} tilt;
	uint16_t rotation;
	uint16_t tanpressure;
	uint16_t devid;
	uint16_t vendor1;
	uint16_t vendor2;
	uint16_t vendor3;
} darwin_iohidsystem_tabletpoint;

typedef union {
	struct {
		uint8_t subx;
		uint8_t suby;
		int16_t buttonid;
		int32_t click;
		uint8_t pressure;
		uint8_t reserved1;
		uint8_t subtype;
		uint8_t reserved2;
		uint32_t reserved3;
		union {
			darwin_iohidsystem_tabletpoint point;	
			darwin_iohidsystem_tabletproxymity proximity;
		} tablet;
	} mouse;
	struct {
		int32_t dx;
		int32_t dy;
		uint8_t subx;
		uint8_t suby;
		uint8_t subtype;
		uint8_t reserved1;
		int32_t reserved2;
		union {
			darwin_iohidsystem_tabletpoint point;	
			darwin_iohidsystem_tabletproxymity proximity;
		} tablet;
	} mouse_move;
	struct {
		uint16_t origi_charset;
		int16_t repeat;
		uint16_t charset;
		uint16_t charcode;
		uint16_t keycode;
		uint16_t orig_charcode;
		int32_t reserved1;
		int32_t keyboardtype;
		int32_t reserved2[7];
	} key;
	struct {
		int16_t reserved;
		int16_t eventnum;
		int32_t trackingnum;
		int32_t userdata;
		int32_t reserved5[9];
	} tracking;
	struct {
		int16_t da1;
		int16_t da2;
		int16_t da3;
		int16_t reserved1;
		int32_t reserved2[10];
	} scrollwheel;
	struct {
		int16_t reserved;
		int16_t subtype;
		union {
			float	F[11];
			long	L[11];
			short	S[22];
			char	C[44];
		} misc;
	} compound;
	struct {
		int32_t x;
		int32_t y;
		int32_t z;
		uint16_t buttons;
		uint16_t pressure;
		struct {
			int16_t x;
			int16_t y;
		} tilt;
		uint16_t rotation;
		int16_t tanpressure;
		uint16_t devid;
		int16_t vendor1;
		int16_t vendor2;
		int16_t vendor3;
		int32_t reserved[4];
	} tablet;
	struct {
		uint16_t vendorid;
		uint16_t tabletid;
		uint16_t pointerid;
		uint16_t deviceid;
		uint16_t systemtabletid;
		uint16_t vendor_pointertype;
		uint32_t pointer_serialnum;
		uint64_t uniqueid;
		uint32_t cap_mask;
		uint8_t ptr_type;
		uint8_t enter_proximity;
		int16_t reserved[9];
	} proximity;
} darwin_iohidsystem_event_data;

typedef volatile struct {
	int die_type;
	int die_location_x;
	int die_location_y;
	unsigned long die_time_hi;
	unsigned long die_time_lo;
	int die_flags;
	unsigned int die_window;
	darwin_iohidsystem_event_data die_data;
} darwin_iohidsystem_event;

typedef struct {
	int diei_next;
	darwin_ev_lock_data_t diei_sem;
	/* 
	 * Avoid automatic alignement, this should be 
	 * darwin_iohidsystem_event, we declare it as a char array.
	 */
	unsigned char diei_event[76];
} darwin_iohidsystem_event_item;

#define DARWIN_IOHIDSYSTEM_EVENTQUEUE_LEN 240

struct darwin_iohidsystem_evglobals {
	darwin_ev_lock_data_t evg_cursor_sem;
	int evg_event_head;
	int evg_event_tail;
	int evg_event_last;
	int evg_uniq_mouseid;
	int evg_buttons;
	int evg_event_flags;
	int evg_event_time;
	darwin_iogpoint evg_cursor_loc;
	int evg_cursor_frame;
	darwin_iogbounds evg_all_screen;
	darwin_iogbounds evg_mouse_rect;
	int evg_version;
	int evg_struct_size;
	int evg_last_frame;
	unsigned int evg_reserved1[31];
	unsigned evg_reserved2:27;
	unsigned evg_want_pressure:1;
	unsigned evg_want_precision:1;
	unsigned evg_dontwant_coalesce:1;
	unsigned evg_dont_coalesce:1;
	unsigned evg_mouserect_valid:1;
	int evg_moved_mask;
	int evg_lastevent_sent;
	int evg_lastevent_consumed;
	darwin_ev_lock_data_t evg_waitcursor_sem;
	int evg_waitcursor;
	char evg_waitcursor_timeout;
	char evg_waitcursor_enabled;
	char evg_globalwaitcursor_enabled;
	int evg_waitcursor_threshold;
	darwin_iohidsystem_event_item 
	    evg_evqueue[DARWIN_IOHIDSYSTEM_EVENTQUEUE_LEN];
};

/* Shared memory between the IOHIDSystem driver and userland */
struct  darwin_iohidsystem_shmem {
	int dis_global_offset;	/* Offset to global zone, usually 8 */
	int dis_private_offset;	/* Offset to private memory. Don't care. */
	struct darwin_iohidsystem_evglobals dis_evglobals; 
};

/* I/O selectors for io_connect_method_{scalar|struct}i_{scalar|struct}o */
#define DARWIN_IOHIDCREATESHMEM 0
#define DARWIN_IOHIDSETEVENTSENABLE 1
#define DARWIN_IOHIDSETCURSORENABLE 2

int darwin_iohidsystem_connect_method_scalari_scalaro(struct mach_trap_args *);
int darwin_iohidsystem_connect_map_memory(struct mach_trap_args *);

/* I/O notifications: XXX not checked on Darwin */

typedef struct {
	mach_msg_header_t req_msgh;
	mach_msg_trailer_t req_trailer;
} mach_notify_iohidsystem_request_t;

#endif /* _DARWIN_IOHIDSYSTEM_H_ */
