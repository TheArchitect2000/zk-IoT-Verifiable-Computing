use plonky2::{
    field::goldilocks_field::GoldilocksField,
    plonk::{
        circuit_data::CircuitData,
        config::PoseidonGoldilocksConfig,
        proof::ProofWithPublicInputs,
    },
};

pub fn verify_instruction_proof(
    proof: &ProofWithPublicInputs<GoldilocksField, PoseidonGoldilocksConfig, 2>,
    circuit: &CircuitData<GoldilocksField, PoseidonGoldilocksConfig, 2>,
) -> bool {
    circuit.verify(proof.clone()).is_ok()
}
