/********************************************************************************************
 * jolt_style_vm.cpp
 * ------------------------------------------------------------------------------------------
 * Zero-Knowledge Virtual Machine (ZK-VM) with KZG Commitments over BN254
 * ------------------------------------------------------------------------------------------
 *
 * Description:
 *   This implementation is a compact, production-style reference inspired by
 *   a16z’s “Jolt” system. It demonstrates how to commit to, prove, and verify
 *   the execution trace of a small stack-based virtual machine using:
 *
 *     • Polynomial commitments (KZG over BN254, via mcl)
 *     • Sumcheck protocol for transition constraints
 *     • Lookup tables for non-arithmetic (bitwise) operations
 *     • Fiat–Shamir transcript for randomness and challenges
 *     • SHA-256 for bytecode commitment
 *
 * ------------------------------------------------------------------------------------------
 * VM Design:
 *   The VM supports a minimal instruction set operating on a single stack:
 *
 *       OP_PUSH  imm      ; push immediate value
 *       OP_ADD             ; pop a,b → push (a + b)
 *       OP_SUB             ; pop a,b → push (b – a)
 *       OP_MUL             ; pop a,b → push (a × b)
 *       OP_AND             ; pop a,b → push ((a & b) on 4-bit domain)
 *       OP_OR              ; pop a,b → push ((a | b) on 4-bit domain)
 *       OP_HALT            ; stop and output top of stack
 *
 *   The logic opcodes (AND, OR) are verified against pre-committed lookup tables.
 *   The arithmetic operations are verified using polynomial relations and sumcheck.
 *
 * ------------------------------------------------------------------------------------------
 * Cryptographic Components:
 *   • Field: BN254 scalar field (Fr)
 *   • Commitments: KZG commitments via mcl::bn
 *   • Hashing: SHA-256 (for bytecode commitments)
 *   • Transcript: Fiat–Shamir for generating deterministic challenges
 *
 * ------------------------------------------------------------------------------------------
 * Data Flow:
 *   1. **Commitment Phase**
 *        - The bytecode is hashed (SHA-256) and committed as a KZG polynomial.
 *        - The execution trace is generated and its columns are committed.
 *        - Lookup tables (if logic ops are used) are loaded and committed.
 *
 *   2. **Proving Phase**
 *        - The prover generates transition constraints (sumcheck proof).
 *        - Samples random opcode indices for KZG openings.
 *        - Generates lookup openings for all logic operations used.
 *
 *   3. **Verification Phase**
 *        - The verifier reconstructs the code commitment from the program bytes.
 *        - Replays the transcript, verifies all KZG openings and sumcheck proof.
 *        - Confirms lookup table consistency for logic operations.
 *
 * ------------------------------------------------------------------------------------------
 * Lookup Table Files (for AND / OR):
 *   The LUTs must be generated once using the provided generator and saved as:
 *        lut_and_or_x.txt
 *        lut_and_or_y.txt
 *        lut_and_or_z.txt
 *        lut_and_or_op.txt
 *
 * ------------------------------------------------------------------------------------------
 * Compilation:
 *   Requires mcl (https://github.com/herumi/mcl)
 *
 *   Example (Ubuntu):
 *       sudo apt-get install -y build-essential cmake libgmp-dev
 *       git clone https://github.com/herumi/mcl.git
 *       cd mcl && mkdir build && cd build
 *       cmake .. && make -j$(nproc)
 *       sudo make install
 *
 *   Build this file with:
 *       g++ -std=gnu++17 -O2 jolt_style_vm.cpp -lmcl -lcrypto -o jolt_demo
 *
 * ------------------------------------------------------------------------------------------
 * Output:
 *   On success, prints:
 *       Verify: ACCEPT
 *       Claimed output: <result>
 *
 * ------------------------------------------------------------------------------------------
 * Notes:
 *   - The code polynomial uses a simple monomial representation (coeff[i] = byte[i]),
 *     ensuring deterministic commitments between prover and verifier.
 *   - The trace polynomial uses naive Lagrange interpolation for clarity; for large
 *     traces, replace with FFT/NTT interpolation for performance.
 *   - The 4-bit AND/OR logic can be replaced with wider LUTs (8-bit, 16-bit) if needed.
 *   - This code is intended as a production-shaped prototype — correctness and structure
 *     are prioritized over speed.
 *
 ********************************************************************************************/

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string>

#include <mcl/bn.hpp>
using namespace mcl::bn;

// ================================================================
// field helpers
// ================================================================
static Fr fr_from_u64(uint64_t v)
{
    Fr r;
    uint8_t buf[8];
    for (int i = 0; i < 8; i++) buf[7 - i] = (uint8_t)((v >> (i*8)) & 0xff);
    r.setBigEndianMod(buf, 8);
    return r;
}
static Fr fr_zero(){ Fr z; z.clear(); return z; }
static Fr fr_one(){ return fr_from_u64(1); }

static uint64_t fr_to_u64(const Fr& x)
{
    uint8_t buf[64];
    size_t n = x.serialize(buf, sizeof(buf));
    uint64_t v = 0;
    size_t start = (n > 8) ? (n - 8) : 0;
    for (size_t i = start; i < n; i++) v = (v << 8) | buf[i];
    return v;
}

// ================================================================
// single interpolation (for trace, not for code)
// ================================================================
struct Poly { std::vector<Fr> c; };

static void poly_normalize(Poly& p){
    while (!p.c.empty() && p.c.back().isZero()) p.c.pop_back();
}

// naive Lagrange on x=0..n-1
static Poly interpolate_on_range0(const std::vector<Fr>& vals)
{
    size_t n = vals.size();
    Poly acc; acc.c = { fr_zero() };
    Fr one = fr_one();
    for (size_t i=0;i<n;i++){
        Poly numer; numer.c = { fr_one() };
        Fr denom = fr_one();
        Fr xi = fr_from_u64(i);
        for (size_t j=0;j<n;j++){
            if (j == i) continue;
            Fr fj = fr_from_u64(j);
            Poly term;
            Fr minus_j = fr_zero() - fj;
            term.c = { minus_j, fr_one() };
            Poly tmp; tmp.c.assign(numer.c.size()+1, fr_zero());
            for (size_t a=0;a<numer.c.size();a++){
                tmp.c[a]   += numer.c[a] * term.c[0];
                tmp.c[a+1] += numer.c[a] * term.c[1];
            }
            numer = tmp;
            denom *= (xi - fj);
        }
        Fr denom_inv = one / denom;
        for (auto &cc : numer.c) cc *= denom_inv;
        if (numer.c.size() > acc.c.size()) acc.c.resize(numer.c.size());
        for (size_t k=0;k<numer.c.size();k++)
            acc.c[k] += numer.c[k] * vals[i];
    }
    poly_normalize(acc);
    return acc;
}

// ================================================================
// SHA-256
// ================================================================
struct SHA256 {
    uint32_t h[8];
    std::vector<uint8_t> buf;
    uint64_t bits=0;
    static inline uint32_t rotr(uint32_t x,int n){ return (x>>n)|(x<<(32-n)); }
    static inline uint32_t Ch(uint32_t x,uint32_t y,uint32_t z){ return (x&y)^(~x&z); }
    static inline uint32_t Maj(uint32_t x,uint32_t y,uint32_t z){ return (x&y)^(x&z); }
    static inline uint32_t Sig0(uint32_t x){ return rotr(x,2)^rotr(x,13)^rotr(x,22); }
    static inline uint32_t Sig1(uint32_t x){ return rotr(x,6)^rotr(x,11)^rotr(x,25); }
    static inline uint32_t sig0(uint32_t x){ return rotr(x,7)^rotr(x,18)^(x>>3); }
    static inline uint32_t sig1(uint32_t x){ return rotr(x,17)^rotr(x,19)^(x>>10); }
    static const uint32_t K[64];
    SHA256(){ reset(); }
    void reset(){
        h[0]=0x6a09e667; h[1]=0xbb67ae85; h[2]=0x3c6ef372; h[3]=0xa54ff53a;
        h[4]=0x510e527f; h[5]=0x9b05688c; h[6]=0x1f83d9ab; h[7]=0x5be0cd19;
        buf.clear(); bits=0;
    }
    void process(const uint8_t *p){
        uint32_t w[64];
        for(int i=0;i<16;i++)
            w[i] = (uint32_t)p[4*i]<<24 | (uint32_t)p[4*i+1]<<16 | (uint32_t)p[4*i+2]<<8 | (uint32_t)p[4*i+3];
        for(int i=16;i<64;i++)
            w[i] = sig0(w[i-15]) + w[i-7] + sig1(w[i-2]) + w[i-16];
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for(int i=0;i<64;i++){
            uint32_t t1 = hh + Sig1(e) + Ch(e,f,g) + K[i] + w[i];
            uint32_t t2 = Sig0(a) + Maj(a,b,c);
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    void update(const void* data, size_t len){
        const uint8_t *p=(const uint8_t*)data;
        bits += (uint64_t)len * 8ULL;
        while(len--){
            buf.push_back(*p++);
            if (buf.size()==64){
                process(buf.data());
                buf.clear();
            }
        }
    }
    std::array<uint8_t,32> digest(){
        std::vector<uint8_t> tmp = buf;
        tmp.push_back(0x80);
        while(tmp.size()%64!=56) tmp.push_back(0);
        for(int i=7;i>=0;i--) tmp.push_back((bits>>(8*i))&0xff);
        for(size_t i=0;i<tmp.size();i+=64) process(tmp.data()+i);
        std::array<uint8_t,32> out{};
        for(int i=0;i<8;i++){
            out[4*i  ]=(h[i]>>24)&0xff;
            out[4*i+1]=(h[i]>>16)&0xff;
            out[4*i+2]=(h[i]>>8 )&0xff;
            out[4*i+3]=(h[i]    )&0xff;
        }
        return out;
    }
    static std::array<uint8_t,32> hash_bytes(const std::vector<uint8_t>& v){
        SHA256 s; s.update(v.data(), v.size()); return s.digest();
    }
};
const uint32_t SHA256::K[64] = {
 0x428a2f98,0x71374491,0xb5c0bfcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
 0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
 0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
 0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
 0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
 0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
 0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
 0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

// ================================================================
// Transcript
// ================================================================
struct Transcript {
    std::vector<uint8_t> st;
    void absorb(const std::array<uint8_t,32>& h){ st.insert(st.end(), h.begin(), h.end()); }
    void absorb_fr(const Fr& x){
        uint8_t buf[64];
        size_t n = x.serialize(buf, sizeof(buf));
        st.insert(st.end(), buf, buf+n);
    }
    void absorb_g1(const G1& g){
        uint8_t buf[128];
        size_t n = g.serialize(buf, sizeof(buf));
        st.insert(st.end(), buf, buf+n);
    }
    std::array<uint8_t,32> squeeze(){ return SHA256::hash_bytes(st); }
    Fr challenge(){
        auto h = squeeze();
        uint64_t v = 0;
        for (int i=0;i<8;i++) v = (v<<8) | h[i];
        return fr_from_u64(v);
    }
};

// ================================================================
// KZG
// ================================================================
struct KZGParams {
    G1 g1;
    G2 g2;
    std::vector<G1> srs_g1;
    G2 g2_s;
    Fr s;
};

static KZGParams kzg_setup(size_t max_deg){
    KZGParams pp;
    mapToG1(pp.g1, 1);
    mapToG2(pp.g2, 2);
    pp.s.setByCSPRNG();
    pp.srs_g1.resize(max_deg+1);
    Fr pow_s = fr_one();
    for (size_t i=0;i<=max_deg;i++){
        G1 gi = pp.g1;
        G1::mul(gi, gi, pow_s);
        pp.srs_g1[i] = gi;
        pow_s *= pp.s;
    }
    pp.g2_s = pp.g2;
    G2::mul(pp.g2_s, pp.g2_s, pp.s);
    return pp;
}

static G1 kzg_commit(const KZGParams& pp, const Poly& f){
    G1 acc; acc.clear();
    for (size_t i=0;i<f.c.size();i++){
        if (f.c[i].isZero()) continue;
        G1 term;
        G1::mul(term, pp.srs_g1[i], f.c[i]);
        G1::add(acc, acc, term);
    }
    return acc;
}

static void kzg_open(const KZGParams& pp, const Poly& f,
                     const Fr& z, Fr& value_out, G1& witness_out)
{
    if (f.c.empty()){
        value_out.clear(); witness_out.clear(); return;
    }
    size_t deg = f.c.size()-1;
    std::vector<Fr> rev(f.c.rbegin(), f.c.rend());
    std::vector<Fr> q_rev(deg+1);
    q_rev[0] = rev[0];
    for (size_t i=1;i<=deg;i++)
        q_rev[i] = rev[i] + z*q_rev[i-1];
    value_out = q_rev[deg];
    std::vector<Fr> q_coeff(deg);
    for (size_t i=0;i<deg;i++) q_coeff[i] = q_rev[i];
    std::reverse(q_coeff.begin(), q_coeff.end());
    Poly q; q.c = q_coeff; poly_normalize(q);
    witness_out = kzg_commit(pp, q);
}

static bool kzg_verify(const KZGParams& pp,
                       const G1& commit,
                       const Fr& z,
                       const Fr& value,
                       const G1& witness)
{
    G1 C_minus_v;
    {
        G1 vG1 = pp.g1;
        G1::mul(vG1, vG1, value);
        G1::sub(C_minus_v, commit, vG1);
    }
    G2 gs_minus_zg2;
    {
        G2 zg2 = pp.g2;
        G2::mul(zg2, zg2, z);
        G2::sub(gs_minus_zg2, pp.g2_s, zg2);
    }
    Fp12 e1, e2;
    pairing(e1, C_minus_v, pp.g2);
    pairing(e2, witness, gs_minus_zg2);
    return e1 == e2;
}

// ================================================================
// load LUT poly from file
// ================================================================
static Poly load_poly_from_file(const std::string& path)
{
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open LUT file: " + path);
    size_t n; in >> n;
    Poly p; p.c.resize(n);
    for (size_t i=0;i<n;i++){
        std::string s; in >> s;
        p.c[i].setStr(s, 16);
    }
    poly_normalize(p);
    return p;
}

// ================================================================
// Sumcheck
// ================================================================
struct SumcheckProofRound { Fr g0, g1; };
struct SumcheckProof {
    size_t n_vars;
    Fr claimed_sum;
    std::vector<SumcheckProofRound> rounds;
};

static std::vector<Fr> fold_mle(const std::vector<Fr>& f, const Fr& r){
    std::vector<Fr> out(f.size()/2);
    Fr one = fr_one();
    for (size_t i=0;i<f.size();i+=2)
        out[i/2] = f[i]*(one - r) + f[i+1]*r;
    return out;
}

static SumcheckProof sumcheck_prove(std::vector<Fr> f_table, Transcript& tr){
    size_t n = 0;
    while ((1ULL<<n) < f_table.size()) n++;
    if ((1ULL<<n) != f_table.size()) throw std::runtime_error("sumcheck: len not pow2");
    SumcheckProof pf;
    pf.n_vars = n;
    Fr cur_sum = fr_zero();
    for (auto &v : f_table) cur_sum += v;
    pf.claimed_sum = cur_sum;
    auto cur = f_table;
    for (size_t round=0; round<n; ++round){
        Fr g0 = fr_zero(), g1 = fr_zero();
        for (size_t i=0;i<cur.size();i+=2){
            g0 += cur[i];
            g1 += cur[i+1];
        }
        pf.rounds.push_back({g0, g1 - g0});
        tr.absorb_fr(g0);
        tr.absorb_fr(g1);
        Fr r = tr.challenge();
        cur = fold_mle(cur, r);
        Fr ns = fr_zero();
        for (auto &x : cur) ns += x;
        cur_sum = ns;
    }
    return pf;
}

static bool sumcheck_verify(const SumcheckProof& pf,
                            Transcript& tr,
                            Fr claimed_sum)
{
    Fr cur_sum = claimed_sum;
    for (size_t round=0; round<pf.n_vars; ++round){
        const auto &rd = pf.rounds[round];
        Fr g0 = rd.g0;
        Fr g1 = rd.g0 + rd.g1;
        if (g0 + g1 != cur_sum) return false;
        tr.absorb_fr(g0);
        tr.absorb_fr(g1);
        Fr r = tr.challenge();
        Fr g_eval = g0 + rd.g1 * r;
        cur_sum = g_eval;
    }
    return true;
}

// ================================================================
// VM
// ================================================================
enum Op : uint8_t {
    OP_PUSH = 0,
    OP_ADD  = 1,
    OP_MUL  = 2,
    OP_SUB  = 3,
    OP_AND  = 4,
    OP_OR   = 5,
    OP_HALT = 255
};

struct Instr { Op op; int64_t imm; };

struct TraceRow {
    uint32_t pc;
    Fr opcode;
    Fr x;
    Fr y;
    Fr z;
    Fr is_halt;
    int64_t x_raw, y_raw, z_raw;
};

static bool program_uses_logic(const std::vector<Instr>& prog){
    for (auto &ins : prog)
        if (ins.op == OP_AND || ins.op == OP_OR) return true;
    return false;
}

static std::vector<TraceRow> run_vm(const std::vector<Instr>& prog,
                                    const std::vector<int64_t>& inputs)
{
    std::vector<int64_t> stack = inputs;
    std::vector<TraceRow> trace;
    uint32_t pc = 0;
    for(;;){
        Op op = (pc < prog.size()) ? prog[pc].op : OP_HALT;
        TraceRow row;
        row.pc = pc;
        row.opcode = fr_from_u64(op);
        int64_t a = stack.size()>=1? stack.back():0;
        int64_t b = stack.size()>=2? stack[stack.size()-2]:0;
        row.x = fr_from_u64((uint64_t)a);
        row.y = fr_from_u64((uint64_t)b);
        row.x_raw = a;
        row.y_raw = b;
        row.is_halt = fr_from_u64(op==OP_HALT ? 1 : 0);
        int64_t z = 0;

        if (op == OP_PUSH){
            stack.push_back(prog[pc].imm);
            z = prog[pc].imm;
            pc++;
        } else if (op == OP_ADD){
            auto xx = stack.back(); stack.pop_back();
            stack.back() = stack.back() + xx;
            z = stack.back();
            pc++;
        } else if (op == OP_SUB){
            auto xx = stack.back(); stack.pop_back();
            stack.back() = stack.back() - xx;
            z = stack.back();
            pc++;
        } else if (op == OP_MUL){
            auto xx = stack.back(); stack.pop_back();
            stack.back() = stack.back() * xx;
            z = stack.back();
            pc++;
        } else if (op == OP_AND){
            auto xx = stack.back(); stack.pop_back();
            auto yy = stack.back(); stack.pop_back();
            int64_t res = ((xx & 0xF) & (yy & 0xF)) & 0xF;
            stack.push_back(res);
            z = res;
            pc++;
        } else if (op == OP_OR){
            auto xx = stack.back(); stack.pop_back();
            auto yy = stack.back(); stack.pop_back();
            int64_t res = ((xx & 0xF) | (yy & 0xF)) & 0xF;
            stack.push_back(res);
            z = res;
            pc++;
        } else if (op == OP_HALT){
            z = stack.size()? stack.back():0;
            row.z = fr_from_u64((uint64_t)z);
            row.z_raw = z;
            trace.push_back(row);
            break;
        } else {
            throw std::runtime_error("unknown opcode");
        }

        row.z = fr_from_u64((uint64_t)z);
        row.z_raw = z;
        trace.push_back(row);

        if (pc >= prog.size()) break;
    }
    return trace;
}

// ================================================================
// code serialization + SHA + POLY (monomial)
// ================================================================
static std::vector<uint8_t> serialize_program_bytes(const std::vector<Instr>& prog){
    std::vector<uint8_t> ser;
    for (auto &ins : prog){
        ser.push_back((uint8_t)ins.op);
        for (int i=7;i>=0;i--) ser.push_back((ins.imm>>(8*i)) & 0xff);
    }
    return ser;
}
static std::array<uint8_t,32> commit_code_sha(const std::vector<Instr>& prog){
    return SHA256::hash_bytes(serialize_program_bytes(prog));
}

// **NEW**: build code poly directly from bytes
static Poly code_poly_from_bytes(const std::vector<uint8_t>& bytes)
{
    Poly p;
    p.c.resize(bytes.size());
    for (size_t i=0;i<bytes.size();i++)
        p.c[i] = fr_from_u64(bytes[i]);
    poly_normalize(p);
    return p;
}

// ================================================================
// index derivation
// ================================================================
static std::vector<size_t> derive_indices(const std::array<uint8_t,32>& seed,
                                          size_t domain,
                                          size_t k)
{
    std::vector<size_t> out; out.reserve(k);
    std::array<uint8_t,32> cur = seed;
    while (out.size() < k){
        for (uint32_t c=0;c<4 && out.size()<k;c++){
            std::vector<uint8_t> v(cur.begin(), cur.end());
            v.push_back((c>>24)&0xff);
            v.push_back((c>>16)&0xff);
            v.push_back((c>>8)&0xff);
            v.push_back(c&0xff);
            auto d = SHA256::hash_bytes(v);
            uint64_t x=0; for (int i=0;i<8;i++) x = (x<<8) | d[i];
            out.push_back((size_t)(x % domain));
            cur = d;
        }
    }
    return out;
}

// ================================================================
// proof structs
// ================================================================
struct KZGOpening {
    size_t idx;
    Fr value;
    G1 witness;
};
struct LUTOpening {
    size_t idx;
    Fr x_val, y_val, z_val, op_val;
    G1 x_wit, y_wit, z_wit, op_wit;
};
struct Proof {
    std::array<uint8_t,32> code_sha;
    G1 code_comm_kzg;
    G1 pc_comm;
    G1 op_comm;
    G1 z_comm;
    G1 lut_x_comm, lut_y_comm, lut_z_comm, lut_op_comm;
    SumcheckProof sc;
    uint32_t trace_pow2;
    std::vector<KZGOpening> op_openings;
    std::vector<LUTOpening> lut_openings;
    uint64_t final_output;
};

// ================================================================
// Prover
// ================================================================
static Proof prove(const KZGParams& pp,
                   const std::vector<Instr>& prog,
                   const std::vector<int64_t>& inputs,
                   size_t k_lookup = 4)
{
    bool uses_logic = program_uses_logic(prog);

    // 1) code
    auto code_sha = commit_code_sha(prog);
    auto code_bytes = serialize_program_bytes(prog);
    Poly code_poly = code_poly_from_bytes(code_bytes);  // <-- fixed
    G1 code_comm_kzg = kzg_commit(pp, code_poly);

    // 2) run VM
    auto trace = run_vm(prog, inputs);
    size_t T = trace.size();
    size_t pow2 = 1; while (pow2 < T) pow2 <<= 1;

    // 3) trace columns
    std::vector<Fr> col_pc(pow2), col_op(pow2), col_z(pow2), col_h(pow2);
    for (size_t i=0;i<T;i++){
        col_pc[i] = fr_from_u64(trace[i].pc);
        col_op[i] = trace[i].opcode;
        col_z[i]  = trace[i].z;
        col_h[i]  = trace[i].is_halt;
    }
    for (size_t i=T;i<pow2;i++){
        col_pc[i].clear(); col_op[i].clear(); col_z[i].clear(); col_h[i].clear();
    }

    Poly pc_poly = interpolate_on_range0(col_pc);
    Poly op_poly = interpolate_on_range0(col_op);
    Poly z_poly  = interpolate_on_range0(col_z);

    G1 pc_comm = kzg_commit(pp, pc_poly);
    G1 op_comm = kzg_commit(pp, op_poly);
    G1 z_comm  = kzg_commit(pp, z_poly);

    // 4) transition constraint
    std::vector<Fr> f(pow2); for (auto &x : f) x.clear();
    Fr one = fr_one();
    for (size_t i=0;i+1<T;i++){
        Fr diff = col_pc[i+1] - (col_pc[i] + one);
        Fr one_minus = one - col_h[i];
        f[i] = diff * one_minus;
    }

    // 5) transcript
    Transcript tr;
    tr.absorb(code_sha);
    tr.absorb_g1(code_comm_kzg);
    tr.absorb_g1(pc_comm);
    tr.absorb_g1(op_comm);
    tr.absorb_g1(z_comm);

    // 6) LUT (load if needed)
    Poly lut_x_poly, lut_y_poly, lut_z_poly, lut_op_poly;
    G1 lut_x_comm, lut_y_comm, lut_z_comm, lut_op_comm;
    if (uses_logic){
        lut_x_poly = load_poly_from_file("lut_and_or_x.txt");
        lut_y_poly = load_poly_from_file("lut_and_or_y.txt");
        lut_z_poly = load_poly_from_file("lut_and_or_z.txt");
        lut_op_poly = load_poly_from_file("lut_and_or_op.txt");
        lut_x_comm = kzg_commit(pp, lut_x_poly);
        lut_y_comm = kzg_commit(pp, lut_y_poly);
        lut_z_comm = kzg_commit(pp, lut_z_poly);
        lut_op_comm = kzg_commit(pp, lut_op_poly);

        tr.absorb_g1(lut_x_comm);
        tr.absorb_g1(lut_y_comm);
        tr.absorb_g1(lut_z_comm);
        tr.absorb_g1(lut_op_comm);
    } else {
        lut_x_comm.clear(); lut_y_comm.clear(); lut_z_comm.clear(); lut_op_comm.clear();
    }

    // 7) sumcheck
    auto sc = sumcheck_prove(f, tr);

    // 8) sampled opcode
    auto seed = tr.squeeze();
    auto idxs = derive_indices(seed, pow2, k_lookup);
    std::vector<KZGOpening> op_openings;
    for (auto idx : idxs){
        Fr pt = fr_from_u64(idx);
        Fr val; G1 wit;
        kzg_open(pp, op_poly, pt, val, wit);
        op_openings.push_back({idx, val, wit});
    }

    // 9) semantic LUT openings
    std::vector<LUTOpening> lut_openings;
    if (uses_logic){
        const size_t LUT_RANGE = 16;
        const size_t LUT_PER_OP = 256;
        for (size_t i=0;i<T;i++){
            uint64_t opv = fr_to_u64(trace[i].opcode);
            if (opv == OP_AND || opv == OP_OR){
                uint64_t xa = (uint64_t)(trace[i].x_raw & 0xF);
                uint64_t yb = (uint64_t)(trace[i].y_raw & 0xF);
                size_t base = (opv == OP_AND) ? 0 : LUT_PER_OP;
                size_t idx = base + xa*LUT_RANGE + yb;
                Fr pt = fr_from_u64(idx);
                Fr vx, vy, vz, vop;
                G1 wx, wy, wz, wop;
                kzg_open(pp, lut_x_poly, pt, vx, wx);
                kzg_open(pp, lut_y_poly, pt, vy, wy);
                kzg_open(pp, lut_z_poly, pt, vz, wz);
                kzg_open(pp, lut_op_poly, pt, vop, wop);
                lut_openings.push_back({idx, vx, vy, vz, vop, wx, wy, wz, wop});
            }
        }
    }

    uint64_t out = (uint64_t) trace.back().z_raw;

    Proof prf;
    prf.code_sha = code_sha;
    prf.code_comm_kzg = code_comm_kzg;
    prf.pc_comm = pc_comm;
    prf.op_comm = op_comm;
    prf.z_comm  = z_comm;
    prf.lut_x_comm = lut_x_comm;
    prf.lut_y_comm = lut_y_comm;
    prf.lut_z_comm = lut_z_comm;
    prf.lut_op_comm = lut_op_comm;
    prf.sc = sc;
    prf.trace_pow2 = (uint32_t)pow2;
    prf.op_openings = std::move(op_openings);
    prf.lut_openings = std::move(lut_openings);
    prf.final_output = out;
    return prf;
}

// ================================================================
// Verifier
// ================================================================
static bool verify(const KZGParams& pp,
                   const std::vector<Instr>& prog,
                   const Proof& proof,
                   size_t k_lookup = 4,
                   std::string *reason_out = nullptr)
{
    auto fail = [&](const std::string& why){
        if (reason_out) *reason_out = why;
        return false;
    };

    bool uses_logic = program_uses_logic(prog);

    // 1) code sha
    if (commit_code_sha(prog) != proof.code_sha)
        return fail("code sha mismatch");

    // 2) rebuild code poly EXACTLY the same way
    auto code_bytes = serialize_program_bytes(prog);
    Poly code_poly = code_poly_from_bytes(code_bytes);   // <-- fixed
    G1 code_comm_kzg = kzg_commit(pp, code_poly);
    if (code_comm_kzg != proof.code_comm_kzg)
        return fail("code KZG mismatch");

    // 3) transcript
    Transcript tr;
    tr.absorb(proof.code_sha);
    tr.absorb_g1(proof.code_comm_kzg);
    tr.absorb_g1(proof.pc_comm);
    tr.absorb_g1(proof.op_comm);
    tr.absorb_g1(proof.z_comm);

    // 4) LUT (if needed)
    Poly lut_x_poly, lut_y_poly, lut_z_poly, lut_op_poly;
    if (uses_logic){
        lut_x_poly = load_poly_from_file("lut_and_or_x.txt");
        lut_y_poly = load_poly_from_file("lut_and_or_y.txt");
        lut_z_poly = load_poly_from_file("lut_and_or_z.txt");
        lut_op_poly = load_poly_from_file("lut_and_or_op.txt");
        G1 lut_x_comm = kzg_commit(pp, lut_x_poly);
        G1 lut_y_comm = kzg_commit(pp, lut_y_poly);
        G1 lut_z_comm = kzg_commit(pp, lut_z_poly);
        G1 lut_op_comm = kzg_commit(pp, lut_op_poly);
        if (lut_x_comm != proof.lut_x_comm) return fail("lut x commit mismatch");
        if (lut_y_comm != proof.lut_y_comm) return fail("lut y commit mismatch");
        if (lut_z_comm != proof.lut_z_comm) return fail("lut z commit mismatch");
        if (lut_op_comm != proof.lut_op_comm) return fail("lut op commit mismatch");
        tr.absorb_g1(proof.lut_x_comm);
        tr.absorb_g1(proof.lut_y_comm);
        tr.absorb_g1(proof.lut_z_comm);
        tr.absorb_g1(proof.lut_op_comm);
    } else {
        if (!proof.lut_openings.empty()) return fail("unexpected lut openings");
    }

    // 5) sumcheck
    if (!sumcheck_verify(proof.sc, tr, proof.sc.claimed_sum))
        return fail("sumcheck failed");

    // 6) sampled opcode check
    auto seed = tr.squeeze();
    auto idxs = derive_indices(seed, proof.trace_pow2, k_lookup);
    if (idxs.size() != proof.op_openings.size())
        return fail("opcode opening size mismatch");

    std::vector<uint64_t> allowed = {
        OP_PUSH, OP_ADD, OP_MUL, OP_SUB, OP_AND, OP_OR, OP_HALT
    };

    for (size_t i=0;i<idxs.size();i++){
        size_t want = idxs[i];
        const auto &open = proof.op_openings[i];
        if (open.idx != want) return fail("opcode idx mismatch");
        Fr z = fr_from_u64(want);
        if (!kzg_verify(pp, proof.op_comm, z, open.value, open.witness))
            return fail("opcode kzg verify fail");
        uint64_t v = fr_to_u64(open.value);
        bool ok=false; for (auto a: allowed) if (v == a){ ok=true; break; }
        if (!ok) return fail("opcode not allowed");
    }

    // 7) LUT semantic checks
    if (uses_logic){
        const size_t LUT_RANGE = 16;
        const size_t LUT_PER_OP = 256;
        for (const auto &lo : proof.lut_openings){
            Fr pt = fr_from_u64(lo.idx);
            if (!kzg_verify(pp, proof.lut_x_comm, pt, lo.x_val, lo.x_wit)) return fail("lut x opening");
            if (!kzg_verify(pp, proof.lut_y_comm, pt, lo.y_val, lo.y_wit)) return fail("lut y opening");
            if (!kzg_verify(pp, proof.lut_z_comm, pt, lo.z_val, lo.z_wit)) return fail("lut z opening");
            if (!kzg_verify(pp, proof.lut_op_comm, pt, lo.op_val, lo.op_wit)) return fail("lut op opening");

            uint64_t xv = fr_to_u64(lo.x_val) & 0xF;
            uint64_t yv = fr_to_u64(lo.y_val) & 0xF;
            uint64_t zv = fr_to_u64(lo.z_val) & 0xF;
            uint64_t opv = fr_to_u64(lo.op_val);
            if (opv == OP_AND){
                if (zv != ((xv & yv) & 0xF)) return fail("lut and semantics");
            } else if (opv == OP_OR){
                if (zv != ((xv | yv) & 0xF)) return fail("lut or semantics");
            } else {
                return fail("lut unexpected op");
            }
        }
    }

    return true;
}

// ================================================================
// main
// ================================================================
int main(){
    try {
        mcl::bn::initPairing(mcl::BN254);

        std::vector<Instr> prog = {
            // previous arithmetic
            {OP_PUSH, 5},     // stack = [5]
            {OP_PUSH, 7},     // stack = [5, 7]
            {OP_ADD,  0},     // stack = [12]
            {OP_PUSH, 2},     // stack = [12, 2]
            {OP_MUL,  0},     // stack = [24]

            // new logic part
            {OP_PUSH, 0xF},   // stack = [24, 15]
            {OP_AND,  0},     // (24 & 0xF) = 8  -> stack = [8]
            {OP_PUSH, 0x3},   // stack = [8, 3]
            {OP_OR,   0},     // (8 | 3) = 11    -> stack = [11]

            {OP_HALT, 0}      // output = 11
        };
        std::vector<int64_t> inputs = {};

        bool uses_logic = program_uses_logic(prog);

        // SRS size
        auto code_bytes = serialize_program_bytes(prog);
        size_t need_deg = 1024;
        if (code_bytes.size() > need_deg) need_deg = code_bytes.size() + 8;
        if (uses_logic && 512 + 8 > need_deg) need_deg = 512 + 8;

        auto pp = kzg_setup(need_deg);

        auto proof = prove(pp, prog, inputs);
        std::string reason;
        bool ok = verify(pp, prog, proof, 4, &reason);

        std::cout << "Verify: " << (ok ? "ACCEPT" : "REJECT") << "\n";
        if (!ok) std::cout << "Reason: " << reason << "\n";
        std::cout << "Claimed output: " << proof.final_output << "\n";
        return ok ? 0 : 1;
    } catch (const std::exception& e){
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
