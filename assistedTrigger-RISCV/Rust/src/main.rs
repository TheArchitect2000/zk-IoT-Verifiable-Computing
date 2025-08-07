use riscv_trace_reader::{parse_trace, convert_trace_to_rows};
use riscv_trace_reader::prove_multi_instruction_constraint;
use riscv_trace_reader::{save_proof_and_circuit, load_proof_and_circuit};
use riscv_trace_reader::run_program;

fn main() {
    let bin = "./test.bin";
    let trace = "./traces/trace_cleaned.log";
    // let trace = "./traces/sample_trace.log";

    run_program(bin);
    let parsed = parse_trace(trace);
    let rows = convert_trace_to_rows(&parsed);

    match prove_multi_instruction_constraint(&rows) {
        Ok((proof, circuit)) => {
            println!("✅ Generated Proof Successfully!");
            println!("Generated proof for {} instructions", rows.len());
            println!("Public inputs: {:?}", proof.public_inputs);

            save_proof_and_circuit(&proof, &circuit, "./proof.bin", "./circuit.bin");
            println!("Proof and circuit saved to files.");

            let (proof_loaded, circuit_loaded) = load_proof_and_circuit("./proof.bin", "./circuit.bin");
            match circuit_loaded.verify(proof_loaded) {
                Ok(_) => println!("✅ Proof verified!"),
                Err(e) => println!("❌ Verification failed: {:?}", e),
            }
        }
        Err(e) => {
            println!("❌ Failed to Generate Proof: {:?}", e);
            std::process::exit(1);
        }
    }
}
