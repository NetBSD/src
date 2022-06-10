;
; Copyright (c) 2018 Yosuke Sugahara. All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
; AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
; OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
; SUCH DAMAGE.
;
;
; LUNA XP multiplexed device firmware
;
; used language:
;  zasm 4.1
;  http://k1.spdns.de/Develop/Projects/zasm
;
; XP memory map
;
; type : SH, PR, IN, NC
;     SH: host shared memory, 64kB, PA 00000 - 0FFFF
;     PR: private memory, 32kB, PA 28000-2FFFF
;     IN: HD647180 internal 512 bytes memory
;     NC: not connected (00 or FF or image readable, maybe)
;
; start end type desc
;  0000 00FF SH RESET/RST etc.
;  0100 01FF SH shared variables
;  0200 0FFF SH resident program
;  1000 7FFF SH PAM/PCM buffer 28K
;  8000 8FFF SH PSG buffer 4K
;  9000 9FFF SH LPR buffer 4K
;  A000 DFFF SH FDC buffer 16K
;  E000 EFFF PR program/stack
;  F000 FDFF NC bus error (00 or FF)
;  FE00 FFDF IN PAM player
;  FFE0 FFFF IN interrupt vector
;
; shared variable area
;  0100    XPBUS
;  0110    TIME
;  0120    PAM
;  0130    PCM
;  0140    PSG
;  0150    SPK
;  0160    LPR
;  0170    FDC
;  0180    SIO0
;  0190    SIO1
; device ID = bit 7-4
;
; XP internal device usage
;  PRT0  device dispatcher/TIME
;  PRT1  PCM
;  PT2   unused
;  ASCI0 SIO0
;  ASCI1 SIO1	本体表記の関係で、入れ替える?
;
; READY-CMD-RESULT-RUN プロトコル
; XP 視点
; READY
;   コマンドを受け付けできるとき != 0
;   受付できないとき 0
; CMD
;   ホスト側が送信してくるコマンド
;   コマンドなしは 0
;   XP がコマンドを受け付けると、READY=0 CMD=0 の順で、XP が 0 にする
; RESULT
;   コマンド実行結果。
;   RESULT=x READY=1 の順で書き込む。
;   ハードリセット、またはホストが 0 クリアする。
; RUN
;   コマンド実行中は != 0
;   実行していないときは 0
;   実行完了時、RESULT=x RUN=0 READY=1 の順で書き込む。
;
; 通常シーケンス
;  READY を上げる
;  CMD の立ち上がりを待つ
;  READY を下げる
;  RUN を上げる
;  CMD を下げる
;  実行
;  RESULT を書く
;  RUN を下げる
;  READY を上げる
;
; ホスト視点
; 実行完了を待つとき
;  while (READY == 0);	// 受付可能待ち
;  RESULT=0;		// 結果クリア
;  CMD=x;		// コマンド送信
;  while (RESULT == 0);	// 実行完了待ち
;  if (RESULT==ERROR) error();	// エラー確認
;

;
; XPBUS
;  +0.b READY
;  +1.b CMD
;  +2.b RESULT
;  +3.b RUN
;
;  +4.b STAT_RESET
;        ファームウェア転送で 0　にされる。
;        リセット位置の実行のたびに +1 
;        すなわち何もなければ 1 になっている。
;  +5.3 align
;  +8.w PRT0_TIMER
;        ==256(1200Hz)
;  +A.w INTR1_DEV
;        bitmap of INTR1 device ID
;  +C.w INTR5_DEV
;        bitmap of INTR5 device ID
;
; TIME
;  +0.b READY
;  +1.b CMD
;  +2.b RESULT
;  +3.b RUN
;
;  +4.w TIMECOUNTER
;
; PAM
;  +0.b READY
;  +1.b CMD
;  +2.b RESULT
;  +3.b RUN
;
;  +4.b ENC
;        エンコードフォーマット識別子。
;  +5.b REPT
;        REPT 回数。
;  +6.w CYCLE_CLK
;        基準クロック数
;        クエリで返される。
;  +8.b REPT_CLK
;        1 REPT あたりのクロック数。
;        クエリで返される。
;  +9.b REPT_MAX
;        REPT に設定できる最大値。
;        クエリで返される。
;
;  +E.w STAT_PTR
;
; PCM
;  +0.b READY
;  +1.b CMD
;  +2.b RESULT
;  +3.b RUN
;  +4.b ENC
;  +6.w PRT1_TIMER
;        PCM >=10(30.72kHz,200clk)
;
;  +E.w STAT_PTR
;
; PSG
;  +0.b READY
;  +1.b CMD
;  +2.b RESULT
;  +3.b RUN
;
; SPK
;  +0.b READY
;  +1.b CMD
;  +2.b RESULT
;  +3.b RUN
;
;  +4.b VOL
;        PSG ボリュームレジスタ値。
;  +6.w FREQ
;        PSG FREQ レジスタ値。
;  +8.w TIME
;        1200Hz 単位の持続時間。
;  +A.w REMAIN
;        内部変数：残り時間。
;
; LPR
;  TBD.
; FDC
;  TBD.
;
; SIO0
;  +0.b READY
;  +1.b CMD
;  +2.b RESULT
;  +3.b RUN
;				; 送受信のバッファリングも検討したが
;				; わりに合わない
;  +4.b TXCMD
;  +5.b TXSTAT
;  +6.b TX
;  +A.b RXCMD
;  +B.b RXSTAT
;  +C.b RX
;
; SIO1
;  +0.b READY
;  +1.b CMD
;  +2.b RESULT
;  +3.b RUN
;				; 送受信のバッファリングも検討したが
;				; わりに合わない
;  +4.b TXCMD
;  +5.b TXSTAT
;  +6.b TX
;  +A.b RXCMD
;  +B.b RXSTAT
;  +C.b RX

	.Z180

; ######## device ID

#define DEVID_XPBUS	0
#define DEVID_TIME	1
#define DEVID_PAM	2
#define DEVID_PCM	3
#define DEVID_PSG	4
#define DEVID_SPK	5
#define DEVID_LPR	6
#define DEVID_FDC	7
#define DEVID_SIO0	8
#define DEVID_SIO1	9
; ######## define

#define PAM_CMD_START	1
#define PAM_CMD_QUERY	2

#define PAM_ENC_PAM2A	1
#define PAM_ENC_PAM2B	2
#define PAM_ENC_PAM3A	3
#define PAM_ENC_PAM3B	4
#define PAM_ENC_PAM1P	5

#define PCM_CMD_START	1

#define PCM_ENC_PCM1	1
#define PCM_ENC_PCM2	2
#define PCM_ENC_PCM3	3

#define SPK_CMD_START	1
#define SPK_CMD_STOP	2
#define SPK_CMD_KEEP	3


; #### RESULT
#define XPLX_R_OK		1
#define XPLX_R_ERROR_PARAM	254
#define XPLX_R_UNKNOWN_CMD	255


; ######## switch
; 0 = USE STAT_PTR for userland test mode
; 1 = USE HOSTINTR for kernel (normal)
#define USE_INTR	1

; ######## constants
; xp to host level 1 interrupt port
HOSTINTR1	.EQU	0B0H
; xp to host level 5 interrupt port
HOSTINTR5	.EQU	0A0H

; PAM use HOSTINTR5
PAM_HOSTINTR	.EQU	HOSTINTR5
; PCM use HOSTINTR5
PCM_HOSTINTR	.EQU	HOSTINTR5

; I/O PORT
TMDR0L	.EQU	0CH
TMDR0H	.EQU	0DH
RLDR0L	.EQU	0EH
RLDR0H	.EQU	0FH
TCR	.EQU	10H
TMDR1L	.EQU	14H
TMDR1H	.EQU	15H
RLDR1L	.EQU	16H
RLDR1H	.EQU	17H

PSG_ADR	.EQU	83H		; PSG address (out)
PSG_DAT	.EQU	82H		; data output
PSG_IN	.EQU	83H		; data input (in)

INITIAL_SP:	.EQU	01000H
PRIVATE_SP:	.EQU	0F000H

; ######## macros

ADD_HL_A:	.MACRO
	ADD	A,L
	LD	L,A
	JR	NC,$ + 3
	INC	H
	.ENDM

WAIT3	.MACRO
	NOP
	.ENDM

WAIT4	.MACRO
	LD	A,A
	.ENDM

WAIT6	.MACRO
	NOP
	NOP
	.ENDM

WAIT7	.MACRO
	LD	A,A			; 4+3=7
	NOP
	.ENDM

WAIT8	.MACRO
	LD	A,A			; 4*2=8
	LD	A,A
	.ENDM

WAIT9	.MACRO
	NOP				; 3*3=9
	NOP
	NOP
	.ENDM

WAIT10	.MACRO
	LD	A,A			; 4+3*2=10
	NOP
	NOP
	.ENDM

WAIT11	.MACRO
	LD	A,A			; 4*2+3=11
	LD	A,A
	NOP
	.ENDM

WAIT12	.MACRO
	LD	A,A			; 4*3=12
	LD	A,A
	LD	A,A
	.ENDM

WAIT13	.MACRO
	LD	A,A			; 4+3*3=13
	NOP
	NOP
	NOP
	.ENDM

WAIT16	.MACRO
	LD	A,A
	LD	A,A
	LD	A,A
	LD	A,A
	.ENDM

WAIT17	.MACRO
	LD	A,A			; 4*2+3*3=17
	LD	A,A
	NOP
	NOP
	NOP
	.ENDM

WAIT19	.MACRO
	LD	A,A			; 4*4+3=19
	LD	A,A
	LD	A,A
	LD	A,A
	NOP
	.ENDM

; ######## RESET/RST
	.ORG	0000H
RESET:
	JP	ENTRY

	.ORG	0038H
INT0:
	JP	INTR_INT0

	.ORG	0066H
NMI:
	RETN

	.ORG	0080H
DEBUG0::	.DB	0
DEBUG1::	.DB	0
DEBUG2::	.DB	0
DEBUG3::	.DB	0
DEBUG4::	.DB	0
DEBUG5::	.DB	0
DEBUG6::	.DB	0
DEBUG7::	.DB	0
DEBUG8::	.DB	0
DEBUG9::	.DB	0
DEBUG10::	.DB	0

	.ORG	00FCH
XPLX_MAGIC::			; MAGIC
	.DB	"XPLX"

; ######## shared variables
; XPBUS
	.ORG	0100H
XPLX_VAR_BASE::
XPBUS_READY::
	.DB	0
XPBUS_CMD::
	.DB	0
XPBUS_RESULT::
	.DB	0
XPBUS_RUN::
	.DB	0

XPBUS_STAT_RESET::		; reset count
	.DB	0
	.DB	0,0,0		; reserved

XPBUS_PRT0_TIMER::		; PRT0 TIMER TLDR (devices dispatch)
	.DW	256
XPBUS_INTR1_DEV::		; HOSTINTR1 device
	.DW	0
XPBUS_INTR5_DEV::		; HOSTINTR5 device
	.DW	0

; TIME
	.ORG	0110H
TIME_READY::
	.DB	0
TIME_CMD::
	.DB	0
TIME_RESULT::
	.DB	0
TIME_RUN::
	.DB	0
TIME_TIMECOUNTER::		; timecounter (TBD.)
	.DW	0

; PAM
	.ORG	0120H
PAM_READY::
	.DB	0
PAM_CMD::
	.DB	0
PAM_RESULT::
	.DB	0
PAM_RUN::
	.DB	0

PAM_ENC::
	.DB	0
PAM_REPT::
	.DB	0
PAM_CYCLE_CLK::
	.DW	0
PAM_REPT_CLK::
	.DB	0
PAM_REPT_MAX::
	.DB	0

	.DB	0,0,0,0		; reserved
PAM_STAT_PTR::
	.DW	0

; PCM
	.ORG	0130H
PCM_READY::
	.DB	0
PCM_CMD::
	.DB	0
PCM_RESULT::
	.DB	0
PCM_RUN::
	.DB	0

PCM_ENC::
	.DB	0
	.DB	0		; reserved
PCM_PRT1_TIMER::			; PRT1 TIMER TLDR (PCM)
	.DW	0

	.DB	0,0,0,0,0,0	; reserved
PCM_STAT_PTR::
	.DW	0

; PSG
	.ORG	0140H
PSG_READY::
	.DB	0
PSG_CMD::
	.DB	0
PSG_RESULT::
	.DB	0
PSG_RUN::
	.DB	0

; SPK
	.ORG	0150H
SPK_READY::
	.DB	0
SPK_CMD::
	.DB	0
SPK_RESULT::
	.DB	0
SPK_RUN::
	.DB	0

SPK_VOL::
	.DB	0
	.DB	0		; reserved
SPK_FREQ::
	.DW	0
SPK_TIME::
	.DW	0
SPK_REMAIN::
	.DW	0

; LPR
	.ORG	0160H
LPR_READY::
	.DB	0
LPR_CMD::
	.DB	0
LPR_RESULT::
	.DB	0
LPR_RUN::
	.DB	0
	; TBD.

LPR_CMD_START	.EQU	1

; FDC
	.ORG	0170H
FDC_READY::
	.DB	0
FDC_CMD::
	.DB	0
FDC_RESULT::
	.DB	0
FDC_RUN::
	.DB	0
; TBD.

FDC_CMD_START	.EQU	1

; SIO0
	.ORG	0180H
SIO0_READY::
	.DB	0
SIO0_CMD::
	.DB	0
SIO0_RESULT::
	.DB	0
SIO0_RUN::
	.DB	0

SIO0_TXCMD::
	.DB	0
SIO0_TXSTAT::
	.DB	0
SIO0_TX::
	.DB	0
	.DS	3
SIO0_RXCMD::
	.DB	0
SIO0_RXSTAT::
	.DB	0
SIO0_RX::
	.DB	0

; SIO1
	.ORG	0190H
SIO1_READY::
	.DB	0
SIO1_CMD::
	.DB	0
SIO1_RESULT::
	.DB	0
SIO1_RUN::
	.DB	0

SIO1_TXCMD::
	.DB	0
SIO1_TXSTAT::
	.DB	0
SIO1_TX::
	.DB	0
	.DS	3
SIO1_RXCMD::
	.DB	0
SIO1_RXSTAT::
	.DB	0
SIO1_RX::
	.DB	0


; ######## Bootstrap program
	.ORG	0200H
ENTRY:
	DI
	LD	SP,INITIAL_SP

				; inc reset count
	LD	HL, XPBUS_STAT_RESET
	INC	(HL)

				; initial devices
				; READY=0
	XOR	A
	LD	(XPBUS_READY),A
	LD	(TIME_READY),A
	LD	(PAM_READY),A
	LD	(PCM_READY),A
	LD	(PSG_READY),A
	LD	(SPK_READY),A
	LD	(LPR_READY),A
	LD	(FDC_READY),A
	LD	(SIO0_READY),A
	LD	(SIO1_READY),A

	LD	A,1
	LD	(DEBUG0),A

				; init XP internal devices
				; internal I/O address = 00H - 3FH
	LD	A,00H		; IOA7[7]=0 IOSTP[5]=0
ICR	.EQU	3FH
	OUT0	(ICR),A

				; memory wait = 0
				; I/O wait = 3
				; no DMA
	LD	A,20H		; MWI[76]=0 IWI[54]=2(3wait) DMS[32]=0 DIM[10]=0
DCNTL	.EQU	32H
	OUT0	(DCNTL),A
				; disable refresh
	LD	A,03H		; REFE[7]=0 REFW[6]=0 CYC[10]=3(80state)
RCR	.EQU	36H
	OUT0	(RCR),A

	LD	A,2
	LD	(DEBUG0),A

				; prepare memory map
				; MMU
CBR	.EQU	38H
BBR	.EQU	39H
CBAR	.EQU	3AH
				; Common0: VA=0000H -> PA=00000H SH
				; Bank   : VA=E000H -> PA=28000H PR
				; Common1: VA=F000H -> PA=FF000H IN
	LD	A,0FEH
	OUT0	(CBAR),A
	LD	A,0F0H
	OUT0	(CBR),A
	LD	A,1AH
	OUT0	(BBR),A

	LD	A,3
	LD	(DEBUG0),A

				; internal RAM addressing
				; for no-wait access
				; PA=FxxxxH にしたらノーウェイトになった。
				; PA=0xxxxH だとウェイトが入った。
				; ほかのアドレスは未調査。
				; built-in RAM VA=FE00H PA=FFE00H
	LD	A,0F0H
RMCR	.EQU	51H
	OUT0	(RMCR),A
				; disable external interrupt
				; TODO: if use "Host to XP" interrupt, change here
	LD	A,00H		; TRAP[7]=0 ITE2[2]=0 ITE1[1]=0 ITE0[0]=0
ITC	.EQU	34H
	OUT0	(ITC),A
				; Interrupt Vector Low = E
				; I = FFH
				; Interrupt Vector Address = FFE0H
	LD	A,0E0H
IL	.EQU	33H
	OUT0	(IL),A
	LD	A,0FFH
	LD	I,A
				; interrupt mode 1
	IM	1

	LD	A,4
	LD	(DEBUG0),A

	CALL	INIT_PSG

	; TODO
	; INIT FDC
	; INIT LPR
	; INIT SIO

				; INIT PRT0,1
				; TIE1[5]=TIE0[4]=0
				; TOC1[3]=TOC0[2]=0
				; TDE1[1]=TDE0[0]=0
	LD	A,00H
	OUT0	(TCR),A
				; prepare PRT0
	LD	HL,(XPBUS_PRT0_TIMER)
	OUT0	(RLDR0L),L
	OUT0	(TMDR0L),L
	OUT0	(RLDR0H),H
	OUT0	(TMDR0H),H
				; TIE0, TID0 ON
				; TIE0[4]=1 TDE0[0]=1
	LD	A,11H
	OUT0	(TCR),A

				; copy to private memory
	LD	HL,PROG_ORG
	LD	DE,PRIVATE_RAM
	LD	BC,PROG_ORG_LEN
	LDIR
				; interrupt vector copy to internal memory
	LD	HL,VECTOR_ORG
	LD	DE,VECTOR
	LD	BC,VECTOR_ORG_LEN
	LDIR

	LD	A,5
	LD	(DEBUG0),A
				; jump to XPBUS
	JP	XPBUS

; initialize PSG registers
; break all regs
INIT_PSG:
				; init PSG
				; PSG R0-R6 All 00H
	LD	A,0
	LD	B,7
	LD	C,PSG_DAT
	LD	D,0
PSG_CLEAR_06:
	OUT	(PSG_ADR),A
	OUT	(C),D
	INC	A
	DJNZ	PSG_CLEAR_06
				; PSG mixer
				; tone = off, noise = off
				; IOA, IOB = output
	LD	A,7
	LD	D,0FFH
	OUT	(PSG_ADR),A
	OUT	(C),D
				; PSG volume and envelope
				; PSG R8-R15 all 0
	LD	A,8
	LD	B,8
	LD	D,0
PSG_CLEAR_8F:
	OUT	(PSG_ADR),A
	OUT	(C),D
	INC	A
	DJNZ	PSG_CLEAR_8F
				; TODO: PSG I/O Port
	RET

; ######## buffers
	.PHASE	1000H
PAM_BUF::
PCM_BUF::
	.DEPHASE
	.PHASE 08000H
PAM_BUF_LEN::	.EQU	$-PAM_BUF
PCM_BUF_LEN::	.EQU	$-PCM_BUF
PSG_BUF::
	.DEPHASE
	.PHASE 09000H
PSG_BUF_LEN::	.EQU	$-PSG_BUF
LPR_BUF::
	.DEPHASE
	.PHASE 0A000H
LPR_BUF_LEN::	.EQU	$-LPR_BUF
FDC_BUF::
	.DEPHASE

; ######## private memory program
	.PHASE 0E000H
FDC_BUF_LEN::	.EQU	$-FDC_BUF

PROG_ORG:	.EQU	$$
PRIVATE_RAM:

XPBUS:
	LD	A,6
	LD	(DEBUG0),A

	LD	SP,PRIVATE_SP

				; devices READY=1
	LD	A,1
	LD	(XPBUS_READY),A
	LD	(TIME_READY),A
	LD	(PAM_READY),A
	LD	(PCM_READY),A
	LD	(PSG_READY),A
	LD	(SPK_READY),A
	LD	(LPR_READY),A
	LD	(FDC_READY),A
	LD	(SIO0_READY),A
	LD	(SIO1_READY),A

				; wait for PRT0
	EI
XPBUS_LOOP:
	HALT
	JR	XPBUS_LOOP

INTR_PRT0:
; #### Periodic devices
; 1200Hz
; ここから呼び出される DISPATCH ルーチンは、
; o. A にコマンドが入っている
; o. AF, HL は破壊して良い。
; o. EI 状態で呼ばれることに注意。
; o. EI 状態でリターンすること。
; o. 裏レジスタは PCM 専用。
; o. PAM 以外、0.83 msec 以内にリターンすること。

	PUSH	AF
	PUSH	HL

	LD	A,7
	LD	(DEBUG0),A
				; reset PRT0 interrupt
	IN0	F,(TCR)
	IN0	F,(TMDR0L)
				; first EI, for PRT1
	EI

TIMECOUNTER_INCR:
				; timecounter
	LD	HL,(TIME_TIMECOUNTER)
	INC	HL
	LD	(TIME_TIMECOUNTER),HL

; #### XPBUS devices dispatcher

DEVICES_DISPATCH:
	LD	A,(XPBUS_CMD)
	OR	A
	CALL	NZ,XPBUS_DISPATCH

	LD	A,(PAM_CMD)
	OR	A
	CALL	NZ,PAM_DISPATCH

	LD	A,(PCM_CMD)
	OR	A
	CALL	NZ,PCM_DISPATCH

	LD	A,(PSG_CMD)
	OR	A
	CALL	NZ,PSG_DISPATCH

	LD	A,(SPK_CMD)
	OR	A
	CALL	NZ,SPK_DISPATCH

	LD	A,(LPR_CMD)
	OR	A
	CALL	NZ,LPR_DISPATCH

	LD	A,(FDC_CMD)
	OR	A
	CALL	NZ,FDC_DISPATCH

	LD	A,(SIO0_CMD)
	OR	A
	CALL	NZ,SIO0_DISPATCH

	LD	A,(SIO1_CMD)
	OR	A
	CALL	NZ,SIO1_DISPATCH

	LD	A,8
	LD	(DEBUG0),A

	POP	HL
	POP	AF
	RETI

; #### XPBUS

XPBUS_DISPATCH:
	; not implemented
	XOR	A
	LD	(XPBUS_CMD),A
	LD	A,XPLX_R_UNKNOWN_CMD
	LD	(XPBUS_RESULT),A
	RET

; #### TIME

TIME_DISPATCH:
	; not implemented
	XOR	A
	LD	(TIME_CMD),A
	LD	A,XPLX_R_UNKNOWN_CMD
	LD	(TIME_RESULT),A
	RET

; #### PAM は末尾

; #### PCM driver core

; PCM 割り込みは裏レジスタを専有します。
; メインルーチン側では裏レジスタを使用してはいけません。

; #### PCM play start
PCM_DISPATCH:
	CP	PCM_CMD_START
	JR	Z,PCM_START

	LD	A,XPLX_R_UNKNOWN_CMD
PCM_ERROR:
	LD	(PCM_RESULT),A
	RET

PCM_START:
				; if READY==0 return
	LD	A,(PCM_READY)
	OR	A
	RET	Z
				; check ENC
	LD	A,(PCM_ENC)
	DEC	A
	JR	Z,PCM_START_OK	; PCM1 = 1
	DEC	A
	JR	Z,PCM_START_OK	; PCM2 = 2
	DEC	A
	JR	Z,PCM_START_OK	; PCM3 = 3

	LD	A,XPLX_R_ERROR_PARAM
	JR	PCM_ERROR

PCM_START_OK:
				; A = 0
	LD	(PCM_READY),A
	LD	(PCM_CMD),A


				; prepare vector
	DI
				; set PRT1 vector
	LD	HL,PCM_INTR
	LD	(VEC_PRT1),HL
				; prepare register
	EXX

	CALL	INIT_PSG

				; make interrupt handler
	LD	A,(PCM_ENC)
	DEC	A
	JR	Z,PCM_SET_PCM1
	DEC	A
	JR	Z,PCM_SET_PCM2
PCM_SET_PCM3:
	LD	HL,PCM3
	JR	PCM_SET
PCM_SET_PCM2:
	LD	HL,PCM2
	JR	PCM_SET
PCM_SET_PCM1:
	LD	HL,PCM1
PCM_SET:
	LD	(PCM_INTR_JMP),HL

	LD	HL,PCM_BUF
	LD	BC,0800H + PSG_ADR
	LD	DE,0709H

	EXX

				; TIE1, TDE1 OFF
	IN0	A,(TCR)
	AND	0DDH		; TIE1[5]=0 TDE1[1]=0
	OUT0	(TCR),A
				; prepare PRT1
	LD	HL,(PCM_PRT1_TIMER)
	OUT0	(RLDR1L),L
	OUT0	(RLDR1H),H
	OUT0	(TMDR1L),L
	OUT0	(TMDR1H),H
				; TIE1, TID1 ON
	OR	22H		; TIE1[5]=1 TDE1[5]=1
	OUT0	(TCR),A

	EI

	LD	A,1
	LD	(PCM_RUN),A

	RET



; #### PCM interrupt handler

PCM_INTR:
				; PRT1 interrupt
	EX	AF,AF
	EXX
				; interrupt acknowledge
				; reset PRT1 Interrupt
	IN0	F,(TCR)
	IN0	F,(TMDR1L)

				; ジャンプ先は書き換えられる
PCM_INTR_JMP:	.EQU	$+1
	JP	PCM1

PCM_INTR_NEXT:
	RLCA
	JR	C,PCM_RELOAD
				; inc ptr after reload check
	INC	HL
	RLCA
	JR	C,PCM_STAT
	RLCA
	JR	NC,PCM_NORMAL

; PCM RESET attention
; in: HL = EXIT address
PCM_RESET:
				; PRT1 intr stop
	IN0	A,(TCR)
				; TIE1,TDE1 OFF
	AND	0DDH		; TIE1[5]=0 TDE1[1]=0
	OUT0	(TCR),A
				; PLAY STOP
	XOR	A
	LD	(PCM_RUN),A
	LD	A,XPLX_R_OK
	LD	(PCM_RESULT),A
	LD	(PCM_READY),A

	JR	PCM_EXIT

; PCM common code

PCM_RELOAD:
	LD	HL,PCM_BUF
PCM_STAT:
#if USE_INTR
	OUT	(PCM_HOSTINTR),A
#else
	LD	(PCM_STAT_PTR),HL
#endif
PCM_NORMAL:
PCM_EXIT:
	EXX
	EX	AF,AF
	EI
	RETI

; #### PCM core code

PCM1:
				; PSG REG=8
	OUT	(C),B
				; read attention or CH0
	LD	A,(HL)
	OUT	(PSG_DAT),A
	JP	PCM_INTR_NEXT

PCM2:
	LD	D,(HL)
	INC	HL
	LD	A,(HL)

	OUT	(C),B
	OUT0	(PSG_DAT),D
	OUT	(C),E
	OUT	(PSG_DAT),A
	JP	PCM_INTR_NEXT

PCM3:
	LD	E,(HL)
	INC	HL
	LD	D,(HL)
	INC	HL
	LD	A,(HL)

	PUSH	HL
	LD	HL,090AH
	OUT	(C),B
	OUT0	(PSG_DAT),E
	OUT	(C),H
	OUT0	(PSG_DAT),D
	OUT	(C),L
	OUT	(PSG_DAT),A
	POP	HL
	JP	PCM_INTR_NEXT

; #### SPK
SPK_DISPATCH:
	CP	SPK_CMD_START
	JR	Z,SPK_START
	CP	SPK_CMD_STOP
	JR	Z,SPK_STOP
	CP	SPK_CMD_KEEP
	JR	Z,SPK_KEEP

	LD	A,XPLX_R_UNKNOWN_CMD
	LD	(SPK_RESULT),A
	RET

SPK_START:
	LD	A,(SPK_READY)
	OR	A
	RET	Z

	XOR	A
	LD	(SPK_READY),A
				; next to CMD_KEEP
	LD	A,SPK_CMD_KEEP
	LD	(SPK_CMD),A
	LD	A,1
	LD	(SPK_RUN),A

				; set REMAIN
	LD	HL,(SPK_TIME)
	LD	(SPK_REMAIN),HL

	DI
				; PSG CH3 FREQ
	LD	HL,(SPK_FREQ)
	LD	A,4
	OUT0	(PSG_ADR),A
	OUT0	(PSG_DAT),L
	LD	A,5
	OUT0	(PSG_ADR),A
	OUT0	(PSG_DAT),H
				; PSG CH3 VOL
	LD	A,10
	OUT	(PSG_ADR),A
	LD	A,(SPK_VOL)
	OUT	(PSG_DAT),A
				; save PSG R7
	LD	A,7
	OUT0	(PSG_ADR),A
	IN	A,(PSG_IN)
	LD	(SPK_PSGR7),A
				; PSG CH3 TONE ON
	AND	0FBH
	OUT	(PSG_DAT),A

	JR	SPK_EXIT

SPK_STOP:
	LD	A,(SPK_READY)
	OR	A
	RET	Z

SPK_STOP_CORE:
	XOR	A
	LD	(SPK_READY),A
	LD	(SPK_CMD),A

	DI
				; restore PSG R7
	LD	A,7
	OUT	(PSG_ADR),A
	LD	A,(SPK_PSGR7)
	OUT	(PSG_DAT),A
				; PSG CH3 VOL=0
	LD	A,10
	OUT	(PSG_ADR),A
	XOR	A
	OUT	(PSG_DAT),A

	LD	(SPK_RUN),A

	JR	SPK_EXIT

SPK_KEEP:
				; REMAIN == 0, then stop
	LD	HL,(SPK_REMAIN)
	LD	A,H
	OR	L
	JR	Z,SPK_STOP_CORE

	DEC	HL
	LD	(SPK_REMAIN),HL

SPK_EXIT:
	EI
	LD	A,XPLX_R_OK
	LD	(SPK_RESULT),A
	LD	(SPK_READY),A
	RET

SPK_PSGR7:
	.DB	0

; ######## PSG
PSG_DISPATCH:
	; not implemented
	XOR	A
	LD	(PSG_CMD),A
	LD	A,XPLX_R_UNKNOWN_CMD
	LD	(PSG_RESULT),A
	RET
; ######## LPR
LPR_DISPATCH:
	; not implemented
	XOR	A
	LD	(LPR_CMD),A
	LD	A,XPLX_R_UNKNOWN_CMD
	LD	(LPR_RESULT),A
	RET
; ######## FDC
FDC_DISPATCH:
	; not implemented
	XOR	A
	LD	(FDC_CMD),A
	LD	A,XPLX_R_UNKNOWN_CMD
	LD	(FDC_RESULT),A
	RET

; ######## SIO
SIO0_DISPATCH:
	; not implemented
	XOR	A
	LD	(SIO0_CMD),A
	LD	A,XPLX_R_UNKNOWN_CMD
	LD	(SIO0_RESULT),A
	RET

SIO1_DISPATCH:
	; not implemented
	XOR	A
	LD	(SIO1_CMD),A
	LD	A,XPLX_R_UNKNOWN_CMD
	LD	(SIO1_RESULT),A
	RET

INTR_INT0:
INTR_ASCI0:
INTR_ASCI1:
				; TBD
	EI
	RETI

; #### PAM play start

PAM_DISPATCH:
	CP	PAM_CMD_START
	JR	Z,PAM_START
	CP	PAM_CMD_QUERY
	JR	Z,PAM_QUERY

	XOR	A
	LD	(PAM_CMD),A
	LD	A,XPLX_R_UNKNOWN_CMD
	LD	(PAM_RESULT),A
	RET

; PAM ENC -> PAM Driver MAP address
; OUT: HL = MAP address
; if error, direct return to main routine
PAM_ENC_MAP:
	LD	A,(PAM_ENC)
	OR	A
	JR	Z,PAM_ERROR_ENC
	DEC	A

	CP	PAM_DRIVER_MAP_LEN / 16		; 16 bytes / entry
	JP	NC,PAM_ERROR_ENC

	ADD	A,A		; A *= 16
	ADD	A,A
	ADD	A,A
	ADD	A,A

	LD	HL,PAM_DRIVER_MAP
	ADD_HL_A
	RET

PAM_ERROR_ENC:
	POP	HL		; discard caller PC
PAM_ERROR_PARAM:
	LD	A,XPLX_R_ERROR_PARAM
	LD	(PAM_RESULT),A
	RET			; return to main

PAM_QUERY:
	CALL	PAM_ENC_MAP	; get ENC to MAP

	LD	A,(PAM_READY)
	OR	A
	RET	Z

	XOR	A
	LD	(PAM_READY),A
	LD	(PAM_CMD),A

	PUSH	BC
	PUSH	DE

	LD	BC,12		; MAP offset 12 = CYCLE_CLK
	ADD	HL,BC

				; CYCLE_CLK, REPT_CLK, REPT_MAX
	LD	DE,PAM_CYCLE_CLK
	LD	BC,4
	LDIR

	POP	DE
	POP	BC

	LD	A,XPLX_R_OK
	LD	(PAM_RESULT),A
	LD	(PAM_READY),A
	RET


PAM_START:
	CALL	PAM_ENC_MAP	; get ENC to MAP

	LD	A,15
	ADD_HL_A		; HL points REPT_MAX

	LD	A,(PAM_REPT)
	CP	(HL)
	JR	Z,PAM_START_OK	; == OK
	JR	C,PAM_START_OK	; < OK
	JR	PAM_ERROR_PARAM

PAM_START_OK:
	LD	A,(PAM_READY)
	OR	A
	RET	Z

	XOR	A
	LD	(PAM_READY),A
	LD	(PAM_CMD),A

				; never normal return
				; PAM never EI
	DI
	CALL	INIT_PSG

	CALL	PAM_ENC_MAP	; re- get ENC to MAP

				; copy to internal RAM
	LD	DE,PAM_DRIVER

	LD	SP,HL		; SP = top of Map entry
	POP	HL		; HEAD
	POP	BC		; HEAD_LEN
	LDIR

	LD	A,(PAM_REPT)
	INC	A		; DEC is not change CY


PAM_REPT_LOOP:
	POP	HL		; REPT
	POP	BC		; REPT_LEN

	DEC	A		; DEC is not change CY
	JR	Z,PAM_REPT_END

	LDIR

	DEC	SP
	DEC	SP
	DEC	SP
	DEC	SP
	JR	PAM_REPT_LOOP
PAM_REPT_END:

	POP	HL		; TAIL
	POP	BC		; TAIL_LEN
	LDIR

				; buffer pointer
	LD	HL,PAM_BUF
#if USE_INTR
#else
	LD	(PAM_STAT_PTR),HL
#endif
				; prefetch
	LD	SP,HL			; 4
	POP	DE

; I/O WAIT 3 -> 2
; PSG の address / write 時間 は 300ns なので、
; 1.8432 clock あればよいので 1 wait から設定できるはずだが、
; 2 wait に設定すると out 命令が 12 clock となり、
; 共有メモリに対する POP の 9+3=12 clock と一致して
; クロック整合を取りやすくなるため、2 wait に設定する。
; なお PSG の read は 400ns 必要なため、2 wait だとあやしい。
; また HOSTINTR 用の I/O への out で wait を満足するかどうかは
; 未確定だけど、HOSTINTR 機構はデータバスの値ではなく
; アドレスへの出力で動作するため、ウェイトに関係なく動作すると
; 期待してみる。
	LD	A,10H		; IWI[54]=1(2wait)
	OUT0	(DCNTL),A

	LD	A,1
	LD	(PAM_RUN),A

	LD	A,8
	OUT	(PSG_ADR),A
	LD	C,PSG_DAT

	JP	PAM_DRIVER

PAM_RESET:
				; XPBUS に制御を戻す
	LD	SP,PRIVATE_SP

; I/O WAIT 2 -> 3
	LD	A,20H		; IWI[54]=2(3wait)
	OUT0	(DCNTL),A

	CALL	INIT_PSG

	XOR	A
	LD	(PAM_RUN),A

	LD	A,XPLX_R_OK
	LD	(PAM_RESULT),A
	LD	(PAM_READY),A

	JP	XPBUS

PAM_DRIVER_MAP:
				; 16 bytes / entry
	DW	PAM2A_HEAD_ORG
	DW	PAM2A_HEAD_LEN
	DW	PAM2A_REPT_ORG
	DW	PAM2A_REPT_LEN
	DW	PAM2A_TAIL_ORG
	DW	PAM2A_TAIL_LEN
	DW	204		;CYCLE_CLK
	DB	36		;REPT_CLK
	DB	37		;REPT_MAX

	DW	PAM2B_HEAD_ORG
	DW	PAM2B_HEAD_LEN
	DW	PAM2B_REPT_ORG
	DW	PAM2B_REPT_LEN
	DW	PAM2B_TAIL_ORG
	DW	PAM2B_TAIL_LEN
	DW	152		;CYCLE_CLK
	DB	24		;REPT_CLK
	DB	57		;REPT_MAX

	DW	PAM3A_HEAD_ORG
	DW	PAM3A_HEAD_LEN
	DW	PAM3A_REPT_ORG
	DW	PAM3A_REPT_LEN
	DW	PAM3A_TAIL_ORG
	DW	PAM3A_TAIL_LEN
	DW	298		;CYCLE_CLK
	DB	51		;REPT_CLK
	DB	24		;REPT_MAX

	DW	PAM3B_HEAD_ORG
	DW	PAM3B_HEAD_LEN
	DW	PAM3B_REPT_ORG
	DW	PAM3B_REPT_LEN
	DW	PAM3B_TAIL_ORG
	DW	PAM3B_TAIL_LEN
	DW	136		;CYCLE_CLK
	DB	36		;REPT_CLK
	DB	38		;REPT_MAX
	


PAM_DRIVER_MAP_LEN:	.EQU	$-PAM_DRIVER_MAP

	.DEPHASE



; ######## PAM drivers
	.PHASE 0FE00H
				; all PAM drivers have same address=0FE00H
PAM_DRIVER:
	.DEPHASE

; #### PAM2A

	.PHASE 0FE00H
PAM2A_HEAD_ORG:	.EQU	$$
PAM2A_HEAD:
PAM2A:
				; PAM2A
				; 12+0:12+12 = 1:2 PAM
				; PAM 36clk 170.667kHz
				; output PAM wave = normal 5 + antinoise 1

				; 1 PAM cycle = 204 clk

				; 6.144E6 / (204 + 36*n)

				; sampling freqs:
				;  0: 30118
				; 37:  4000

				; no STAT for first time
	JP	PAM2A_LOOP

PAM2A_RELOAD:
	OUT	(C),E
	OUT	(C),D
	LD	SP,PAM_BUF		;9
	WAIT3

PAM2A_STAT:
#if USE_INTR
	OUT	(C),E
	OUT	(C),D
	OUT	(PAM_HOSTINTR),A		;10+2
#else
				; STAT_PTR モードでの遅れはしょうがない
	OUT	(C),E
	OUT	(C),D
	LD	(PAM_STAT_PTR),SP		;19+3
#endif

PAM2A_NORMAL:
	OUT	(C),E
	OUT	(C),D
				; prefetch
	POP	DE			;9+3

	OUT	(C),L
	OUT	(C),H
				; うまくいくかはわからない
				; 本来 wait 12 だが PAM 遷移ノイズを
				; 低減するため待たない
PAM2A_LOOP:
				; prefetched DE
	OUT	(C),E
	OUT	(C),D
				; HL = DE for save current sample
	LD	L,E			;4
	LD	H,D			;4
				; A = attention
	LD	A,E			;4

PAM2A_HEAD_LEN:	.EQU	$-PAM2A_HEAD

PAM2A_REPT_ORG:	.EQU	$$
PAM2A_REPT:
	OUT	(C),E
	OUT	(C),D
	WAIT12
PAM2A_REPT_LEN:	.EQU	$-PAM2A_REPT

PAM2A_TAIL_ORG:	.EQU	$$
PAM2A_TAIL:
				; このブロックは動的再配置されるので
				; このブロック"への"ジャンプは困難
				; "からの"ジャンプは可能。
	OUT	(C),E
	OUT	(C),D
	RLCA
				; attention bit
				; bit7=1, reload
				; must be JP
	JP	C,PAM2A_RELOAD		; jump=9 no=6

	WAIT3
	OUT	(C),E
	OUT	(C),D
	RLCA				; 3
				; bit6=1, stat
				; must be JP
	JP	C,PAM2A_STAT		; jump=9 no=6

	WAIT3
	OUT	(C),E
	OUT	(C),D
	RLCA				; 3
				; bit5=0, normal
				; must be JP
	JP	NC,PAM2A_NORMAL		; jump=9 no=6
				; attention=001, reset
	JP	PAM_RESET
PAM2A_TAIL_LEN:	.EQU	$-PAM2A_TAIL

				; cycle
				; 5 * (12*3) + 12*2 = 204

	.DEPHASE

; #### PAM2B

	.PHASE 0FE00H
				; all PAM drivers have same address=0FE00H
PAM2B_HEAD_ORG:	.EQU	$$
PAM2B_HEAD:
PAM2B:
				; PAM2B
				; averaged 1:1 PAM
				; wait (4,7), (3,9), (9,12), (12,0)
				; phase wait 28:28
				; clk  35, 36, 45, 36
				; PAM 176, 171, 137, 171 kHz
				; output PAM wave = 4

				; 1 PAM cycle = 152 clk

				; 6.144E6 / (152 + 24*n)

				; sampling freqs:
				;  0: 40421
				; 57:  4042

				; no STAT for first time
	JP	PAM2B_LOOP

PAM2B_RELOAD:
	OUT	(C),E
	LD	SP,PAM_BUF		;9

PAM2B_STAT:
#if USE_INTR
	OUT	(C),D
	OUT	(PAM_HOSTINTR),A		;10+2
#else
				; STAT_PTR モードでの遅れはしょうがない
	OUT	(C),D
	LD	(PAM_STAT_PTR),SP		;19+3
#endif

PAM2B_NORMAL:
	OUT	(C),E
				; prefetch
	POP	DE			;9+3
	OUT	(C),B
PAM2B_LOOP:
				; prefetched DE
	OUT	(C),E
				; A = attention
	LD	A,E			;4
	OUT	(C),D
				; B = save D
	LD	B,D			;4
	WAIT3

PAM2B_HEAD_LEN:	.EQU	$-PAM2B_HEAD

PAM2B_REPT_ORG:	.EQU	$$
PAM2B_REPT:
	OUT	(C),E
	OUT	(C),D
PAM2B_REPT_LEN:	.EQU	$-PAM2B_REPT

PAM2B_TAIL_ORG:	.EQU	$$
PAM2B_TAIL:
				; このブロックは動的再配置されるので
				; このブロック"への"ジャンプは困難
				; "からの"ジャンプは可能。
	OUT	(C),E
	RLCA				;3
	OUT	(C),D
				; attention bit
				; bit7=1, reload
				; must be JP
	JP	C,PAM2B_RELOAD		; jump=9 no=6

	RLCA				; 3
	OUT	(C),E
				; bit6=1, stat
				; must be JP
	JP	C,PAM2B_STAT		; jump=9 no=6

	RLCA				; 3
	OUT	(C),D
	WAIT3
				; bit5=0, normal
				; must be JP
	JP	NC,PAM2B_NORMAL		; jump=9 no=6
				; attention=001, reset
	JP	PAM_RESET
PAM2B_TAIL_LEN:	.EQU	$-PAM2B_TAIL

				; cycle
				; 4 * 12*2 + (4+7 + 3+9 + 9+12 + 12+0) = 152

	.DEPHASE

; #### PAM3A

	.PHASE 0FE00H
PAM3A_HEAD_ORG:	.EQU	$$
PAM3A_HEAD:
PAM3A:
				; PAM3A
				; 12+0:12+3:12+12 = 4:5:8 PAM
				; PAM 51clk 120.471kHz
				; output PAM wave = normal 5 + antinoise 1

				; 1 PAM cycle = 298 clk

				; 6.144E6 / (298 + 51*n)

				; sampling freqs:
				; 0: 20617
				; 24: 4037

				; prefetch
	POP	AF
	LD	B,A
				; no STAT for first time
	JP	PAM3A_LOOP

PAM3A_RELOAD:
	OUT	(C),L
	OUT	(C),H
	WAIT3
	OUT	(C),B
	LD	SP,PAM_BUF		;9
	WAIT3

PAM3A_STAT:
#if USE_INTR
	OUT	(C),L
	OUT	(C),H
	WAIT3
	OUT	(C),B
	OUT	(PAM_HOSTINTR),A		;10+2
#else
				; STAT_PTR モードでの遅れはしょうがない
	OUT	(C),L
	OUT	(C),H
	WAIT3
	OUT	(C),B
	LD	(PAM_STAT_PTR),SP		;19+3
#endif

PAM3A_NORMAL:
	OUT	(C),L
	OUT	(C),H
	WAIT3
	OUT	(C),B
				; prefetch
	POP	DE			;9+3

	OUT	(C),L
	OUT	(C),H
	WAIT3
	OUT	(C),B
				; prefetch
	POP	AF			;9+3

	OUT	(C),L
	OUT	(C),H
	WAIT3
	OUT	(C),B
				; うまくいくかはわからない
				; 本来 wait 12 だが PAM 遷移ノイズを
				; 低減するのも含めて4clkだけ消費する
	LD	B,A			;4
PAM3A_LOOP:
				; prefetched DE, A=B

PAM3A_HEAD_LEN:	.EQU	$-PAM3A_HEAD

PAM3A_REPT_ORG:	.EQU	$$
PAM3A_REPT:
	OUT	(C),E
	OUT	(C),D
	WAIT3
	OUT	(C),B
	WAIT12
PAM3A_REPT_LEN:	.EQU	$-PAM3A_REPT

PAM3A_TAIL_ORG:	.EQU	$$
PAM3A_TAIL:
				; このブロックは動的再配置されるので
				; このブロック"への"ジャンプは困難
				; "からの"ジャンプは可能。
	OUT	(C),E
	OUT	(C),D
	EX	DE,HL			;3
	OUT	(C),B
	RLCA
				; attention bit
				; bit7=1, reload
				; must be JP
	JP	C,PAM3A_RELOAD		; jump=9 no=6

	WAIT3
	OUT	(C),L
	OUT	(C),H
	WAIT3
	OUT	(C),B
	RLCA				; 3
				; bit6=1, stat
				; must be JP
	JP	C,PAM3A_STAT		; jump=9 no=6

	WAIT3
	OUT	(C),L
	OUT	(C),H
	WAIT3
	OUT	(C),B
	RLCA				; 3
				; bit5=0, normal
				; must be JP
	JP	NC,PAM3A_NORMAL		; jump=9 no=6
				; attention=001, reset
	JP	PAM_RESET
PAM3A_TAIL_LEN:	.EQU	$-PAM3A_TAIL

				; cycle
				; 5 * (12*3+3+12) + (12*3+3+4) = 298

	.DEPHASE

; #### PAM3B

	.PHASE 0FE00H
PAM3B_HEAD_ORG:	.EQU	$$
PAM3B_HEAD:
PAM3B:
				; PAM3B
				; approx 1:1:1
				; wait (9,9,12), (12,12,10)
				; phase wait 21:21:22
				; clk 66, 70
				; PAM 93, 88 kHz
				; output PAM wave = 2

				; 1 PAM cycle = 136 clk

				; 6.144E6 / (136 + 36*n)

				; sampling freqs:
				; 0: 45176
				; 38: 4085

				; prefetch
	POP	AF
	LD	B,A
	RLCA
				; no STAT for first time
	JP	PAM3B_LOOP

PAM3B_RELOAD:
	OUT	(C),D
	LD	SP,PAM_BUF		;9

PAM3B_STAT:
#if USE_INTR
	OUT	(C),B
	OUT	(PAM_HOSTINTR),A		;10+2
#else
				; STAT_PTR モードでの遅れはしょうがない
	OUT	(C),B
	LD	(PAM_STAT_PTR),SP		;19+3
#endif

PAM3B_NORMAL:
	OUT	(C),E
				; prefetch
	POP	HL			;9+3

	OUT	(C),D
				; prefetch
	POP	AF			;9+3

	OUT	(C),B
	EX	DE,HL			;3
	LD	B,A			;4
	RLCA				;3
PAM3B_LOOP:
				; prefetched DE,B A=RLCA-ed flag

PAM3B_HEAD_LEN:	.EQU	$-PAM3B_HEAD

PAM3B_REPT_ORG:	.EQU	$$
PAM3B_REPT:
	OUT	(C),E
	OUT	(C),D
	OUT	(C),B
PAM3B_REPT_LEN:	.EQU	$-PAM3B_REPT

PAM3B_TAIL_ORG:	.EQU	$$
PAM3B_TAIL:
				; このブロックは動的再配置されるので
				; このブロック"への"ジャンプは困難
				; "からの"ジャンプは可能。
	OUT	(C),E
				; attention bit
				; bit7=1, reload
				; must be JP
	JP	C,PAM3B_RELOAD		; jump=9 no=6

	RLCA				; 3
	OUT	(C),D
				; bit6=1, stat
				; must be JP
	JP	C,PAM3B_STAT		; jump=9 no=6

	RLCA				; 3
	OUT	(C),B
	WAIT3
				; bit5=0, normal
				; must be JP
	JP	NC,PAM3B_NORMAL		; jump=9 no=6
				; attention=001, reset
	JP	PAM_RESET
PAM3B_TAIL_LEN:	.EQU	$-PAM3B_TAIL


	.DEPHASE

; #### PAM1P

	.PHASE	0FE00H
PAM1P_HEAD_ORG:	.EQU	$$
PAM1P_HEAD:
PAM1P:
				; PAM1P
				; PAM1P は正確にはPCMだが
				; 動作方式はPAMに近いためこちら。
				; Polyphase PCM

				; 1 cycle = 87 clk
				; 6.144E6 / (87 + 3*n)

				; sampling freqs:
				; 0: 70621
				; 255: 7420

	LD	HL,PAM_BUF		;9

	LD	C,PSG_ADR
				; initial CH0
	LD	A,8
	OUT	(PSG_ADR),A
				; rotated next CH
	LD	B,9
	LD	DE,080AH

				; no STAT for first time
	JP	PAM1P_LOOP

PAM1P_RELOAD:
	LD	HL,PAM_BUF		;9

PAM1P_STAT:
#if USE_INTR
	OUT	(PAM_HOSTINTR),A		;10+2
#else
				; STAT_PTR モードでの遅れはしょうがない
	LD	(PAM_STAT_PTR),HL		;16+3
#endif

PAM1P_NORMAL:
				; rotate B,E,D
	LD	A,B			;4
	LD	B,E			;4
	LD	E,D			;4
	LD	D,A			;4
	OUT	(C),B			;10+2

PAM1P_LOOP:

	LD	A,(HL)			;6+3
	INC	HL			;4

	OUT	(PSG_DAT),A			;10+2

PAM1P_HEAD_LEN:	.EQU	$-PAM1P_HEAD

PAM1P_REPT_ORG:	.EQU	$$
PAM1P_REPT:
	WAIT3
PAM1P_REPT_LEN:	.EQU	$-PAM1P_REPT

PAM1P_TAIL_ORG:	.EQU	$$
PAM1P_TAIL:
				; このブロックは動的再配置されるので
				; このブロック"への"ジャンプは困難
				; "からの"ジャンプは可能。
	RLCA				;3
				; attention bit
				; bit7=1, reload
				; must be JP
	JP	C,PAM1P_RELOAD		; jump=9 no=6

	RLCA				; 3
				; bit6=1, stat
				; must be JP
	JP	C,PAM1P_STAT		; jump=9 no=6

	RLCA				; 3
	WAIT3
				; bit5=0, normal
				; must be JP
	JP	NC,PAM1P_NORMAL		; jump=9 no=6
				; attention=001, reset
	JP	PAM_RESET
PAM1P_TAIL_LEN:	.EQU	$-PAM1P_TAIL

				; cycle
				; 63 + 12 + 12 = 87

	.DEPHASE

PROG_ORG_LEN:	.EQU	$$-PROG_ORG

; #### interrupt vector
	.PHASE	0FFE0H
VECTOR_ORG:	.EQU	$$
VECTOR:

VEC_INT1:
	DW	INTR_IGN
VEC_INT2:
	DW	INTR_IGN
VEC_PRT0:
	DW	INTR_PRT0
VEC_PRT1:
	DW	INTR_IGN
VEC_DMAC0:
	DW	INTR_IGN
VEC_DMAC1:
	DW	INTR_IGN
VEC_SIO:
	DW	INTR_IGN
VEC_ASCI0:
	DW	INTR_ASCI0
VEC_ASCI1:
	DW	INTR_ASCI1
VEC_PT2IN:
	DW	INTR_IGN
VEC_PT2OUT:
	DW	INTR_IGN
VEC_PT2OVF:
	DW	INTR_IGN
			; 本当はここはベクタテーブルだが
			; 使われることはないので押し込む。
INTR_IGN:
	EI
	RETI

VECTOR_ORG_LEN:	.EQU	$$-VECTOR_ORG

	.DEPHASE
XPLX_FIRMWARE_LEN::	.EQU	$
