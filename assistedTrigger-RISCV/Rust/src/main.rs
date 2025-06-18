use riscv_trace_reader::{
    convert_trace_to_rows, parse_trace, prove_aggregated_instructions, verify_instruction_proof,
};

fn main() {
    let trace_path = "./traces/sample_trace.log";
    let parsed = parse_trace(trace_path);
    let rows = convert_trace_to_rows(&parsed);

    let (proof, circuit) = prove_aggregated_instructions(&rows);

    println!("🔒 Aggregated proof public inputs: {:?}", proof.public_inputs);

    if verify_instruction_proof(&proof, &circuit) {
        println!("✅ Aggregated proof verified successfully!");
    } else {
        println!("❌ Aggregated proof verification failed!");
    }
}
