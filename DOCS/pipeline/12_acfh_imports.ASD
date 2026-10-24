; jmp 
E9 00 00 00 00 
; push ebp
55 
; mov ebp, esp
89 E5 
; sub esp, 0x8
83 EC 08 
; mov eax, 0xFF00
B8 00 FF 00 00 
; push eax
50 
; mov eax, 0x32
B8 32 00 00 00 
; push eax
50 
; mov eax, 0x64
B8 64 00 00 00 
; push eax
50 
; mov eax, 0x14
B8 14 00 00 00 
; push eax
50 
; mov eax, 0xA
B8 0A 00 00 00 
; push eax
50 
; call 
E8 D2 FF FF FF 
; add esp, 0x14
83 C4 14 
; mov eax, 0x0
B8 00 00 00 00 
; push eax
50 
; call 
E8 C4 FF FF FF 
; add esp, 0x4
83 C4 04 
; mov eax, 0x400
B8 00 04 00 00 
; push eax
50 
; call 
E8 B6 FF FF FF 
; add esp, 0x4
83 C4 04 
; mov [ebp-4], eax
89 45 FC 
; mov eax, 0x1
B8 01 00 00 00 
; push eax
50 
; mov eax, 0x1
B8 01 00 00 00 
; mov ebx, eax
89 C3 
; pop eax
58 
; cmp eax, ebx
39 D8 
; sete al
0F 94 C0 
; movzx eax, al
0F B6 C0 
; test eax, eax
85 C0 
; jz 
74 1B 
; mov eax, [ebp-4]
8B 45 FC 
; push eax
50 
; call 
E8 8D FF FF FF 
; add esp, 0x4
83 C4 04 
; mov eax, 0x1
B8 01 00 00 00 
; add esp, 0x8
83 C4 08 
; pop ebp
5D 
; ret
C3 
; jmp 
E9 00 00 00 00 
; mov eax, 0x29E
B8 9E 02 00 00 
; push eax
50 
; call 
E8 70 FF FF FF 
; add esp, 0x4
83 C4 04 
; mov [ebp-8], eax
89 45 F8 
; mov eax, [ebp-4]
8B 45 FC 
; push eax
50 
; call 
E8 61 FF FF FF 
; add esp, 0x4
83 C4 04 
; mov eax, [ebp-8]
8B 45 F8 
; push eax
50 
; call 
E8 55 FF FF FF 
; add esp, 0x4
83 C4 04 
; mov eax, 0x0
B8 00 00 00 00 
; add esp, 0x8
83 C4 08 
; pop ebp
5D 
; ret
C3 
; add esp, 0x8
83 C4 08 
; pop ebp
5D 
; ret
C3 
