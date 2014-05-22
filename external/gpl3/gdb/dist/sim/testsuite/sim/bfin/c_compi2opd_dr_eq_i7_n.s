//Original:/testcases/core/c_compi2opd_dr_eq_i7_n/c_compi2opd_dr_eq_i7_n.dsp
// Spec Reference: compi2opd dregs = imm7 negative
# mach: bfin

.include "testutils.inc"
	start


INIT_R_REGS 0;


R0 = -0;
R1 = -1;
R2 = -2;
R3 = -3;
R4 = -4;
R5 = -5;
R6 = -6;
R7 = -7;
CHECKREG r0, -0;
CHECKREG r1, -1;
CHECKREG r2, -2;
CHECKREG r3, -3;
CHECKREG r4, -4;
CHECKREG r5, -5;
CHECKREG r6, -6;
CHECKREG r7, -7;

R0 = -8;
R1 = -9;
R2 = -10;
R3 = -11;
R4 = -12;
R5 = -13;
R6 = -14;
R7 = -15;
CHECKREG r0, -8;
CHECKREG r1, -9;
CHECKREG r2, -10;
CHECKREG r3, -11;
CHECKREG r4, -12;
CHECKREG r5, -13;
CHECKREG r6, -14;
CHECKREG r7, -15;

R0 = -16;
R1 = -17;
R2 = -18;
R3 = -19;
R4 = -20;
R5 = -21;
R6 = -22;
R7 = -23;
CHECKREG r0, -16;
CHECKREG r1, -17;
CHECKREG r2, -18;
CHECKREG r3, -19;
CHECKREG r4, -20;
CHECKREG r5, -21;
CHECKREG r6, -22;
CHECKREG r7, -23;

R0 = -24;
R1 = -25;
R2 = -26;
R3 = -27;
R4 = -28;
R5 = -29;
R6 = -30;
R7 = -31;
CHECKREG r0, -24;
CHECKREG r1, -25;
CHECKREG r2, -26;
CHECKREG r3, -27;
CHECKREG r4, -28;
CHECKREG r5, -29;
CHECKREG r6, -30;
CHECKREG r7, -31;

R0 = -32;
R1 = -33;
R2 = -34;
R3 = -35;
R4 = -36;
R5 = -37;
R6 = -38;
R7 = -39;
CHECKREG r0, -32;
CHECKREG r1, -33;
CHECKREG r2, -34;
CHECKREG r3, -35;
CHECKREG r4, -36;
CHECKREG r5, -37;
CHECKREG r6, -38;
CHECKREG r7, -39;

R0 = -40;
R1 = -41;
R2 = -42;
R3 = -43;
R4 = -44;
R5 = -45;
R6 = -46;
R7 = -47;
CHECKREG r0, -40;
CHECKREG r1, -41;
CHECKREG r2, -42;
CHECKREG r3, -43;
CHECKREG r4, -44;
CHECKREG r5, -45;
CHECKREG r6, -46;
CHECKREG r7, -47;

R0 = -48;
R1 = -49;
R2 = -50;
R3 = -51;
R4 = -52;
R5 = -53;
R6 = -54;
R7 = -55;
CHECKREG r0, -48;
CHECKREG r1, -49;
CHECKREG r2, -50;
CHECKREG r3, -51;
CHECKREG r4, -52;
CHECKREG r5, -53;
CHECKREG r6, -54;
CHECKREG r7, -55;

R0 = -56;
R1 = -57;
R2 = -58;
R3 = -59;
R4 = -60;
R5 = -61;
R6 = -62;
R7 = -63;
CHECKREG r0, -56;
CHECKREG r1, -57;
CHECKREG r2, -58;
CHECKREG r3, -59;
CHECKREG r4, -60;
CHECKREG r5, -61;
CHECKREG r6, -62;
CHECKREG r7, -63;

R0 = -64;
R1 = -64;
R2 = -64;
R3 = -64;
R4 = -64;
R5 = -64;
R6 = -64;
R7 = -64;
CHECKREG r0, -64;
CHECKREG r1, -64;
CHECKREG r2, -64;
CHECKREG r3, -64;
CHECKREG r4, -64;
CHECKREG r5, -64;
CHECKREG r6, -64;
CHECKREG r7, -64;


pass
