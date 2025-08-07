use std::collections::HashMap;
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
    pub imm: Option<i64>,
    pub reg_values_before: HashMap<String, u64>, // rs1, rs2 values before instruction
    pub reg_values_after: HashMap<String, u64>,  // rd value after instruction
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
    let mut pending_regs: Option<HashMap<String, u64>> = None;
    let mut last_reg_dump: HashMap<String, u64> = HashMap::new();
    let mut current_entry: Option<TraceEntry> = None;

    for line in reader.lines().flatten() {
        if re_instr.is_match(&line) {
            // Push previous instruction before processing new one
            if let Some(entry) = current_entry.take() {
                entries.push(entry);
            }

            // Update last_regs with the pending register dump (pre-instruction state)
            if let Some(new_regs) = pending_regs.take() {
                last_regs = new_regs;
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

            // Prepare reg_values_before from last_regs for rs1, rs2
            let mut reg_values_before = HashMap::new();
            if let Some(rs1) = &rs1 {
                if let Some(val) = last_regs.get(rs1) {
                    reg_values_before.insert(rs1.clone(), *val);
                }
            }
            if let Some(rs2) = &rs2 {
                if let Some(val) = last_regs.get(rs2) {
                    reg_values_before.insert(rs2.clone(), *val);
                }
            }

            current_entry = Some(TraceEntry {
                pc,
                opcode,
                rd,
                rs1,
                rs2,
                imm,
                reg_values_before,
                reg_values_after: HashMap::new(), // fill after parsing register dump
            });
        } else if line.starts_with('x') {
            let mut regs = HashMap::new();
            for cap in re_reg.captures_iter(&line) {
                let reg = cap[1].to_string();
                let val = u64::from_str_radix(&cap[2], 16).unwrap_or(0);
                regs.insert(reg, val);
            }

            // Update rd value (after-instruction) for current entry if present
            if let Some(entry) = current_entry.as_mut() {
                if let Some(rd) = &entry.rd {
                    if let Some(val) = regs.get(rd) {
                        entry.reg_values_after.insert(rd.clone(), *val);
                    }
                }
            }

            // Store current register dump for next instruction's use (before-instruction state)
            pending_regs = Some(regs.clone());
            // Also update last_reg_dump (for final instruction fix)
            last_reg_dump = regs;
        }
    }

    // Push final entry if exists
    if let Some(mut entry) = current_entry {
        // It's possible the final instruction has not had reg_values_after fully updated if no trailing reg dump
        // So merge from last_reg_dump here as a safety net
        if let Some(rd) = &entry.rd {
            if let Some(val) = last_reg_dump.get(rd) {
                entry.reg_values_after.insert(rd.clone(), *val);
            }
        }
        entries.push(entry);
    }

    entries
}
