use plonky2::field::goldilocks_field::GoldilocksField;
use plonky2::plonk::config::PoseidonGoldilocksConfig;
use plonky2::plonk::circuit_data::CircuitData;
use plonky2::plonk::proof::ProofWithPublicInputs;

pub fn verify_instruction_proof(
    proof: ProofWithPublicInputs<GoldilocksField, PoseidonGoldilocksConfig, 2>,
    data: &CircuitData<GoldilocksField, PoseidonGoldilocksConfig, 2>,
) -> bool {
    data.verify(proof).is_ok()
}