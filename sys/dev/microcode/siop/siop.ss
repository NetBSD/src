;	$NetBSD: siop.ss,v 1.6.2.1 2000/06/22 17:07:14 minoura Exp $

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
ABSOLUTE int_done	= 0xff00;
ABSOLUTE int_msgin	= 0xff01;
ABSOLUTE int_extmsgin	= 0xff02;
ABSOLUTE int_extmsgdata	= 0xff03;
ABSOLUTE int_resel	= 0xff04;
ABSOLUTE int_reseltag	= 0xff05;
ABSOLUTE int_resfail	= 0xff06;
ABSOLUTE int_disc	= 0xff07;
ABSOLUTE int_err 	= 0xffff;

; flags for scratcha0
ABSOLUTE flag_sdp 	= 0x01 ; got save data pointer
ABSOLUTE flag_data 	= 0x02 ; we're in data phase
ABSOLUTE flag_data_mask	= 0xfd ; ~flag_data

ENTRY waitphase;
ENTRY send_msgout;
ENTRY msgout;
ENTRY msgin;
ENTRY msgin_ack;
ENTRY dataout;
ENTRY datain;
ENTRY cmdout;
ENTRY status;
ENTRY disconnect;
ENTRY reselect;
ENTRY selected;
ENTRY get_extmsgdata;
ENTRY slot;
ENTRY idsa0;
ENTRY idsa1;
ENTRY idsa2;
ENTRY idsa3;
ENTRY slotdata;
ENTRY nextslot;
ENTRY endslot;

EXTERN script_abs_shed;
EXTERN slot_nextp;
EXTERN slot_shed_addrsrc;
EXTERN slot_abs_reselect;
EXTERN slot_abs_selected;

EXTERN endslot_abs_reselect;

PROC  siop_script:

selected:
; starting a new session, init 'local variables'
	MOVE 0 to SCRATCHA0	; flags
	MOVE 0 to SCRATCHA1	; DSA offset (for S/G save data pointer)
	CLEAR ACK;
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
	WAIT RESELECT REL(reselect_fail)
	MOVE SSID & 0x8f to SFBR
	MOVE SFBR to SCRATCHA0 ; save reselect ID
	INT int_err, WHEN NOT MSG_IN;
	MOVE FROM t_msg_in, WHEN MSG_IN;
	INT int_resel;

reselect_fail:
	; check that host asserted SIGP, this'll clear SIGP in ISTAT
	MOVE CTEST2 & 0x40 TO SFBR;
	INT int_resfail,  IF 0x00;
	JUMP script_abs_shed;

msgin:
	CLEAR ATN
	MOVE FROM t_msg_in, WHEN MSG_IN;
	JUMP REL(handle_dis), IF 0x04         ; disconnect message
	JUMP REL(handle_cmpl), IF 0x00        ; command complete message
	JUMP REL(handle_sdp), IF 0x02	      ; save data pointer message
	JUMP REL(handle_extin), IF 0x01	      ; extended message
	INT int_msgin;
msgin_ack:
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
	JUMP script_abs_shed, if 0x00; 
	MOVE SCRATCHA1 TO SFBR;
	JUMP script_abs_shed, if 0x00; 
; Ok, we need to save data pointers
	INT int_disc;

handle_cmpl:
	CALL REL(disconnect);
	INT int_done;
handle_sdp:
	CLEAR ACK;
	MOVE SCRATCHA0 | flag_sdp TO SCRATCHA0;
	JUMP REL(msgin) ; should get a disconnect message now

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
idsa0:
	MOVE 0x00 to dsa0;
idsa1:
	MOVE 0x01 to dsa1;
idsa2:
	MOVE 0x02 to dsa2;
idsa3:
	MOVE 0x03 to dsa3;
	SELECT ATN FROM t_id, slot_abs_reselect;
	MOVE MEMORY 4, slot_shed_addrsrc, slot_nextp;
	JUMP slot_abs_selected;  
slotdata:
	NOP; slot variables: dsa & jumppatchp
nextslot:	NOP; /* will be changed to the next slot entry

PROC  endslot_script:
	JUMP endslot_abs_reselect;
