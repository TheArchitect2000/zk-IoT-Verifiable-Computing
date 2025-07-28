target remote localhost:1234
set $pc = 0x80000000
set pagination off
set confirm off
set logging file trace.log
set logging on

define dumpregs
  printf "pc=0x%08x\n", $pc
  printf "x0=0x%08x x1=0x%08x x2=0x%08x x3=0x%08x\n", $x0, $x1, $x2, $x3
  printf "x4=0x%08x x5=0x%08x x6=0x%08x x7=0x%08x\n", $x4, $x5, $x6, $x7
  printf "x8=0x%08x x9=0x%08x x10=0x%08x x11=0x%08x\n", $x8, $x9, $x10, $x11
  printf "x12=0x%08x x13=0x%08x x14=0x%08x x15=0x%08x\n", $x12, $x13, $x14, $x15
  printf "x16=0x%08x x17=0x%08x x18=0x%08x x19=0x%08x\n", $x16, $x17, $x18, $x19
  printf "x20=0x%08x x21=0x%08x x22=0x%08x x23=0x%08x\n", $x20, $x21, $x22, $x23
  printf "x24=0x%08x x25=0x%08x x26=0x%08x x27=0x%08x\n", $x24, $x25, $x26, $x27
  printf "x28=0x%08x x29=0x%08x x30=0x%08x x31=0x%08x\n\n", $x28, $x29, $x30, $x31
end

define hook-stop
  x/i $pc
  dumpregs
end

while 1
  stepi
end
