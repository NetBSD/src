ARCH 720

; offsets of sym_xfer
ABSOLUTE t_id = 4;
ABSOLUTE t_msg_in = 12;
ABSOLUTE t_msg_out = 20;
ABSOLUTE t_cmd = 28;
ABSOLUTE t_status = 36;
ABSOLUTE t_data = 44;

;; interrupt codes
ABSOLUTE int_done	= 0xff00;
ABSOLUTE int_msgin	= 0xff01;
ABSOLUTE int_resel	= 0xff02;
ABSOLUTE int_resfail	= 0xff03;
ABSOLUTE int_disc	= 0xff04;
ABSOLUTE int_err 	= 0xffff;

; flags for scratcha0
ABSOLUTE flag_sdp 	= 0x01 ; got save data pointer

ENTRY waitphase;
ENTRY select;
ENTRY msgout;
ENTRY msgin;
ENTRY dataout;
ENTRY datain;
ENTRY cmdout;
ENTRY status;
ENTRY disconnect;
ENTRY reselect;
ENTRY reselected;
ENTRY handle_reselect;

PROC  siop_script:

select:
	SELECT ATN FROM t_id, REL(reselect);
reselected:
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
	INT int_resfail;

msgin:
	CLEAR ATN
	MOVE FROM t_msg_in, WHEN MSG_IN;
	JUMP REL(handle_dis), IF 0x04         ; disconnect message
	JUMP REL(handle_cmpl), IF 0x00        ; command complete message
	JUMP REL(handle_sdp), IF 0x02	      ; save data pointer message
	INT int_msgin;
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
	JUMP REL(reselect), if 0x00; 
	MOVE SCRATCHA1 TO SFBR;
	JUMP REL(reselect), if 0x00; 
; Ok, we need to save data pointers
	INT int_disc;

handle_cmpl:
	CALL REL(disconnect);
	INT int_done;
handle_sdp:
	CLEAR ACK;
	MOVE SCRATCHA0 | flag_sdp TO SCRATCHA0;
	JUMP REL(msgin) ; should get a disconnect message now
