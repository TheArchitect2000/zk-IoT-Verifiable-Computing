use plonky2::{
    field::goldilocks_field::GoldilocksField,
    plonk::{
        circuit_data::CircuitData,
        config::PoseidonGoldilocksConfig,
        proof::ProofWithPublicInputs,
    },
};

use crate::{circuit::build_instruction_circuit, trace_parser::InstructionRow};

pub fn prove_aggregated_instructions(
    rows: &[InstructionRow],
) -> (
    ProofWithPublicInputs<GoldilocksField, PoseidonGoldilocksConfig, 2>,
    CircuitData<GoldilocksField, PoseidonGoldilocksConfig, 2>,
) {
    build_instruction_circuit(rows)
}
