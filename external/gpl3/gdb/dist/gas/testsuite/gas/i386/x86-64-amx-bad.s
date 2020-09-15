.text
	#tdpbf16ps %tmm5,%tmm4,%tmm3 set VEX.W = 1 (illegal value).
	.byte 0xc4
	.byte 0xe2
	.byte 0xd2
	.byte 0x5c
	.byte 0xdc
	.fill 0x05, 0x01, 0x90
	#tdpbf16ps %tmm5,%tmm4,%tmm3 set VEX.L = 1 (illegal value).
	.byte 0xc4
	.byte 0xe2
	.byte 0x56
	.byte 0x5c
	.byte 0xdc
	.fill 0x05, 0x01, 0x90
	#tdpbf16ps %tmm5,%tmm4,%tmm3 set VEX.R = 0 (illegal value).
	.byte 0xc4
	.byte 0x62
	.byte 0x52
	.byte 0x5c
	.byte 0xdc
	#tdpbf16ps %tmm5,%tmm4,%tmm3 set VEX.B = 0 (illegal value).
	.byte 0xc4
	.byte 0xc2
	.byte 0x52
	.byte 0x5c
	.byte 0xdc
	#tdpbf16ps %tmm5,%tmm4,%tmm3 set VEX.VVVV = 0110 (illegal value).
	.byte 0xc4
	.byte 0xe2
	.byte 0x32
	.byte 0x5c
	.byte 0xdc
	#tileloadd (%rax),%tmm1 set R/M= 001 (illegal value) without SIB.
	.byte 0xc4
	.byte 0xe2
	.byte 0x7b
	.byte 0x4b
	.byte 0x09
	#tdpbuud %tmm1,%tmm1,%tmm1 All 3 TMM registers can't be identical.
	.byte 0xc4
	.byte 0xe2
	.byte 0x70
	.byte 0x5e
	.byte 0xc9
	#tdpbuud %tmm0,%tmm1,%tmm1 All 3 TMM registers can't be identical.
	.byte 0xc4
	.byte 0xe2
	.byte 0x78
	.byte 0x5e
	.byte 0xc9
	#tdpbuud %tmm1,%tmm0,%tmm1 All 3 TMM registers can't be identical.
	.byte 0xc4
	.byte 0xe2
	.byte 0x70
	.byte 0x5e
	.byte 0xc8
	#tdpbuud %tmm1,%tmm1,%tmm0 All 3 TMM registers can't be identical.
	.byte 0xc4
	.byte 0xe2
	.byte 0x70
	.byte 0x5e
	.byte 0xc1
