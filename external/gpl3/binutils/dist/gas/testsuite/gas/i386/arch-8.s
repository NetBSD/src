# Test .arch .sse5
.arch generic32
.arch .sse5
popcnt	%ecx,%ebx
frczss	%xmm2, %xmm1
