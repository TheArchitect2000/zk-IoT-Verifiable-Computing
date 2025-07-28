use riscv_trace_reader::{run_program, parse_trace, convert_trace_to_rows};
use riscv_trace_reader::prove_multi_instruction_constraint;
use riscv_trace_reader::{save_proof_and_circuit, load_proof_and_circuit};

fn main() {
    // let bin = "./test.bin";
    let trace = "./traces/sample_trace.log";

    // run_program(bin, trace);
    let parsed = parse_trace(trace);
    let rows = convert_trace_to_rows(&parsed);

    let (proof, circuit) = prove_multi_instruction_constraint(&rows);
    println!("Generated proof for {} instructions", rows.len());
    println!("Public inputs: {:?}", proof.public_inputs);

    save_proof_and_circuit(&proof, &circuit, "./proof.bin", "./circuit.bin");
    println!("Proof and circuit saved to files.");

    let (proof, circuit) = load_proof_and_circuit("./proof.bin", "./circuit.bin");
    match circuit.verify(proof) {
        Ok(_) => println!("✅ Proof verified!"),
        Err(e) => println!("❌ Verification failed: {:?}", e),
    }
}
