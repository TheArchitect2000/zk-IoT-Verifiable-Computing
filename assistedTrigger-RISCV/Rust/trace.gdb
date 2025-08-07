target remote localhost:1234
set pagination off
set confirm off
set disassemble-next-line on
set $pc = *main
set $done = 0
set $i = 0
define do_step
  si
  printf "\nPC: 0x%x\n", $pc
  x/i $pc
  info registers
  set $i = $i + 1
  if $i >= 1000
    set $done = 1
  end
end
while !$done
  do_step
end
quit
