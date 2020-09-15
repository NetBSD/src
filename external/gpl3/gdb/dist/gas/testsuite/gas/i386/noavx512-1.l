.*: Assembler messages:
.*:25: Error: .*unsupported instruction.*
.*:26: Error: .*unsupported masking.*
.*:27: Error: .*unsupported masking.*
.*:47: Error: .*unsupported instruction.*
.*:48: Error: .*unsupported masking.*
.*:49: Error: .*unsupported masking.*
.*:50: Error: .*not supported.*
.*:51: Error: .*not supported.*
.*:52: Error: .*not supported.*
.*:69: Error: .*unsupported instruction.*
.*:70: Error: .*unsupported masking.*
.*:71: Error: .*unsupported masking.*
.*:72: Error: .*not supported.*
.*:73: Error: .*not supported.*
.*:74: Error: .*not supported.*
.*:75: Error: .*not supported.*
.*:76: Error: .*not supported.*
.*:77: Error: .*not supported.*
.*:91: Error: .*unsupported instruction.*
.*:92: Error: .*unsupported masking.*
.*:93: Error: .*unsupported masking.*
.*:94: Error: .*not supported.*
.*:95: Error: .*not supported.*
.*:96: Error: .*not supported.*
.*:97: Error: .*not supported.*
.*:98: Error: .*not supported.*
.*:99: Error: .*not supported.*
.*:100: Error: .*not supported.*
.*:113: Error: .*unsupported instruction.*
.*:114: Error: .*unsupported masking.*
.*:115: Error: .*unsupported masking.*
.*:116: Error: .*not supported.*
.*:117: Error: .*not supported.*
.*:118: Error: .*not supported.*
.*:119: Error: .*not supported.*
.*:120: Error: .*not supported.*
.*:121: Error: .*not supported.*
.*:122: Error: .*not supported.*
.*:126: Error: .*not supported.*
.*:127: Error: .*not supported.*
.*:128: Error: .*not supported.*
.*:135: Error: .*unsupported instruction.*
.*:136: Error: .*unsupported masking.*
.*:137: Error: .*unsupported masking.*
.*:138: Error: .*not supported.*
.*:139: Error: .*not supported.*
.*:140: Error: .*not supported.*
.*:141: Error: .*not supported.*
.*:142: Error: .*not supported.*
.*:143: Error: .*not supported.*
.*:144: Error: .*not supported.*
.*:148: Error: .*not supported.*
.*:149: Error: .*not supported.*
.*:150: Error: .*not supported.*
.*:151: Error: .*not supported.*
.*:157: Error: .*unsupported instruction.*
.*:158: Error: .*unsupported masking.*
.*:159: Error: .*unsupported masking.*
.*:160: Error: .*not supported.*
.*:161: Error: .*not supported.*
.*:162: Error: .*not supported.*
.*:163: Error: .*not supported.*
.*:164: Error: .*not supported.*
.*:165: Error: .*not supported.*
.*:166: Error: .*not supported.*
.*:170: Error: .*not supported.*
.*:171: Error: .*not supported.*
.*:172: Error: .*not supported.*
.*:173: Error: .*not supported.*
.*:174: Error: .*not supported.*
.*:175: Error: .*not supported.*
.*:176: Error: .*not supported.*
.*:179: Error: .*bad register name.*
.*:180: Error: .*unknown vector operation.*
.*:181: Error: .*unknown vector operation.*
.*:182: Error: .*not supported.*
.*:183: Error: .*not supported.*
.*:184: Error: .*not supported.*
.*:185: Error: .*not supported.*
.*:186: Error: .*not supported.*
.*:187: Error: .*not supported.*
.*:188: Error: .*not supported.*
.*:189: Error: .*bad register name.*
.*:190: Error: .*unknown vector operation.*
.*:191: Error: .*unknown vector operation.*
.*:192: Error: .*not supported.*
.*:193: Error: .*not supported.*
.*:194: Error: .*not supported.*
.*:195: Error: .*not supported.*
.*:196: Error: .*not supported.*
.*:197: Error: .*not supported.*
.*:198: Error: .*not supported.*
GAS LISTING .*
#...
[ 	]*1[ 	]+\# Test \.arch \.noavx512XX
[ 	]*2[ 	]+\.text
[ 	]*3[ 	]+\?\?\?\? 62F27D4F 		vpabsb %zmm5, %zmm6\{%k7\}		\# AVX512BW
[ 	]*3[ 	]+1CF5
[ 	]*4[ 	]+\?\?\?\? 62F27D0F 		vpabsb %xmm5, %xmm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*4[ 	]+1CF5
[ 	]*5[ 	]+\?\?\?\? 62F27D2F 		vpabsb %ymm5, %ymm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*5[ 	]+1CF5
[ 	]*6[ 	]+\?\?\?\? 62F27D48 		vpconflictd %zmm5, %zmm6		\# AVX412CD
[ 	]*6[ 	]+C4F5
[ 	]*7[ 	]+\?\?\?\? 62F27D08 		vpconflictd %xmm5, %xmm6		\# AVX412CD \+ AVX512VL
[ 	]*7[ 	]+C4F5
[ 	]*8[ 	]+\?\?\?\? 62F27D28 		vpconflictd %ymm5, %ymm6		\# AVX412CD \+ AVX512VL
[ 	]*8[ 	]+C4F5
[ 	]*9[ 	]+\?\?\?\? 62F1FD4F 		vcvtpd2qq \(%ecx\), %zmm6\{%k7\}		\# AVX512DQ
[ 	]*9[ 	]+7B31
[ 	]*10[ 	]+\?\?\?\? 62F1FD0F 		vcvtpd2qq \(%ecx\), %xmm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*10[ 	]+7B31
[ 	]*11[ 	]+\?\?\?\? 62F1FD2F 		vcvtpd2qq \(%ecx\), %ymm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*11[ 	]+7B31
[ 	]*12[ 	]+\?\?\?\? 62F27D4F 		vexp2ps %zmm5, %zmm6\{%k7\}		\# AVX512ER
[ 	]*12[ 	]+C8F5
[ 	]*13[ 	]+\?\?\?\? 62F1D54F 		vaddpd %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512F
[ 	]*13[ 	]+58F4
[ 	]*14[ 	]+\?\?\?\? 62F1D50F 		vaddpd %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*14[ 	]+58F4
[ 	]*15[ 	]+\?\?\?\? 62F1D52F 		vaddpd %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*15[ 	]+58F4
[ 	]*16[ 	]+\?\?\?\? 62F2D54F 		vpmadd52luq %zmm4, %zmm5, %zmm6\{%k7\}	\# AVX512IFMA
[ 	]*16[ 	]+B4F4
[ 	]*17[ 	]+\?\?\?\? 62F2D50F 		vpmadd52luq %xmm4, %xmm5, %xmm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*17[ 	]+B4F4
[ 	]*18[ 	]+\?\?\?\? 62F2D52F 		vpmadd52luq %ymm4, %ymm5, %ymm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*18[ 	]+B4F4
[ 	]*19[ 	]+\?\?\?\? 62F2FD49 		vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}	\# AVX512PF
[ 	]*19[ 	]+C68CFD17 
[ 	]*19[ 	]+000000
[ 	]*20[ 	]+\?\?\?\? 62F2554F 		vpermb %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512VBMI
[ 	]*20[ 	]+8DF4
[ 	]*21[ 	]+\?\?\?\? 62F2550F 		vpermb %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*21[ 	]+8DF4
[ 	]*22[ 	]+\?\?\?\? 62F2552F 		vpermb %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*22[ 	]+8DF4
[ 	]*23[ 	]+
[ 	]*24[ 	]+\.arch \.noavx512bw
[ 	]*25[ 	]+vpabsb %zmm5, %zmm6\{%k7\}		\# AVX512BW
[ 	]*26[ 	]+vpabsb %xmm5, %xmm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*27[ 	]+vpabsb %ymm5, %ymm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*28[ 	]+\?\?\?\? 62F27D48 		vpconflictd %zmm5, %zmm6		\# AVX412CD
[ 	]*28[ 	]+C4F5
[ 	]*29[ 	]+\?\?\?\? 62F27D08 		vpconflictd %xmm5, %xmm6		\# AVX412CD \+ AVX512VL
[ 	]*29[ 	]+C4F5
[ 	]*30[ 	]+\?\?\?\? 62F27D28 		vpconflictd %ymm5, %ymm6		\# AVX412CD \+ AVX512VL
[ 	]*30[ 	]+C4F5
[ 	]*31[ 	]+\?\?\?\? 62F1FD4F 		vcvtpd2qq \(%ecx\), %zmm6\{%k7\}		\# AVX512DQ
[ 	]*31[ 	]+7B31
[ 	]*32[ 	]+\?\?\?\? 62F1FD0F 		vcvtpd2qq \(%ecx\), %xmm6\{%k7\}		\# AVX512DQ \+ AVX512VL
GAS LISTING .*


[ 	]*32[ 	]+7B31
[ 	]*33[ 	]+\?\?\?\? 62F1FD2F 		vcvtpd2qq \(%ecx\), %ymm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*33[ 	]+7B31
[ 	]*34[ 	]+\?\?\?\? 62F27D4F 		vexp2ps %zmm5, %zmm6\{%k7\}		\# AVX512ER
[ 	]*34[ 	]+C8F5
[ 	]*35[ 	]+\?\?\?\? 62F1D54F 		vaddpd %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512F
[ 	]*35[ 	]+58F4
[ 	]*36[ 	]+\?\?\?\? 62F1D50F 		vaddpd %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*36[ 	]+58F4
[ 	]*37[ 	]+\?\?\?\? 62F1D52F 		vaddpd %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*37[ 	]+58F4
[ 	]*38[ 	]+\?\?\?\? 62F2D54F 		vpmadd52luq %zmm4, %zmm5, %zmm6\{%k7\}	\# AVX512IFMA
[ 	]*38[ 	]+B4F4
[ 	]*39[ 	]+\?\?\?\? 62F2D50F 		vpmadd52luq %xmm4, %xmm5, %xmm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*39[ 	]+B4F4
[ 	]*40[ 	]+\?\?\?\? 62F2D52F 		vpmadd52luq %ymm4, %ymm5, %ymm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*40[ 	]+B4F4
[ 	]*41[ 	]+\?\?\?\? 62F2FD49 		vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}	\# AVX512PF
[ 	]*41[ 	]+C68CFD17 
[ 	]*41[ 	]+000000
[ 	]*42[ 	]+\?\?\?\? 62F2554F 		vpermb %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512VBMI
[ 	]*42[ 	]+8DF4
[ 	]*43[ 	]+\?\?\?\? 62F2550F 		vpermb %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*43[ 	]+8DF4
[ 	]*44[ 	]+\?\?\?\? 62F2552F 		vpermb %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*44[ 	]+8DF4
[ 	]*45[ 	]+
[ 	]*46[ 	]+\.arch \.noavx512cd
[ 	]*47[ 	]+vpabsb %zmm5, %zmm6\{%k7\}		\# AVX512BW
[ 	]*48[ 	]+vpabsb %xmm5, %xmm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*49[ 	]+vpabsb %ymm5, %ymm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*50[ 	]+vpconflictd %zmm5, %zmm6		\# AVX412CD
[ 	]*51[ 	]+vpconflictd %xmm5, %xmm6		\# AVX412CD \+ AVX512VL
[ 	]*52[ 	]+vpconflictd %ymm5, %ymm6		\# AVX412CD \+ AVX512VL
[ 	]*53[ 	]+\?\?\?\? 62F1FD4F 		vcvtpd2qq \(%ecx\), %zmm6\{%k7\}		\# AVX512DQ
[ 	]*53[ 	]+7B31
[ 	]*54[ 	]+\?\?\?\? 62F1FD0F 		vcvtpd2qq \(%ecx\), %xmm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*54[ 	]+7B31
[ 	]*55[ 	]+\?\?\?\? 62F1FD2F 		vcvtpd2qq \(%ecx\), %ymm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*55[ 	]+7B31
[ 	]*56[ 	]+\?\?\?\? 62F27D4F 		vexp2ps %zmm5, %zmm6\{%k7\}		\# AVX512ER
[ 	]*56[ 	]+C8F5
[ 	]*57[ 	]+\?\?\?\? 62F1D54F 		vaddpd %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512F
[ 	]*57[ 	]+58F4
[ 	]*58[ 	]+\?\?\?\? 62F1D50F 		vaddpd %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*58[ 	]+58F4
[ 	]*59[ 	]+\?\?\?\? 62F1D52F 		vaddpd %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*59[ 	]+58F4
[ 	]*60[ 	]+\?\?\?\? 62F2D54F 		vpmadd52luq %zmm4, %zmm5, %zmm6\{%k7\}	\# AVX512IFMA
[ 	]*60[ 	]+B4F4
[ 	]*61[ 	]+\?\?\?\? 62F2D50F 		vpmadd52luq %xmm4, %xmm5, %xmm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*61[ 	]+B4F4
[ 	]*62[ 	]+\?\?\?\? 62F2D52F 		vpmadd52luq %ymm4, %ymm5, %ymm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*62[ 	]+B4F4
[ 	]*63[ 	]+\?\?\?\? 62F2FD49 		vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}	\# AVX512PF
[ 	]*63[ 	]+C68CFD17 
[ 	]*63[ 	]+000000
GAS LISTING .*


[ 	]*64[ 	]+\?\?\?\? 62F2554F 		vpermb %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512VBMI
[ 	]*64[ 	]+8DF4
[ 	]*65[ 	]+\?\?\?\? 62F2550F 		vpermb %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*65[ 	]+8DF4
[ 	]*66[ 	]+\?\?\?\? 62F2552F 		vpermb %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*66[ 	]+8DF4
[ 	]*67[ 	]+
[ 	]*68[ 	]+\.arch \.noavx512dq
[ 	]*69[ 	]+vpabsb %zmm5, %zmm6\{%k7\}		\# AVX512BW
[ 	]*70[ 	]+vpabsb %xmm5, %xmm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*71[ 	]+vpabsb %ymm5, %ymm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*72[ 	]+vpconflictd %zmm5, %zmm6		\# AVX412CD
[ 	]*73[ 	]+vpconflictd %xmm5, %xmm6		\# AVX412CD \+ AVX512VL
[ 	]*74[ 	]+vpconflictd %ymm5, %ymm6		\# AVX412CD \+ AVX512VL
[ 	]*75[ 	]+vcvtpd2qq \(%ecx\), %zmm6\{%k7\}		\# AVX512DQ
[ 	]*76[ 	]+vcvtpd2qq \(%ecx\), %xmm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*77[ 	]+vcvtpd2qq \(%ecx\), %ymm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*78[ 	]+\?\?\?\? 62F27D4F 		vexp2ps %zmm5, %zmm6\{%k7\}		\# AVX512ER
[ 	]*78[ 	]+C8F5
[ 	]*79[ 	]+\?\?\?\? 62F1D54F 		vaddpd %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512F
[ 	]*79[ 	]+58F4
[ 	]*80[ 	]+\?\?\?\? 62F1D50F 		vaddpd %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*80[ 	]+58F4
[ 	]*81[ 	]+\?\?\?\? 62F1D52F 		vaddpd %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*81[ 	]+58F4
[ 	]*82[ 	]+\?\?\?\? 62F2D54F 		vpmadd52luq %zmm4, %zmm5, %zmm6\{%k7\}	\# AVX512IFMA
[ 	]*82[ 	]+B4F4
[ 	]*83[ 	]+\?\?\?\? 62F2D50F 		vpmadd52luq %xmm4, %xmm5, %xmm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*83[ 	]+B4F4
[ 	]*84[ 	]+\?\?\?\? 62F2D52F 		vpmadd52luq %ymm4, %ymm5, %ymm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*84[ 	]+B4F4
[ 	]*85[ 	]+\?\?\?\? 62F2FD49 		vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}	\# AVX512PF
[ 	]*85[ 	]+C68CFD17 
[ 	]*85[ 	]+000000
[ 	]*86[ 	]+\?\?\?\? 62F2554F 		vpermb %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512VBMI
[ 	]*86[ 	]+8DF4
[ 	]*87[ 	]+\?\?\?\? 62F2550F 		vpermb %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*87[ 	]+8DF4
[ 	]*88[ 	]+\?\?\?\? 62F2552F 		vpermb %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*88[ 	]+8DF4
[ 	]*89[ 	]+
[ 	]*90[ 	]+\.arch \.noavx512er
[ 	]*91[ 	]+vpabsb %zmm5, %zmm6\{%k7\}		\# AVX512BW
[ 	]*92[ 	]+vpabsb %xmm5, %xmm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*93[ 	]+vpabsb %ymm5, %ymm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*94[ 	]+vpconflictd %zmm5, %zmm6		\# AVX412CD
[ 	]*95[ 	]+vpconflictd %xmm5, %xmm6		\# AVX412CD \+ AVX512VL
[ 	]*96[ 	]+vpconflictd %ymm5, %ymm6		\# AVX412CD \+ AVX512VL
[ 	]*97[ 	]+vcvtpd2qq \(%ecx\), %zmm6\{%k7\}		\# AVX512DQ
[ 	]*98[ 	]+vcvtpd2qq \(%ecx\), %xmm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*99[ 	]+vcvtpd2qq \(%ecx\), %ymm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*100[ 	]+vexp2ps %zmm5, %zmm6\{%k7\}		\# AVX512ER
[ 	]*101[ 	]+\?\?\?\? 62F1D54F 		vaddpd %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512F
[ 	]*101[ 	]+58F4
[ 	]*102[ 	]+\?\?\?\? 62F1D50F 		vaddpd %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*102[ 	]+58F4
[ 	]*103[ 	]+\?\?\?\? 62F1D52F 		vaddpd %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512F \+ AVX512VL
GAS LISTING .*


[ 	]*103[ 	]+58F4
[ 	]*104[ 	]+\?\?\?\? 62F2D54F 		vpmadd52luq %zmm4, %zmm5, %zmm6\{%k7\}	\# AVX512IFMA
[ 	]*104[ 	]+B4F4
[ 	]*105[ 	]+\?\?\?\? 62F2D50F 		vpmadd52luq %xmm4, %xmm5, %xmm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*105[ 	]+B4F4
[ 	]*106[ 	]+\?\?\?\? 62F2D52F 		vpmadd52luq %ymm4, %ymm5, %ymm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*106[ 	]+B4F4
[ 	]*107[ 	]+\?\?\?\? 62F2FD49 		vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}	\# AVX512PF
[ 	]*107[ 	]+C68CFD17 
[ 	]*107[ 	]+000000
[ 	]*108[ 	]+\?\?\?\? 62F2554F 		vpermb %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512VBMI
[ 	]*108[ 	]+8DF4
[ 	]*109[ 	]+\?\?\?\? 62F2550F 		vpermb %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*109[ 	]+8DF4
[ 	]*110[ 	]+\?\?\?\? 62F2552F 		vpermb %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*110[ 	]+8DF4
[ 	]*111[ 	]+
[ 	]*112[ 	]+\.arch \.noavx512ifma
[ 	]*113[ 	]+vpabsb %zmm5, %zmm6\{%k7\}		\# AVX512BW
[ 	]*114[ 	]+vpabsb %xmm5, %xmm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*115[ 	]+vpabsb %ymm5, %ymm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*116[ 	]+vpconflictd %zmm5, %zmm6		\# AVX412CD
[ 	]*117[ 	]+vpconflictd %xmm5, %xmm6		\# AVX412CD \+ AVX512VL
[ 	]*118[ 	]+vpconflictd %ymm5, %ymm6		\# AVX412CD \+ AVX512VL
[ 	]*119[ 	]+vcvtpd2qq \(%ecx\), %zmm6\{%k7\}		\# AVX512DQ
[ 	]*120[ 	]+vcvtpd2qq \(%ecx\), %xmm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*121[ 	]+vcvtpd2qq \(%ecx\), %ymm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*122[ 	]+vexp2ps %zmm5, %zmm6\{%k7\}		\# AVX512ER
[ 	]*123[ 	]+\?\?\?\? 62F1D54F 		vaddpd %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512F
[ 	]*123[ 	]+58F4
[ 	]*124[ 	]+\?\?\?\? 62F1D50F 		vaddpd %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*124[ 	]+58F4
[ 	]*125[ 	]+\?\?\?\? 62F1D52F 		vaddpd %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*125[ 	]+58F4
[ 	]*126[ 	]+vpmadd52luq %zmm4, %zmm5, %zmm6\{%k7\}	\# AVX512IFMA
[ 	]*127[ 	]+vpmadd52luq %xmm4, %xmm5, %xmm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*128[ 	]+vpmadd52luq %ymm4, %ymm5, %ymm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*129[ 	]+\?\?\?\? 62F2FD49 		vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}	\# AVX512PF
[ 	]*129[ 	]+C68CFD17 
[ 	]*129[ 	]+000000
[ 	]*130[ 	]+\?\?\?\? 62F2554F 		vpermb %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512VBMI
[ 	]*130[ 	]+8DF4
[ 	]*131[ 	]+\?\?\?\? 62F2550F 		vpermb %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*131[ 	]+8DF4
[ 	]*132[ 	]+\?\?\?\? 62F2552F 		vpermb %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*132[ 	]+8DF4
[ 	]*133[ 	]+
[ 	]*134[ 	]+\.arch \.noavx512pf
[ 	]*135[ 	]+vpabsb %zmm5, %zmm6\{%k7\}		\# AVX512BW
[ 	]*136[ 	]+vpabsb %xmm5, %xmm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*137[ 	]+vpabsb %ymm5, %ymm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*138[ 	]+vpconflictd %zmm5, %zmm6		\# AVX412CD
[ 	]*139[ 	]+vpconflictd %xmm5, %xmm6		\# AVX412CD \+ AVX512VL
[ 	]*140[ 	]+vpconflictd %ymm5, %ymm6		\# AVX412CD \+ AVX512VL
[ 	]*141[ 	]+vcvtpd2qq \(%ecx\), %zmm6\{%k7\}		\# AVX512DQ
[ 	]*142[ 	]+vcvtpd2qq \(%ecx\), %xmm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*143[ 	]+vcvtpd2qq \(%ecx\), %ymm6\{%k7\}		\# AVX512DQ \+ AVX512VL
GAS LISTING .*


[ 	]*144[ 	]+vexp2ps %zmm5, %zmm6\{%k7\}		\# AVX512ER
[ 	]*145[ 	]+\?\?\?\? 62F1D54F 		vaddpd %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512F
[ 	]*145[ 	]+58F4
[ 	]*146[ 	]+\?\?\?\? 62F1D50F 		vaddpd %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*146[ 	]+58F4
[ 	]*147[ 	]+\?\?\?\? 62F1D52F 		vaddpd %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*147[ 	]+58F4
[ 	]*148[ 	]+vpmadd52luq %zmm4, %zmm5, %zmm6\{%k7\}	\# AVX512IFMA
[ 	]*149[ 	]+vpmadd52luq %xmm4, %xmm5, %xmm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*150[ 	]+vpmadd52luq %ymm4, %ymm5, %ymm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*151[ 	]+vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}	\# AVX512PF
[ 	]*152[ 	]+\?\?\?\? 62F2554F 		vpermb %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512VBMI
[ 	]*152[ 	]+8DF4
[ 	]*153[ 	]+\?\?\?\? 62F2550F 		vpermb %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*153[ 	]+8DF4
[ 	]*154[ 	]+\?\?\?\? 62F2552F 		vpermb %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*154[ 	]+8DF4
[ 	]*155[ 	]+
[ 	]*156[ 	]+\.arch \.noavx512vbmi
[ 	]*157[ 	]+vpabsb %zmm5, %zmm6\{%k7\}		\# AVX512BW
[ 	]*158[ 	]+vpabsb %xmm5, %xmm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*159[ 	]+vpabsb %ymm5, %ymm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*160[ 	]+vpconflictd %zmm5, %zmm6		\# AVX412CD
[ 	]*161[ 	]+vpconflictd %xmm5, %xmm6		\# AVX412CD \+ AVX512VL
[ 	]*162[ 	]+vpconflictd %ymm5, %ymm6		\# AVX412CD \+ AVX512VL
[ 	]*163[ 	]+vcvtpd2qq \(%ecx\), %zmm6\{%k7\}		\# AVX512DQ
[ 	]*164[ 	]+vcvtpd2qq \(%ecx\), %xmm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*165[ 	]+vcvtpd2qq \(%ecx\), %ymm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*166[ 	]+vexp2ps %zmm5, %zmm6\{%k7\}		\# AVX512ER
[ 	]*167[ 	]+\?\?\?\? 62F1D54F 		vaddpd %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512F
[ 	]*167[ 	]+58F4
[ 	]*168[ 	]+\?\?\?\? 62F1D50F 		vaddpd %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*168[ 	]+58F4
[ 	]*169[ 	]+\?\?\?\? 62F1D52F 		vaddpd %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*169[ 	]+58F4
[ 	]*170[ 	]+vpmadd52luq %zmm4, %zmm5, %zmm6\{%k7\}	\# AVX512IFMA
[ 	]*171[ 	]+vpmadd52luq %xmm4, %xmm5, %xmm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*172[ 	]+vpmadd52luq %ymm4, %ymm5, %ymm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*173[ 	]+vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}	\# AVX512PF
[ 	]*174[ 	]+vpermb %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512VBMI
[ 	]*175[ 	]+vpermb %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*176[ 	]+vpermb %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*177[ 	]+
[ 	]*178[ 	]+\.arch \.noavx512f
[ 	]*179[ 	]+vpabsb %zmm5, %zmm6\{%k7\}		\# AVX512BW
[ 	]*180[ 	]+vpabsb %xmm5, %xmm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*181[ 	]+vpabsb %ymm5, %ymm6\{%k7\}		\# AVX512BW \+ AVX512VL
[ 	]*182[ 	]+vpconflictd %zmm5, %zmm6		\# AVX412CD
[ 	]*183[ 	]+vpconflictd %xmm5, %xmm6		\# AVX412CD \+ AVX512VL
[ 	]*184[ 	]+vpconflictd %ymm5, %ymm6		\# AVX412CD \+ AVX512VL
[ 	]*185[ 	]+vcvtpd2qq \(%ecx\), %zmm6\{%k7\}		\# AVX512DQ
[ 	]*186[ 	]+vcvtpd2qq \(%ecx\), %xmm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*187[ 	]+vcvtpd2qq \(%ecx\), %ymm6\{%k7\}		\# AVX512DQ \+ AVX512VL
[ 	]*188[ 	]+vexp2ps %zmm5, %zmm6\{%k7\}		\# AVX512ER
[ 	]*189[ 	]+vaddpd %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512F
[ 	]*190[ 	]+vaddpd %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512F \+ AVX512VL
[ 	]*191[ 	]+vaddpd %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512F \+ AVX512VL
GAS LISTING .*


[ 	]*192[ 	]+vpmadd52luq %zmm4, %zmm5, %zmm6\{%k7\}	\# AVX512IFMA
[ 	]*193[ 	]+vpmadd52luq %xmm4, %xmm5, %xmm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*194[ 	]+vpmadd52luq %ymm4, %ymm5, %ymm6\{%k7\}	\# AVX512IFMA \+ AVX512VL
[ 	]*195[ 	]+vgatherpf0dpd 23\(%ebp,%ymm7,8\)\{%k1\}	\# AVX512PF
[ 	]*196[ 	]+vpermb %zmm4, %zmm5, %zmm6\{%k7\}		\# AVX512VBMI
[ 	]*197[ 	]+vpermb %xmm4, %xmm5, %xmm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*198[ 	]+vpermb %ymm4, %ymm5, %ymm6\{%k7\}		\# AVX512VBMI \+ AVX512VL
[ 	]*199[ 	]+
[ 	]*200[ 	]+\?\?\?\? C4E2791C 		vpabsb %xmm5, %xmm6
[ 	]*200[ 	]+F5
[ 	]*201[ 	]+\?\?\?\? C4E27D1C 		vpabsb %ymm5, %ymm6
[ 	]*201[ 	]+F5
[ 	]*202[ 	]+\?\?\?\? C5D158F4 		vaddpd %xmm4, %xmm5, %xmm6
[ 	]*203[ 	]+\?\?\?\? C5D558F4 		vaddpd %ymm4, %ymm5, %ymm6
[ 	]*204[ 	]+\?\?\?\? 660F381C 		pabsb %xmm5, %xmm6
[ 	]*204[ 	]+F5
[ 	]*205[ 	]+\?\?\?\? 660F58F4 		addpd %xmm4, %xmm6
[ 	]*206[ 	]+
[ 	]*207[ 	]+\?\?\?\? 0F1F8000 		\.p2align 4
[ 	]*207[ 	]+000000
#pass
