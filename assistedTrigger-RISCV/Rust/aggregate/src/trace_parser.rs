use plonky2::field::goldilocks_field::GoldilocksField;
use plonky2::field::types::Field;
use std::fs::File;
use std::io::{BufRead, BufReader};


#[derive(Debug, Clone)]
pub struct InstructionRow {
    pub opcode: GoldilocksField,
    pub rs1_val: GoldilocksField,
    pub rs2_val: GoldilocksField,
    pub rd_val: GoldilocksField,
}

// Stub implementations
pub fn run_qemu(_bin: &str, _trace_path: &str) {}

pub fn parse_trace(_trace_path: &str) -> Vec<String> {
    vec![] // stub
}
fn parse_opcode(opcode_str: &str) -> GoldilocksField {
    // Real RISC-V opcodes (base integer instructions)
    match opcode_str {
        "LUI"   => GoldilocksField::from_canonical_u64(0b0110111),
        "AUIPC" => GoldilocksField::from_canonical_u64(0b0010111),
        "JAL"   => GoldilocksField::from_canonical_u64(0b1101111),
        "JALR"  => GoldilocksField::from_canonical_u64(0b1100111),
        "BEQ"   => GoldilocksField::from_canonical_u64(0b1100011),
        "BNE"   => GoldilocksField::from_canonical_u64(0b1100011),
        "BLT"   => GoldilocksField::from_canonical_u64(0b1100011),
        "BGE"   => GoldilocksField::from_canonical_u64(0b1100011),
        "BLTU"  => GoldilocksField::from_canonical_u64(0b1100011),
        "BGEU"  => GoldilocksField::from_canonical_u64(0b1100011),
        "LB"    => GoldilocksField::from_canonical_u64(0b0000011),
        "LH"    => GoldilocksField::from_canonical_u64(0b0000011),
        "LW"    => GoldilocksField::from_canonical_u64(0b0000011),
        "LBU"   => GoldilocksField::from_canonical_u64(0b0000011),
        "LHU"   => GoldilocksField::from_canonical_u64(0b0000011),
        "SB"    => GoldilocksField::from_canonical_u64(0b0100011),
        "SH"    => GoldilocksField::from_canonical_u64(0b0100011),
        "SW"    => GoldilocksField::from_canonical_u64(0b0100011),
        "ADDI"  => GoldilocksField::from_canonical_u64(0b0010011),
        "SLTI"  => GoldilocksField::from_canonical_u64(0b0010011),
        "SLTIU" => GoldilocksField::from_canonical_u64(0b0010011),
        "XORI"  => GoldilocksField::from_canonical_u64(0b0010011),
        "ORI"   => GoldilocksField::from_canonical_u64(0b0010011),
        "ANDI"  => GoldilocksField::from_canonical_u64(0b0010011),
        "SLLI"  => GoldilocksField::from_canonical_u64(0b0010011),
        "SRLI"  => GoldilocksField::from_canonical_u64(0b0010011),
        "SRAI"  => GoldilocksField::from_canonical_u64(0b0010011),
        "ADD"   => GoldilocksField::from_canonical_u64(0b0110011),
        "SUB"   => GoldilocksField::from_canonical_u64(0b0110011),
        "SLL"   => GoldilocksField::from_canonical_u64(0b0110011),
        "SLT"   => GoldilocksField::from_canonical_u64(0b0110011),
        "SLTU"  => GoldilocksField::from_canonical_u64(0b0110011),
        "XOR"   => GoldilocksField::from_canonical_u64(0b0110011),
        "SRL"   => GoldilocksField::from_canonical_u64(0b0110011),
        "SRA"   => GoldilocksField::from_canonical_u64(0b0110011),
        "OR"    => GoldilocksField::from_canonical_u64(0b0110011),
        "AND"   => GoldilocksField::from_canonical_u64(0b0110011),
        _ => GoldilocksField::from_canonical_u64(0),
    }
}

// Reads and parses the sample trace file
pub fn parse_trace(_trace_path: &str) -> Vec<String> {
    let file = File::open("./traces/sample_trace.log").expect("Failed to open trace file");
    let reader = BufReader::new(file);
    reader.lines().filter_map(Result::ok).collect()
}

pub fn convert_trace_to_rows(parsed: &[String]) -> Vec<InstructionRow> {
    let mut rows = Vec::new();
    for line in parsed {
        // Example trace line format: "ADD x1 x2 x3 5 10 15"
        // opcode rd rs1 rs2 rs1_val rs2_val rd_val
        let tokens: Vec<&str> = line.split_whitespace().collect();
        if tokens.len() < 7 {
            continue;
        }
        let opcode = parse_opcode(tokens[0]);
        let rs1_val = tokens[4].parse::<u64>().unwrap_or(0);
        let rs2_val = tokens[5].parse::<u64>().unwrap_or(0);
        let rd_val = tokens[6].parse::<u64>().unwrap_or(0);

        rows.push(InstructionRow {
            opcode,
            rs1_val: GoldilocksField::from_canonical_u64(rs1_val),
            rs2_val: GoldilocksField::from_canonical_u64(rs2_val),
            rd_val: GoldilocksField::from_canonical_u64(rd_val),
        });
    }
    rows
}
pub fn convert_trace_to_rows(_parsed: &[String]) -> Vec<InstructionRow> {
    vec![
        InstructionRow {
            opcode: GoldilocksField::from_canonical_u64(1),
            rs1_val: GoldilocksField::from_canonical_u64(5),
            rs2_val: GoldilocksField::from_canonical_u64(10),
            rd_val: GoldilocksField::from_canonical_u64(15),
        },
    ]
}
