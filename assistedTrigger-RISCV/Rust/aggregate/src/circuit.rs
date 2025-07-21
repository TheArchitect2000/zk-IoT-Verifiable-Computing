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
use crate::zk::InstructionRow;

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


pub fn prove_multi_instruction_constraint(
    rows: &[InstructionRow<GoldilocksField>],
) -> (
    ProofWithPublicInputs<GoldilocksField, PoseidonGoldilocksConfig, 2>,
    CircuitData<GoldilocksField, PoseidonGoldilocksConfig, 2>,
) {
    let config = CircuitConfig::standard_recursion_config();
    let mut builder = CircuitBuilder::<GoldilocksField, 2>::new(config);

    let mut opcode_targets = vec![];
    let mut rs1_targets = vec![];
    let mut rs2_targets = vec![];
    let mut rd_targets = vec![];

    for _ in rows {
        opcode_targets.push(builder.add_virtual_target());
        rs1_targets.push(builder.add_virtual_target());
        rs2_targets.push(builder.add_virtual_target());
        rd_targets.push(builder.add_virtual_target());
    }

    for i in 0..rows.len() {
        let opcode = &opcode_targets[i];
        let rs1 = &rs1_targets[i];
        let rs2 = &rs2_targets[i];
        let rd = &rd_targets[i];

        // Precompute constants to avoid double borrow
        let c1 = builder.constant(GoldilocksField::from_canonical_u64(1));
        let c2 = builder.constant(GoldilocksField::from_canonical_u64(2));
        let c3 = builder.constant(GoldilocksField::from_canonical_u64(3));
        let c4 = builder.constant(GoldilocksField::from_canonical_u64(4));
        let c5 = builder.constant(GoldilocksField::from_canonical_u64(5));
        let c7 = builder.constant(GoldilocksField::from_canonical_u64(7));
        let c8 = builder.constant(GoldilocksField::from_canonical_u64(8));

        let is_add = builder.is_equal(*opcode, c1);
        let add_res = builder.add(*rs1, *rs2);

        let is_sub = builder.is_equal(*opcode, c2);
        let sub_res = builder.sub(*rs1, *rs2);

        let is_mul = builder.is_equal(*opcode, c3);
        let mul_res = builder.mul(*rs1, *rs2);

        let is_addi = builder.is_equal(*opcode, c4);
        let addi_res = builder.add(*rs1, *rs2);

        let is_div = builder.is_equal(*opcode, c5);
        let div_res = builder.div(*rs1, *rs2);

        let is_and = builder.is_equal(*opcode, c7);
        let and_res = builder.mul(*rs1, *rs2); // Field logic approximation

        let is_or = builder.is_equal(*opcode, c8);
        let a_plus_b = builder.add(*rs1, *rs2);
        let a_times_b = builder.mul(*rs1, *rs2);
        let or_res = builder.sub(a_plus_b, a_times_b);

        // Combine all operations based on opcode match
        let zero = builder.zero();
        let mut result = builder.select(is_add, add_res, zero);
        result = builder.select(is_sub, sub_res, result);
        result = builder.select(is_mul, mul_res, result);
        result = builder.select(is_addi, addi_res, result);
        result = builder.select(is_div, div_res, result);
        result = builder.select(is_and, and_res, result);
        result = builder.select(is_or, or_res, result);

        // Enforce computed == rd
        let eq = builder.is_equal(result, *rd);
        builder.assert_one(eq.target);

        // Register all public inputs
        builder.register_public_input(*opcode);
        builder.register_public_input(*rs1);
        builder.register_public_input(*rs2);
        builder.register_public_input(*rd);
    }


    let data = builder.build::<PoseidonGoldilocksConfig>();
    let mut pw = PartialWitness::new();

    for (i, row) in rows.iter().enumerate() {
        pw.set_target(opcode_targets[i], row.opcode).unwrap();
        pw.set_target(rs1_targets[i], row.rs1_val).unwrap();
        pw.set_target(rs2_targets[i], row.rs2_val).unwrap();
        pw.set_target(rd_targets[i], row.rd_val).unwrap();
    }

    let mut timing = TimingTree::new("prove", Level::Info);
    let proof = prove(&data.prover_only, &data.common, pw, &mut timing).expect("Proof failed");

    (proof, data)
}
