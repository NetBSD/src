#as: -march=mwaitx
.*: Assembler messages:
#eax
.*:[0-9]*: Error: .*eax.* 2 .*monitorx.*
.*:[0-9]*: Error: .*eax.* 3 .*monitorx.*
.*:[0-9]*: Error: .*eax.* 2 .*mwaitx.*
.*:[0-9]*: Error: .*eax.* 3 .*mwaitx.*

#ebx
.*:[0-9]*: Error: .*ebx.* 1 .*monitorx.*
.*:[0-9]*: Error: .*ebx.* 2 .*monitorx.*
.*:[0-9]*: Error: .*ebx.* 3 .*monitorx.*
.*:[0-9]*: Error: .*ebx.* 1 .*mwaitx.*
.*:[0-9]*: Error: .*ebx.* 2 .*mwaitx.*

#ecx
.*:[0-9]*: Error: .*ecx.* 1 .*monitorx.*
.*:[0-9]*: Error: .*ecx.* 3 .*monitorx.*
.*:[0-9]*: Error: .*ecx.* 1 .*mwaitx.*
.*:[0-9]*: Error: .*ecx.* 3 .*mwaitx.*

#edx
.*:[0-9]*: Error: .*edx.* 1 .*monitorx.*
.*:[0-9]*: Error: .*edx.* 2 .*monitorx.*
.*:[0-9]*: Error: .*edx.* 1 .*mwaitx.*
.*:[0-9]*: Error: .*edx.* 2 .*mwaitx.*
.*:[0-9]*: Error: .*edx.* 3 .*mwaitx.*

#esp
.*:[0-9]*: Error: .*esp.* 1 .*monitorx.*
.*:[0-9]*: Error: .*esp.* 2 .*monitorx.*
.*:[0-9]*: Error: .*esp.* 3 .*monitorx.*
.*:[0-9]*: Error: .*esp.* 1 .*mwaitx.*
.*:[0-9]*: Error: .*esp.* 2 .*mwaitx.*
.*:[0-9]*: Error: .*esp.* 3 .*mwaitx.*

#ebp
.*:[0-9]*: Error: .*ebp.* 1 .*monitorx.*
.*:[0-9]*: Error: .*ebp.* 2 .*monitorx.*
.*:[0-9]*: Error: .*ebp.* 3 .*monitorx.*
.*:[0-9]*: Error: .*ebp.* 1 .*mwaitx.*
.*:[0-9]*: Error: .*ebp.* 2 .*mwaitx.*
.*:[0-9]*: Error: .*ebp.* 3 .*mwaitx.*

#esi
.*:[0-9]*: Error: .*esi.* 1 .*monitorx.*
.*:[0-9]*: Error: .*esi.* 2 .*monitorx.*
.*:[0-9]*: Error: .*esi.* 3 .*monitorx.*
.*:[0-9]*: Error: .*esi.* 1 .*mwaitx.*
.*:[0-9]*: Error: .*esi.* 2 .*mwaitx.*
.*:[0-9]*: Error: .*esi.* 3 .*mwaitx.*

#edi
.*:[0-9]*: Error: .*edi.* 1 .*monitorx.*
.*:[0-9]*: Error: .*edi.* 2 .*monitorx.*
.*:[0-9]*: Error: .*edi.* 3 .*monitorx.*
.*:[0-9]*: Error: .*edi.* 1 .*mwaitx.*
.*:[0-9]*: Error: .*edi.* 2 .*mwaitx.*
.*:[0-9]*: Error: .*edi.* 3 .*mwaitx.*
