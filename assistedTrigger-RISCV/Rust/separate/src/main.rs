use plonky2::field::goldilocks_field::GoldilocksField;
use plonky2::plonk::config::PoseidonGoldilocksConfig;
use plonky2::plonk::circuit_data::CircuitData;
use plonky2::plonk::proof::ProofWithPublicInputs;
use plonky2::field::types::PrimeField64;

use riscv_trace_reader::{parse_trace, convert_trace_to_rows, prove_instruction_constraint};
use riscv_trace_reader::verify_instruction_proof;

use std::fs::File;
use std::io::{BufWriter, Result as IoResult};
use bincode;
use serde::{Serialize, Deserialize};

type MyProof = ProofWithPublicInputs<GoldilocksField, PoseidonGoldilocksConfig, 2>;

fn save_all_proofs(proofs: &[MyProof], path: &str) -> std::io::Result<()> {
    let file = File::create(path)?;
    let writer = BufWriter::new(file);
    bincode::serialize_into(writer, proofs).expect("Failed to serialize proofs");
    Ok(())
}


fn main() {
    let _bin = "./test.bin";
    let trace = "./traces/sample_trace.log";

    // run_qemu(bin, trace);
    let parsed = parse_trace(trace);
    let rows = convert_trace_to_rows(&parsed);

    for (i, row) in rows.iter().enumerate() {
        let opcode_id = row.opcode.to_canonical_u64();
        let rs1 = row.rs1_val.to_canonical_u64();
        let rs2 = row.rs2_val.to_canonical_u64();
        let rd = row.rd_val.to_canonical_u64();

        if [1, 2, 3, 4, 5, 7, 8].contains(&opcode_id) {
            let (proof, circuit) = prove_instruction_constraint(opcode_id, rs1, rs2, rd);
            println!("Row {}: Proof generated: {:?}", i, proof.public_inputs);
            if verify_instruction_proof(proof, &circuit) {
                println!("✅ Proof verification succeeded for opcode {}", opcode_id);
            } else {
                println!("❌ Proof verification failed for opcode {}", opcode_id);
            }
        } else {
            println!("Skipping opcode {} at row {}", opcode_id, i);
        }
    }

    // The aggregated proof for all rows
    let mut proofs_and_circuits = vec![];

    for row in &rows {
        let opcode_id = row.opcode.to_canonical_u64();
        let rs1 = row.rs1_val.to_canonical_u64();
        let rs2 = row.rs2_val.to_canonical_u64();
        let rd = row.rd_val.to_canonical_u64();

        if [1, 2, 3, 4, 5, 7, 8].contains(&opcode_id) {
            let (proof, circuit) = prove_instruction_constraint(opcode_id, rs1, rs2, rd);
            proofs_and_circuits.push((proof, circuit));
        }
    }

    if !proofs_and_circuits.is_empty() {
        println!("Aggregated proof generated for {} instructions.", proofs_and_circuits.len());

        let aggregated_public_inputs: Vec<_> = proofs_and_circuits
            .iter()
            .flat_map(|(proof, _)| proof.public_inputs.clone())
            .collect();
        println!("Aggregated public inputs: {:?}", aggregated_public_inputs);

        let all_proofs: Vec<_> = proofs_and_circuits.iter().map(|(p, c)| p.clone()).collect();
        save_all_proofs(&all_proofs, "./proof.bin").unwrap();
    } else {
        println!("No valid instructions found for aggregation.");
    }

}
