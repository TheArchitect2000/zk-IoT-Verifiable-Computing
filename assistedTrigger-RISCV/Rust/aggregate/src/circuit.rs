use plonky2::field::goldilocks_field::GoldilocksField;
use plonky2::field::types::Field;
use plonky2::plonk::circuit_builder::CircuitBuilder;
use plonky2::plonk::circuit_data::{CircuitConfig, CircuitData};
use plonky2::plonk::config::PoseidonGoldilocksConfig;
use plonky2::iop::witness::{PartialWitness, WitnessWrite};
use plonky2::plonk::prover::prove;
use plonky2::util::timing::TimingTree;
use log::Level;
use plonky2::plonk::proof::ProofWithPublicInputs;
use crate::zk::InstructionRow;
use anyhow::Error;

pub fn prove_multi_instruction_constraint(
    rows: &[InstructionRow<GoldilocksField>],
) -> Result<
    (
        ProofWithPublicInputs<GoldilocksField, PoseidonGoldilocksConfig, 2>,
        CircuitData<GoldilocksField, PoseidonGoldilocksConfig, 2>,
    ),
    anyhow::Error,
> {
    assert!(!rows.is_empty(), "Instruction row trace is empty!");

    println!("Parsed {} instruction rows", rows.len());
    for row in rows {
        println!(
            "opcode: {}, rs1: {}, rs2: {}, imm_flag: {}, imm_val: {}, rd: {}",
            row.opcode, row.rs1_val, row.rs2_val, row.imm_flag, row.imm_val, row.rd_val
        );
    }

    let config = CircuitConfig::standard_recursion_config();
    let mut builder = CircuitBuilder::<GoldilocksField, 2>::new(config);

    let mut opcode_targets = vec![];
    let mut rs1_targets = vec![];
    let mut rs2_targets = vec![];
    let mut imm_flag_targets = vec![];
    let mut imm_val_targets = vec![];
    let mut rd_targets = vec![];

    for _ in rows {
        opcode_targets.push(builder.add_virtual_target());
        rs1_targets.push(builder.add_virtual_target());
        rs2_targets.push(builder.add_virtual_target());
        imm_flag_targets.push(builder.add_virtual_target());
        imm_val_targets.push(builder.add_virtual_target());
        rd_targets.push(builder.add_virtual_target());
    }

    let zero = builder.zero();
    let one = builder.one();

    for i in 0..rows.len() {
        let opcode = &opcode_targets[i];
        let rs1 = &rs1_targets[i];
        let rs2 = &rs2_targets[i];
        let imm_flag = &imm_flag_targets[i];
        let imm_val = &imm_val_targets[i];
        let rd = &rd_targets[i];

        let imm_bool = builder.is_equal(*imm_flag, one);

        let one_const = builder.one();
        let imm_bool = builder.is_equal(*imm_flag, one_const);
        let rs2_or_imm = builder.select(imm_bool, *imm_val, *rs2);

        // Precompute constants for opcodes
        let c1 = builder.constant(GoldilocksField::from_canonical_u64(1)); // add
        let c2 = builder.constant(GoldilocksField::from_canonical_u64(2)); // sub
        let c3 = builder.constant(GoldilocksField::from_canonical_u64(3)); // mul
        let c4 = builder.constant(GoldilocksField::from_canonical_u64(4)); // addi
        let c5 = builder.constant(GoldilocksField::from_canonical_u64(5)); // div
        let c7 = builder.constant(GoldilocksField::from_canonical_u64(7)); // and
        let c8 = builder.constant(GoldilocksField::from_canonical_u64(8)); // or

        // Match opcodes
        let is_add = builder.is_equal(*opcode, c1);
        let is_sub = builder.is_equal(*opcode, c2);
        let is_mul = builder.is_equal(*opcode, c3);
        let is_addi = builder.is_equal(*opcode, c4);
        let is_div = builder.is_equal(*opcode, c5);
        let is_and = builder.is_equal(*opcode, c7);
        let is_or = builder.is_equal(*opcode, c8);

        // Operations
        let add_res = builder.add(*rs1, rs2_or_imm);
        let sub_res = builder.sub(*rs1, rs2_or_imm);
        let mul_res = builder.mul(*rs1, rs2_or_imm);
        let addi_res = builder.add(*rs1, rs2_or_imm);

        let is_rs2_zero = builder.is_equal(rs2_or_imm, zero);
        let safe_divisor = builder.select(is_rs2_zero, one, rs2_or_imm);
        let raw_div = builder.div(*rs1, safe_divisor);
        let div_res = builder.select(is_rs2_zero, zero, raw_div);

        let and_res = builder.mul(*rs1, rs2_or_imm);
        let or_res = {
            let sum = builder.add(*rs1, rs2_or_imm);
            let product = builder.mul(*rs1, rs2_or_imm);
            builder.sub(sum, product)
        };

        // Conditional result based on opcode
        let mut result = builder.select(is_add, add_res, zero);
        result = builder.select(is_sub, sub_res, result);
        result = builder.select(is_mul, mul_res, result);
        result = builder.select(is_addi, addi_res, result);
        result = builder.select(is_div, div_res, result);
        result = builder.select(is_and, and_res, result);
        result = builder.select(is_or, or_res, result);

        // Enforce result == rd
        let is_eq = builder.is_equal(result, *rd);
        
        // Register the equality check as a public input instead of asserting
        builder.assert_one(is_eq.target);

        // Register public inputs
        builder.register_public_input(*opcode);
        builder.register_public_input(*rs1);
        builder.register_public_input(*rs2);
        builder.register_public_input(*imm_flag);
        builder.register_public_input(*imm_val);
        builder.register_public_input(*rd);
    }

    let data = builder.build::<PoseidonGoldilocksConfig>();
    let mut pw = PartialWitness::new();

    for (i, row) in rows.iter().enumerate() {
        pw.set_target(opcode_targets[i], row.opcode).unwrap();
        pw.set_target(rs1_targets[i], row.rs1_val).unwrap();
        pw.set_target(rs2_targets[i], row.rs2_val).unwrap();
        pw.set_target(imm_flag_targets[i], row.imm_flag).unwrap();
        pw.set_target(imm_val_targets[i], row.imm_val).unwrap();
        pw.set_target(rd_targets[i], row.rd_val).unwrap();
    }

    let mut timing = TimingTree::new("prove", Level::Info);

    let proof = prove(&data.prover_only, &data.common, pw, &mut timing)?;
    Ok((proof, data))
}
