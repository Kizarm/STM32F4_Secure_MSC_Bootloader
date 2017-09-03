  .section .data
  .globl image_disc
  .globl image_size
  
  
image_disc:
  .incbin "disk.img"
image_end:

image_size:
  .long . - image_disc
  .long 0
