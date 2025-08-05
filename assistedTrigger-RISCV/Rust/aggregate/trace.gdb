set pagination off
target remote :1234
define hook-stop
  printf "pc=%p: %s\n", $pc, $instruction
  info registers
end
si
while 1
  si
end
