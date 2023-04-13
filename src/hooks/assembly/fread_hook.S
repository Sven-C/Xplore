default rel ; enable RIP relative addressing by default

SECTION .text           ; Code section.

; The following dq pseudo-instructions are here to reserve space.
; These memory locations will be patched by Xplore such that they contain the correct information
; after the hook is injected.
original_function_address:
    dq 0x0 ; Cache where original function address will be stored.
plt_entry_address:
    dq 0x0 ; Address of corresponding .plt entry.
got_entry_address:
    dq 0x0 ; Address of corresponding .got entry.
hook_address:
    dq 0x0 ; Address where this hook is placed.
; The scratch pad is memory shared between hooks. The memory roughly corresponds with the following c struct:
; struct {
;   long string_table_end;
;   char* filenames;
;   char[] string_table;
;} scratch_pad;
; In more The first 8 bytes of the memory page are reserved for the string table length.
; The next 2040 bytes are reserved for the array of char*. This array is indexed by the file descriptor.
; This means that this array should not be indexed with an fd 0, which is stdin.
; The pointers point into the string table.
; The last 2048 bytes are reserved for the string table. This is a series of null terminated strings.
scratch_pad_address:
    dq 0x0 ; Address where shared memory is located. This memory is shared between each hook.
    ; Pretend we are a proper function by storing the frame pointer.
    push rbp
    mov rbp,rsp
    ; Store the registers which we use throughout.
    push rax
    push rdi
    push rsi
    push rdx
    push rcx
    push r11
    lea rax, [str_prologue]
    call print
    ; The fopen hook puts a char* at (scratch_pad_address + fd * 8).
    ; The fd can be retrieved from the FILE* struct passed to fread.
    mov rax, [rsp + 0x08]   ; Load FILE* argument into rax.
    add rax, 0x70           ; The fd is located at offset 0x70.
    mov eax, [rax]          ; Store the fd into eax.
    and rax, 0xffffffff
    mov rbx, [scratch_pad_address] ; Compute the offset where the corresponding char* is located. 
    mov rax, [rbx + rax * 8]
    call print
    ; Store the address of the buffer into which the data will be read.
    mov rax, [rsp + 0x28]
    mov [local_fread_buffer], rax
    ; Call the original function.
    mov rdx, [original_function_address]
    test rdx,rdx
    ; Restore registers and stack to the original state.
    pop r11
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax
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
    push rax
    push rdx
    push rsi
    lea rax, [str_newline]
    call print
    ; Write data read to stdout.
    mov rsi, [local_fread_buffer]
    mov rdx, [rsp + 0x10]
    call write
    lea rax, [str_newline]
    call print
    ; Clean up.
    pop rsi
    pop rdx
    pop rax
    mov rsp, rbp
    pop rbp
    ret
; Corresponds to function wrapping the write syscall.
; void write(char* buf, long len)
; Input: buf in rsi.
; Input: len in rdx.
write:
    push rax
    push rdi
    mov rdi, 1           ; file descriptor (stdout)
    mov rax, 1           ; system call number (sys_write)
    syscall             ; call kernel
    pop rdi
    pop rax
    ret
; Corresponds to
; long strlen(char* string) {
;    long i = 0;
;    while (string[i]) {
;      i++;
;    }
;    return i;
; }
; Input: string in rax.
; Output: rax.
strlen:
    push rsi ; current char
    push rdi ; counter
    mov rdi, 0
_loop:
    mov sil, [rax+rdi]
    test sil, sil
    jz _end
    inc rdi
    jmp _loop
_end:
    mov rax, rdi
    pop rdi
    pop rsi
    ret
; Corresponds to
; long print(char* string) {
;    long len = strlen(string);
;    write(string, len);
; }
; Input: string in rax.
print:
    mov rsi, rax
    call strlen
    mov rdx, rax
    call write
    ret
local_fread_buffer:
    dq 0x0 ; Location where the char* pointer passed to fread is stored. 
str_prologue:
    db 'fread: Read from ', 0
str_newline:
    db 0xa, 0
