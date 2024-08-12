# mach: all
# output:
# sim: --environment operating

.include "t-macros.i"

	start

	;; clear FX
	loadpsw2 0x8005
	loadacc2 a1 0x7f 0xffff 0xffff
	load r8 0xffff
	load r9 0x8001
test_macu1:
	MACU a1, r9, r8
	checkacc2 1 a1 0x80 0x8000 0x7FFE

	;; set FX
	loadpsw2 0x8085
	loadacc2 a1 0x7f 0xffff 0xffff
	load r8 0xffff
	load r9 0x8001
test_macu2:
	MACU a1, r9, r8
	checkacc2 2 a1 0x81 0x0000 0xfffd




	;; clear FX
	ldi r2, #0x8005
	mvtc r2, cr0

	loadacc2 a1 0x7f 0xffff 0xffff
	ldi r8, #0xffff
	ldi r9, #0x7FFF
test_macsu1:
	MACSU a1, r9, r8
	checkacc2 3 a1 0x80 0x7FFE 0x8000

	;; set FX
	ldi r2, #0x8085
	mvtc r2, cr0

	loadacc2 a1 0x7f 0xffff 0xffff
	ldi r8, #0xffff
	ldi r9, #0x7FFF
test_macsu2:
	MACSU a1, r9, r8
	checkacc2 4 a1 0x80 0xfffd 0x0001

	;; clear FX
	ldi r2, #0x8005
	mvtc r2, cr0

	loadacc2 a1 0x7f 0xffff 0xffff
	ldi r8, 0xffff
	ldi r9, 0x8001
test_macsu3:
	MACSU a1, r9, r8
	checkacc2 5 a1 0x7F 0x8001 0x7FFE

	;; set FX
	ldi r2, #0x8085
	mvtc r2, cr0

	loadacc2 a1 0x7f 0xffff 0xffff
	ldi r8, #0xffff
	ldi r9, #0x8001
test_macsu4:
	MACSU a1, r9, r8
	checkacc2 6 a1 0x7f 0x0002 0xFFFD

	exit0

