	.text
	.align
load_store:
	cfldrseq	mvf5, [sp, #255]
	cfldrsmi	mvf14, [r11, #73]
	cfldrsvc	mvf2, [r12, #-239]
	cfldrslt	mvf0, [r10, #-255]
	cfldrscc	mvf12, [r1, #-39]
	cfldrs	mvf13, [r15, #104]!
	cfldrscs	mvf9, [r0, #-0]!
	cfldrsge	mvf9, [lr, #72]!
	cfldrshi	mvf13, [r5, #37]!
	cfldrsgt	mvf6, [r3, #0]!
	cfldrspl	mvf14, [r4], #64
	cfldrsne	mvf8, [r2], #-157
	cfldrslt	mvf4, [r9], #1
	cfldrspl	mvf15, [r7], #-63
	cfldrsal	mvf3, [r8], #-136
	cfldrdcs	mvd1, [r6, #-68]
	cfldrdeq	mvd7, [r13, #255]
	cfldrdgt	mvd10, [r11, #73]
	cfldrdle	mvd6, [r12, #-239]
	cfldrdls	mvd0, [r10, #-255]
	cfldrdls	mvd4, [r1, #-39]!
	cfldrdle	mvd7, [pc, #104]!
	cfldrdvs	mvd11, [r0, #-0]!
	cfldrdal	mvd3, [r14, #72]!
	cfldrdhi	mvd15, [r5, #37]!
	cfldrdmi	mvd2, [r3], #0
	cfldrd	mvd10, [r4], #64
	cfldrdcc	mvd8, [r2], #-157
	cfldrdne	mvd12, [r9], #1
	cfldrdvc	mvd5, [r7], #-63
	cfldr32ge	mvfx1, [r8, #-136]
	cfldr32vs	mvfx11, [r6, #-68]
	cfldr32eq	mvfx5, [sp, #255]
	cfldr32mi	mvfx14, [r11, #73]
	cfldr32vc	mvfx2, [r12, #-239]
	cfldr32lt	mvfx0, [r10, #-255]!
	cfldr32cc	mvfx12, [r1, #-39]!
	cfldr32	mvfx13, [r15, #104]!
	cfldr32cs	mvfx9, [r0, #-0]!
	cfldr32ge	mvfx9, [lr, #72]!
	cfldr32hi	mvfx13, [r5], #37
	cfldr32gt	mvfx6, [r3], #0
	cfldr32pl	mvfx14, [r4], #64
	cfldr32ne	mvfx8, [r2], #-157
	cfldr32lt	mvfx4, [r9], #1
	cfldr64pl	mvdx15, [r7, #-63]
	cfldr64al	mvdx3, [r8, #-136]
	cfldr64cs	mvdx1, [r6, #-68]
	cfldr64eq	mvdx7, [r13, #255]
	cfldr64gt	mvdx10, [r11, #73]
	cfldr64le	mvdx6, [r12, #-239]!
	cfldr64ls	mvdx0, [r10, #-255]!
	cfldr64ls	mvdx4, [r1, #-39]!
	cfldr64le	mvdx7, [pc, #104]!
	cfldr64vs	mvdx11, [r0, #-0]!
	cfldr64al	mvdx3, [r14], #72
	cfldr64hi	mvdx15, [r5], #37
	cfldr64mi	mvdx2, [r3], #0
	cfldr64	mvdx10, [r4], #64
	cfldr64cc	mvdx8, [r2], #-157
	cfstrsne	mvf12, [r9, #1]
	cfstrsvc	mvf5, [r7, #-63]
	cfstrsge	mvf1, [r8, #-136]
	cfstrsvs	mvf11, [r6, #-68]
	cfstrseq	mvf5, [sp, #255]
	cfstrsmi	mvf14, [r11, #73]!
	cfstrsvc	mvf2, [r12, #-239]!
	cfstrslt	mvf0, [r10, #-255]!
	cfstrscc	mvf12, [r1, #-39]!
	cfstrs	mvf13, [r15, #104]!
	cfstrscs	mvf9, [r0], #-0
	cfstrsge	mvf9, [lr], #72
	cfstrshi	mvf13, [r5], #37
	cfstrsgt	mvf6, [r3], #0
	cfstrspl	mvf14, [r4], #64
	cfstrdne	mvd8, [r2, #-157]
	cfstrdlt	mvd4, [r9, #1]
	cfstrdpl	mvd15, [r7, #-63]
	cfstrdal	mvd3, [r8, #-136]
	cfstrdcs	mvd1, [r6, #-68]
	cfstrdeq	mvd7, [r13, #255]!
	cfstrdgt	mvd10, [r11, #73]!
	cfstrdle	mvd6, [r12, #-239]!
	cfstrdls	mvd0, [r10, #-255]!
	cfstrdls	mvd4, [r1, #-39]!
	cfstrdle	mvd7, [pc], #104
	cfstrdvs	mvd11, [r0], #-0
	cfstrdal	mvd3, [r14], #72
	cfstrdhi	mvd15, [r5], #37
	cfstrdmi	mvd2, [r3], #0
	cfstr32	mvfx10, [r4, #64]
	cfstr32cc	mvfx8, [r2, #-157]
	cfstr32ne	mvfx12, [r9, #1]
	cfstr32vc	mvfx5, [r7, #-63]
	cfstr32ge	mvfx1, [r8, #-136]
	cfstr32vs	mvfx11, [r6, #-68]!
	cfstr32eq	mvfx5, [sp, #255]!
	cfstr32mi	mvfx14, [r11, #73]!
	cfstr32vc	mvfx2, [r12, #-239]!
	cfstr32lt	mvfx0, [r10, #-255]!
	cfstr32cc	mvfx12, [r1], #-39
	cfstr32	mvfx13, [r15], #104
	cfstr32cs	mvfx9, [r0], #-0
	cfstr32ge	mvfx9, [lr], #72
	cfstr32hi	mvfx13, [r5], #37
	cfstr64gt	mvdx6, [r3, #0]
	cfstr64pl	mvdx14, [r4, #64]
	cfstr64ne	mvdx8, [r2, #-157]
	cfstr64lt	mvdx4, [r9, #1]
	cfstr64pl	mvdx15, [r7, #-63]
	cfstr64al	mvdx3, [r8, #-136]!
	cfstr64cs	mvdx1, [r6, #-68]!
	cfstr64eq	mvdx7, [r13, #255]!
	cfstr64gt	mvdx10, [r11, #73]!
	cfstr64le	mvdx6, [r12, #-239]!
	cfstr64ls	mvdx0, [r10], #-255
	cfstr64ls	mvdx4, [r1], #-39
	cfstr64le	mvdx7, [pc], #104
	cfstr64vs	mvdx11, [r0], #-0
	cfstr64al	mvdx3, [r14], #72
move:
	cfmvsrhi	mvf15, r5
	cfmvsrvs	mvf11, r6
	cfmvsrcs	mvf9, r0
	cfmvsrpl	mvf15, r7
	cfmvsrls	mvf4, r1
	cfmvrscc	r8, mvf13
	cfmvrsvc	pc, mvf1
	cfmvrsgt	r9, mvf11
	cfmvrseq	r10, mvf5
	cfmvrsal	r4, mvf12
	cfmvdlrge	mvd1, r8
	cfmvdlr	mvd13, r15
	cfmvdlrlt	mvd4, r9
	cfmvdlrls	mvd0, r10
	cfmvdlr	mvd10, r4
	cfmvrdlmi	r1, mvd3
	cfmvrdlhi	r2, mvd7
	cfmvrdlcs	r12, mvd12
	cfmvrdlvs	r3, mvd0
	cfmvrdlvc	r13, mvd14
	cfmvdhrcc	mvd12, r1
	cfmvdhrne	mvd8, r2
	cfmvdhrle	mvd6, r12
	cfmvdhrmi	mvd2, r3
	cfmvdhreq	mvd5, sp
	cfmvrdhge	r4, mvd4
	cfmvrdhal	r11, mvd8
	cfmvrdhle	r5, mvd2
	cfmvrdhne	r6, mvd6
	cfmvrdhlt	r0, mvd7
	cfmv64lrpl	mvdx14, r4
	cfmv64lrgt	mvdx10, r11
	cfmv64lrhi	mvdx15, r5
	cfmv64lrvs	mvdx11, r6
	cfmv64lrcs	mvdx9, r0
	cfmvr64lpl	sp, mvdx10
	cfmvr64lls	lr, mvdx14
	cfmvr64lcc	r8, mvdx13
	cfmvr64lvc	pc, mvdx1
	cfmvr64lgt	r9, mvdx11
	cfmv64hreq	mvdx7, r13
	cfmv64hral	mvdx3, r14
	cfmv64hrge	mvdx1, r8
	cfmv64hr	mvdx13, r15
	cfmv64hrlt	mvdx4, r9
	cfmvr64hls	r0, mvdx5
	cfmvr64h	r7, mvdx9
	cfmvr64hmi	r1, mvdx3
	cfmvr64hhi	r2, mvdx7
	cfmvr64hcs	r12, mvdx12
	cfmval32vs	mvax1, mvfx0
	cfmval32vc	mvax3, mvfx14
	cfmval32cc	mvax0, mvfx10
	cfmval32ne	mvax1, mvfx15
	cfmval32le	mvax0, mvfx11
	cfmv32almi	mvfx2, mvax1
	cfmv32aleq	mvfx5, mvax3
	cfmv32alge	mvfx9, mvax0
	cfmv32alal	mvfx3, mvax1
	cfmv32alle	mvfx7, mvax0
	cfmvam32ne	mvax2, mvfx6
	cfmvam32lt	mvax0, mvfx7
	cfmvam32pl	mvax2, mvfx3
	cfmvam32gt	mvax1, mvfx1
	cfmvam32hi	mvax3, mvfx13
	cfmv32amvs	mvfx11, mvax2
	cfmv32amcs	mvfx9, mvax0
	cfmv32ampl	mvfx15, mvax2
	cfmv32amls	mvfx4, mvax1
	cfmv32amcc	mvfx8, mvax3
	cfmvah32vc	mvax0, mvfx1
	cfmvah32gt	mvax0, mvfx11
	cfmvah32eq	mvax1, mvfx5
	cfmvah32al	mvax2, mvfx12
	cfmvah32ge	mvax3, mvfx8
	cfmv32ah	mvfx13, mvax0
	cfmv32ahlt	mvfx4, mvax0
	cfmv32ahls	mvfx0, mvax1
	cfmv32ah	mvfx10, mvax2
	cfmv32ahmi	mvfx14, mvax3
	cfmva32hi	mvax3, mvfx7
	cfmva32cs	mvax3, mvfx12
	cfmva32vs	mvax1, mvfx0
	cfmva32vc	mvax3, mvfx14
	cfmva32cc	mvax0, mvfx10
	cfmv32ane	mvfx8, mvax3
	cfmv32ale	mvfx6, mvax3
	cfmv32ami	mvfx2, mvax1
	cfmv32aeq	mvfx5, mvax3
	cfmv32age	mvfx9, mvax0
	cfmva64al	mvax3, mvdx8
	cfmva64le	mvax2, mvdx2
	cfmva64ne	mvax2, mvdx6
	cfmva64lt	mvax0, mvdx7
	cfmva64pl	mvax2, mvdx3
	cfmv64agt	mvdx10, mvax3
	cfmv64ahi	mvdx15, mvax2
	cfmv64avs	mvdx11, mvax2
	cfmv64acs	mvdx9, mvax0
	cfmv64apl	mvdx15, mvax2
	cfmvsc32ls	dspsc, mvfx14
	cfmvsc32cc	dspsc, mvfx13
	cfmvsc32vc	dspsc, mvfx1
	cfmvsc32gt	dspsc, mvfx11
	cfmvsc32eq	dspsc, mvfx5
	cfmv32scal	mvfx3, dspsc
	cfmv32scge	mvfx1, dspsc
	cfmv32sc	mvfx13, dspsc
	cfmv32sclt	mvfx4, dspsc
	cfmv32scls	mvfx0, dspsc
	cfcpys	mvf10, mvf9
	cfcpysmi	mvf14, mvf3
	cfcpyshi	mvf13, mvf7
	cfcpyscs	mvf1, mvf12
	cfcpysvs	mvf11, mvf0
	cfcpydvc	mvd5, mvd14
	cfcpydcc	mvd12, mvd10
	cfcpydne	mvd8, mvd15
	cfcpydle	mvd6, mvd11
	cfcpydmi	mvd2, mvd9
conv:
	cfcvtsdeq	mvd5, mvf15
	cfcvtsdge	mvd9, mvf4
	cfcvtsdal	mvd3, mvf8
	cfcvtsdle	mvd7, mvf2
	cfcvtsdne	mvd12, mvf6
	cfcvtdslt	mvf0, mvd7
	cfcvtdspl	mvf14, mvd3
	cfcvtdsgt	mvf10, mvd1
	cfcvtdshi	mvf15, mvd13
	cfcvtdsvs	mvf11, mvd4
	cfcvt32scs	mvf9, mvfx0
	cfcvt32spl	mvf15, mvfx10
	cfcvt32sls	mvf4, mvfx14
	cfcvt32scc	mvf8, mvfx13
	cfcvt32svc	mvf2, mvfx1
	cfcvt32dgt	mvd6, mvfx11
	cfcvt32deq	mvd7, mvfx5
	cfcvt32dal	mvd3, mvfx12
	cfcvt32dge	mvd1, mvfx8
	cfcvt32d	mvd13, mvfx6
	cfcvt64slt	mvf4, mvdx2
	cfcvt64sls	mvf0, mvdx5
	cfcvt64s	mvf10, mvdx9
	cfcvt64smi	mvf14, mvdx3
	cfcvt64shi	mvf13, mvdx7
	cfcvt64dcs	mvd1, mvdx12
	cfcvt64dvs	mvd11, mvdx0
	cfcvt64dvc	mvd5, mvdx14
	cfcvt64dcc	mvd12, mvdx10
	cfcvt64dne	mvd8, mvdx15
	cfcvts32le	mvfx6, mvf11
	cfcvts32mi	mvfx2, mvf9
	cfcvts32eq	mvfx5, mvf15
	cfcvts32ge	mvfx9, mvf4
	cfcvts32al	mvfx3, mvf8
	cfcvtd32le	mvfx7, mvd2
	cfcvtd32ne	mvfx12, mvd6
	cfcvtd32lt	mvfx0, mvd7
	cfcvtd32pl	mvfx14, mvd3
	cfcvtd32gt	mvfx10, mvd1
	cftruncs32hi	mvfx15, mvf13
	cftruncs32vs	mvfx11, mvf4
	cftruncs32cs	mvfx9, mvf0
	cftruncs32pl	mvfx15, mvf10
	cftruncs32ls	mvfx4, mvf14
	cftruncd32cc	mvfx8, mvd13
	cftruncd32vc	mvfx2, mvd1
	cftruncd32gt	mvfx6, mvd11
	cftruncd32eq	mvfx7, mvd5
	cftruncd32al	mvfx3, mvd12
shift:
	cfrshl32ge	mvfx1, mvfx8, r2
	cfrshl32vs	mvfx11, mvfx4, r9
	cfrshl32eq	mvfx5, mvfx15, r7
	cfrshl32mi	mvfx14, mvfx3, r8
	cfrshl32vc	mvfx2, mvfx1, r6
	cfrshl64lt	mvdx0, mvdx7, r13
	cfrshl64cc	mvdx12, mvdx10, r11
	cfrshl64	mvdx13, mvdx6, r12
	cfrshl64cs	mvdx9, mvdx0, r10
	cfrshl64ge	mvdx9, mvdx4, r1
	cfsh32hi	mvfx13, mvfx7, #33
	cfsh32gt	mvfx6, mvfx11, #0
	cfsh32pl	mvfx14, mvfx3, #32
	cfsh32ne	mvfx8, mvfx15, #-31
	cfsh32lt	mvfx4, mvfx2, #1
	cfsh64pl	mvdx15, mvdx10, #-32
	cfsh64al	mvdx3, mvdx8, #-27
	cfsh64cs	mvdx1, mvdx12, #-5
	cfsh64eq	mvdx7, mvdx5, #63
	cfsh64gt	mvdx10, mvdx1, #9
comp:
	cfcmpsle	r15, mvf11, mvf4
	cfcmpsls	r0, mvf5, mvf15
	cfcmpsls	lr, mvf14, mvf3
	cfcmpsle	r5, mvf2, mvf1
	cfcmpsvs	r3, mvf0, mvf7
	cfcmpdal	r4, mvd12, mvd10
	cfcmpdhi	r2, mvd13, mvd6
	cfcmpdmi	r9, mvd9, mvd0
	cfcmpd	r7, mvd9, mvd4
	cfcmpdcc	r8, mvd13, mvd7
	cfcmp32ne	r6, mvfx6, mvfx11
	cfcmp32vc	r13, mvfx14, mvfx3
	cfcmp32ge	r11, mvfx8, mvfx15
	cfcmp32vs	r12, mvfx4, mvfx2
	cfcmp32eq	r10, mvfx15, mvfx10
	cfcmp64mi	r1, mvdx3, mvdx8
	cfcmp64vc	pc, mvdx1, mvdx12
	cfcmp64lt	r0, mvdx7, mvdx5
	cfcmp64cc	r14, mvdx10, mvdx1
	cfcmp64	r5, mvdx6, mvdx11
fp_arith:
	cfabsscs	mvf9, mvf0
	cfabsspl	mvf15, mvf10
	cfabssls	mvf4, mvf14
	cfabsscc	mvf8, mvf13
	cfabssvc	mvf2, mvf1
	cfabsdgt	mvd6, mvd11
	cfabsdeq	mvd7, mvd5
	cfabsdal	mvd3, mvd12
	cfabsdge	mvd1, mvd8
	cfabsd	mvd13, mvd6
	cfnegslt	mvf4, mvf2
	cfnegsls	mvf0, mvf5
	cfnegs	mvf10, mvf9
	cfnegsmi	mvf14, mvf3
	cfnegshi	mvf13, mvf7
	cfnegdcs	mvd1, mvd12
	cfnegdvs	mvd11, mvd0
	cfnegdvc	mvd5, mvd14
	cfnegdcc	mvd12, mvd10
	cfnegdne	mvd8, mvd15
	cfaddsle	mvf6, mvf11, mvf4
	cfaddsls	mvf0, mvf5, mvf15
	cfaddsls	mvf4, mvf14, mvf3
	cfaddsle	mvf7, mvf2, mvf1
	cfaddsvs	mvf11, mvf0, mvf7
	cfadddal	mvd3, mvd12, mvd10
	cfadddhi	mvd15, mvd13, mvd6
	cfadddmi	mvd2, mvd9, mvd0
	cfaddd	mvd10, mvd9, mvd4
	cfadddcc	mvd8, mvd13, mvd7
	cfsubsne	mvf12, mvf6, mvf11
	cfsubsvc	mvf5, mvf14, mvf3
	cfsubsge	mvf1, mvf8, mvf15
	cfsubsvs	mvf11, mvf4, mvf2
	cfsubseq	mvf5, mvf15, mvf10
	cfsubdmi	mvd14, mvd3, mvd8
	cfsubdvc	mvd2, mvd1, mvd12
	cfsubdlt	mvd0, mvd7, mvd5
	cfsubdcc	mvd12, mvd10, mvd1
	cfsubd	mvd13, mvd6, mvd11
	cfmulscs	mvf9, mvf0, mvf5
	cfmulsge	mvf9, mvf4, mvf14
	cfmulshi	mvf13, mvf7, mvf2
	cfmulsgt	mvf6, mvf11, mvf0
	cfmulspl	mvf14, mvf3, mvf12
	cfmuldne	mvd8, mvd15, mvd13
	cfmuldlt	mvd4, mvd2, mvd9
	cfmuldpl	mvd15, mvd10, mvd9
	cfmuldal	mvd3, mvd8, mvd13
	cfmuldcs	mvd1, mvd12, mvd6
int_arith:
	cfabs32eq	mvfx7, mvfx5
	cfabs32al	mvfx3, mvfx12
	cfabs32ge	mvfx1, mvfx8
	cfabs32	mvfx13, mvfx6
	cfabs32lt	mvfx4, mvfx2
	cfabs64ls	mvdx0, mvdx5
	cfabs64	mvdx10, mvdx9
	cfabs64mi	mvdx14, mvdx3
	cfabs64hi	mvdx13, mvdx7
	cfabs64cs	mvdx1, mvdx12
	cfneg32vs	mvfx11, mvfx0
	cfneg32vc	mvfx5, mvfx14
	cfneg32cc	mvfx12, mvfx10
	cfneg32ne	mvfx8, mvfx15
	cfneg32le	mvfx6, mvfx11
	cfneg64mi	mvdx2, mvdx9
	cfneg64eq	mvdx5, mvdx15
	cfneg64ge	mvdx9, mvdx4
	cfneg64al	mvdx3, mvdx8
	cfneg64le	mvdx7, mvdx2
	cfadd32ne	mvfx12, mvfx6, mvfx11
	cfadd32vc	mvfx5, mvfx14, mvfx3
	cfadd32ge	mvfx1, mvfx8, mvfx15
	cfadd32vs	mvfx11, mvfx4, mvfx2
	cfadd32eq	mvfx5, mvfx15, mvfx10
	cfadd64mi	mvdx14, mvdx3, mvdx8
	cfadd64vc	mvdx2, mvdx1, mvdx12
	cfadd64lt	mvdx0, mvdx7, mvdx5
	cfadd64cc	mvdx12, mvdx10, mvdx1
	cfadd64	mvdx13, mvdx6, mvdx11
	cfsub32cs	mvfx9, mvfx0, mvfx5
	cfsub32ge	mvfx9, mvfx4, mvfx14
	cfsub32hi	mvfx13, mvfx7, mvfx2
	cfsub32gt	mvfx6, mvfx11, mvfx0
	cfsub32pl	mvfx14, mvfx3, mvfx12
	cfsub64ne	mvdx8, mvdx15, mvdx13
	cfsub64lt	mvdx4, mvdx2, mvdx9
	cfsub64pl	mvdx15, mvdx10, mvdx9
	cfsub64al	mvdx3, mvdx8, mvdx13
	cfsub64cs	mvdx1, mvdx12, mvdx6
	cfmul32eq	mvfx7, mvfx5, mvfx14
	cfmul32gt	mvfx10, mvfx1, mvfx8
	cfmul32le	mvfx6, mvfx11, mvfx4
	cfmul32ls	mvfx0, mvfx5, mvfx15
	cfmul32ls	mvfx4, mvfx14, mvfx3
	cfmul64le	mvdx7, mvdx2, mvdx1
	cfmul64vs	mvdx11, mvdx0, mvdx7
	cfmul64al	mvdx3, mvdx12, mvdx10
	cfmul64hi	mvdx15, mvdx13, mvdx6
	cfmul64mi	mvdx2, mvdx9, mvdx0
	cfmac32	mvfx10, mvfx9, mvfx4
	cfmac32cc	mvfx8, mvfx13, mvfx7
	cfmac32ne	mvfx12, mvfx6, mvfx11
	cfmac32vc	mvfx5, mvfx14, mvfx3
	cfmac32ge	mvfx1, mvfx8, mvfx15
	cfmsc32vs	mvfx11, mvfx4, mvfx2
	cfmsc32eq	mvfx5, mvfx15, mvfx10
	cfmsc32mi	mvfx14, mvfx3, mvfx8
	cfmsc32vc	mvfx2, mvfx1, mvfx12
	cfmsc32lt	mvfx0, mvfx7, mvfx5
acc_arith:
	cfmadd32cc	mvax0, mvfx10, mvfx1, mvfx8
	cfmadd32	mvax2, mvfx6, mvfx11, mvfx4
	cfmadd32cs	mvax1, mvfx0, mvfx5, mvfx15
	cfmadd32ge	mvax2, mvfx4, mvfx14, mvfx3
	cfmadd32hi	mvax3, mvfx7, mvfx2, mvfx1
	cfmsub32gt	mvax0, mvfx11, mvfx0, mvfx7
	cfmsub32pl	mvax2, mvfx3, mvfx12, mvfx10
	cfmsub32ne	mvax1, mvfx15, mvfx13, mvfx6
	cfmsub32lt	mvax2, mvfx2, mvfx9, mvfx0
	cfmsub32pl	mvax3, mvfx10, mvfx9, mvfx4
	cfmadda32al	mvax3, mvax1, mvfx13, mvfx7
	cfmadda32cs	mvax3, mvax2, mvfx6, mvfx11
	cfmadda32eq	mvax1, mvax3, mvfx14, mvfx3
	cfmadda32gt	mvax1, mvax3, mvfx8, mvfx15
	cfmadda32le	mvax0, mvax3, mvfx4, mvfx2
	cfmsuba32ls	mvax0, mvax1, mvfx15, mvfx10
	cfmsuba32ls	mvax0, mvax1, mvfx3, mvfx8
	cfmsuba32le	mvax2, mvax0, mvfx1, mvfx12
	cfmsuba32vs	mvax1, mvax0, mvfx7, mvfx5
	cfmsuba32al	mvax2, mvax0, mvfx10, mvfx1
