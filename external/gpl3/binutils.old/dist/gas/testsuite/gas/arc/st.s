# st test

	st	r1,[r2]
	st	r1,[r2,14]
	stb	r1,[r2]
	st.a	r1,[r3,14]
	stw.a	r1,[r2,2]
	st	r1,[900]
	stb	0,[r2]
	st	-8,[r2,-8]
	st	r2,[@foo]
	st.di	r1,[r2,2]
	st.a.di	r1,[r2,3]
	stw.a.di r1,[r2,4]

	st @.L1,[r12,4]
.L1:

	sr	r1,[r2]
	sr	r1,[14]
	sr	1000, [r1]
	sr	100,  [r2]
	sr	r1,[10000]
	sr	100,[63]
	sr	10000,[100]
