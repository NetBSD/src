[^:]*: Assembler messages:
[^:]*:4: Error: missing shift amount at operand 2 -- `strb w7,\[x30,x0,lsl\]'
[^:]*:5: Error: operand mismatch -- `ubfm w0,x1,8,31'
[^:]*:5: Info:    did you mean this\?
[^:]*:5: Info:    	ubfm w0,w1,#8,#31
[^:]*:5: Info:    other valid variant\(s\):
[^:]*:5: Info:    	ubfm x0,x1,#8,#31
[^:]*:6: Error: immediate value out of range 0 to 31 at operand 4 -- `bfm w0,w1,8,43'
[^:]*:7: Error: invalid shift amount at operand 2 -- `strb w7,\[x30,x0,lsl#1\]'
[^:]*:8: Error: invalid addressing mode at operand 2 -- `st2 {v4.2d,v5.2d},\[x3,#3\]'
[^:]*:9: Error: the top half of a 128-bit FP/SIMD register is expected at operand 1 -- `fmov v1.D\[0\],x0'
[^:]*:10: Error: invalid number of registers in the list; only 1 register is expected at operand 1 -- `ld1r \{v1.4s,v2.4s,v3.4s\},\[x3\],x4'
[^:]*:11: Error: missing immediate expression at operand 1 -- `svc'
[^:]*:12: Error: operand mismatch -- `add v0.4s,v1.4s,v2.2s'
[^:]*:12: Info:    did you mean this\?
[^:]*:12: Info:    	add v0.4s,v1.4s,v2.4s
[^:]*:12: Info:    other valid variant\(s\):
[^:]*:12: Info:    	add v0.8b,v1.8b,v2.8b
[^:]*:12: Info:    	add v0.16b,v1.16b,v2.16b
[^:]*:12: Info:    	add v0.4h,v1.4h,v2.4h
[^:]*:12: Info:    	add v0.8h,v1.8h,v2.8h
[^:]*:12: Info:    	add v0.2s,v1.2s,v2.2s
[^:]*:12: Info:    	add v0.2d,v1.2d,v2.2d
[^:]*:13: Error: operand mismatch -- `urecpe v0.1d,v7.1d'
[^:]*:13: Info:    did you mean this\?
[^:]*:13: Info:    	urecpe v0.2s,v7.2s
[^:]*:13: Info:    other valid variant\(s\):
[^:]*:13: Info:    	urecpe v0.4s,v7.4s
[^:]*:14: Error: operand mismatch -- `adds w0,wsp,x0,uxtx#1'
[^:]*:14: Info:    did you mean this\?
[^:]*:14: Info:    	adds w0,wsp,w0, uxtx #1
[^:]*:14: Info:    other valid variant\(s\):
[^:]*:14: Info:    	adds x0,sp,w0, uxtx #1
[^:]*:14: Info:    	adds x0,sp,x0, lsl #1
[^:]*:15: Error: operand mismatch -- `fmov d0,s0'
[^:]*:15: Info:    did you mean this\?
[^:]*:15: Info:    	fmov s0,s0
[^:]*:15: Info:    other valid variant\(s\):
[^:]*:15: Info:    	fmov d0,d0
[^:]*:16: Error: operand mismatch -- `ldnp h3,h7,\[sp\],#16'
[^:]*:16: Info:    did you mean this\?
[^:]*:16: Info:    	ldnp s3,s7,\[sp\],#16
[^:]*:16: Info:    other valid variant\(s\):
[^:]*:16: Info:    	ldnp d3,d7,\[sp\],#16
[^:]*:16: Info:    	ldnp q3,q7,\[sp\],#16
