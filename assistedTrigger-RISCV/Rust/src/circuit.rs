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
use plonky2::field::types::Field64;

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

        // let imm_bool = builder.is_equal(*imm_flag, one);

        let one_const = builder.one();
        let imm_bool = builder.is_equal(*imm_flag, one_const);
        let rs2_or_imm = builder.select(imm_bool, *imm_val, *rs2);

        // // Precompute constants for opcodes
        // let c1 = builder.constant(GoldilocksField::from_canonical_u64(1)); // add
        // let c2 = builder.constant(GoldilocksField::from_canonical_u64(2)); // sub
        // let c3 = builder.constant(GoldilocksField::from_canonical_u64(3)); // mul
        // let c4 = builder.constant(GoldilocksField::from_canonical_u64(4)); // addi
        // let c5 = builder.constant(GoldilocksField::from_canonical_u64(5)); // div
        // let c6 = builder.constant(GoldilocksField::from_canonical_u64(6)); // sd
        // let c7 = builder.constant(GoldilocksField::from_canonical_u64(7)); // ld
        // let c8 = builder.constant(GoldilocksField::from_canonical_u64(8)); // li
        // let c9 = builder.constant(GoldilocksField::from_canonical_u64(9)); // lw
        // let c10 = builder.constant(GoldilocksField::from_canonical_u64(10)); // addw
        // let c11 = builder.constant(GoldilocksField::from_canonical_u64(11)); // subw
        // let c12 = builder.constant(GoldilocksField::from_canonical_u64(12)); // slliw
        // let c13 = builder.constant(GoldilocksField::from_canonical_u64(13)); // sraiw
        // let c14 = builder.constant(GoldilocksField::from_canonical_u64(14)); // xor
        // let c15 = builder.constant(GoldilocksField::from_canonical_u64(15)); // or
        // let c16 = builder.constant(GoldilocksField::from_canonical_u64(16)); // and
        // let c17 = builder.constant(GoldilocksField::from_canonical_u64(17)); // mv

        // === Opcode constants ===
        let mut c = |v| builder.constant(GoldilocksField::from_canonical_u64(v));
        let c1 = c(1);  let c2 = c(2);  let c3 = c(3);  let c4 = c(4);
        let c5 = c(5);  let c6 = c(6);  let c7 = c(7);  let c8 = c(8);
        let c9 = c(9);  let c10 = c(10); let c11 = c(11); let c12 = c(12);
        let c13 = c(13); let c14 = c(14); let c15 = c(15); let c16 = c(16);
        let c17 = c(17);


        // // Match opcodes
        // let is_add = builder.is_equal(*opcode, c1);
        // let is_sub = builder.is_equal(*opcode, c2);
        // let is_mul = builder.is_equal(*opcode, c3);
        // let is_addi = builder.is_equal(*opcode, c4);
        // let is_div = builder.is_equal(*opcode, c5);
        // let is_sd = builder.is_equal(*opcode, c6);
        // let is_ld = builder.is_equal(*opcode, c7);
        // let is_li = builder.is_equal(*opcode, c8);
        // let is_lw = builder.is_equal(*opcode, c9);
        // let is_addw = builder.is_equal(*opcode, c10);
        // let is_subw = builder.is_equal(*opcode, c11);
        // let is_slliw = builder.is_equal(*opcode, c12);
        // let is_sraiw = builder.is_equal(*opcode, c13);
        // let is_xor = builder.is_equal(*opcode, c14);
        // let is_or = builder.is_equal(*opcode, c15);
        // let is_and = builder.is_equal(*opcode, c16);
        // let is_mv = builder.is_equal(*opcode, c17);

        // === Match opcodes ===
        let is_add    = builder.is_equal(*opcode, c1);
        let is_sub    = builder.is_equal(*opcode, c2);
        let is_mul    = builder.is_equal(*opcode, c3);
        let is_addi   = builder.is_equal(*opcode, c4);
        let is_div    = builder.is_equal(*opcode, c5);
        let is_sd     = builder.is_equal(*opcode, c6);
        let is_ld     = builder.is_equal(*opcode, c7);
        let is_li     = builder.is_equal(*opcode, c8);
        let is_lw     = builder.is_equal(*opcode, c9);
        let is_addw   = builder.is_equal(*opcode, c10);
        let is_subw   = builder.is_equal(*opcode, c11);
        let is_slliw  = builder.is_equal(*opcode, c12);
        let is_sraiw  = builder.is_equal(*opcode, c13);
        let is_xor    = builder.is_equal(*opcode, c14);
        let is_or     = builder.is_equal(*opcode, c15);
        let is_and    = builder.is_equal(*opcode, c16);
        let is_mv     = builder.is_equal(*opcode, c17);

        // Operations
        // let add_res = builder.add(*rs1, rs2_or_imm);
        // let sub_res = builder.sub(*rs1, rs2_or_imm);
        // let mul_res = builder.mul(*rs1, rs2_or_imm);
        // let addi_res = builder.add(*rs1, rs2_or_imm);
        // let is_rs2_zero = builder.is_equal(rs2_or_imm, zero);
        // let safe_divisor = builder.select(is_rs2_zero, one, rs2_or_imm);
        // let raw_div = builder.div(*rs1, safe_divisor);
        // let div_res = builder.select(is_rs2_zero, zero, raw_div);
        
        // let is_sd_or_ld = builder.or(is_sd, is_ld);
        // let is_li_or_lw = builder.or(is_li, is_lw);
        // let is_addw_or_subw = builder.or(is_addw, is_subw);
        // let is_slliw_or_sraiw = builder.or(is_slliw, is_sraiw);
        // let is_xor_or_or = builder.or(is_xor, is_or);
        // let is_mv = builder.is_equal(*opcode, c17); 

        // let and_res = builder.mul(*rs1, rs2_or_imm);
        // let or_res = {
        //     let sum = builder.add(*rs1, rs2_or_imm);
        //     let product = builder.mul(*rs1, rs2_or_imm);
        //     builder.sub(sum, product)
        // };

        

        // === Arithmetic operations ===
        let add_res = builder.add(*rs1, rs2_or_imm);
        let sub_res = builder.sub(*rs1, rs2_or_imm);
        let mul_res = builder.mul(*rs1, rs2_or_imm);

        // Safe division
        let is_rs2_zero = builder.is_equal(rs2_or_imm, zero);
        let safe_divisor = builder.select(is_rs2_zero, one, rs2_or_imm);
        let raw_div = builder.div(*rs1, safe_divisor);
        let div_res = builder.select(is_rs2_zero, zero, raw_div);

        // Placeholder logic for unimplemented ops
        let dummy_shift = *rs1; // shift ops not implemented yet
        let dummy_bitwise = *rs1; // xor/or/and not supported on Target




        // // Conditional result based on opcode
        // let mut result = builder.select(is_add, add_res, zero);
        // result = builder.select(is_sub, sub_res, result);
        // result = builder.select(is_mul, mul_res, result);
        // result = builder.select(is_addi, addi_res, result);
        // result = builder.select(is_div, div_res, result);
        // result = builder.select(is_sd_or_ld, zero, result); // sd/ld do not produce a result
        // result = builder.select(is_li_or_lw, *imm_val, result); // li produces imm_val, lw does not produce a result
        // result = builder.select(is_addw_or_subw, result, zero); // addw/subw do not produce a result
        // result = builder.select(is_slliw_or_sraiw, *rs1, result); // slliw/sraiw do not produce a result
        // result = builder.select(is_xor_or_or, or_res, result); // xor/or produce or_res
        // result = builder.select(is_and, and_res, result); // and produces and_res
        // result = builder.select(is_mv, *rs1, result); // mv produces rs1

        // Result logic: step-by-step cascade
        let mut result = builder.select(is_add, add_res, zero);
        result = builder.select(is_sub, sub_res, result);
        result = builder.select(is_mul, mul_res, result);
        result = builder.select(is_addi, add_res, result); // rs1 + imm
        result = builder.select(is_div, div_res, result);
        result = builder.select(is_li, *imm_val, result);
        result = builder.select(is_mv, *rs1, result);

        // Temporarily forward rs1 for unsupported ops
        result = builder.select(is_addw, dummy_shift, result);
        result = builder.select(is_subw, dummy_shift, result);
        result = builder.select(is_slliw, dummy_shift, result);
        result = builder.select(is_sraiw, dummy_shift, result);
        result = builder.select(is_xor, dummy_bitwise, result);
        result = builder.select(is_or, dummy_bitwise, result);
        result = builder.select(is_and, dummy_bitwise, result);





        // // Enforce result == rd
        // let is_eq = builder.is_equal(result, *rd);
        
        // No output ops
        let no_output = builder.or(is_sd, is_ld);
        let no_output = builder.or(no_output, is_lw);
        result = builder.select(no_output, zero, result);


        // // Register the equality check as a public input instead of asserting
        // builder.assert_one(is_eq.target);

        // === Enforce output ===
        let is_result_correct = builder.is_equal(result, *rd);
        builder.assert_one(is_result_correct.target);

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

    // for (i, row) in rows.iter().enumerate() {
    //     pw.set_target(opcode_targets[i], row.opcode).unwrap();
    //     pw.set_target(rs1_targets[i], row.rs1_val).unwrap();
    //     pw.set_target(rs2_targets[i], row.rs2_val).unwrap();
    //     pw.set_target(imm_flag_targets[i], row.imm_flag).unwrap();
    //     pw.set_target(imm_val_targets[i], row.imm_val).unwrap();
    //     pw.set_target(rd_targets[i], row.rd_val).unwrap();
    // }

    let to_field = |v: u128| {
        assert!(v < GoldilocksField::ORDER as u128, "Value too large for GoldilocksField: {}", v);
        GoldilocksField::from_canonical_u64(v as u64)
    };

    for (i, row) in rows.iter().enumerate() {
        pw.set_target(opcode_targets[i], to_field(row.opcode.0 as u128));
        pw.set_target(rs1_targets[i], to_field(row.rs1_val.0 as u128));
        pw.set_target(rs2_targets[i], to_field(row.rs2_val.0 as u128));
        pw.set_target(imm_flag_targets[i], to_field(row.imm_flag.0 as u128));
        pw.set_target(imm_val_targets[i], to_field(row.imm_val.0 as u128));
        pw.set_target(rd_targets[i], to_field(row.rd_val.0 as u128));
    }

    let mut timing = TimingTree::new("prove", Level::Info);

    let proof = prove(&data.prover_only, &data.common, pw, &mut timing)?;
    Ok((proof, data))
}
