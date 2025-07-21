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

pub fn opcode_to_id(op: &str) -> u64 {
    match op {
        "add" => 1,
        "sub" => 2,
        "mul" => 3,
        "addi" => 4,
        "div" => 5,
        "sw" => 6,
        "and" => 7,
        "or" => 8,
        _ => 0,
    }
}

pub fn convert_trace_to_rows(entries: &[TraceEntry]) -> Vec<InstructionRow<GoldilocksField>> {
    let mut rows = Vec::new();

    // Simulate register file as a map reg_index -> u64 value
    let mut registers: HashMap<usize, u64> = HashMap::new();

    // Initialize all registers to 0
    for i in 0..32 {
        registers.insert(i, 0);
    }

    for entry in entries {
        let opcode_id = opcode_to_id(&entry.opcode);
        let opcode = GoldilocksField::from_canonical_u64(opcode_id);

        // Get rs1 value from registers or 0 if missing
        let rs1_val = entry.rs1.as_ref()
            .and_then(|r| reg_name_to_index(r))
            .and_then(|idx| registers.get(&idx))
            .copied()
            .unwrap_or(0);

        // rs2_val can be register value or immediate value
        let rs2_val = if let Some(imm) = entry.imm {
            imm as u64
        } else {
            entry.rs2.as_ref()
                .and_then(|r| reg_name_to_index(r))
                .and_then(|idx| registers.get(&idx))
                .copied()
                .unwrap_or(0)
        };

        // Compute rd_val based on opcode semantics (basic ALU ops)
        let rd_val = match entry.opcode.as_str() {
            "add" => rs1_val.wrapping_add(rs2_val),
            "sub" => rs1_val.wrapping_sub(rs2_val),
            "mul" => rs1_val.wrapping_mul(rs2_val),
            "addi" => rs1_val.wrapping_add(rs2_val),
            "div" => {
                if rs2_val != 0 {
                    rs1_val / rs2_val
                } else {
                    0 // Division by zero, return 0
                }
            }
            "and" => rs1_val & rs2_val,
            "or" => rs1_val | rs2_val,
            _ => 0,
        };

        // Update register file with rd_val
        if let Some(rd_reg) = &entry.rd {
            if let Some(rd_idx) = reg_name_to_index(rd_reg) {
                // x0 is hardwired zero in RISCV, skip updating it
                if rd_idx != 0 {
                    registers.insert(rd_idx, rd_val);
                }
            }
        }

        rows.push(InstructionRow {
            pc: GoldilocksField::from_canonical_u64(entry.pc),
            opcode,
            rs1_val: GoldilocksField::from_canonical_u64(rs1_val),
            rs2_val: GoldilocksField::from_canonical_u64(rs2_val),
            rd_val: GoldilocksField::from_canonical_u64(rd_val),
        });
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