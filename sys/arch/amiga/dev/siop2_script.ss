;	$NetBSD: siop2_script.ss,v 1.5 2005/07/25 19:27:42 is Exp $

;
; Copyright (c) 1998 Michael L. Hitch
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
; 3. All advertising materials mentioning features or use of this software
;    must display the following acknowledgement:
;      This product includes software developed by Michael L. Hitch.
; 4. The name of the author may not be used to endorse or promote products
;    derived from this software without specific prior written permission
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
; NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;

; NCR 53c770 script
;
ARCH 720
;
ABSOLUTE ds_Device	= 0
ABSOLUTE ds_MsgOut 	= ds_Device + 4
ABSOLUTE ds_Cmd		= ds_MsgOut + 8
ABSOLUTE ds_Status	= ds_Cmd + 8
ABSOLUTE ds_Msg		= ds_Status + 8
ABSOLUTE ds_MsgIn	= ds_Msg + 8
ABSOLUTE ds_ExtMsg	= ds_MsgIn + 8
ABSOLUTE ds_NegMsg	= ds_ExtMsg + 8
ABSOLUTE ds_Data1	= ds_NegMsg + 8
ABSOLUTE ds_Data2	= ds_Data1 + 8
ABSOLUTE ds_Data3	= ds_Data2 + 8
ABSOLUTE ds_Data4	= ds_Data3 + 8
ABSOLUTE ds_Data5	= ds_Data4 + 8
ABSOLUTE ds_Data6	= ds_Data5 + 8
ABSOLUTE ds_Data7	= ds_Data6 + 8
ABSOLUTE ds_Data8	= ds_Data7 + 8
ABSOLUTE ds_Data9	= ds_Data8 + 8

ABSOLUTE ok		= 0xff00
ABSOLUTE err1		= 0xff01
ABSOLUTE err2		= 0xff02
ABSOLUTE err3		= 0xff03
ABSOLUTE err4		= 0xff04
ABSOLUTE err5		= 0xff05
ABSOLUTE err6		= 0xff06
ABSOLUTE err7		= 0xff07
ABSOLUTE err8		= 0xff08
ABSOLUTE err9		= 0xff09
ABSOLUTE err10		= 0xff0a
ABSOLUTE err11		= 0xff0b

ENTRY	scripts
ENTRY	switch
ENTRY	wait_reselect
ENTRY	dataout
ENTRY	datain
ENTRY	clear_ack

PROC	siopng_scripts:

scripts:

	SELECT ATN FROM ds_Device, REL(reselect)
;
switch:
	MOVE GPREG | 0x10 TO GPREG
	JUMP REL(msgin), WHEN MSG_IN
	JUMP REL(msgout), IF MSG_OUT
	JUMP REL(command_phase), IF CMD
	JUMP REL(dataout), IF DATA_OUT
	JUMP REL(datain), IF DATA_IN
	JUMP REL(end), IF STATUS

	INT err5			; Unrecognized phase

msgin:
	MOVE FROM ds_MsgIn, WHEN MSG_IN
	JUMP REL(ext_msg), IF 0x01	; extended message
	JUMP REL(disc), IF 0x04		; disconnect message
	JUMP REL(msg_sdp), IF 0x02	; save data pointers
	JUMP REL(msg_rej), IF 0x07	; message reject
	JUMP REL(msg_rdp), IF 0x03	; restore data pointers
	INT err6			; unrecognized message

msg_rej:
; Do we need to interrupt host here to let it handle the reject?
msg_rdp:
clear_ack:
	CLEAR ACK
	CLEAR ATN
	JUMP REL(switch)

ext_msg:
	CLEAR ACK
	MOVE FROM ds_ExtMsg, WHEN MSG_IN
	JUMP REL(neg_msg), IF 0x03	; extended message might be SDTR
	JUMP REL(neg_msg), IF 0x02	; extended message might be WDTR
	INT err7			; extended message not SDTR

neg_msg:
	CLEAR ACK
	MOVE FROM ds_NegMsg, WHEN MSG_IN
	INT err11			; Let host handle the message
; If we continue from the interrupt, the host has set up a response
; message to be sent.  Set ATN, clear ACK, and continue.
	SET ATN
	CLEAR ACK
	JUMP REL(switch)

disc:
	MOVE SCNTL2 & 0x7f TO SCNTL2
	MOVE GPREG & 0xEF TO GPREG
	CLEAR ACK
	WAIT DISCONNECT

	INT err2			; signal disconnect w/o save DP

msg_sdp:
	CLEAR ACK			; acknowledge message
	JUMP REL(switch), WHEN NOT MSG_IN
	MOVE FROM ds_ExtMsg, WHEN MSG_IN
	INT err8, IF NOT 0x04		; interrupt if not disconnect
	MOVE SCNTL2 & 0x7f TO SCNTL2
	CLEAR ACK
	WAIT DISCONNECT

	INT err1			; signal disconnect

reselect:
wait_reselect:
	WAIT RESELECT REL(select_adr)
	MOVE SSID & 0x8f to SFBR	; Save reselect ID
	MOVE SFBR to SCRATCHA0

	INT err9, WHEN NOT MSG_IN	; didn't get IDENTIFY
	MOVE FROM ds_Msg, WHEN MSG_IN
	INT err3			; let host know about reconnect
	CLEAR ACK			; acknowlege the message
	JUMP REL(switch)


select_adr:
	MOVE SCNTL1 & 0x10 to SFBR	; get connected status
	INT err4, IF 0x00		; tell host if not connected
	MOVE CTEST2 & 0x40 to SFBR	; clear Sig_P
	JUMP REL(wait_reselect)		; and try reselect again

msgout:
	MOVE FROM ds_MsgOut, WHEN MSG_OUT
	JUMP REL(switch)

command_phase:
	CLEAR ATN
	MOVE FROM ds_Cmd, WHEN CMD
	JUMP REL(switch)

dataout:
	MOVE FROM ds_Data1, WHEN DATA_OUT
	CALL REL(switch), WHEN NOT DATA_OUT
	MOVE FROM ds_Data2, WHEN DATA_OUT
	CALL REL(switch), WHEN NOT DATA_OUT
	MOVE FROM ds_Data3, WHEN DATA_OUT
	CALL REL(switch), WHEN NOT DATA_OUT
	MOVE FROM ds_Data4, WHEN DATA_OUT
	CALL REL(switch), WHEN NOT DATA_OUT
	MOVE FROM ds_Data5, WHEN DATA_OUT
	CALL REL(switch), WHEN NOT DATA_OUT
	MOVE FROM ds_Data6, WHEN DATA_OUT
	CALL REL(switch), WHEN NOT DATA_OUT
	MOVE FROM ds_Data7, WHEN DATA_OUT
	CALL REL(switch), WHEN NOT DATA_OUT
	MOVE FROM ds_Data8, WHEN DATA_OUT
	CALL REL(switch), WHEN NOT DATA_OUT
	MOVE FROM ds_Data9, WHEN DATA_OUT
	CALL REL(switch)

datain:
	MOVE FROM ds_Data1, WHEN DATA_IN
	CALL REL(switch), WHEN NOT DATA_IN
	MOVE FROM ds_Data2, WHEN DATA_IN
	CALL REL(switch), WHEN NOT DATA_IN
	MOVE FROM ds_Data3, WHEN DATA_IN
	CALL REL(switch), WHEN NOT DATA_IN
	MOVE FROM ds_Data4, WHEN DATA_IN
	CALL REL(switch), WHEN NOT DATA_IN
	MOVE FROM ds_Data5, WHEN DATA_IN
	CALL REL(switch), WHEN NOT DATA_IN
	MOVE FROM ds_Data6, WHEN DATA_IN
	CALL REL(switch), WHEN NOT DATA_IN
	MOVE FROM ds_Data7, WHEN DATA_IN
	CALL REL(switch), WHEN NOT DATA_IN
	MOVE FROM ds_Data8, WHEN DATA_IN
	CALL REL(switch), WHEN NOT DATA_IN
	MOVE FROM ds_Data9, WHEN DATA_IN
	CALL REL(switch)

end:
	MOVE FROM ds_Status, WHEN STATUS
	INT err10, WHEN NOT MSG_IN	; status not followed by msg
	MOVE FROM ds_Msg, WHEN MSG_IN
	MOVE SCNTL2 & 0x7f TO SCNTL2
	CLEAR ACK
	WAIT DISCONNECT
	MOVE GPREG & 0xEF TO GPREG
	INT ok				; signal completion
	JUMP REL(wait_reselect)
