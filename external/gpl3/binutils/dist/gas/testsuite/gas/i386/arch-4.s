# Test .arch .sse5
.arch generic32
.arch .sse5
ptest           %xmm1,%xmm0
roundpd		$0,%xmm1,%xmm0
roundps		$0,%xmm1,%xmm0
roundsd		$0,%xmm1,%xmm0
roundss		$0,%xmm1,%xmm0
frczss          %xmm2, %xmm1
