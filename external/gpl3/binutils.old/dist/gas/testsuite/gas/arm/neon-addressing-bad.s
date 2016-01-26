.syntax unified

VLD1.8 {d0}, 1f
1:
VLD1.8 {D0}, R0
VLD1.8 {Q1}, R0
VLD1.8 {D0}, [PC]
VLD1.8 {D0}, [PC, #0]
VST1.8 {D0}, R0
VST1.8 {Q1}, R0
VST1.8 {D0}, [PC]
VST1.8 {D0}, [PC, #0]
.thumb
VLD1.8 {d0}, 2f
2:
VLD1.8 {D0}, R0
VLD1.8 {Q1}, R0
VLD1.8 {D0}, [PC]
VLD1.8 {D0}, [PC, #0]
VST1.8 {D0}, R0
VST1.8 {Q1}, R0
VST1.8 {D0}, [PC]
VST1.8 {D0}, [PC, #0]
