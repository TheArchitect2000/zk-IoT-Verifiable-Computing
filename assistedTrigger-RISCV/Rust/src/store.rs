use std::fs;
use std::fs::File;
use std::io::Write;

use plonky2::field::goldilocks_field::GoldilocksField;
use plonky2::plonk::circuit_data::CircuitData;
use plonky2::plonk::config::PoseidonGoldilocksConfig;
use plonky2::plonk::proof::ProofWithPublicInputs;
use plonky2::util::serialization::{DefaultGateSerializer, DefaultGeneratorSerializer};

pub fn save_proof_and_circuit(
    proof: &ProofWithPublicInputs<GoldilocksField, PoseidonGoldilocksConfig, 2>,
    data: &CircuitData<GoldilocksField, PoseidonGoldilocksConfig, 2>,
    proof_path: &str,
    circuit_path: &str,
) {
    let gate_serializer = DefaultGateSerializer;
    let generator_serializer = DefaultGeneratorSerializer::<PoseidonGoldilocksConfig, 2>::default();
    let proof_bytes = proof.to_bytes();
    let data_bytes = data
        .to_bytes(&gate_serializer, &generator_serializer)
        .unwrap();

    fs::write("./proof.bin", proof_bytes).unwrap();
    fs::write("./circuit.bin", data_bytes).unwrap();
    
    // let mut file = File::create(proof_path).expect("Unable to create file");
    // write!(file, "{:#?}", proof).expect("Unable to write proof debug output");

    // file = File::create(circuit_path).expect("Unable to create file");
    // write!(file, "{:#?}", data).expect("Unable to write proof debug output");

}

pub fn load_proof_and_circuit(
    proof_path: &str,
    circuit_path: &str,
) -> (
    ProofWithPublicInputs<GoldilocksField, PoseidonGoldilocksConfig, 2>,
    CircuitData<GoldilocksField, PoseidonGoldilocksConfig, 2>,
) {
    let gate_serializer = DefaultGateSerializer;
    let generator_serializer = DefaultGeneratorSerializer::<PoseidonGoldilocksConfig, 2>::default();
    let circuit_bytes = fs::read(circuit_path).unwrap();
    let circuit = CircuitData::from_bytes(
        &circuit_bytes,
        &gate_serializer,
        &generator_serializer,
    )
    .unwrap();

    let proof_bytes = fs::read(proof_path).unwrap();
    let proof = ProofWithPublicInputs::from_bytes(proof_bytes, &circuit.common).unwrap();

    (proof, circuit)
}
