	.text

	p0.H = 0x12345678;
	P0.l = 0x12345678;

	CC = R3 < 4;
	CC = R3 < 7;
	CC = R3 < 8;
	CC = R3 <= 4;
	CC = R3 <= 7;
	CC = R3 <= 8;

	A1 -= M2.h * R3.L, A0 -= M2.l * R3.L;

	R1.H = (A1=R7.L*R5.L) , A0 += R1.L*R0.L (IS);

	a0 += R2.L * R3.L (IU);
	a0 += R2.L * R3.L (T);
	a0 += R2.L * R3.L (TFU);
	a0 += R2.L * R3.L (S2RND);
	a0 += R2.L * R3.L (ISS2);
	a0 += R2.L * R3.L (IH);
	R0.H = (A1 = R4.L * R3.L) (T), A0 = R4.H * R3.L;
	R0.L = (A0 = R7.L * R4.H) (T), A1 += R7.H * R4.H;

	R0 = (A1 += R1.H * R3.H) (IU)
	R0.L = (A1 += R1.H * R3.H) (IU)
	R1 = (A0 += R1.H * R3.H) (IU)
	R1.H = (A0 += R1.H * R3.H) (IU)

	W [p0 + 1] = r0;
	[p0 + 1] = r0;
	[p0 + 2] = r0;
	[p0 + 3] = r0;

	B [p0 + 32768] = r0;
	W [p0 + 65536] = r0;
	[p0 + 131072] = r0;

	B [p0 + -32769] = r0;
	W [p0 + -65538] = r0;
	[p0 + -131076] = r0;

	r0 = W [p0 + 1] (x);
	r0 = [p0 + 1];
	r0 = [p0 + 2];
	r0 = [p0 + 3];

	r0 = [p0 + foo];
	r0 = W [p0 + foo];
	r0 = B [p0 + foo];

	r0 = [p0 + 131076];
	r0 = W [p0 + 65536];
	r0 = B [p0 + 131076];

	[ R0 ++ M2 ] = R2;
	[ I0 ++ R2 ] = R2;
	[ R0 ++ P2 ] = R2;
	[ P0 ++ R2 ] = R2;
	[ P0 ++ M2 ] = R2;
	[ I0 ++ P2 ] = R2;

	W [ R0 ++ M2 ] = R2.h;
	W [ I0 ++ R2 ] = R2.h;
	W [ R0 ++ P2 ] = R2.h;
	W [ P0 ++ R2 ] = R2.h;
	W [ P0 ++ M2 ] = R2.h;
	W [ I0 ++ P2 ] = R2.h;

	[ R0 ++ ] = R2;
	[ I0 ++ ] = P2;

	W [ R0 ++ ] = R2.h;
	W [ I0 ++ ] = P2.h;

	W [ R0 ++ ] = R2;
	W [ I0 ++ ] = R2;
	W [ P0 ++ ] = P2;

	B [ R0 ++ ] = R2;
	B [ I0 ++ ] = R2;
	B [ P0 ++ ] = P2;

	R2 = [ R0 ++ M2 ];
	R2 = [ I0 ++ R2 ];
	R2 = [ R0 ++ P2 ];
	R2 = [ P0 ++ R2 ];
	R2 = [ P0 ++ M2 ];
	R2 = [ I0 ++ P2 ];

	R2.h = W [ R0 ++ M2 ];
	R2.h = W [ I0 ++ R2 ];
	R2.h = W [ R0 ++ P2 ];
	R2.h = W [ P0 ++ R2 ];
	R2.h = W [ P0 ++ M2 ];
	R2.h = W [ I0 ++ P2 ];

	R2 = [ R0 ++ ];
	P2 = [ I0 ++ ];

	R0.l = B [ P0 ++ ];
	R0.l = B [ I0 ++ ];

	R0.l = W [ P0 ++ ];
	R2.h = W [ R0 ++ ];
	P2.h = W [ I0 ++ ];

	R2 = W [ R0 ++ ] (X);
	R2 = W [ I0 ++ ] (X);
	P2 = W [ P0 ++ ] (X);

	R2 = B [ R0 ++ ] (X);
	R2 = B [ I0 ++ ] (X);
	P2 = B [ P0 ++ ] (X);
