;	$NetBSD: siop.ss,v 1.11 2000/10/19 07:20:16 bouyer Exp $

;
;  Copyright (c) 2000 Manuel Bouyer.
; 
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions
;  are met:
;  1. Redistributions of source code must retain the above copyright
;     notice, this list of conditions and the following disclaimer.
;  2. Redistributions in binary form must reproduce the above copyright
;     notice, this list of conditions and the following disclaimer in the
;     documentation and/or other materials provided with the distribution.
;  3. All advertising materials mentioning features or use of this software
;     must display the following acknowledgement:
; 	This product includes software developed by Manuel Bouyer
;  4. The name of the author may not be used to endorse or promote products
;     derived from this software without specific prior written permission.
; 
;  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
;  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
;  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
;  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,     
;  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
;  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
;  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

ARCH 720

; offsets in sym_xfer
ABSOLUTE t_id = 24;
ABSOLUTE t_msg_in = 32;
ABSOLUTE t_ext_msg_in = 40;
ABSOLUTE t_ext_msg_data = 48;
ABSOLUTE t_msg_tag = 56;
ABSOLUTE t_msg_out = 64;
ABSOLUTE t_cmd = 72;
ABSOLUTE t_status = 80;
ABSOLUTE t_data = 88;

;; interrupt codes
; interrupts that need a valid DSA
ABSOLUTE int_done	= 0xff00;
ABSOLUTE int_msgin	= 0xff01;
ABSOLUTE int_extmsgin	= 0xff02;
ABSOLUTE int_extmsgdata	= 0xff03;
ABSOLUTE int_disc	= 0xff04;
; interrupts that don't have a valid DSA
ABSOLUTE int_reseltarg	= 0xff80;
ABSOLUTE int_resellun	= 0xff81;
ABSOLUTE int_reseltag	= 0xff82;
ABSOLUTE int_resfail	= 0xff83;
ABSOLUTE int_err 	= 0xffff;

; flags for scratcha0
ABSOLUTE flag_sdp 	= 0x01 ; got save data pointer
ABSOLUTE flag_data 	= 0x02 ; we're in data phase
ABSOLUTE flag_data_mask	= 0xfd ; ~flag_data

; main script symbols

ENTRY waitphase;
ENTRY send_msgout;
ENTRY msgout;
ENTRY msgin;
ENTRY handle_msgin;
ENTRY msgin_ack;
ENTRY dataout;
ENTRY datain;
ENTRY cmdout;
ENTRY status;
ENTRY disconnect;
ENTRY reselect;
ENTRY reselected;
ENTRY selected;
ENTRY script_sched;
ENTRY get_extmsgdata;
ENTRY resel_targ0;
ENTRY msgin_space;
ENTRY lunsw_return;
EXTERN abs_targ0;
EXTERN abs_msgin;

; lun switch symbols
ENTRY lun_switch_entry;
ENTRY resel_lun0;
ENTRY restore_scntl3;
EXTERN abs_lun0;
EXTERN abs_lunsw_return;

; command reselect script symbols
ENTRY rdsa0;
ENTRY rdsa1;
ENTRY rdsa2;
ENTRY rdsa3;
ENTRY reload_dsa;

EXTERN resel_abs_reselected;

; command scheduler symbols
ENTRY slot;
ENTRY slotdata;
ENTRY nextslot;

EXTERN script_abs_sched;
EXTERN slot_nextp;
EXTERN slot_sched_addrsrc;
EXTERN slot_abs_reselect;
EXTERN slot_abs_selected;
EXTERN slot_abs_loaddsa;

EXTERN endslot_abs_reselect;

; main script

PROC  siop_script:

reselected:
; starting a new session, init 'local variables'
	MOVE 0 to SCRATCHA0	; flags
	MOVE 0 to SCRATCHA1	; DSA offset (for S/G save data pointer)
	MOVE SCRATCHA3 to SFBR  ; pending message ?
	JUMP REL(handle_msgin), IF not 0x08;
waitphase:
	JUMP REL(msgout), WHEN MSG_OUT;
	JUMP REL(msgin), WHEN MSG_IN;
	JUMP REL(dataout), WHEN DATA_OUT;
	JUMP REL(datain), WHEN DATA_IN;
	JUMP REL(cmdout), WHEN CMD;
	JUMP REL(status), WHEN STATUS;
err:
	INT int_err;

reselect:
	; Clear DSA and init status
	MOVE 0xff to DSA0;
	MOVE 0xff to DSA1;
	MOVE 0xff to DSA2;
	MOVE 0xff to DSA3;
	MOVE 0xff to SCRATCHA2; no tag
	MOVE 0x08 to SCRATCHA3; NOP message
	WAIT RESELECT REL(reselect_fail)
	MOVE SSID & 0x8f to SFBR
	MOVE SFBR to SCRATCHA0 ; save reselect ID
; find the rigth param for this target
resel_targ0:
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	JUMP abs_targ0, IF 0xff;
	INT int_reseltarg;
lunsw_return:
	INT int_err, WHEN NOT MSG_IN;
	MOVE 1, abs_msgin, WHEN MSG_IN;
	MOVE SFBR & 0x07 to SCRATCHA1; save LUN
	CLEAR ACK;
	RETURN, WHEN NOT MSG_IN; If no more message, jump to lun sw
	MOVE 1, abs_msgin, WHEN MSG_IN;
	CLEAR ACK;
	JUMP REL(gettag), IF 0x20; simple tag message ?
	MOVE SFBR  to SCRATCHA3; save message
	RETURN; jump to lun sw and handle message
gettag:
	INT int_err, WHEN NOT MSG_IN;
	MOVE 1, abs_msgin, WHEN MSG_IN; get tag
	CLEAR ACK;
	MOVE SFBR  to SCRATCHA2; save tag
	RETURN; jump to lun sw

reselect_fail:
	; check that host asserted SIGP, this'll clear SIGP in ISTAT
	MOVE CTEST2 & 0x40 TO SFBR;
	INT int_resfail,  IF 0x00;
script_sched:
	; Clear DSA and init status
	MOVE 0xff to DSA0;
	MOVE 0xff to DSA1;
	MOVE 0xff to DSA2;
	MOVE 0xff to DSA3;
	MOVE 0 to SCRATCHA0	; flags
	MOVE 0 to SCRATCHA1	; DSA offset (for S/G save data pointer)
	JUMP script_abs_sched;

handle_sdp:
	CLEAR ACK;
	MOVE SCRATCHA0 | flag_sdp TO SCRATCHA0;
	; should get a disconnect message now
msgin:
	CLEAR ATN
	MOVE FROM t_msg_in, WHEN MSG_IN;
handle_msgin:
	JUMP REL(handle_dis), IF 0x04         ; disconnect message
	JUMP REL(handle_cmpl), IF 0x00        ; command complete message
	JUMP REL(handle_sdp), IF 0x02	      ; save data pointer message
	JUMP REL(handle_extin), IF 0x01	      ; extended message
	INT int_msgin;
msgin_ack:
selected:
	CLEAR ACK;
	JUMP REL(waitphase);

; entry point for msgout after a msgin or status phase
send_msgout:
	SET ATN;
	CLEAR ACK;
msgout:
	MOVE FROM t_msg_out, WHEN MSG_OUT;
	CLEAR ATN;
	JUMP REL(waitphase);
cmdout:
	MOVE FROM t_cmd, WHEN CMD;
	JUMP REL(waitphase);
status:
	MOVE FROM t_status, WHEN STATUS;
	JUMP REL(waitphase);
datain:
	CALL REL(savedsa);
	MOVE SCRATCHA0 | flag_data TO SCRATCHA0;
datain_loop:
	MOVE FROM t_data, WHEN DATA_IN;
	MOVE SCRATCHA1 + 1 TO SCRATCHA1	; adjust offset
	MOVE DSA0 + 8 to DSA0;
	MOVE DSA1 + 0 to DSA1 WITH CARRY;
	MOVE DSA2 + 0 to DSA2 WITH CARRY;
	MOVE DSA3 + 0 to DSA3 WITH CARRY;
	JUMP REL(datain_loop), WHEN DATA_IN;
	CALL REL(restoredsa);
	MOVE SCRATCHA0 & flag_data_mask TO SCRATCHA0;
	JUMP REL(waitphase);

dataout:
	CALL REL(savedsa);
	MOVE SCRATCHA0 | flag_data TO SCRATCHA0;
dataout_loop:
	MOVE FROM t_data, WHEN DATA_OUT;
	MOVE SCRATCHA1 + 1 TO SCRATCHA1	; adjust offset
	MOVE DSA0 + 8 to DSA0;
	MOVE DSA1 + 0 to DSA1 WITH CARRY;
	MOVE DSA2 + 0 to DSA2 WITH CARRY;
	MOVE DSA3 + 0 to DSA3 WITH CARRY;
	JUMP REL(dataout_loop), WHEN DATA_OUT;
	CALL REL(restoredsa);
	MOVE SCRATCHA0 & flag_data_mask TO SCRATCHA0;
	JUMP REL(waitphase);

savedsa:
	MOVE DSA0 to SFBR;
	MOVE SFBR to SCRATCHB0;
	MOVE DSA1 to SFBR;
	MOVE SFBR to SCRATCHB1;
	MOVE DSA2 to SFBR;
	MOVE SFBR to SCRATCHB2;
	MOVE DSA3 to SFBR;
	MOVE SFBR to SCRATCHB3;
	RETURN;

restoredsa:
	MOVE SCRATCHB0 TO SFBR;
	MOVE SFBR TO DSA0;
	MOVE SCRATCHB1 TO SFBR;
	MOVE SFBR TO DSA1;
	MOVE SCRATCHB2 TO SFBR;
	MOVE SFBR TO DSA2;
	MOVE SCRATCHB3 TO SFBR;
	MOVE SFBR TO DSA3;
	RETURN;

disconnect:
	MOVE SCNTL2 & 0x7f TO SCNTL2;
	CLEAR ATN;
	CLEAR ACK;
	WAIT DISCONNECT;
	RETURN;

handle_dis:
	CALL REL(disconnect);
; if we didn't get sdp, or if offset is 0, no need to interrupt
	MOVE SCRATCHA0 & flag_sdp TO SFBR;
	JUMP REL(script_sched), if 0x00; 
	MOVE SCRATCHA1 TO SFBR;
	JUMP REL(script_sched), if 0x00; 
; Ok, we need to save data pointers
	INT int_disc;

handle_cmpl:
	CALL REL(disconnect);
	INT int_done;

handle_extin:
	CLEAR ACK;
	INT int_err, IF NOT MSG_IN;
	MOVE FROM t_ext_msg_in, WHEN MSG_IN;
	INT int_extmsgin; /* let host fill in t_ext_msg_data */
get_extmsgdata:
	CLEAR ACK;
	INT int_err, IF NOT MSG_IN;
	MOVE FROM t_ext_msg_data, WHEN MSG_IN;
	INT int_extmsgdata;
msgin_space:
	NOP; space to store msgin when reselect


; script used for the scheduler: when a slot is free the JUMP points to
; the next slot so that instructions for this slot are not run.
; once the CPU has set up the slot variables (DSA address) it changes
; the JUMP address to 0 (so that it'll jump to the next instruction) and
; this command will be processed next time the scheduler is executed.
; When the target has been successfully selected the script changes the jump
; addr back to the next slot, so that it's ignored the next time.
;

PROC  slot_script:
slot:
        JUMP REL(nextslot);
	CALL slot_abs_loaddsa;
	SELECT ATN FROM t_id, slot_abs_reselect;
	MOVE MEMORY 4, slot_sched_addrsrc, slot_nextp;
	JUMP slot_abs_selected;  
slotdata:
	NOP; slot variables: jumppatchp
nextslot:
	NOP; /* will be changed to the next slot entry

PROC  endslot_script:
	JUMP endslot_abs_reselect;

;; per-target switch script for LUNs
; hack: we first to a call to the target-specific code, so that a return
; in the main switch will jump to the lun switch.
PROC lun_switch:
restore_scntl3:
	MOVE 0xff TO SCNTL3;
	MOVE 0xff TO SXFER;
	JUMP abs_lunsw_return;
lun_switch_entry:
	CALL REL(restore_scntl3);
	MOVE SCRATCHA1 TO SFBR;
resel_lun0:
	JUMP abs_lun0, IF 0x00;
	JUMP abs_lun0, IF 0x01;
	JUMP abs_lun0, IF 0x02;
	JUMP abs_lun0, IF 0x03;
	JUMP abs_lun0, IF 0x04;
	JUMP abs_lun0, IF 0x05;
	JUMP abs_lun0, IF 0x06;
	JUMP abs_lun0, IF 0x07;
	INT int_resellun;

;; script used to load the DSA after a reselect.

PROC load_dsa:
; Can't use MOVE MEMORY to load DSA, doesn't work I/O mapped
rdsa0:
	MOVE 0xf0 to DSA0;
rdsa1:
	MOVE 0xf1 to DSA1;
rdsa2:
	MOVE 0xf2 to DSA2;
rdsa3:
	MOVE 0xf3 to DSA3;
	RETURN;
reload_dsa:
	CALL REL(rdsa0);
	JUMP resel_abs_reselected;
