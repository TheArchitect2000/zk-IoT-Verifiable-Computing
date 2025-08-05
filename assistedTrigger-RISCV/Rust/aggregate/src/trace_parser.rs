use std::process::{Command, Stdio};
use std::fs::File;
use std::io::{BufRead, BufReader, Write};
use std::{thread, time::Duration};
use std::net::TcpStream;
use regex::Regex;
use std::collections::HashMap;

#[derive(Debug, Clone, PartialEq)]
pub struct TraceEntry {
    pub pc: u64,
    pub opcode: String,
    pub rd: Option<String>,
    pub rs1: Option<String>,
    pub rs2: Option<String>,
    pub imm: Option<i64>,
    pub reg_values: HashMap<String, u64>, // ← Add this
}

fn spawn_qemu(bin: &str, port: u16) -> u32 {
    // Start QEMU in background with GDB server
    let qemu = Command::new("qemu-system-riscv64")
        .args([
            "-machine", "virt",
            "-cpu", "rv64",
            "-nographic",
            "-bios", "none",
            "-device", "loader", &format!("file={}", bin), "addr=0x80000000",
            "-S",
            "-gdb", &format!("tcp:127.0.0.1:{}", port),
        ])
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
        .expect("Failed to start QEMU");
    println!("✅ QEMU process started (pid {})", qemu.id());
    qemu.id()
}

fn wait_for_qemu(port: u16) {
    let addr = format!("127.0.0.1:{}", port);

    // Initial grace delay
    thread::sleep(Duration::from_secs(1));

    for attempt in 0..40 {
        match TcpStream::connect(&addr) {
            Ok(_) => {
                println!("✅ GDB server is accepting connections");
                return;
            }
            Err(e) => {
                println!("⏳ Waiting for GDB server (attempt {}): {}", attempt + 1, e);
                thread::sleep(Duration::from_millis(250));
            }
        }
    }
    panic!("❌ Timed out waiting for QEMU GDB server at {}", addr);
}


fn run_gdb(bin: &str, script: &str) {
    // Run GDB and execute script
    let status = Command::new("riscv64-unknown-elf-gdb")
        .args(["-x", script, bin])
        .status()
        .expect("Failed to run GDB");

    if status.success() {
        println!("✅ GDB trace completed. Log written to trace.log");
    } else {
        println!("❌ GDB trace failed");
    }
}

pub fn run_program(binary_path: &str, trace_path: &str) {
    let bin_path = binary_path;
    let qemu_port = 1234;
    let qemu_pid = spawn_qemu(bin_path, qemu_port);

    wait_for_qemu(qemu_port);

    let gdb_script_path = "./trace.gdb";
    let raw_trace_path = "./trace_raw.txt";

    run_gdb(binary_path, gdb_script_path);

    get_trace(raw_trace_path, trace_path);

    println!("✅ Execution trace written to '{}'", trace_path);

    // Optionally kill QEMU if it didn't exit
    let _ = Command::new("kill").arg(qemu_pid.to_string()).output();
}

fn get_trace(input: &str, output: &str) {
    use std::collections::HashMap;

    let reader = BufReader::new(File::open(input).unwrap());
    let mut out = File::create(output).unwrap();

    let mut reg_state: HashMap<String, String> = HashMap::new();
    let mut pc = String::new();
    let mut instr = String::new();

    let mut printed_header = false;

    for line in reader.lines() {
        let line = line.unwrap();

        if line.starts_with("pc=0x") {
            if !instr.is_empty() {
                // Print instruction
                writeln!(out, "{}: {}", pc, instr).unwrap();

                // Print register state
                let mut regs: Vec<_> = reg_state.iter().collect();
                regs.sort_by_key(|(k, _)| k.clone()); // Sort registers
                let reg_line = regs.iter()
                    .map(|(k, v)| format!("{}={}", k, v))
                    .collect::<Vec<_>>()
                    .join(" ");
                writeln!(out, "{}", reg_line).unwrap();

                instr.clear();
            }

            if let Some((p, i)) = line.strip_prefix("pc=").and_then(|l| l.split_once(':')) {
                pc = p.trim().to_string();
                instr = i.trim().to_string();
            }

        } else if line.contains('\t') {
            if let Some((reg, val)) = line.split_once('\t') {
                let reg_name = reg.trim().to_string();
                let reg_val = val.trim().split('\t').next().unwrap_or("").to_string();
                reg_state.insert(reg_name, reg_val);
            }
        }
    }

    // Handle last instruction
    if !instr.is_empty() {
        writeln!(out, "{}: {}", pc, instr).unwrap();
        let mut regs: Vec<_> = reg_state.iter().collect();
        regs.sort_by_key(|(k, _)| k.clone());
        let reg_line = regs.iter()
            .map(|(k, v)| format!("{}={}", k, v))
            .collect::<Vec<_>>()
            .join(" ");
        writeln!(out, "{}", reg_line).unwrap();
    }
}


pub fn parse_trace(trace_path: &str) -> Vec<TraceEntry> {
    let file = File::open(trace_path).expect("Cannot open trace log");
    let reader = BufReader::new(file);

    let re_instr = Regex::new(
        r"^\s*(0x[0-9a-fA-F]+):\s+([a-z0-9]+)\s+([x][0-9]+)(?:,\s*([x][0-9]+))?(?:,\s*(-?\d+|[x][0-9]+))?"
    ).unwrap();
    let re_reg = Regex::new(r"(x[0-9]+)=0x([0-9a-fA-F]+)").unwrap();

    let mut entries = Vec::new();
    let mut last_regs: HashMap<String, u64> = HashMap::new();
    let mut current_entry: Option<TraceEntry> = None;

    for line in reader.lines().flatten() {
        if re_instr.is_match(&line) {
            if let Some(entry) = current_entry.take() {
                entries.push(entry); // Save previous instruction before processing new one
            }

            let caps = re_instr.captures(&line).unwrap();
            let pc = u64::from_str_radix(&caps[1][2..], 16).unwrap();
            let opcode = caps[2].to_string();
            let rd = Some(caps[3].to_string());
            let rs1 = caps.get(4).map(|m| m.as_str().to_string());
            let third = caps.get(5).map(|m| m.as_str().to_string());

            let (rs2, imm) = match (&opcode[..], third) {
                ("addi", Some(s)) => {
                    if let Ok(val) = s.parse::<i64>() {
                        (None, Some(val))
                    } else {
                        panic!("❌ Invalid `addi` syntax: expected immediate but got {}", s);
                    }
                }
                (_, Some(s)) if s.starts_with('x') => (Some(s), None),
                (_, Some(s)) => (None, s.parse::<i64>().ok()),
                (_, None) => (None, None),
            };

            // Use last register snapshot for rs1 and rs2
            let mut reg_values = HashMap::new();
            if let Some(rs1) = &rs1 {
                if let Some(val) = last_regs.get(rs1) {
                    reg_values.insert(rs1.clone(), *val);
                }
            }
            if let Some(rs2) = &rs2 {
                if let Some(val) = last_regs.get(rs2) {
                    reg_values.insert(rs2.clone(), *val);
                }
            }

            current_entry = Some(TraceEntry {
                pc,
                opcode,
                rd,
                rs1,
                rs2,
                imm,
                reg_values,
            });
        } else if line.starts_with('x') {
            let mut regs = HashMap::new();
            for cap in re_reg.captures_iter(&line) {
                let reg = cap[1].to_string();
                let val = u64::from_str_radix(&cap[2], 16).unwrap_or(0);
                regs.insert(reg, val);
            }

            // Use this current line to assign rd value (after-instruction state)
            if let Some(entry) = current_entry.as_mut() {
                if let Some(rd) = &entry.rd {
                    if let Some(val) = regs.get(rd) {
                        entry.reg_values.insert(rd.clone(), *val);
                    }
                }
            }

            // Update register state snapshot for next instruction
            last_regs = regs;
        }
    }

    // Push final entry if it exists
    if let Some(entry) = current_entry {
        entries.push(entry);
    }

    entries
}
