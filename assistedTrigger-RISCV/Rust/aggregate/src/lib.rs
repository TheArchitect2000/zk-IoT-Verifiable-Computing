pub mod circuit;
pub mod trace_parser;
pub mod verifier;
pub mod zk;

pub use trace_parser::{run_qemu, parse_trace, convert_trace_to_rows};
pub use zk::prove_aggregated_instructions;
pub use verifier::verify_instruction_proof;
