use plonky2::field::goldilocks_field::GoldilocksField;
use plonky2::field::types::Field;
use plonky2::plonk::circuit_builder::CircuitBuilder;
use plonky2::plonk::circuit_data::CircuitConfig;
use plonky2::plonk::circuit_data::CircuitData;
use plonky2::plonk::config::PoseidonGoldilocksConfig;
use plonky2::iop::witness::PartialWitness;
use plonky2::iop::witness::WitnessWrite;
use plonky2::plonk::prover::prove;
use plonky2::util::timing::TimingTree;
use log::Level;
use plonky2::plonk::proof::ProofWithPublicInputs;

pub fn prove_instruction_constraint(
    opcode_id: u64,
    rs1: u64,
    rs2: u64,
    rd: u64,
) -> (
    ProofWithPublicInputs<GoldilocksField, PoseidonGoldilocksConfig, 2>,
    CircuitData<GoldilocksField, PoseidonGoldilocksConfig, 2>,
) {
    let config = CircuitConfig::standard_recursion_config();
    let mut builder = CircuitBuilder::<GoldilocksField, 2>::new(config);

    // Correct: single Targets
    let opcode = builder.add_virtual_target();
    let rs1_val = builder.add_virtual_target();
    let rs2_val = builder.add_virtual_target();
    let rd_val = builder.add_virtual_target();

    // Compute
    let computed = match opcode_id {
        1 => builder.add(rs1_val, rs2_val),       // add
        2 => builder.sub(rs1_val, rs2_val),       // sub
        3 => builder.mul(rs1_val, rs2_val),       // mul
        4 => builder.add(rs1_val, rs2_val),       // addi (same as add)
        5 => builder.div(rs1_val, rs2_val),       // div (field logic)
        7 => builder.mul(rs1_val, rs2_val),       // and (field logic)
        8 => {                                    // or (field logic: a + b - ab)
            let a_plus_b = builder.add(rs1_val, rs2_val);
            let a_times_b = builder.mul(rs1_val, rs2_val);
            builder.sub(a_plus_b, a_times_b)
        }
        _ => builder.constant(GoldilocksField::ZERO),
    };

    // Enforce computed == rd_val
    let eq = builder.is_equal(computed, rd_val);
    builder.assert_one(eq.target);

    // Register public inputs
    builder.register_public_input(opcode);
    builder.register_public_input(rs1_val);
    builder.register_public_input(rs2_val);
    builder.register_public_input(rd_val);

    let data = builder.build::<PoseidonGoldilocksConfig>();
    let mut pw = PartialWitness::new();
    let _ = pw.set_target(opcode, GoldilocksField::from_canonical_u64(opcode_id)).expect("Failed to set witness target");
    let _ = pw.set_target(rs1_val, GoldilocksField::from_canonical_u64(rs1)).expect("Failed to set witness target");
    let _ = pw.set_target(rs2_val, GoldilocksField::from_canonical_u64(rs2)).expect("Failed to set witness target");
    let _ = pw.set_target(rd_val, GoldilocksField::from_canonical_u64(rd)).expect("Failed to set witness target");


    let mut timing = TimingTree::new("prove", Level::Info);
    let proof = prove(&data.prover_only, &data.common, pw, &mut timing)
        .expect("Proof should succeed");

    (proof, data)
}
