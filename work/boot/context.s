  .globl index_ctxt
  .globl index_size
  .globl image_ctxt
@ .globl image_size

  .section .rodata.index_ctx
@  .align 4
index_ctxt:
  .incbin "./fil/index.html"
index_end:
  .word 0
  
  .section .user.firmware
image_ctxt:
  .incbin "./fil/FIRMWARE.BIN"
image_end:
  .word 0

  .section .rodata.sizes
  .align 8
index_size:
  .long index_end - index_ctxt
  .long 0
@image_size:
@ .long image_end - image_ctxt
@ .long 0
  