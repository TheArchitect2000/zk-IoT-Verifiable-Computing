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
    let mut qemu = Command::new("qemu-system-riscv64")
        .args([
            "-machine", "virt",
            "-cpu", "rv64",
            "-nographic",
            "-bios", "none",
            "-device", "loader", &format!("file={}", bin), "addr=0x80000000",
            "-S",
            "-gdb", &format!("tcp::{}", port),
        ])
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
        .expect("Failed to start QEMU");
    println!("✅ QEMU GDB server is up");
    qemu.id()
}

fn wait_for_qemu(port: u16) {
    let addr = format!("127.0.0.1:{}", port);
    for _ in 0..20 {
        if TcpStream::connect(&addr).is_ok() {
            println!("✅ QEMU GDB server is up");
            return;
        }
        thread::sleep(Duration::from_millis(250));
    }
    panic!("❌ Timed out waiting for QEMU GDB server");
}

fn run_gdb(bin: &str, script: &str) {
    // Command::new("riscv64-unknown-elf-gdb")
    //     .args(["-q", "-x", script, bin])
    //     .status()
    //     .expect("Failed to run GDB");

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
    // let final_trace_path = "trace_cleaned.txt";


    run_gdb(binary_path, gdb_script_path);

    get_trace(raw_trace_path, trace_path);

    println!("✅ Execution trace written to '{}'", trace_path);

    // Optionally kill QEMU if it didn't exit
    let _ = Command::new("kill").arg(qemu_pid.to_string()).output();
}

fn get_trace(input: &str, output: &str) {
    let reader = BufReader::new(File::open(input).unwrap());
    let mut out = File::create(output).unwrap();

    let mut pc = String::new();
    let mut instr = String::new();
    let mut regs = Vec::new();

    for line in reader.lines() {
        let line = line.unwrap();
        if line.starts_with("pc=0x") {
            if !instr.is_empty() {
                writeln!(out, "{}: {}", pc, instr).unwrap();
                writeln!(out, "{}", regs.join(" ")).unwrap();
                writeln!(out).unwrap();
                regs.clear();
            }
            if let Some((p, i)) = line.strip_prefix("pc=").and_then(|l| l.split_once(':')) {
                pc = p.trim().to_string();
                instr = i.trim().to_string();
            }
        } else if line.starts_with("x") || line.starts_with("ra") || line.starts_with("sp") {
            if let Some((reg, val)) = line.split_once('\t') {
                regs.push(format!("{}={}", reg.trim(), val.trim().split('\t').next().unwrap_or("")));
            }
        }
    }

    // Last entry
    if !instr.is_empty() {
        writeln!(out, "{}: {}", pc, instr).unwrap();
        writeln!(out, "{}", regs.join(" ")).unwrap();
    }
}

pub fn parse_trace(trace_path: &str) -> Vec<TraceEntry> {
    let file = File::open(trace_path).expect("Cannot open trace log");
    let reader = BufReader::new(file);
    let re_instr = Regex::new(
        r"^\s*(0x?[0-9a-f]+):\s+([a-z0-9]+)\s+([x][0-9]+)(?:,\s*([x][0-9]+))?(?:,\s*(-?\d+|[x][0-9]+))?"
    ).unwrap();

    let re_reg = Regex::new(r"(x[0-9]+)=0x([0-9a-fA-F]+)").unwrap();

    let mut entries = Vec::new();
    let mut last_instr: Option<TraceEntry> = None;

    for line in reader.lines().flatten() {
        if let Some(caps) = re_instr.captures(&line) {
            // Flush the previous instruction, if present
            if let Some(entry) = last_instr.take() {
                entries.push(entry);
            }

            let pc_str = &caps[1];
            let pc = u64::from_str_radix(pc_str.trim_start_matches("0x"), 16).unwrap();

            let opcode = caps[2].to_string();
            let rd = Some(caps[3].to_string());
            let rs1 = caps.get(4).map(|m| m.as_str().to_string());

            let imm_or_rs2 = caps.get(5).map(|m| m.as_str().to_string());
            let (rs2, imm) = match imm_or_rs2 {
                Some(s) if s.starts_with('x') => (Some(s), None),
                Some(s) => {
                    let imm_val = s.parse::<i64>().ok();
                    (None, imm_val)
                }
                None => (None, None),
            };

            last_instr = Some(TraceEntry {
                pc,
                opcode,
                rd,
                rs1,
                rs2,
                imm,
                reg_values: HashMap::new(), // Filled in next lines
            });
        } else if line.starts_with("x") {
            // Register value line
            if let Some(entry) = last_instr.as_mut() {
                let mut reg_values = HashMap::new();

                for reg_cap in re_reg.captures_iter(&line) {
                    let reg = reg_cap[1].to_string();
                    let val = u64::from_str_radix(&reg_cap[2], 16).unwrap_or(0);
                    reg_values.insert(reg, val);
                }

                entry.reg_values = reg_values;
            }
        }
    }

    // Push last instruction
    if let Some(entry) = last_instr {
        entries.push(entry);
    }

    entries
}
