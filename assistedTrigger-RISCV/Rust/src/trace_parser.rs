use plonky2::field::goldilocks_field::GoldilocksField;
use plonky2::field::types::Field;


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
