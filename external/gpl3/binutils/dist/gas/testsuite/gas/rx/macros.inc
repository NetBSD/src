macro	reg	{r0;r15}
macro	reg1	{r1;r15}
macro	reg2	{r0;r14}
macro	reg7	{r0;r7}
macro	regPlo	{r1;r7}
macro	regPhi	{r8;r14}
macro	creg	{psw;fpsw;usp;isp;intb;bpsw;bpc;fintv}
macro	pswf	{U;I;O;S;Z;C}

macro	simm8	{-128;127}
macro	simm16	{-32768;32768}
macro	simm24	{-8388608;8388607}
macro	simm32	{-2147483648;2147483647}

macro	imm	{{simm8};{simm16};{simm24};{simm32}}

macro	uimm2	{0;3}
macro	uimm3	{0;7}
macro	uimm4	{0;15}
macro	uimm5	{0;31}
macro	uimm8	{0;255}
macro	uimm16	{0;65535}
macro	uimm24	{0;16777215}
macro	uimm32	{0;4294967295}

macro	imm32	{-2147483648;4294967295}

macro	dsp5	{4;28}
macro	dsp8	252
macro	dsp16	65532
macro	dsp8L	1020
macro	dsp16L	262140

macro	dsp	{;{dsp8};{dsp16}}
macro	dspL	{;{dsp8L};{dsp16L}}

macro	memex	{.ub;.b;.uw;.w;.l}
macro	memx	{dsp}[{reg}]{memex}
macro	mem	{dsp}[{reg}]
macro	bwl	{.b;.w;.l}
macro	bw	{.b;.w}

macro	cnd	{geu;c;eq;z;gtu;pz;ge;gt;o;ltu;nc;ne;nz;leu;n;lt;le;no}
macro	lab_s	{.+3;.+10;foo}
macro	lab_b	{.-128;.+127;foo}
macro	lab_w	{.-32768;.+32767;foo}
macro	lab_a	{.-8388608;.+8388607;foo}
macro	lab_l	{.-2147483648;.+2147483647;foo}

	.text
