/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2016 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

	.text
	.globl	func
func:
	.long	0x7c642e98	/* <+0>:   lxvd2x  vs3,r4,r5         */
	.long	0x7c642ed8	/* <+4>:   lxvd2ux vs3,r4,r5         */
	.long	0x7d642e99	/* <+8>:   lxvd2x  vs43,r4,r5        */
	.long	0x7d642ed9	/* <+12>:  lxvd2ux vs43,r4,r5        */
	.long	0x7c642f98	/* <+16>:  stxvd2x vs3,r4,r5         */
	.long	0x7c642fd8	/* <+20>:  stxvd2ux vs3,r4,r5        */
	.long	0x7d642f99	/* <+24>:  stxvd2x vs43,r4,r5        */
	.long	0x7d642fd9	/* <+28>:  stxvd2ux vs43,r4,r5       */
	.long	0xf0642850	/* <+32>:  xxmrghd vs3,vs4,vs5       */
	.long	0xf16c6857	/* <+36>:  xxmrghd vs43,vs44,vs45    */
	.long	0xf0642b50	/* <+40>:  xxmrgld vs3,vs4,vs5       */
	.long	0xf16c6b57	/* <+44>:  xxmrgld vs43,vs44,vs45    */
	.long	0xf0642850	/* <+48>:  xxmrghd vs3,vs4,vs5       */
	.long	0xf16c6857	/* <+52>:  xxmrghd vs43,vs44,vs45    */
	.long	0xf0642b50	/* <+56>:  xxmrgld vs3,vs4,vs5       */
	.long	0xf16c6b57	/* <+60>:  xxmrgld vs43,vs44,vs45    */
	.long	0xf0642950	/* <+64>:  xxpermdi vs3,vs4,vs5,1    */
	.long	0xf16c6957	/* <+68>:  xxpermdi vs43,vs44,vs45,1 */
	.long	0xf0642a50	/* <+72>:  xxpermdi vs3,vs4,vs5,2    */
	.long	0xf16c6a57	/* <+76>:  xxpermdi vs43,vs44,vs45,2 */
	.long	0xf0642780	/* <+80>:  xvmovdp vs3,vs4           */
	.long	0xf16c6787	/* <+84>:  xvmovdp vs43,vs44         */
	.long	0xf0642780	/* <+88>:  xvmovdp vs3,vs4           */
	.long	0xf16c6787	/* <+92>:  xvmovdp vs43,vs44         */
	.long	0xf0642f80	/* <+96>:  xvcpsgndp vs3,vs4,vs5     */
	.long	0xf16c6f87	/* <+100>: xvcpsgndp vs43,vs44,vs45  */
	.long	0x7c00007c	/* <+104>: wait                      */
	.long	0x7c00007c	/* <+108>: wait                      */
	.long	0x7c20007c	/* <+112>: waitrsv                   */
	.long	0x7c20007c	/* <+116>: waitrsv                   */
	.long	0x7c40007c	/* <+120>: waitimpl                  */
	.long	0x7c40007c	/* <+124>: waitimpl                  */
	.long	0x4c000324	/* <+128>: doze                      */
	.long	0x4c000364	/* <+132>: nap                       */
	.long	0x4c0003a4	/* <+136>: sleep                     */
	.long	0x4c0003e4	/* <+140>: rvwinkle                  */
	.long	0x7c830134	/* <+144>: prtyw   r3,r4             */
	.long	0x7dcd0174	/* <+148>: prtyd   r13,r14           */
	.long	0x7d5c02a6	/* <+152>: mfcfar  r10               */
	.long	0x7d7c03a6	/* <+156>: mtcfar  r11               */
	.long	0x7c832bf8	/* <+160>: cmpb    r3,r4,r5          */
	.long	0x7d4b662a	/* <+164>: lwzcix  r10,r11,r12       */
	.long	0xee119004	/* <+168>: dadd    f16,f17,f18       */
	.long	0xfe96c004	/* <+172>: daddq   f20,f22,f24       */
	.long	0x7c60066c	/* <+176>: dss     3                 */
	.long	0x7e00066c	/* <+180>: dssall                    */
	.long	0x7c2522ac	/* <+184>: dst     r5,r4,1           */
	.long	0x7e083aac	/* <+188>: dstt    r8,r7,0           */
	.long	0x7c6532ec	/* <+192>: dstst   r5,r6,3           */
	.long	0x7e442aec	/* <+196>: dststt  r4,r5,2           */
	.long	0x7d4b6356	/* <+200>: divwe   r10,r11,r12       */
	.long	0x7d6c6b57	/* <+204>: divwe.  r11,r12,r13       */
	.long	0x7d8d7756	/* <+208>: divweo  r12,r13,r14       */
	.long	0x7dae7f57	/* <+212>: divweo. r13,r14,r15       */
	.long	0x7d4b6316	/* <+216>: divweu  r10,r11,r12       */
	.long	0x7d6c6b17	/* <+220>: divweu. r11,r12,r13       */
	.long	0x7d8d7716	/* <+224>: divweuo r12,r13,r14       */
	.long	0x7dae7f17	/* <+228>: divweuo. r13,r14,r15      */
	.long	0x7e27d9f8	/* <+232>: bpermd  r7,r17,r27        */
	.long	0x7e8a02f4	/* <+236>: popcntw r10,r20           */
	.long	0x7e8a03f4	/* <+240>: popcntd r10,r20           */
	.long	0x7e95b428	/* <+244>: ldbrx   r20,r21,r22       */
	.long	0x7e95b528	/* <+248>: stdbrx  r20,r21,r22       */
	.long	0x7d4056ee	/* <+252>: lfiwzx  f10,0,r10         */
	.long	0x7d4956ee	/* <+256>: lfiwzx  f10,r9,r10        */
	.long	0xec802e9c	/* <+260>: fcfids  f4,f5             */
	.long	0xec802e9d	/* <+264>: fcfids. f4,f5             */
	.long	0xec802f9c	/* <+268>: fcfidus f4,f5             */
	.long	0xec802f9d	/* <+272>: fcfidus. f4,f5            */
	.long	0xfc80291c	/* <+276>: fctiwu  f4,f5             */
	.long	0xfc80291d	/* <+280>: fctiwu. f4,f5             */
	.long	0xfc80291e	/* <+284>: fctiwuz f4,f5             */
	.long	0xfc80291f	/* <+288>: fctiwuz. f4,f5            */
	.long	0xfc802f5c	/* <+292>: fctidu  f4,f5             */
	.long	0xfc802f5d	/* <+296>: fctidu. f4,f5             */
	.long	0xfc802f5e	/* <+300>: fctiduz f4,f5             */
	.long	0xfc802f5f	/* <+304>: fctiduz. f4,f5            */
	.long	0xfc802f9c	/* <+308>: fcfidu  f4,f5             */
	.long	0xfc802f9d	/* <+312>: fcfidu. f4,f5             */
	.long	0xfc0a5900	/* <+316>: ftdiv   cr0,f10,f11       */
	.long	0xff8a5900	/* <+320>: ftdiv   cr7,f10,f11       */
	.long	0xfc005140	/* <+324>: ftsqrt  cr0,f10           */
	.long	0xff805140	/* <+328>: ftsqrt  cr7,f10           */
	.long	0x7e084a2c	/* <+332>: dcbtt   r8,r9             */
	.long	0x7e0849ec	/* <+336>: dcbtstt r8,r9             */
	.long	0xed406644	/* <+340>: dcffix  f10,f12           */
	.long	0xee80b645	/* <+344>: dcffix. f20,f22           */
	.long	0x7d4b6068	/* <+348>: lbarx   r10,r11,r12       */
	.long	0x7d4b6068	/* <+352>: lbarx   r10,r11,r12       */
	.long	0x7d4b6069	/* <+356>: lbarx   r10,r11,r12,1     */
	.long	0x7e95b0e8	/* <+360>: lharx   r20,r21,r22       */
	.long	0x7e95b0e8	/* <+364>: lharx   r20,r21,r22       */
	.long	0x7e95b0e9	/* <+368>: lharx   r20,r21,r22,1     */
	.long	0x7d4b656d	/* <+372>: stbcx.  r10,r11,r12       */
	.long	0x7d4b65ad	/* <+376>: sthcx.  r10,r11,r12       */
	.long	0xfdc07830	/* <+380>: fre     f14,f15           */
	.long	0xfdc07831	/* <+384>: fre.    f14,f15           */
	.long	0xedc07830	/* <+388>: fres    f14,f15           */
	.long	0xedc07831	/* <+392>: fres.   f14,f15           */
	.long	0xfdc07834	/* <+396>: frsqrte f14,f15           */
	.long	0xfdc07835	/* <+400>: frsqrte. f14,f15          */
	.long	0xedc07834	/* <+404>: frsqrtes f14,f15          */
	.long	0xedc07835	/* <+408>: frsqrtes. f14,f15         */
	.long	0x7c43271e	/* <+412>: isel    r2,r3,r4,28       */
	.long	0x7c284fec	/* <+416>: dcbzl   r8,r9             */
	.long	0xed405834	/* <+420>: frsqrtes f10,f11          */
	.long	0xec220804	/* <+424>: dadd    f1,f2,f1          */
	.long	0xfc020004	/* <+428>: daddq   f0,f2,f0          */
	.long	0xec220c04	/* <+432>: dsub    f1,f2,f1          */
	.long	0xfc020404	/* <+436>: dsubq   f0,f2,f0          */
	.long	0xec220844	/* <+440>: dmul    f1,f2,f1          */
	.long	0xfc020044	/* <+444>: dmulq   f0,f2,f0          */
	.long	0xec220c44	/* <+448>: ddiv    f1,f2,f1          */
	.long	0xfc020444	/* <+452>: ddivq   f0,f2,f0          */
	.long	0xec820d04	/* <+456>: dcmpu   cr1,f2,f1         */
	.long	0xfc820504	/* <+460>: dcmpuq  cr1,f2,f0         */
	.long	0x7c05071d	/* <+464>: tabort. r5                */
	.long	0x7ce8861d	/* <+468>: tabortwc. 7,r8,r16        */
	.long	0x7e8b565d	/* <+472>: tabortdc. 20,r11,r10      */
	.long	0x7e2a9e9d	/* <+476>: tabortwci. 17,r10,-13     */
	.long	0x7fa3dedd	/* <+480>: tabortdci. 29,r3,-5       */
	.long	0x7c00051d	/* <+484>: tbegin.                   */
	.long	0x7f80059c	/* <+488>: tcheck  cr7               */
	.long	0x7c00055d	/* <+492>: tend.                     */
	.long	0x7e00055d	/* <+496>: tendall.                  */
	.long	0x7c18075d	/* <+500>: treclaim. r24             */
	.long	0x7c0007dd	/* <+504>: trechkpt.                 */
	.long	0x7c0005dd	/* <+508>: tsuspend.                 */
	.long	0x7c2005dd	/* <+512>: tresume.                  */
	.long	0x4c000124	/* <+516>: rfebb   0                 */
	.long	0x4c000924	/* <+520>: rfebb   1                 */
	.long	0x4d950460	/* <+524>: bctar-  12,4*cr5+gt       */
	.long	0x4c870461	/* <+528>: bctarl- 4,4*cr1+so        */
	.long	0x4dac0460	/* <+532>: bctar+  12,4*cr3+lt       */
	.long	0x4ca20461	/* <+536>: bctarl+ 4,eq              */
	.long	0x4c880c60	/* <+540>: bctar   4,4*cr2+lt,1      */
	.long	0x4c871461	/* <+544>: bctarl  4,4*cr1+so,2      */
	.long	0x7c00003c	/* <+548>: waitasec                  */
	.long	0x7c00411c	/* <+552>: msgsndp r8                */
	.long	0x7c200126	/* <+556>: mtsle   1                 */
	.long	0x7c00d95c	/* <+560>: msgclrp r27               */
	.long	0x7d4a616d	/* <+564>: stqcx.  r10,r10,r12       */
	.long	0x7f80396d	/* <+568>: stqcx.  r28,0,r7          */
	.long	0x7f135a28	/* <+572>: lqarx   r24,r19,r11       */
	.long	0x7ec05a28	/* <+576>: lqarx   r22,0,r11         */
	.long	0x7e80325c	/* <+580>: mfbhrbe r20,6             */
	.long	0x7fb18329	/* <+584>: pbt.    r29,r17,r16       */
	.long	0x7dc03b29	/* <+588>: pbt.    r14,0,r7          */
	.long	0x7c00035c	/* <+592>: clrbhrb                   */
	.long	0x116a05ed	/* <+596>: vpermxor v11,v10,v0,v23   */
	.long	0x1302393c	/* <+600>: vaddeuqm v24,v2,v7,v4     */
	.long	0x114a40bd	/* <+604>: vaddecuq v10,v10,v8,v2    */
	.long	0x10af44fe	/* <+608>: vsubeuqm v5,v15,v8,v19    */
	.long	0x119f877f	/* <+612>: vsubecuq v12,v31,v16,v29  */
	.long	0x129d6888	/* <+616>: vmulouw v20,v29,v13       */
	.long	0x13a0d089	/* <+620>: vmuluwm v29,v0,v26        */
	.long	0x1115e0c0	/* <+624>: vaddudm v8,v21,v28        */
	.long	0x103a08c2	/* <+628>: vmaxud  v1,v26,v1         */
	.long	0x128308c4	/* <+632>: vrld    v20,v3,v1         */
	.long	0x109358c7	/* <+636>: vcmpequd v4,v19,v11       */
	.long	0x12eef100	/* <+640>: vadduqm v23,v14,v30       */
	.long	0x11086940	/* <+644>: vaddcuq v8,v8,v13         */
	.long	0x139b2188	/* <+648>: vmulosw v28,v27,v4        */
	.long	0x106421c2	/* <+652>: vmaxsd  v3,v4,v4          */
	.long	0x1013aa88	/* <+656>: vmuleuw v0,v19,v21        */
	.long	0x13149ac2	/* <+660>: vminud  v24,v20,v19       */
	.long	0x101c7ac7	/* <+664>: vcmpgtud v0,v28,v15       */
	.long	0x12a01388	/* <+668>: vmulesw v21,v0,v2         */
	.long	0x113a4bc2	/* <+672>: vminsd  v9,v26,v9         */
	.long	0x133d5bc4	/* <+676>: vsrad   v25,v29,v11       */
	.long	0x117c5bc7	/* <+680>: vcmpgtsd v11,v28,v11      */
	.long	0x10a8d601	/* <+684>: bcdadd. v5,v8,v26,1       */
	.long	0x10836408	/* <+688>: vpmsumb v4,v3,v12         */
	.long	0x135fae41	/* <+692>: bcdsub. v26,v31,v21,1     */
	.long	0x10b18448	/* <+696>: vpmsumh v5,v17,v16        */
	.long	0x12f1a44e	/* <+700>: vpkudum v23,v17,v20       */
	.long	0x1315ec88	/* <+704>: vpmsumw v24,v21,v29       */
	.long	0x11366cc8	/* <+708>: vpmsumd v9,v22,v13        */
	.long	0x125394ce	/* <+712>: vpkudus v18,v19,v18       */
	.long	0x13d0b500	/* <+716>: vsubuqm v30,v16,v22       */
	.long	0x11cb3d08	/* <+720>: vcipher v14,v11,v7        */
	.long	0x1142b509	/* <+724>: vcipherlast v10,v2,v22    */
	.long	0x12e06d0c	/* <+728>: vgbbd   v23,v13           */
	.long	0x12198540	/* <+732>: vsubcuq v16,v25,v16       */
	.long	0x13e12d44	/* <+736>: vorc    v31,v1,v5         */
	.long	0x1091fd48	/* <+740>: vncipher v4,v17,v31       */
	.long	0x1302dd49	/* <+744>: vncipherlast v24,v2,v27   */
	.long	0x12f5bd4c	/* <+748>: vbpermq v23,v21,v23       */
	.long	0x13724d4e	/* <+752>: vpksdus v27,v18,v9        */
	.long	0x137ddd84	/* <+756>: vnand   v27,v29,v27       */
	.long	0x1273c5c4	/* <+760>: vsld    v19,v19,v24       */
	.long	0x10ad05c8	/* <+764>: vsbox   v5,v13            */
	.long	0x13233dce	/* <+768>: vpksdss v25,v3,v7         */
	.long	0x138804c7	/* <+772>: vcmpequd. v28,v8,v0       */
	.long	0x1340d64e	/* <+776>: vupkhsw v26,v26           */
	.long	0x10a73682	/* <+780>: vshasigmaw v5,v7,0,6      */
	.long	0x13957684	/* <+784>: veqv    v28,v21,v14       */
	.long	0x10289e8c	/* <+788>: vmrgow  v1,v8,v19         */
	.long	0x100a56c2	/* <+792>: vshasigmad v0,v10,0,10    */
	.long	0x10bb76c4	/* <+796>: vsrd    v5,v27,v14        */
	.long	0x11606ece	/* <+800>: vupklsw v11,v13           */
	.long	0x11c08702	/* <+804>: vclzb   v14,v16           */
	.long	0x1280df03	/* <+808>: vpopcntb v20,v27          */
	.long	0x13805f42	/* <+812>: vclzh   v28,v11           */
	.long	0x13004f43	/* <+816>: vpopcnth v24,v9           */
	.long	0x1360ff82	/* <+820>: vclzw   v27,v31           */
	.long	0x12209f83	/* <+824>: vpopcntw v17,v19          */
	.long	0x1180efc2	/* <+828>: vclzd   v12,v29           */
	.long	0x12e0b7c3	/* <+832>: vpopcntd v23,v22          */
	.long	0x1314eec7	/* <+836>: vcmpgtud. v24,v20,v29     */
	.long	0x1126dfc7	/* <+840>: vcmpgtsd. v9,v6,v27       */
	.long	0x7fced019	/* <+844>: lxsiwzx vs62,r14,r26      */
	.long	0x7d00c819	/* <+848>: lxsiwzx vs40,0,r25        */
	.long	0x7f20d098	/* <+852>: lxsiwax vs25,0,r26        */
	.long	0x7c601898	/* <+856>: lxsiwax vs3,0,r3          */
	.long	0x7fcc0067	/* <+860>: mfvsrd  r12,vs62          */
	.long	0x7d9400e6	/* <+864>: mffprwz r20,f12           */
	.long	0x7dc97118	/* <+868>: stxsiwx vs14,r9,r14       */
	.long	0x7ea04118	/* <+872>: stxsiwx vs21,0,r8         */
	.long	0x7e0b0167	/* <+876>: mtvsrd  vs48,r11          */
	.long	0x7ff701a7	/* <+880>: mtvrwa  v31,r23           */
	.long	0x7e1a01e6	/* <+884>: mtfprwz f16,r26           */
	.long	0x7db36c18	/* <+888>: lxsspx  vs13,r19,r13      */
	.long	0x7e406c18	/* <+892>: lxsspx  vs18,0,r13        */
	.long	0x7d622519	/* <+896>: stxsspx vs43,r2,r4        */
	.long	0x7ee05d19	/* <+900>: stxsspx vs55,0,r11        */
	.long	0xf2d0c805	/* <+904>: xsaddsp vs54,vs48,vs25    */
	.long	0xf1d2080c	/* <+908>: xsmaddasp vs14,vs50,vs1   */
	.long	0xf3565042	/* <+912>: xssubsp vs26,vs22,vs42    */
	.long	0xf375a04e	/* <+916>: xsmaddmsp vs27,vs53,vs52  */
	.long	0xf100d82a	/* <+920>: xsrsqrtesp vs8,vs59       */
	.long	0xf180482e	/* <+924>: xssqrtsp vs12,vs41        */
	.long	0xf32b0083	/* <+928>: xsmulsp vs57,vs11,vs32    */
	.long	0xf0d4d089	/* <+932>: xsmsubasp vs38,vs20,vs26  */
	.long	0xf35330c0	/* <+936>: xsdivsp vs26,vs19,vs6     */
	.long	0xf065b8cf	/* <+940>: xsmsubmsp vs35,vs37,vs55  */
	.long	0xf3604069	/* <+944>: xsresp  vs59,vs8          */
	.long	0xf1810c0f	/* <+948>: xsnmaddasp vs44,vs33,vs33 */
	.long	0xf23ef44c	/* <+952>: xsnmaddmsp vs17,vs62,vs30 */
	.long	0xf2d4fc8d	/* <+956>: xsnmsubasp vs54,vs52,vs31 */
	.long	0xf0a5d4cb	/* <+960>: xsnmsubmsp vs37,vs5,vs58  */
	.long	0xf3d66556	/* <+964>: xxlorc  vs30,vs54,vs44    */
	.long	0xf22eed91	/* <+968>: xxlnand vs49,vs14,vs29    */
	.long	0xf3d6f5d1	/* <+972>: xxleqv  vs62,vs22,vs30    */
	.long	0xf380b42f	/* <+976>: xscvdpspn vs60,vs54       */
	.long	0xf2c06c66	/* <+980>: xsrsp   vs22,vs45         */
	.long	0xf340dca2	/* <+984>: xscvuxdsp vs26,vs59       */
	.long	0xf0c08ce3	/* <+988>: xscvsxdsp vs38,vs49       */
	.long	0xf360d52d	/* <+992>: xscvspdpn vs59,vs26       */
	.long	0xff0e168c	/* <+996>: fmrgow  f24,f14,f2        */
	.long	0xfec72f8c	/* <+1000>: fmrgew  f22,f7,f5        */
