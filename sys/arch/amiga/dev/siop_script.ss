; NCR 53c710 script
;
ABSOLUTE ds_Device	= 0
ABSOLUTE ds_MsgOut 	= ds_Device + 4
ABSOLUTE ds_Cmd		= ds_MsgOut + 8
ABSOLUTE ds_Status	= ds_Cmd + 8
ABSOLUTE ds_Msg		= ds_Status + 8
ABSOLUTE ds_MsgIn	= ds_Msg + 8
ABSOLUTE ds_ExtMsg	= ds_MsgIn + 8
ABSOLUTE ds_SyncMsg	= ds_ExtMsg + 8
ABSOLUTE ds_Data1	= ds_SyncMsg + 8
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

ENTRY	scripts
ENTRY	switch
ENTRY	wait_reselect

PROC	scripts:

scripts:

	SELECT ATN FROM ds_Device, REL(reselect)
;
switch:
	JUMP REL(msgin), WHEN MSG_IN
	JUMP REL(msgout), IF MSG_OUT
	JUMP REL(command_phase), IF CMD
	JUMP REL(dataout), IF DATA_OUT
	JUMP REL(datain), IF DATA_IN
	JUMP REL(end), IF STATUS

	INT err1

msgin:
	MOVE FROM ds_MsgIn, WHEN MSG_IN
	CLEAR ACK
	JUMP REL(ext_msg), IF 0x01	; extended message
	JUMP REL(disc), IF 0x04		; disconnect message
	JUMP REL(switch), IF 0x02	; ignore save data pointers
	JUMP REL(msg_rej), IF 0x07	; ignore message reject
	JUMP REL(switch), IF 0x03	; ignore restore data pointers
	INT err2

msg_rej:
	CLEAR ATN
	JUMP REL(switch)

ext_msg:
	MOVE FROM ds_ExtMsg, WHEN MSG_IN
	JUMP REL(sync_msg), IF 0x03
	int err3

sync_msg:
	CLEAR ACK
	MOVE FROM ds_SyncMsg, WHEN MSG_IN
	CLEAR ACK
	JUMP REL(switch)

disc:
	WAIT DISCONNECT

	INT err10			; let host know about disconnect

wait_reselect:
	WAIT RESELECT REL(select_adr)

	INT err4, WHEN NOT MSG_IN
	MOVE FROM ds_Msg, WHEN MSG_IN
	CLEAR ACK
	INT err9			; let host know about reconnect
	JUMP REL(switch)

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
	int err5, WHEN NOT MSG_IN
	MOVE FROM ds_Msg, WHEN MSG_IN
	CLEAR ACK
	WAIT DISCONNECT
	INT ok
	JUMP REL(wait_reselect)

reselect:
	WAIT RESELECT REL(select_adr)
	INT err6

	INT err4, WHEN NOT MSG_IN
	MOVE FROM ds_Msg, WHEN MSG_IN
	CLEAR ACK
	JUMP REL(switch)

select_adr:
	MOVE CTEST2 & 0x40 to SFBR	; get Sig_P
	INT err7
	JUMP REL(scripts)
