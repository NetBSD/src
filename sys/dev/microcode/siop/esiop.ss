;	$NetBSD: esiop.ss,v 1.15 2003/08/06 11:39:58 bouyer Exp $

;
; Copyright (c) 2002 Manuel Bouyer.
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
;	This product includes software developed by Manuel Bouyer.
; 4. Neither the name of the University nor the names of its contributors
;    may be used to endorse or promote products derived from this software
;    without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
; OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
; OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
; SUCH DAMAGE.
;

ARCH 825

; offsets in siop_common_xfer
ABSOLUTE t_id = 40;
ABSOLUTE t_msg_in = 48;
ABSOLUTE t_ext_msg_in = 56;
ABSOLUTE t_ext_msg_data = 64;
ABSOLUTE t_msg_out = 72;
ABSOLUTE t_cmd = 80;
ABSOLUTE t_status = 88;
ABSOLUTE t_data = 96;

; offsets in the per-target lun table
ABSOLUTE target_id = 0x0;
ABSOLUTE target_luntbl = 0x8;
ABSOLUTE target_luntbl_tag = 0xc;

;; interrupt codes
; interrupts that needs a valid target/lun/tag
ABSOLUTE int_done	= 0xff00;
ABSOLUTE int_msgin	= 0xff01;
ABSOLUTE int_extmsgin	= 0xff02;
ABSOLUTE int_extmsgdata	= 0xff03;
ABSOLUTE int_disc	= 0xff04;
; interrupts that don't have a valid I/T/Q
ABSOLUTE int_resfail	= 0xff80;     
ABSOLUTE int_err	= 0xffff;     

; We use the various scratch[a-j] registers to keep internal status:

; scratchA1: offset in data DSA (for save data pointer)
; scratchB: save/restore DSA in data loop
; scratchC: current target/lun/tag
; scratchC0: flags
ABSOLUTE f_c_target	= 0x01 ; target valid
ABSOLUTE f_c_lun	= 0x02 ; lun valid
ABSOLUTE f_c_tag	= 0x04 ; tag valid
ABSOLUTE f_c_data	= 0x08 ; data I/O in progress
ABSOLUTE f_c_data_mask	= 0xf7 ; ~f_c_data
ABSOLUTE f_c_sdp	= 0x10 ; got save data pointer message
; scratchC[1-3]: target/lun/tag

; scratchD: current DSA in start cmd ring
; scratchE0: index in start cmd ring
ABSOLUTE ncmd_slots	= 256 ; number of slots in CMD ring
ABSOLUTE ncmd_slots_last = 0 ; == ncmd_slots in a 8bit counter
; flags in a cmd slot
ABSOLUTE f_cmd_free	= 0x01 ; this slot is free
ABSOLUTE f_cmd_ignore	= 0x02 ; this slot is not free but don't start it
; offsets in a cmd slot
ABSOLUTE o_cmd_dsa	= 0; also holds f_cmd_*
; size of a cmd slot (for DSA increments)
ABSOLUTE cmd_slot_size	= 4;

; SCRATCHE1: last status

; SCRATCHE2: current command done slot
ABSOLUTE ndone_slots	= 256 ; number of slots in CMD ring
ABSOLUTE ndone_slots_last = 0 ; == ndonemd_slots in a 8bit counter
; SCRATCHF: pointer in command done ring

ENTRY cmdr0;
ENTRY cmdr1;
ENTRY cmdr2;
ENTRY cmdr3;
ENTRY doner0;
ENTRY doner1;
ENTRY doner2;
ENTRY doner3;
ENTRY reselect;
ENTRY led_on1;
ENTRY led_on2;
ENTRY led_off;
ENTRY status;
ENTRY msgin;
ENTRY msgin_ack;
ENTRY get_extmsgdata;
ENTRY send_msgout;
ENTRY script_sched;
ENTRY load_targtable;

EXTERN tlq_offset;
EXTERN abs_msgin2;

EXTERN abs_sem; a 32bits word used a semaphore between script and driver
ABSOLUTE sem_done = 0x01; there are pending done commands
ABSOLUTE sem_start = 0x02; a CMD slot was freed

PROC  esiop_script:

no_cmd:
	LOAD SCRATCHB0, 4, abs_sem; pending done command ?
	MOVE SCRATCHB0 & sem_done TO SFBR;
	INTFLY 0, IF NOT 0x00; 
	MOVE SCRATCHB0 | sem_start TO SCRATCHB0; we are there because the 
	STORE NOFLUSH SCRATCHB0, 4, abs_sem;     cmd ring is empty
reselect:
	MOVE 0x00 TO SCRATCHA1;
	MOVE 0x00 TO SCRATCHC0;
	MOVE 0xff TO SCRATCHE1;
; a NOP by default; patched with MOVE GPREG | 0x01 to GPREG on compile-time
; option "SIOP_SYMLED"
led_off:
	NOP;
	WAIT RESELECT REL(reselect_fail);
; a NOP by default; patched with MOVE GPREG & 0xfe to GPREG on compile-time
; option "SIOP_SYMLED"
led_on2:
        NOP;
	MOVE SSID & 0x0f to SFBR;
	MOVE SFBR to SCRATCHC1;
	MOVE SCRATCHC0 | f_c_target to SCRATCHC0; save target
	CLEAR CARRY;
	MOVE SCRATCHC1 SHL SFBR;
	MOVE SFBR SHL DSA0; target * 4 in dsa
	MOVE 0x0 to DSA1;
	MOVE 0x0 to DSA2;
	MOVE 0x0 to DSA3;
; load DSA for the target table
load_targtable:
	MOVE DSA0 + 0x00 to DSA0; host will patch 0x0 with base of table
	MOVE DSA1 + 0x00 to DSA1 with carry;
	MOVE DSA2 + 0x00 to DSA2 with carry;
	MOVE DSA3 + 0x00 to DSA3 with carry; now dsa -> basetable + target * 4
	LOAD DSA0, 4, FROM 0; now load DSA for this target
	SELECT FROM target_id, REL(nextisn);
nextisn:
	MOVE 1, abs_msgin2, WHEN MSG_IN;
	MOVE SFBR & 0x07 to SCRATCHC2;
	MOVE SCRATCHC0 | f_c_lun to SCRATCHC0; save LUN
	CLEAR ACK and CARRY;
	MOVE SCRATCHC2 SHL SFBR; 
	MOVE SFBR SHL SFBR;
	MOVE SFBR SHL SFBR; lun * 8
	MOVE DSA0 + SFBR TO DSA0;
	MOVE DSA1 + 0x0 TO DSA1 with carry;
	MOVE DSA2 + 0x0 TO DSA2 with carry;
	MOVE DSA3 + 0x0 TO DSA3 with carry;
	LOAD SCRATCHB0, 4, from target_luntbl_tag; in case it's a tagged cmd
	LOAD DSA0, 4, from target_luntbl; load DSA for this LUN
	JUMP REL(waitphase), WHEN NOT MSG_IN;
	MOVE 1, abs_msgin2, WHEN MSG_IN;
	CLEAR ACK;
	JUMP REL(handle_msgin), IF NOT 0x20; not a simple tag message
	MOVE 1, abs_msgin2, WHEN MSG_IN; get tag
	MOVE SFBR to SCRATCHA2;
	MOVE SFBR to SCRATCHC3;
	MOVE SCRATCHC0 | f_c_tag to SCRATCHC0; save TAG
	CALL REL(restoredsa); switch to tag table DSA
	MOVE 0x0 to SCRATCHA3;
	CLEAR CARRY;
	MOVE SCRATCHA2 SHL SCRATCHA2;
	MOVE SCRATCHA3 SHL SCRATCHA3;
	MOVE SCRATCHA2 SHL SCRATCHA2;
	MOVE SCRATCHA3 SHL SCRATCHA3; TAG * 4 to SCRATCHA(2,3)
	MOVE SCRATCHA2 TO SFBR;
	MOVE DSA0 + SFBR TO DSA0;
	MOVE DSA1 + 0x00 TO DSA1 with CARRY;
	MOVE DSA2 + 0x00 TO DSA2 with CARRY;
	MOVE DSA3 + 0x00 TO DSA3 with CARRY;
	MOVE SCRATCHA3 TO SFBR;
	MOVE DSA1 + SFBR TO DSA1;
	MOVE DSA2 + 0x00 TO DSA2 with CARRY;
	MOVE DSA3 + 0x00 TO DSA3 with CARRY; SCRACHA(2,3) + DSA to DSA
	LOAD DSA0, 4, from 0; load DSA for this tag
msgin_ack:
	CLEAR ACK;
waitphase:
	JUMP REL(msgout), WHEN MSG_OUT;
	JUMP REL(msgin), WHEN MSG_IN;
	JUMP REL(dataout), WHEN DATA_OUT;
	JUMP REL(datain), WHEN DATA_IN;
	JUMP REL(cmdout), WHEN CMD;
	JUMP REL(status), WHEN STATUS;
	INT int_err;

handle_cmpl:
	CALL REL(disconnect);
	MOVE SCRATCHE1 to SFBR;
	INT int_done, IF NOT 0x00; if status is not "done", let host handle it
	MOVE SCRATCHF0 to SFBR; load pointer in done ring
	MOVE SFBR to DSA0;
	MOVE SCRATCHF1 to SFBR;
	MOVE SFBR to DSA1;
	MOVE SCRATCHF2 to SFBR;
	MOVE SFBR to DSA2;
	MOVE SCRATCHF3 to SFBR;
	MOVE SFBR to DSA3;
wait_free:
	LOAD SCRATCHA0, 1, from 0;
	MOVE SCRATCHA0 to SFBR;
	JUMP REL(wait_free), if not 0; wait for slot to be free
	STORE NOFLUSH SCRATCHC0, 4, from 0; save current target/lun/flag
	MOVE SCRATCHF0 + 4 to SCRATCHF0; advance to next slot
	MOVE SCRATCHF1 + 0 to SCRATCHF1 with carry;
	MOVE SCRATCHF2 + 0 to SCRATCHF2 with carry;
	MOVE SCRATCHF3 + 0 to SCRATCHF3 with carry;
	MOVE SCRATCHE2 + 1 to SCRATCHE2;
	MOVE SCRATCHE2 to SFBR;
	JUMP REL(is_done), if not ndone_slots_last;
doner0:
	MOVE 0xff to SCRATCHF0; driver will change 0xff to base of ring
doner1:
	MOVE 0xff to SCRATCHF1;
doner2:
	MOVE 0xff to SCRATCHF2;
doner3:
	MOVE 0xff to SCRATCHF3;
	MOVE 0  to SCRATCHE2;
is_done:
	LOAD SCRATCHB0, 4, abs_sem; signal that a command is done
	MOVE SCRATCHB0 | sem_done TO SCRATCHB0;
	STORE NOFLUSH SCRATCHB0, 4, abs_sem;
; and attempt next command

reselect_fail:
	; clear SIGP in ISTAT
	MOVE CTEST2 & 0x40 TO SFBR;
script_sched:
; Load ring DSA
	MOVE SCRATCHD0 to SFBR;
	MOVE SFBR to DSA0;
	MOVE SCRATCHD1 to SFBR;
	MOVE SFBR to DSA1;
	MOVE SCRATCHD2 to SFBR;
	MOVE SFBR to DSA2;
	MOVE SCRATCHD3 to SFBR;
	MOVE SFBR to DSA3;
	LOAD DSA0,4, from o_cmd_dsa; get DSA and flags for this slot
	MOVE DSA0 & f_cmd_free to SFBR; check flags
	JUMP REL(no_cmd), IF NOT 0x0;
	MOVE DSA0 & f_cmd_ignore to SFBR;
	JUMP REL(ignore_cmd), IF NOT 0x0;
	LOAD SCRATCHC0, 4, FROM tlq_offset;
; this slot is busy, attempt to exec command
	SELECT ATN FROM t_id, REL(reselect);
; select either succeeded or timed out.
; if timed out the STO interrupt will be posted at the first SCSI bus access
; waiting for a valid phase, so we have to do it now. If not a MSG_OUT phase,
; this is an error anyway (we selected with ATN)
	INT int_err, WHEN NOT MSG_OUT;
ignore_cmd:
	MOVE SCRATCHD0 to SFBR; restore scheduler DSA
	MOVE SFBR to DSA0;
	MOVE SCRATCHD1 to SFBR;
	MOVE SFBR to DSA1;
	MOVE SCRATCHD2 to SFBR;
	MOVE SFBR to DSA2;
	MOVE SCRATCHD3 to SFBR;
	MOVE SFBR to DSA3;
	MOVE SCRATCHE0 + 1 to SCRATCHE0;
	MOVE SCRATCHD0 + cmd_slot_size to SCRATCHD0; 
	MOVE SCRATCHD1 + 0 to SCRATCHD1 WITH CARRY;
	MOVE SCRATCHD2 + 0 to SCRATCHD2 WITH CARRY;
	MOVE SCRATCHD3 + 0 to SCRATCHD3 WITH CARRY;
	MOVE SCRATCHE0 TO SFBR;
	JUMP REL(handle_cmd), IF  NOT ncmd_slots_last;
; reset pointers to beggining of area
cmdr0:
	MOVE 0xff to SCRATCHD0; correct value will be patched by driver
cmdr1:
	MOVE 0xff to SCRATCHD1;
cmdr2:
	MOVE 0xff to SCRATCHD2;
cmdr3:
	MOVE 0xff to SCRATCHD3;
	MOVE 0x00 to SCRATCHE0;
handle_cmd:
; to avoid race condition we have to load the DSA value before setting the
; free flag, so we have to use a temp register.
; use SCRATCHB0 so that we can CALL restoredsa later
	LOAD SCRATCHB0, 4, FROM o_cmd_dsa; load DSA for this command in temp reg
	MOVE SCRATCHB0 | f_cmd_free to SCRATCHB0; mark slot as free
	STORE noflush SCRATCHB0, 4, FROM o_cmd_dsa;
	MOVE SCRATCHB0 & f_cmd_ignore to SFBR;
	JUMP REL(script_sched), IF NOT 0x00; next command if ignore
	MOVE SCRATCHB0 & 0xfc to SCRATCHB0; clear f_cmd_*
	CALL REL(restoredsa); and move SCRATCHB to DSA
	LOAD SCRATCHB0, 4, abs_sem;
	MOVE SCRATCHB0 | sem_start TO SCRATCHB0;
	STORE NOFLUSH SCRATCHB0, 4, abs_sem;

; a NOP by default; patched with MOVE GPREG & 0xfe to GPREG on compile-time
; option "SIOP_SYMLED"
led_on1:
	NOP;
	MOVE 0x00 TO SCRATCHA1;
	MOVE 0xff TO SCRATCHE1;
;we can now send our identify message
send_msgout: ; entry point for msgout after a msgin or status phase
	SET ATN;
	CLEAR ACK;
msgout: 
        MOVE FROM t_msg_out, WHEN MSG_OUT;
	CLEAR ATN;  
	JUMP REL(waitphase);


handle_sdp:
	CLEAR ACK;
	MOVE SCRATCHC0 | f_c_sdp TO SCRATCHC0;
	; should get a disconnect message now
msgin:
	CLEAR ATN
	MOVE FROM t_msg_in, WHEN MSG_IN;
handle_msgin:
	JUMP REL(handle_cmpl), IF 0x00	; command complete message
	JUMP REL(handle_sdp), IF 0x02	; save data pointer message
	JUMP REL(handle_extin), IF 0x01	; extended message
	INT int_msgin, IF NOT 0x04;
	CALL REL(disconnect)		; disconnect message
; if we didn't get sdp, or if offset is 0, no need to interrupt
	MOVE SCRATCHC0 & f_c_sdp TO SFBR;
	JUMP REL(script_sched), if 0x00;
	MOVE SCRATCHA1 TO SFBR;
	JUMP REL(script_sched), if 0x00;
; Ok, we need to save data pointers
	INT int_disc;

cmdout:
        MOVE FROM t_cmd, WHEN CMD; 
	JUMP REL(waitphase);
status: 
        MOVE FROM t_status, WHEN STATUS;
	MOVE SFBR TO SCRATCHE1;
	JUMP REL(waitphase);
datain:
        CALL REL(savedsa);
	MOVE SCRATCHC0 | f_c_data TO SCRATCHC0;
	datain_loop:
	MOVE FROM t_data, WHEN DATA_IN;
	MOVE SCRATCHA1 + 1 TO SCRATCHA1 ; adjust offset
	MOVE DSA0 + 8 to DSA0;
	MOVE DSA1 + 0 to DSA1 WITH CARRY;
	MOVE DSA2 + 0 to DSA2 WITH CARRY;
	MOVE DSA3 + 0 to DSA3 WITH CARRY;
	JUMP REL(datain_loop), WHEN DATA_IN;
	CALL REL(restoredsa);
	MOVE SCRATCHC0 & f_c_data_mask TO SCRATCHC0;
	JUMP REL(waitphase);

dataout:
        CALL REL(savedsa);
	MOVE SCRATCHC0 | f_c_data TO SCRATCHC0;
dataout_loop:
	MOVE FROM t_data, WHEN DATA_OUT;
	MOVE SCRATCHA1 + 1 TO SCRATCHA1 ; adjust offset
	MOVE DSA0 + 8 to DSA0;
	MOVE DSA1 + 0 to DSA1 WITH CARRY;
	MOVE DSA2 + 0 to DSA2 WITH CARRY;
	MOVE DSA3 + 0 to DSA3 WITH CARRY;
	JUMP REL(dataout_loop), WHEN DATA_OUT;
	CALL REL(restoredsa);
	MOVE SCRATCHC0 & f_c_data_mask TO SCRATCHC0;
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

handle_extin:
	CLEAR ACK;
	MOVE FROM t_ext_msg_in, WHEN MSG_IN;
	INT int_extmsgin; /* let host fill in t_ext_msg_data */
get_extmsgdata:
	CLEAR ACK;
	MOVE FROM t_ext_msg_data, WHEN MSG_IN;
	INT int_extmsgdata; 

PROC esiop_led_on:
	MOVE GPREG & 0xfe TO GPREG;

PROC esiop_led_off:
	MOVE GPREG | 0x01 TO GPREG;
