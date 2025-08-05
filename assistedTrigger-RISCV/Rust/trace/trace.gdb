target remote localhost:1234
set pagination off
set confirm off
set disassemble-next-line on
set $pc = *main
define hook-stop
  x/i $pc
  info registers
end
set $done = 0
while !$done
  si
  if $pc == 0
    set $done = 1
  end
end
quit
