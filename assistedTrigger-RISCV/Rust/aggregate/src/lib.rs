pub mod trace_parser;
pub mod zk;
pub mod circuit;
pub mod verifier;
pub mod store;

pub use trace_parser::{TraceEntry, run_program, parse_trace};
pub use zk::{InstructionRow, convert_trace_to_rows};
pub use circuit::{prove_instruction_constraint, prove_multi_instruction_constraint};
pub use verifier::verify_instruction_proof;
pub use store::{save_proof_and_circuit, load_proof_and_circuit};