use plonky2::{
    field::{goldilocks_field::GoldilocksField, types::Field},
    iop::{witness::PartialWitness},
    plonk::{
        circuit_builder::CircuitBuilder,
        circuit_data::{CircuitConfig, CircuitData},
        config::PoseidonGoldilocksConfig,
        proof::ProofWithPublicInputs,
    },
};

use crate::trace_parser::InstructionRow;

pub fn build_instruction_circuit(rows: &[InstructionRow]) -> (
    ProofWithPublicInputs<GoldilocksField, PoseidonGoldilocksConfig, 2>,
    CircuitData<GoldilocksField, PoseidonGoldilocksConfig, 2>,
) {
    const D: usize = 2;
    let config = CircuitConfig::standard_recursion_config();
    let mut builder = CircuitBuilder::<GoldilocksField, D>::new(config);

    let mut public_inputs = Vec::new();

    for row in rows {
        let opcode = builder.constant(row.opcode);
        let rs1 = builder.constant(row.rs1_val);
        let rs2 = builder.constant(row.rs2_val);
        let rd = builder.constant(row.rd_val);

        // Example constraint: rd = rs1 + rs2 for opcode 1
        let computed = builder.add(rs1, rs2);
        let const_opcode = builder.constant(GoldilocksField::from_canonical_u64(1));
        let is_add = builder.is_equal(opcode, const_opcode);
        let diff = builder.sub(rd, computed);
        let is_add_tgt = is_add.target;
        let zero = builder.mul(diff, is_add_tgt);

        builder.assert_zero(zero);

        public_inputs.extend_from_slice(&[opcode, rs1, rs2, rd]);
    }

    builder.register_public_inputs(&public_inputs);
    let data = builder.build::<PoseidonGoldilocksConfig>();

    let pw = PartialWitness::new();
    let proof = data.prove(pw).unwrap();

    (proof, data)
}
