# Test .arch .sse
.arch generic32
.arch .sse
divss %xmm1,%xmm0
pminub %mm1,%mm0
