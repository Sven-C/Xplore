default rel ; enable RIP relative addressing by default

SECTION .text           ; Code section.

; The following dq pseudo-instructions are here to reserve space.
; These memory locations will be patched by Xplore such that they contain the correct information
; after the hook is injected.
original_function_address:
    dq 0x0 ; Cache where original function address will be stored.
; The scratch pad is memory shared between hooks. The memory roughly corresponds with the following c struct:
; struct {
;   long string_table_length;
;   char* filenames;
;   char[] string_table;
;} scratch_pad;
; In more The first 8 bytes of the memory page are reserved for the string table length.
; The next 2040 bytes are reserved for the array of char*. This array is indexed by the file descriptor.
; This means that this array should not be indexed with an fd 0, which is stdin.
; The pointers point into the string table.
; The last 2048 bytes are reserved for the string table. This is a series of null terminated strings.
scratch_pad:
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
    ; Offset 0 in the scratch pad is used to store the end of the string table. Initialize it.
    xor rdi, rdi
    mov rax, [scratch_pad]
    mov [rax], rdi
    lea rax, [str_prologue]
    call print
    ; The filename which is being opened is originally stored in rax, which was saved on the stack.
    mov rax, [rsp+0x28]
    call print
    lea rax, [str_mode]
    call print
    ; The mode is originally stored in rsi, which was saved on the stack.
    mov rax, [rsp+0x18]
    call print
    lea rax, [str_newline]
    call print
    ; Store filename in the string table in the scratch pad memory
    ; and save the address if the local_filename variable.
    mov rdx, [scratch_pad]
    mov rax, [rdx]
    add rdx, 0x800
    add rdx, rax
    mov rax, [rsp+0x28]
    mov [local_filename], rdx
    call strcpy
    mov rdi, [scratch_pad]
    mov rdi, [rdi]
    add rdi, rax
    mov rax, [scratch_pad]
    mov [rax], rdi
    ; Restore registers and stack to the original state.
    pop r11
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax
    call [original_function_address]
    push rax
    push rbx
    push rcx
    add rax, 0x70 ; The return value (stored in rax) is a FILE* pointer. At offset 0x70 the fd is located.
    mov eax, [rax]
    and rax, 0x00000000ffffffff
    ; Store the pointer into the string table in the array filenames.
    mov rbx, [scratch_pad]
    lea rbx, [rbx + rax * 8]
    mov rax, [local_filename]
    mov [rbx], rax
    ; Clean up.
    pop rcx
    pop rbx
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
.loop:
    mov sil, [rax+rdi]
    test sil, sil
    jz .end
    inc rdi
    jmp .loop
.end:
    mov rax, rdi
    pop rdi
    pop rsi
    ret
; Corresponds to
; long strcpy(char* dest, char* source) {
;    long i = 0;
;    do {
;       dest[i] = source[i];
;    } while (source[i]);
; }
; Input: string in rax.
; Output: rax.
strcpy:
    push rsi
    push rdi
    mov rdi, 0
.loop:
    mov sil, [rax+rdi]
    mov [rdx+rdi], sil
    test sil, sil
    jz .end
    inc rdi
    jmp .loop
.end:
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
local_filename:
    dq 0x0 ; Pointer to the last stored string in the string table.
str_prologue:
    db 'fopen: Opening ', 0
str_mode:
    db', mode ', 0
str_newline:
    db 0xa, 0
