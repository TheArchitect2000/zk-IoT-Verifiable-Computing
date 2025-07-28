use plonky2::field::goldilocks_field::GoldilocksField;
use plonky2::field::types::Field;
use crate::trace_parser::TraceEntry;
use std::collections::HashMap;

#[derive(Debug, Clone)]
pub struct InstructionRow<F: Field> {
    pub pc: F,
    pub opcode: F,
    pub rs1_val: F,
    pub rs2_val: F,
    pub rd_val: F,
}

// Map register name "x0".."x31" to usize index
fn reg_name_to_index(reg: &str) -> Option<usize> {
    if reg.starts_with('x') {
        reg[1..].parse::<usize>().ok()
    } else {
        None
    }
}

pub fn opcode_to_id(op: &str) -> Option<u64> {
    match op {
        "add" => Some(1),
        "sub" => Some(2),
        "mul" => Some(3),
        "addi" => Some(4),
        "div" => Some(5),
        "sw" => Some(6),
        "and" => Some(7),
        "or" => Some(8),
        _ => None, // Invalid or unrecognized opcode
    }
}

pub fn convert_trace_to_rows(entries: &[TraceEntry]) -> Vec<InstructionRow<GoldilocksField>> {
    let mut rows = Vec::new();
    let mut registers: HashMap<usize, u64> = (0..32).map(|i| (i, 0)).collect();

    for i in 0..entries.len() {
        let entry = &entries[i];

        // Lookup opcode
        let Some(opcode_id) = opcode_to_id(&entry.opcode) else {
            println!("⚠️ Skipping unsupported opcode: {}", entry.opcode);
            continue;
        };
        let opcode = GoldilocksField::from_canonical_u64(opcode_id);

        // Resolve rs1 value
        let rs1_val = entry.rs1
            .as_ref()
            .and_then(|r| reg_name_to_index(r))
            .and_then(|idx| registers.get(&idx).copied())
            .unwrap_or(0);

        // Resolve rs2 or immediate value
        let rs2_val = if let Some(imm) = entry.imm {
            imm as u64
        } else {
            entry.rs2
                .as_ref()
                .and_then(|r| reg_name_to_index(r))
                .and_then(|idx| registers.get(&idx).copied())
                .unwrap_or(0)
        };

        // Predict rd_val: take value from the next state (if available)
        let rd_val = entry.rd
            .as_ref()
            .and_then(|rd_reg| reg_name_to_index(rd_reg))
            .map(|rd_idx| {
                if i + 1 < entries.len() {
                    // Look ahead: use simulated future register state
                    registers.get(&rd_idx).copied().unwrap_or(0)
                } else {
                    0
                }
            })
            .unwrap_or(0);

        // Append current instruction
        rows.push(InstructionRow {
            pc: GoldilocksField::from_canonical_u64(entry.pc),
            opcode,
            rs1_val: GoldilocksField::from_canonical_u64(rs1_val),
            rs2_val: GoldilocksField::from_canonical_u64(rs2_val),
            rd_val: GoldilocksField::from_canonical_u64(rd_val),
        });

        // Simulate register state update
        if let Some(rd_reg) = &entry.rd {
            if let Some(rd_idx) = reg_name_to_index(rd_reg) {
                if rd_idx != 0 {
                    // Simulate write-back after instruction execution
                    registers.insert(rd_idx, rd_val);
                }
            }
        }
    }

    rows
}


#[cfg(test)]
mod tests {
    use super::*;
    use crate::trace_parser::TraceEntry;

    #[test]
    fn test_convert_trace_to_rows() {
        let trace = vec![
            TraceEntry { pc: 0x1000, opcode: "addi".to_string(), rd: Some("x1".to_string()), rs1: Some("x0".to_string()), rs2: None, imm: Some(5) },
            TraceEntry { pc: 0x1004, opcode: "addi".to_string(), rd: Some("x2".to_string()), rs1: Some("x0".to_string()), rs2: None, imm: Some(10) },
            TraceEntry { pc: 0x1008, opcode: "add".to_string(), rd: Some("x3".to_string()), rs1: Some("x1".to_string()), rs2: Some("x2".to_string()), imm: None },
            TraceEntry { pc: 0x100C, opcode: "mul".to_string(), rd: Some("x4".to_string()), rs1: Some("x1".to_string()), rs2: Some("x2".to_string()), imm: None },
        ];

        let rows = convert_trace_to_rows(&trace);

        assert_eq!(rows.len(), 4);
        assert_eq!(rows[0].opcode, GoldilocksField::from_canonical_u64(4)); // addi
        assert_eq!(rows[0].rd_val, GoldilocksField::from_canonical_u64(5));
        assert_eq!(rows[1].rd_val, GoldilocksField::from_canonical_u64(10));
        assert_eq!(rows[2].opcode, GoldilocksField::from_canonical_u64(1)); // add
        assert_eq!(rows[2].rd_val, GoldilocksField::from_canonical_u64(15));
        assert_eq!(rows[3].opcode, GoldilocksField::from_canonical_u64(3)); // mul
        assert_eq!(rows[3].rd_val, GoldilocksField::from_canonical_u64(50));
    }
}