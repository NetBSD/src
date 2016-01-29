	.syntax unified
	.arch armv8-a
	.arch_extension pan

A1:     
	.arm
        setpan #0
        setpan #1

T1:     
        .thumb
        setpan #0
        setpan #1

