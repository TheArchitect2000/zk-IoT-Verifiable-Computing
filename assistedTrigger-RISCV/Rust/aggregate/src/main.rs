use riscv_trace_reader::{run_qemu, parse_trace, convert_trace_to_rows};
use riscv_trace_reader::prove_multi_instruction_constraint;
use riscv_trace_reader::{save_proof_and_circuit, load_proof_and_circuit};

fn main() {
    // let bin = "./test.bin";
    let trace = "./traces/trace.log";

    // run_qemu(bin, trace);
    let parsed = parse_trace(trace);
    let rows = convert_trace_to_rows(&parsed);

    let (proof, circuit) = prove_multi_instruction_constraint(&rows);
    println!("Generated proof for {} instructions", rows.len());
    println!("Public inputs: {:?}", proof.public_inputs);

    let (proof, data) = prove_multi_instruction_constraint(&rows);

    save_proof_and_circuit(&proof, &data, "./proof.bin", "./circuit.bin");
    println!("Proof and circuit saved to files.");

    let (proof, data) = load_proof_and_circuit("./proof.bin", "./circuit.bin");
    match data.verify(proof) {
        Ok(_) => println!("✅ Proof verified!"),
        Err(e) => println!("❌ Verification failed: {:?}", e),
    }
}
