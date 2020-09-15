	.macro csr val
	csrw \val, a1
	.endm

       # Supported the current priv spec 1.11.
	csr ustatus
	csr uie
	csr utvec

	csr uscratch
	csr uepc
	csr ucause
	csr utval               # Added in 1.10
	csr uip

	csr fflags
	csr frm
	csr fcsr

	csr cycle
	csr time
	csr instret
	csr hpmcounter3
	csr hpmcounter4
	csr hpmcounter5
	csr hpmcounter6
	csr hpmcounter7
	csr hpmcounter8
	csr hpmcounter9
	csr hpmcounter10
	csr hpmcounter11
	csr hpmcounter12
	csr hpmcounter13
	csr hpmcounter14
	csr hpmcounter15
	csr hpmcounter16
	csr hpmcounter17
	csr hpmcounter18
	csr hpmcounter19
	csr hpmcounter20
	csr hpmcounter21
	csr hpmcounter22
	csr hpmcounter23
	csr hpmcounter24
	csr hpmcounter25
	csr hpmcounter26
	csr hpmcounter27
	csr hpmcounter28
	csr hpmcounter29
	csr hpmcounter30
	csr hpmcounter31
	csr cycleh
	csr timeh
	csr instreth
	csr hpmcounter3h
	csr hpmcounter4h
	csr hpmcounter5h
	csr hpmcounter6h
	csr hpmcounter7h
	csr hpmcounter8h
	csr hpmcounter9h
	csr hpmcounter10h
	csr hpmcounter11h
	csr hpmcounter12h
	csr hpmcounter13h
	csr hpmcounter14h
	csr hpmcounter15h
	csr hpmcounter16h
	csr hpmcounter17h
	csr hpmcounter18h
	csr hpmcounter19h
	csr hpmcounter20h
	csr hpmcounter21h
	csr hpmcounter22h
	csr hpmcounter23h
	csr hpmcounter24h
	csr hpmcounter25h
	csr hpmcounter26h
	csr hpmcounter27h
	csr hpmcounter28h
	csr hpmcounter29h
	csr hpmcounter30h
	csr hpmcounter31h

	csr sstatus
	csr sedeleg
	csr sideleg
	csr sie
	csr stvec
	csr scounteren          # Added in 1.10

	csr sscratch
	csr sepc
	csr scause
	csr stval               # Added in 1.10
	csr sip

	csr satp                # Added in 1.10

	csr mvendorid
	csr marchid
	csr mimpid
	csr mhartid

	csr mstatus
	csr misa                # 0xf10 in 1.9, but changed to 0x301 since 1.9.1.
	csr medeleg
	csr mideleg
	csr mie
	csr mtvec
	csr mcounteren          # Added in 1.10

	csr mscratch
	csr mepc
	csr mcause
	csr mtval               # Added in 1.10
	csr mip

	csr pmpcfg0             # Added in 1.10
	csr pmpcfg1             # Added in 1.10
	csr pmpcfg2             # Added in 1.10
	csr pmpcfg3             # Added in 1.10
	csr pmpaddr0            # Added in 1.10
	csr pmpaddr1            # Added in 1.10
	csr pmpaddr2            # Added in 1.10
	csr pmpaddr3            # Added in 1.10
	csr pmpaddr4            # Added in 1.10
	csr pmpaddr5            # Added in 1.10
	csr pmpaddr6            # Added in 1.10
	csr pmpaddr7            # Added in 1.10
	csr pmpaddr8            # Added in 1.10
	csr pmpaddr9            # Added in 1.10
	csr pmpaddr10           # Added in 1.10
	csr pmpaddr11           # Added in 1.10
	csr pmpaddr12           # Added in 1.10
	csr pmpaddr13           # Added in 1.10
	csr pmpaddr14           # Added in 1.10
	csr pmpaddr15           # Added in 1.10

	csr mcycle
	csr minstret
	csr mhpmcounter3
	csr mhpmcounter4
	csr mhpmcounter5
	csr mhpmcounter6
	csr mhpmcounter7
	csr mhpmcounter8
	csr mhpmcounter9
	csr mhpmcounter10
	csr mhpmcounter11
	csr mhpmcounter12
	csr mhpmcounter13
	csr mhpmcounter14
	csr mhpmcounter15
	csr mhpmcounter16
	csr mhpmcounter17
	csr mhpmcounter18
	csr mhpmcounter19
	csr mhpmcounter20
	csr mhpmcounter21
	csr mhpmcounter22
	csr mhpmcounter23
	csr mhpmcounter24
	csr mhpmcounter25
	csr mhpmcounter26
	csr mhpmcounter27
	csr mhpmcounter28
	csr mhpmcounter29
	csr mhpmcounter30
	csr mhpmcounter31
	csr mcycleh
	csr minstreth
	csr mhpmcounter3h
	csr mhpmcounter4h
	csr mhpmcounter5h
	csr mhpmcounter6h
	csr mhpmcounter7h
	csr mhpmcounter8h
	csr mhpmcounter9h
	csr mhpmcounter10h
	csr mhpmcounter11h
	csr mhpmcounter12h
	csr mhpmcounter13h
	csr mhpmcounter14h
	csr mhpmcounter15h
	csr mhpmcounter16h
	csr mhpmcounter17h
	csr mhpmcounter18h
	csr mhpmcounter19h
	csr mhpmcounter20h
	csr mhpmcounter21h
	csr mhpmcounter22h
	csr mhpmcounter23h
	csr mhpmcounter24h
	csr mhpmcounter25h
	csr mhpmcounter26h
	csr mhpmcounter27h
	csr mhpmcounter28h
	csr mhpmcounter29h
	csr mhpmcounter30h
	csr mhpmcounter31h

	csr mcountinhibit       # Added in 1.11
	csr mhpmevent3
	csr mhpmevent4
	csr mhpmevent5
	csr mhpmevent6
	csr mhpmevent7
	csr mhpmevent8
	csr mhpmevent9
	csr mhpmevent10
	csr mhpmevent11
	csr mhpmevent12
	csr mhpmevent13
	csr mhpmevent14
	csr mhpmevent15
	csr mhpmevent16
	csr mhpmevent17
	csr mhpmevent18
	csr mhpmevent19
	csr mhpmevent20
	csr mhpmevent21
	csr mhpmevent22
	csr mhpmevent23
	csr mhpmevent24
	csr mhpmevent25
	csr mhpmevent26
	csr mhpmevent27
	csr mhpmevent28
	csr mhpmevent29
	csr mhpmevent30
	csr mhpmevent31

	csr tselect
	csr tdata1
	csr tdata2
	csr tdata3

	csr dcsr
	csr dpc
	csr dscratch0           # Added in 1.11
	csr dscratch1           # Added in 1.11

	# Supported in previous priv spec, but dropped now.
	csr ubadaddr            # 0x043 in 1.9.1, but the value is utval since 1.10
	csr sbadaddr            # 0x143 in 1.9.1, but the value is stval since 1.10
	csr sptbr               # 0x180 in 1.9.1, but the value is satp since 1.10
	csr mbadaddr            # 0x343 in 1.9.1, but the value is mtval since 1.10
	csr mucounteren         # 0x320 in 1.9.1, dropped in 1.10, but the value is mcountinhibit since 1.11
	csr dscratch            # 0x7b2 in 1.10,  but the value is dscratch0 since 1.11

	csr hstatus             # 0x200, dropped in 1.10
	csr hedeleg             # 0x202, dropped in 1.10
	csr hideleg             # 0x203, dropped in 1.10
	csr hie                 # 0x204, dropped in 1.10
	csr htvec               # 0x205, dropped in 1.10
	csr hscratch            # 0x240, dropped in 1.10
	csr hepc                # 0x241, dropped in 1.10
	csr hcause              # 0x242, dropped in 1.10
	csr hbadaddr            # 0x243, dropped in 1.10
	csr hip                 # 0x244, dropped in 1.10
	csr mbase               # 0x380, dropped in 1.10
	csr mbound              # 0x381, dropped in 1.10
	csr mibase              # 0x382, dropped in 1.10
	csr mibound             # 0x383, dropped in 1.10
	csr mdbase              # 0x384, dropped in 1.10
	csr mdbound             # 0x385, dropped in 1.10
	csr mscounteren         # 0x321, dropped in 1.10
	csr mhcounteren         # 0x322, dropped in 1.10
