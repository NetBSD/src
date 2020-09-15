#as: -march=rv32ifc
#objdump: -dr

.*:[ 	]+file format .*


Disassembly of section .text:

0+000 <target>:
[^:]+:[ 	]+00c58533[ 	]+add[ 	]+a0,a1,a2
[^:]+:[ 	]+00d58513[ 	]+addi[ 	]+a0,a1,13
[^:]+:[ 	]+00a58567[ 	]+jalr[ 	]+a0,10\(a1\)
[^:]+:[ 	]+00458503[ 	]+lb[ 	]+a0,4\(a1\)
[^:]+:[ 	]+feb508e3[ 	]+beq[ 	]+a0,a1,0 \<target\>
[^:]+: R_RISCV_BRANCH[	]+target
[^:]+:[ 	]+feb506e3[ 	]+beq[ 	]+a0,a1,0 \<target\>
[^:]+: R_RISCV_BRANCH[	]+target
[^:]+:[ 	]+00a58223[ 	]+sb[ 	]+a0,4\(a1\)
[^:]+:[ 	]+00fff537[ 	]+lui[ 	]+a0,0xfff
[^:]+:[ 	]+fe1ff56f[ 	]+jal[ 	]+a0,0 \<target\>
[^:]+: R_RISCV_JAL[	]+target
[^:]+:[ 	]+fddff56f[ 	]+jal[ 	]+a0,0 \<target\>
[^:]+: R_RISCV_JAL[	]+target
[^:]+:[ 	]+0511[ 	]+addi[ 	]+a0,a0,4
[^:]+:[ 	]+852e[ 	]+mv[ 	]+a0,a1
[^:]+:[ 	]+002c[ 	]+addi[ 	]+a1,sp,8
[^:]+:[ 	]+d9e9[ 	]+beqz[ 	]+a1,0 \<target\>
[^:]+: R_RISCV_RVC_BRANCH[	]+target
[^:]+:[ 	]+bfc1[ 	]+j[ 	]+0 \<target\>
[^:]+: R_RISCV_RVC_JUMP[	]+target
[^:]+:[ 	]+00c58533[ 	]+add[ 	]+a0,a1,a2
[^:]+:[ 	]+00d58513[ 	]+addi[ 	]+a0,a1,13
[^:]+:[ 	]+00a58567[ 	]+jalr[ 	]+a0,10\(a1\)
[^:]+:[ 	]+00458503[ 	]+lb[ 	]+a0,4\(a1\)
[^:]+:[ 	]+fab50fe3[ 	]+beq[ 	]+a0,a1,0 \<target\>
[^:]+: R_RISCV_BRANCH[	]+target
[^:]+:[ 	]+fab50de3[ 	]+beq[ 	]+a0,a1,0 \<target\>
[^:]+: R_RISCV_BRANCH[	]+target
[^:]+:[ 	]+00a58223[ 	]+sb[ 	]+a0,4\(a1\)
[^:]+:[ 	]+00fff537[ 	]+lui[ 	]+a0,0xfff
[^:]+:[ 	]+fafff56f[ 	]+jal[ 	]+a0,0 \<target\>
[^:]+: R_RISCV_JAL[	]+target
[^:]+:[ 	]+fabff56f[ 	]+jal[ 	]+a0,0 \<target\>
[^:]+: R_RISCV_JAL[	]+target
[^:]+:[ 	]+0511[ 	]+addi[ 	]+a0,a0,4
[^:]+:[ 	]+852e[ 	]+mv[ 	]+a0,a1
[^:]+:[ 	]+002c[ 	]+addi[ 	]+a1,sp,8
[^:]+:[ 	]+8d6d[ 	]+and[ 	]+a0,a0,a1
[^:]+:[ 	]+ddd9[ 	]+beqz[ 	]+a1,0 \<target\>
[^:]+: R_RISCV_RVC_BRANCH[	]+target
[^:]+:[ 	]+bf71[ 	]+j[ 	]+0 \<target\>
[^:]+: R_RISCV_RVC_JUMP[	]+target
[^:]+:[ 	]+68c58543[ 	]+fmadd.s[ 	]+fa0,fa1,fa2,fa3,rne
[^:]+:[ 	]+68c58543[ 	]+fmadd.s[ 	]+fa0,fa1,fa2,fa3,rne
[^:]+:[ 	]+68c58543[ 	]+fmadd.s[ 	]+fa0,fa1,fa2,fa3,rne
[^:]+:[ 	]+68c58543[ 	]+fmadd.s[ 	]+fa0,fa1,fa2,fa3,rne
[^:]+:[ 	]+68c58543[ 	]+fmadd.s[ 	]+fa0,fa1,fa2,fa3,rne
[^:]+:[ 	]+68c58543[ 	]+fmadd.s[ 	]+fa0,fa1,fa2,fa3,rne
[^:]+:[ 	]+00c58533[ 	]+add[ 	]+a0,a1,a2
[^:]+:[ 	]+00c58533[ 	]+add[ 	]+a0,a1,a2
[^:]+:[ 	]+00c58533[ 	]+add[ 	]+a0,a1,a2
[^:]+:[ 	]+00c58533[ 	]+add[ 	]+a0,a1,a2
[^:]+:[ 	]+00c58533[ 	]+add[ 	]+a0,a1,a2
[^:]+:[ 	]+00c58533[ 	]+add[ 	]+a0,a1,a2
[^:]+:[ 	]+00c58533[ 	]+add[ 	]+a0,a1,a2
