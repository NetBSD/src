;	$NetBSD: oosiop.ss,v 1.1 2001/12/05 18:27:13 fredette Exp $

;
; Copyright (c) 2001 Matt Fredette
; Copyright (c) 1995 Michael L. Hitch
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

; NCR 53c700 script
;
ARCH 700
;
EXTERNAL ds_Device
EXTERNAL ds_MsgOut
EXTERNAL ds_Cmd
EXTERNAL ds_Status
EXTERNAL ds_Msg
EXTERNAL ds_MsgIn
EXTERNAL ds_ExtMsg
EXTERNAL ds_SyncMsg
EXTERNAL ds_Data1


ABSOLUTE ok		= 0xff00
ABSOLUTE int_disc	= 0xff01
ABSOLUTE int_disc_wodp	= 0xff02
ABSOLUTE int_reconnect	= 0xff03
ABSOLUTE int_connect	= 0xff04
ABSOLUTE int_phase	= 0xff05
ABSOLUTE int_msgin	= 0xff06
ABSOLUTE int_extmsg	= 0xff07
ABSOLUTE int_msgsdp	= 0xff08
ABSOLUTE int_identify	= 0xff09
ABSOLUTE int_status	= 0xff0a
ABSOLUTE int_syncmsg	= 0xff0b

ENTRY	scripts
ENTRY	switch
ENTRY	wait_reselect
ENTRY	dataout
ENTRY	datain
ENTRY	clear_ack

PROC	oosiop_script:

scripts:

	SELECT ATN ds_Device, reselect
;
switch:
	JUMP msgin, WHEN MSG_IN
	JUMP msgout, IF MSG_OUT
	JUMP command_phase, IF CMD
	JUMP dataout, IF DATA_OUT
	JUMP datain, IF DATA_IN
	JUMP end, IF STATUS

	INT int_phase			; Unrecognized phase

msgin:
	MOVE 0, ds_MsgIn, WHEN MSG_IN
	JUMP ext_msg, IF 0x01		; extended message
	JUMP disc, IF 0x04		; disconnect message
	JUMP msg_sdp, IF 0x02		; save data pointers
	JUMP msg_rej, IF 0x07		; message reject
	JUMP msg_rdp, IF 0x03		; restore data pointers
	INT int_msgin			; unrecognized message

msg_rej:
; Do we need to interrupt host here to let it handle the reject?
msg_rdp:
clear_ack:
	CLEAR ACK
	CLEAR ATN
	JUMP switch

ext_msg:
	CLEAR ACK
	MOVE 0, ds_ExtMsg, WHEN MSG_IN
	JUMP sync_msg, IF 0x03
	int int_extmsg			; extended message not SDTR

sync_msg:
	CLEAR ACK
	MOVE 0, ds_SyncMsg, WHEN MSG_IN
	int int_syncmsg			; Let host handle the message
; If we continue from the interrupt, the host has set up a response
; message to be sent.  Set ATN, clear ACK, and continue.
	SET ATN
	CLEAR ACK
	JUMP switch

disc:
	CLEAR ACK
	WAIT DISCONNECT

	int int_disc_wodp		; signal disconnect w/o save DP

msg_sdp:
	CLEAR ACK			; acknowledge message
	JUMP switch, WHEN NOT MSG_IN
	MOVE 0, ds_ExtMsg, WHEN MSG_IN
	INT int_msgsdp, IF NOT 0x04	; interrupt if not disconnect
	CLEAR ACK
	WAIT DISCONNECT

	INT int_disc			; signal disconnect

reselect:
wait_reselect:
	WAIT RESELECT select_adr
	; NB: these NOPs are CRITICAL to preserve the 1:1
	; correspondence between instructions in this script
	; and instructions in the osiop (53c710) script:
	NOP
	NOP				; reselect ID already in SFBR

	INT int_identify, WHEN NOT MSG_IN
	MOVE 0, ds_Msg, WHEN MSG_IN
	INT int_reconnect		; let host know about reconnect
	CLEAR ACK			; acknowlege the message
	JUMP switch

select_adr:
	MOVE SCNTL1 & 0x10 to SFBR	; get connected status
	INT int_connect, IF 0x00	; tell host if not connected
	NOP				; Sig_P doesn't exist on the 53c700
	JUMP wait_reselect		; and try reselect again

msgout:
	MOVE 0, ds_MsgOut, WHEN MSG_OUT
	JUMP switch

command_phase:
	CLEAR ATN
	MOVE 0, ds_Cmd, WHEN CMD
	JUMP switch

dataout:
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch, WHEN NOT DATA_OUT
	MOVE 0, ds_Data1, WHEN DATA_OUT
	CALL switch

datain:
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch, WHEN NOT DATA_IN
	MOVE 0, ds_Data1, WHEN DATA_IN
	CALL switch

end:
	MOVE 0, ds_Status, WHEN STATUS
	int int_status, WHEN NOT MSG_IN	; status not followed by msg
	MOVE 0, ds_Msg, WHEN MSG_IN
	CLEAR ACK
	WAIT DISCONNECT
	INT ok				; signal completion
	JUMP wait_reselect
