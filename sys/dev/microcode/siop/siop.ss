ARCH 720

; offsets in sym_xfer
ABSOLUTE t_id = 20;
ABSOLUTE t_msg_in = 28;
ABSOLUTE t_ext_msg_in = 36;
ABSOLUTE t_ext_msg_data = 44;
ABSOLUTE t_ext_msg_tag = 52;
ABSOLUTE t_msg_out = 60;
ABSOLUTE t_cmd = 68;
ABSOLUTE t_status = 76;
ABSOLUTE t_data = 84;

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
ENTRY sheduler;
ENTRY slot;
ENTRY idsa0;
ENTRY idsa1;
ENTRY idsa2;
ENTRY idsa3;
ENTRY slotdata;
ENTRY nextslot;
ENTRY endslot;

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
	CLEAR ACK;
	INT int_resel;

reselect_fail:
	; check that host asserted SIGP, this'll clear SIGP in ISTAT
	MOVE CTEST2 & 0x40 TO SFBR;
	INT int_resfail,  IF 0x00;
	JUMP REL(sheduler);

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
datain_loop:
	MOVE FROM t_data, WHEN DATA_IN;
	MOVE SCRATCHA1 + 1 TO SCRATCHA1	; adjust offset
	MOVE DSA0 + 8 to DSA0;
	MOVE DSA1 + 0 to DSA1 WITH CARRY;
	MOVE DSA2 + 0 to DSA2 WITH CARRY;
	MOVE DSA3 + 0 to DSA3 WITH CARRY;
	JUMP REL(datain_loop), WHEN DATA_IN;
	CALL REL(restoredsa);
	JUMP REL(waitphase);

dataout:
	CALL REL(savedsa);
dataout_loop:
	MOVE FROM t_data, WHEN DATA_OUT;
	MOVE SCRATCHA1 + 1 TO SCRATCHA1	; adjust offset
	MOVE DSA0 + 8 to DSA0;
	MOVE DSA1 + 0 to DSA1 WITH CARRY;
	MOVE DSA2 + 0 to DSA2 WITH CARRY;
	MOVE DSA3 + 0 to DSA3 WITH CARRY;
	JUMP REL(dataout_loop), WHEN DATA_OUT;
	CALL REL(restoredsa);
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
	JUMP REL(sheduler), if 0x00; 
	MOVE SCRATCHA1 TO SFBR;
	JUMP REL(sheduler), if 0x00; 
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

sheduler:
	NOP; /* will be changed by the slot scripts */

; script used for the sheduler: when a slot is free the JUMP points to
; the next slot so that instructions for this slot are not run.
; once the CPU has set up the slot variables (DSA address) it changes
; the JUMP address to 0 (so that it'll jump to the next instruction) and
; this command will be processed next time the sheduler is executed.
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
