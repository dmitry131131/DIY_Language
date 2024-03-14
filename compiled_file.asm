push rpx          ; save old rpx value
call main         ; Jump to start point
hlt               ; End of programm


; Function declaration with name: главная
:main
; Saving memory stack pointer
push rpx       ; Save old stack pointr
push rpx
push 1
add
pop rpx
call Function_21
push rax
pop [rpx+0]
; Print function
push [rpx+0]     ; Variable with name: "(null)"
out
pop rpx
ret


; Function declaration with name: вв
:Function_21
; Return
push 0
pop rax
pop rpx
ret
pop rpx
ret
