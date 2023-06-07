; This hook wraps .plt stubs such that we can handle
; .plt stubs and function pointers in GOT equally in other hooks.
; This occurs due to the difference in
; how lazy and early linked function symbols are linked by the dynamic linker.

default rel ; enable RIP relative addressing by default

SECTION .text           ; Code section.

; The following dq pseudo-instructions are here to reserve space.
; These memory locations will be patched by Xplore such that they contain the correct information
; after the hook is injected.
plt_entry_address:
    dq 0x0 ; Address of corresponding .plt entry.
got_entry_address:
    dq 0x0 ; Address of corresponding .got entry.
hook_address:
    dq 0x0 ; Address where this hook is placed.
; Pretend we are a proper function by storing the frame pointer.
    push rbp
    mov rbp,rsp
    ; Call the original function if it's cached.
    push rdx
    mov rdx, [original_function_address]
    test rdx,rdx
    ; Restore registers and stack to the original state.
    pop rdx
    jz link_func
    ; The cached function address is not null, so we can reuse it.
    call [original_function_address]
    jmp after_call
link_func:
    ; The cached function address is null.
    ; In this case, call the plt entry, which will cause the dynamic linker
    ; to resolve the address and store it in the corresponding GOT entry.
    ; After the call, we need to grab the address and replace the address in the GOT entry
    ; with our address to restore the hook.
    call [plt_entry_address]
    push rax
    push rbx
    mov rax, [got_entry_address]
    mov rax, [rax]
    mov [original_function_address], rax
    mov rax, [hook_address]
    mov rbx, [got_entry_address]
    mov [rbx], rax
    pop rbx
    pop rax
after_call:
    ; Clean up.
    mov rsp, rbp
    pop rbp
    ret
original_function_address:
    dq 0x0 ; Cache where original function address will be stored.