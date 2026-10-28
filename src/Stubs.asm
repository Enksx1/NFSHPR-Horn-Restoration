EXTERN HornFrame:PROC
EXTERN OnEntryCreated:PROC
EXTERN OnEntryUnload:PROC
EXTERN g_frame_orig:QWORD
EXTERN g_n2_call:QWORD
EXTERN g_n1_skip:QWORD
EXTERN g_n2_skip:QWORD
EXTERN g_localSlotOff:QWORD
EXTERN g_wireBit:DWORD
EXTERN g_rx:BYTE

.code

hpr_frame_stub PROC FRAME
    sub rsp, 28h
    .allocstack 28h
    .endprolog
    mov rcx, rbp
    call HornFrame
    mov rcx, rbp
    call qword ptr [g_frame_orig]
    add rsp, 28h
    ret
hpr_frame_stub ENDP

hpr_send_stub PROC FRAME
    pushfq
    .allocstack 8
    .endprolog
    mov edi, dword ptr [rcx + rsi + 3484h]
    mov qword ptr [g_localSlotOff], rcx
    cmp dword ptr [g_wireBit], 0
    je send_done
    or r14d, 4
send_done:
    popfq
    ret
hpr_send_stub ENDP

hpr_recv_stub PROC FRAME
    .endprolog
    cmp r14d, 1Fh
    ja recv_write
    mov rax, r14
    imul rax, rax, 1C00h
    cmp rax, qword ptr [g_localSlotOff]
    je recv_strip
    lea rax, g_rx
    test ebx, 4
    setne byte ptr [rax + r14]
recv_strip:
    and ebx, 0FFFFFFFBh
recv_write:
    mov dword ptr [r12 + rdi + 4A10h], ebx
    ret
hpr_recv_stub ENDP

hpr_null1_stub PROC FRAME
    sub rsp, 28h
    .allocstack 28h
    .endprolog
    test rcx, rcx
    je null1_skip
    mulss xmm6, dword ptr [rdx + rcx + 20h]
    add rsp, 28h
    ret
null1_skip:
    mov rcx, rdi
    mov rax, qword ptr [rdi]
    call qword ptr [rax + 28h]
    mov rax, qword ptr [g_n1_skip]
    mov qword ptr [rsp + 28h], rax
    add rsp, 28h
    ret
hpr_null1_stub ENDP

hpr_null2_stub PROC FRAME
    sub rsp, 28h
    .allocstack 28h
    .endprolog
    call qword ptr [g_n2_call]
    mov r9, qword ptr [rdi + 10h]
    test r9, r9
    je null2_skip
    cmp qword ptr [r9 + 28h], 0
    je null2_skip
    add rsp, 28h
    ret
null2_skip:
    mov rcx, rdi
    mov rax, qword ptr [rdi]
    call qword ptr [rax + 28h]
    mov rax, qword ptr [g_n2_skip]
    mov qword ptr [rsp + 28h], rax
    add rsp, 28h
    ret
hpr_null2_stub ENDP

hpr_create_stub PROC FRAME
    push rax
    .pushreg rax
    push rcx
    .pushreg rcx
    push rdx
    .pushreg rdx
    push r9
    .pushreg r9
    push r10
    .pushreg r10
    push r11
    .pushreg r11
    sub rsp, 88h
    .allocstack 88h
    movaps xmmword ptr [rsp + 20h], xmm0
    .savexmm128 xmm0, 20h
    movaps xmmword ptr [rsp + 30h], xmm1
    .savexmm128 xmm1, 30h
    movaps xmmword ptr [rsp + 40h], xmm2
    .savexmm128 xmm2, 40h
    movaps xmmword ptr [rsp + 50h], xmm3
    .savexmm128 xmm3, 50h
    movaps xmmword ptr [rsp + 60h], xmm4
    .savexmm128 xmm4, 60h
    movaps xmmword ptr [rsp + 70h], xmm5
    .savexmm128 xmm5, 70h
    .endprolog
    mov rcx, r10
    call OnEntryCreated
    movaps xmm0, xmmword ptr [rsp + 20h]
    movaps xmm1, xmmword ptr [rsp + 30h]
    movaps xmm2, xmmword ptr [rsp + 40h]
    movaps xmm3, xmmword ptr [rsp + 50h]
    movaps xmm4, xmmword ptr [rsp + 60h]
    movaps xmm5, xmmword ptr [rsp + 70h]
    add rsp, 88h
    pop r11
    pop r10
    pop r9
    pop rdx
    pop rcx
    pop rax
    mov r8, qword ptr [rbp + 2F0h]
    ret
hpr_create_stub ENDP

hpr_unload_stub PROC FRAME
    push rcx
    .pushreg rcx
    push rdx
    .pushreg rdx
    push r8
    .pushreg r8
    push r9
    .pushreg r9
    sub rsp, 20h
    .allocstack 20h
    .endprolog
    mov rcx, rdx
    call OnEntryUnload
    add rsp, 20h
    pop r9
    pop r8
    pop rdx
    pop rcx
    mov qword ptr [rsp + 10h], rbx
    ret
hpr_unload_stub ENDP

END
