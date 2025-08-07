use plonky2::field::goldilocks_field::GoldilocksField;
use plonky2::field::types::Field;
use plonky2::field::types::Field64;
use crate::trace_parser::TraceEntry;
use std::collections::HashMap;

#[derive(Debug, Clone)]
pub struct InstructionRow<F: Field> {
    pub pc: F,
    pub opcode: F,
    pub rs1_val: F,
    pub rs2_val: F,
    pub imm_flag: F,
    pub imm_val: F,
    pub rd_val: F,
}

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
        _ => None,
    }
}

pub fn convert_trace_to_rows(entries: &[TraceEntry]) -> Vec<InstructionRow<GoldilocksField>> {
    let mut rows = Vec::new();
    let mut registers: HashMap<usize, u64> = (0..32).map(|i| (i, 0)).collect();

    for entry in entries {
        println!("0x{:08x}: {}", entry.pc, entry.opcode);

        if let Some(rd) = &entry.rd {
            let val = entry.reg_values_after.get(rd).unwrap_or(&0);
            println!("  rd:  {} = {}", rd, val);
        }

        if let Some(rs1) = &entry.rs1 {
            let val = entry.reg_values_before.get(rs1).unwrap_or(&0);
            println!("  rs1: {} = {}", rs1, val);
        }

        if let Some(rs2) = &entry.rs2 {
            let val = entry.reg_values_before.get(rs2).unwrap_or(&0);
            println!("  rs2: {} = {}", rs2, val);
        }

        if let Some(imm) = entry.imm {
            println!("  imm: {}", imm);
        }
    }

    for entry in entries {
        // Update registers AFTER instruction execution
        for (reg_name, val) in &entry.reg_values_after {
            if let Some(idx) = reg_name_to_index(reg_name) {
                registers.insert(idx, *val);
            }
        }

        let Some(opcode_id) = opcode_to_id(&entry.opcode) else {
            println!("⚠️ Skipping unsupported opcode: {}", entry.opcode);
            continue;
        };

        let opcode = GoldilocksField::from_canonical_u64(opcode_id % GoldilocksField::ORDER);

        // rs1 value from BEFORE instruction (reg_values_before)
        let rs1_val = entry.rs1
            .as_ref()
            .and_then(|r| {
                let idx = reg_name_to_index(r)?;
                entry.reg_values_before.get(r)
                    .copied()
                    .or_else(|| registers.get(&idx).copied())
            })
            .unwrap_or(0);

        // rs2 or imm value
        let (rs2_val, imm_flag, imm_val) = if let Some(imm) = entry.imm {
            (imm as u64, GoldilocksField::ONE, GoldilocksField::from_canonical_i64(imm))
        } else {
            let val = entry.rs2
                .as_ref()
                .and_then(|r| {
                    let idx = reg_name_to_index(r)?;
                    entry.reg_values_before.get(r)
                        .copied()
                        .or_else(|| registers.get(&idx).copied())
                })
                .unwrap_or(0);
            (val, GoldilocksField::ZERO, GoldilocksField::ZERO)
        };

        // rd value from AFTER instruction (reg_values_after)
        let rd_val = entry.rd
            .as_ref()
            .and_then(|rd| {
                let idx = reg_name_to_index(rd)?;
                entry.reg_values_after.get(rd)
                    .copied()
                    .or_else(|| registers.get(&idx).copied())
            })
            .unwrap_or(0);

        rows.push(InstructionRow {
            pc: GoldilocksField::from_canonical_u64(entry.pc % GoldilocksField::ORDER),
            opcode,
            rs1_val: GoldilocksField::from_canonical_u64(rs1_val % GoldilocksField::ORDER),
            rs2_val: GoldilocksField::from_canonical_u64(rs2_val % GoldilocksField::ORDER),
            imm_flag,
            imm_val,
            rd_val: GoldilocksField::from_canonical_u64(rd_val % GoldilocksField::ORDER),
        });
    }

    rows
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::HashMap;

    fn map_from(pairs: &[(&str, u64)]) -> HashMap<String, u64> {
        pairs.iter()
            .map(|(k, v)| (k.to_string(), *v))
            .collect()
    }

    #[test]
    fn test_convert_trace_to_rows() {
        let trace = vec![
            TraceEntry {
                pc: 0x1000,
                opcode: "addi".to_string(),
                rd: Some("x1".to_string()),
                rs1: Some("x0".to_string()),
                rs2: None,
                imm: Some(5),
                reg_values_before: map_from(&[("x0", 0), ("x1", 0)]),
                reg_values_after: map_from(&[("x0", 0), ("x1", 5)]),
            },
            TraceEntry {
                pc: 0x1004,
                opcode: "addi".to_string(),
                rd: Some("x2".to_string()),
                rs1: Some("x0".to_string()),
                rs2: None,
                imm: Some(10),
                reg_values_before: map_from(&[("x0", 0), ("x1", 5), ("x2", 0)]),
                reg_values_after: map_from(&[("x0", 0), ("x1", 5), ("x2", 10)]),
            },
            TraceEntry {
                pc: 0x1008,
                opcode: "add".to_string(),
                rd: Some("x3".to_string()),
                rs1: Some("x1".to_string()),
                rs2: Some("x2".to_string()),
                imm: None,
                reg_values_before: map_from(&[("x0", 0), ("x1", 5), ("x2", 10), ("x3", 0)]),
                reg_values_after: map_from(&[("x0", 0), ("x1", 5), ("x2", 10), ("x3", 15)]),
            },
            TraceEntry {
                pc: 0x100C,
                opcode: "mul".to_string(),
                rd: Some("x4".to_string()),
                rs1: Some("x1".to_string()),
                rs2: Some("x2".to_string()),
                imm: None,
                reg_values_before: map_from(&[("x0", 0), ("x1", 5), ("x2", 10), ("x3", 15), ("x4", 0)]),
                reg_values_after: map_from(&[("x0", 0), ("x1", 5), ("x2", 10), ("x3", 15), ("x4", 50)]),
            },
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
