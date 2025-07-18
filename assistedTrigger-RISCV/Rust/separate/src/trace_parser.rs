use std::process::Command;
use std::fs::File;
use std::io::{BufRead, BufReader};
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
    Command::new("qemu-riscv64")
        .arg("-d")
        .arg("in_asm,cpu")
        .arg("-D")
        .arg(trace_path)
        .arg(binary_path)
        .status()
        .expect("Failed to run QEMU");
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