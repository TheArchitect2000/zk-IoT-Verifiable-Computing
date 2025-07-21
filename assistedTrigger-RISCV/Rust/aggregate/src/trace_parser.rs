use std::process::{Command};
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::{thread, time::Duration};
use regex::Regex;

#[derive(Debug, Clone, PartialEq)]
pub struct TraceEntry {
    pub pc: u64,
    pub opcode: String,
    pub rd: Option<String>,
    pub rs1: Option<String>,
    pub rs2: Option<String>,
    pub imm: Option<i64>, // signed immediate values
}

pub fn run_qemu(binary_path: &str, trace_path: &str) {
    // Start QEMU with GDB server
    let mut qemu = Command::new("qemu-riscv64")
        .args([
            "-d",
            "in_asm,cpu",
            "-D", binary_path,
            // "-S", // freeze CPU at startup
            "-gdb", "tcp::1234" // start gdb server on port 1234
        ])
        .spawn()
        .expect("Failed to start QEMU");

    println!("QEMU launched. Waiting for GDB...");

    // Give QEMU time to initialize
    thread::sleep(Duration::from_secs(1));

    // Create a temporary GDB script
    let gdb_script = r#"
set pagination off
target remote :1234
layout asm
layout regs

# Optional: set $pc to start address manually
# set $pc = 0x00000000

define dump_state
    set $addr = $pc
    x/i $pc
    info registers
end

define step_and_log
    dump_state
    si
end

# run 20 steps
define run_trace
    set logging file trace.log
    set logging on
    set $i = 0
    while $i < 20
        step_and_log
        set $i = $i + 1
    end
    set logging off
end

run_trace
quit
"#;

    let script_path = "/tmp/gdb_trace_script.gdb";
    std::fs::write(script_path, gdb_script).expect("Failed to write gdb script");

    // Run GDB and execute the script
    let status = Command::new("gdb-multiarch")
        .args([binary_path, "-x", script_path])
        .status()
        .expect("Failed to run GDB");

    if status.success() {
        // Move log to desired path
        std::fs::rename("trace.log", trace_path).expect("Failed to move trace log");
        println!("Trace written to {}", trace_path);
    } else {
        println!("GDB failed.");
    }

    // Kill QEMU when done
    let _ = qemu.kill();
}

pub fn parse_trace(trace_path: &str) -> Vec<TraceEntry> {
    let file = File::open(trace_path).expect("Cannot open trace log");
    let reader = BufReader::new(file);
    let re_instr = Regex::new(
        r"^\s*(0x?[0-9a-f]+):\s+([a-z0-9]+)\s+([x][0-9]+)(?:,\s*([x][0-9]+))?(?:,\s*(-?\d+|[x][0-9]+))?"
    ).unwrap();

    reader
        .lines()
        .filter_map(|line| {
            let line = line.ok()?;
            let caps = re_instr.captures(&line)?;

            let pc_str = &caps[1];
            let pc = u64::from_str_radix(pc_str.trim_start_matches("0x"), 16).ok()?;

            let opcode = caps[2].to_string();
            let rd = Some(caps[3].to_string());
            let rs1 = caps.get(4).map(|m| m.as_str().to_string());

            // For the third operand, check if it is immediate (number) or register (xN)
            let imm_or_rs2 = caps.get(5).map(|m| m.as_str().to_string());

            // Determine if imm_or_rs2 is immediate or register
            let (rs2, imm) = match imm_or_rs2 {
                Some(s) if s.starts_with('x') => (Some(s), None), // register
                Some(s) => {
                    // parse immediate as i64, could be negative
                    let imm_val = s.parse::<i64>().ok();
                    (None, imm_val)
                }
                None => (None, None),
            };

            Some(TraceEntry {
                pc,
                opcode,
                rd,
                rs1,
                rs2,
                imm,
            })
        })
        .collect()
}