pub mod trace_parser;
pub mod zk;
pub mod circuit;
pub mod verifier;

pub use trace_parser::{TraceEntry, run_qemu, parse_trace};
pub use zk::{InstructionRow, convert_trace_to_rows};
pub use circuit::prove_instruction_constraint;
pub use verifier::verify_instruction_proof;