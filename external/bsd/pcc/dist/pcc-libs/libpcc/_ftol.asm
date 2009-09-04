; $Id: _ftol.asm,v 1.1.1.1 2009/09/04 00:27:36 gmcgarry Exp $
;
; Implementation of _ftol(), _ftol2() for win32 and masm.
;
                .386
_TEXT           segment use32 para public 'CODE'
                public  __ftol2
__ftol2         proc    near
                assume  cs:_TEXT
                fnstcw  word ptr [esp-2]
                mov     ax, word ptr [esp-2]
                or      ax, 0C00h
                mov     word ptr [esp-4], ax
                fldcw   word ptr [esp-4]
                fistp   qword ptr [esp-12]
                fldcw   word ptr [esp-2]
                mov     eax, dword ptr [esp-12]
                mov     edx, dword ptr [esp-8]
                ret
__ftol2         endp

_TEXT           ends
                end

		alias	__ftol = __ftol2
		public	__ftol
