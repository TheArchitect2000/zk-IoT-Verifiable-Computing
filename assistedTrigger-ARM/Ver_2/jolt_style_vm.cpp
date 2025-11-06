/********************************************************************************************
 * jolt_style_vm.cpp
 * ------------------------------------------------------------------------------------------
 * Zero-Knowledge VM (BN254 KZG) — 3-stage pipeline with anti-replay, session-salted code
 * commitment, and random-row spot checks (Merkle-like using KZG).
 *
 * Stages
 *   1) Publisher : commit_code(pp, prog) -> CodeCommit { code_sha, code_comm_kzg_base }
 *   2) Prover    : prove(pp, CodeCommit, PublicInstance, prog, inputs)
 *                  - Creates session-salted code commitment:
 *                        C_sess = C_base + Commit(b(domain_tag))
 *                    where b is a public blinding polynomial derived from domain_tag.
 *   3) Verifier  : verify(pp, CodeCommit, PublicInstance, proof)  (no program access)
 *                  - Recomputes Commit(b(domain_tag)) and checks proof.code_comm_kzg == C_sess.
 *                  - Rebuilds transcript and verifies sumcheck + KZG openings + random row checks.
 *
 * Anti-replay (copy/paste proof protection)
 *   • PublicInstance.domain_tag : 32-byte verifier nonce/context
 *   • PublicInstance.input_sha  : SHA-256 hash of public inputs
 *   • Both are absorbed into the transcript BEFORE challenges, changing them per run.
 *
 * Random-row checks
 *   • Verifier samples random indices i (via transcript) and checks KZG openings for:
 *        pc[i], pc[i+1], op[i], x[i], y[i], z[i], h[i]
 *   • Local transition: pc[i+1] = pc[i] + 1 unless h[i] = 1
 *   • Local semantics for op in {ADD,SUB,MUL,AND,OR}
 *
 * LUT files (only needed by prover if AND/OR is used)
 *   lut_and_or_x.txt
 *   lut_and_or_y.txt
 *   lut_and_or_z.txt
 *   lut_and_or_op.txt
 *
 * Build
 *   g++ -std=gnu++17 -O2 jolt_style_vm.cpp -lmcl -lcrypto -o jolt_demo
 *
 * License: MIT
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
#include <iomanip>

#include <mcl/bn.hpp>
using namespace mcl::bn;

/*----------------------------- Small utils ----------------------------------*/
static std::string toHex(const uint8_t* p, size_t n){
    std::ostringstream oss; oss << std::hex << std::setfill('0');
    for (size_t i=0;i<n;i++) oss << std::setw(2) << (unsigned)p[i];
    return oss.str();
}
static std::string g1_to_hex(const G1& g){
    uint8_t buf[128]; size_t n = g.serialize(buf, sizeof(buf));
    return toHex(buf, n);
}
static std::string arr32_to_hex(const std::array<uint8_t,32>& a){
    return toHex(a.data(), a.size());
}

/*----------------------------- Field helpers --------------------------------*/
static Fr fr_from_u64(uint64_t v)
{
    Fr r;
    uint8_t buf[8];
    for (int i = 0; i < 8; i++) buf[7-i] = (uint8_t)((v >> (i*8)) & 0xff);
    r.setBigEndianMod(buf, 8);
    return r;
}
static Fr fr_zero(){ Fr z; z.clear(); return z; }
static Fr fr_one(){ return fr_from_u64(1); }
static uint64_t fr_to_u64(const Fr& x)
{
    uint8_t buf[64];
    size_t n = x.serialize(buf, sizeof(buf));
    uint64_t v=0; size_t start = (n>8)?(n-8):0;
    for (size_t i=start;i<n;i++) v = (v<<8) | buf[i];
    return v;
}

/*----------------------------- SHA-256 --------------------------------------*/
struct SHA256 {
    uint32_t h[8]; std::vector<uint8_t> buf; uint64_t bits=0;
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
        const uint8_t *p=(const uint8_t*)data; bits += (uint64_t)len*8ULL;
        while(len--){ buf.push_back(*p++); if (buf.size()==64){ process(buf.data()); buf.clear(); } }
    }
    std::array<uint8_t,32> digest(){
        std::vector<uint8_t> tmp = buf; tmp.push_back(0x80);
        while(tmp.size()%64!=56) tmp.push_back(0);
        for(int i=7;i>=0;i--) tmp.push_back((bits>>(8*i))&0xff);
        for(size_t i=0;i<tmp.size();i+=64) process(tmp.data()+i);
        std::array<uint8_t,32> out{};
        for(int i=0;i<8;i++){
            out[4*i]=(h[i]>>24)&0xff; out[4*i+1]=(h[i]>>16)&0xff;
            out[4*i+2]=(h[i]>>8)&0xff; out[4*i+3]=(h[i])&0xff;
        }
        return out;
    }
    static std::array<uint8_t,32> hash_bytes(const std::vector<uint8_t>& v){ SHA256 s; s.update(v.data(), v.size()); return s.digest(); }
};
const uint32_t SHA256::K[64] = {
 0x428a2f98,0x71374491,0xb5c0bfcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
 0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
 0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5b9cca4f,0x682e6ff3,
 0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

/*----------------------------- Transcript -----------------------------------*/
struct Transcript {
    std::vector<uint8_t> st;
    void absorb(const std::array<uint8_t,32>& a){ st.insert(st.end(), a.begin(), a.end()); }
    void absorb_fr(const Fr& x){
        uint8_t buf[64]; size_t n = x.serialize(buf, sizeof(buf));
        st.insert(st.end(), buf, buf+n);
    }
    void absorb_g1(const G1& g){
        uint8_t buf[128]; size_t n = g.serialize(buf, sizeof(buf));
        st.insert(st.end(), buf, buf+n);
    }
    std::array<uint8_t,32> squeeze(){ return SHA256::hash_bytes(st); }
    Fr challenge(){ auto h = squeeze(); uint64_t v=0; for (int i=0;i<8;i++) v = (v<<8)|h[i]; return fr_from_u64(v); }
};

/*----------------------------- KZG ------------------------------------------*/
struct Poly { std::vector<Fr> c; };
static void poly_normalize(Poly& p){ while(!p.c.empty() && p.c.back().isZero()) p.c.pop_back(); }

struct KZGParams { G1 g1; G2 g2; std::vector<G1> srs_g1; G2 g2_s; Fr s; };

static KZGParams kzg_setup(size_t max_deg){
    KZGParams pp; mapToG1(pp.g1, 1); mapToG2(pp.g2, 2); pp.s.setByCSPRNG();
    pp.srs_g1.resize(max_deg+1);
    Fr pow_s = fr_one();
    for (size_t i=0;i<=max_deg;i++){ G1 gi = pp.g1; G1::mul(gi, gi, pow_s); pp.srs_g1[i]=gi; pow_s *= pp.s; }
    pp.g2_s = pp.g2; G2::mul(pp.g2_s, pp.g2_s, pp.s);
    return pp;
}
static G1 kzg_commit(const KZGParams& pp, const Poly& f){
    G1 acc; acc.clear();
    for (size_t i=0;i<f.c.size();i++){
        if (f.c[i].isZero()) continue;
        G1 term; G1::mul(term, pp.srs_g1[i], f.c[i]); G1::add(acc, acc, term);
    }
    return acc;
}
static void kzg_open(const KZGParams& pp, const Poly& f, const Fr& z, Fr& value_out, G1& witness_out){
    if (f.c.empty()){ value_out.clear(); witness_out.clear(); return; }
    size_t deg = f.c.size()-1;
    std::vector<Fr> rev(f.c.rbegin(), f.c.rend()), q_rev(deg+1);
    q_rev[0]=rev[0]; for(size_t i=1;i<=deg;i++) q_rev[i] = rev[i] + z*q_rev[i-1];
    value_out = q_rev[deg];
    std::vector<Fr> q_coeff(deg);
    for (size_t i=0;i<deg;i++) q_coeff[i]=q_rev[i];
    std::reverse(q_coeff.begin(), q_coeff.end());
    Poly q; q.c = q_coeff; poly_normalize(q);
    witness_out = kzg_commit(pp, q);
}
static bool kzg_verify(const KZGParams& pp, const G1& commit, const Fr& z, const Fr& value, const G1& witness){
    G1 C_minus_v; { G1 vG1 = pp.g1; G1::mul(vG1, vG1, value); G1::sub(C_minus_v, commit, vG1); }
    G2 gs_minus_zg2; { G2 zg2 = pp.g2; G2::mul(zg2, zg2, z); G2::sub(gs_minus_zg2, pp.g2_s, zg2); }
    Fp12 e1,e2; pairing(e1, C_minus_v, pp.g2); pairing(e2, witness, gs_minus_zg2);
    return e1 == e2;
}

/*----------------------------- Interpolation (trace) ------------------------*/
// Naive Lagrange on x=0..n-1 (for small traces). For big traces, replace with FFT/NTT.
static Poly interpolate_on_range0(const std::vector<Fr>& vals){
    size_t n = vals.size();
    Poly acc; acc.c = { fr_zero() }; Fr one = fr_one();
    for (size_t i=0;i<n;i++){
        Poly numer; numer.c = { fr_one() };
        Fr denom = fr_one(), xi = fr_from_u64(i);
        for (size_t j=0;j<n;j++){
            if (j==i) continue;
            Fr fj = fr_from_u64(j);
            Poly term; Fr minus_j = fr_zero() - fj; term.c = { minus_j, fr_one() };
            Poly tmp; tmp.c.assign(numer.c.size()+1, fr_zero());
            for (size_t a=0;a<numer.c.size();a++){ tmp.c[a] += numer.c[a]*term.c[0]; tmp.c[a+1] += numer.c[a]*term.c[1]; }
            numer = tmp; denom *= (xi - fj);
        }
        Fr denom_inv = one / denom;
        for (auto &cc : numer.c) cc *= denom_inv;
        if (numer.c.size() > acc.c.size()) acc.c.resize(numer.c.size());
        for (size_t k=0;k<numer.c.size();k++) acc.c[k] += numer.c[k]*vals[i];
    }
    poly_normalize(acc);
    return acc;
}

/*----------------------------- Sumcheck -------------------------------------*/
struct SumcheckProofRound { Fr g0, g1; };
struct SumcheckProof { size_t n_vars; Fr claimed_sum; std::vector<SumcheckProofRound> rounds; };
static std::vector<Fr> fold_mle(const std::vector<Fr>& f, const Fr& r){
    std::vector<Fr> out(f.size()/2); Fr one = fr_one();
    for (size_t i=0;i<f.size();i+=2) out[i/2] = f[i]*(one - r) + f[i+1]*r;
    return out;
}
static SumcheckProof sumcheck_prove(std::vector<Fr> f_table, Transcript& tr){
    size_t n=0; while ((1ULL<<n) < f_table.size()) n++; if ((1ULL<<n) != f_table.size()) throw std::runtime_error("sumcheck: len not pow2");
    SumcheckProof pf; pf.n_vars=n;
    Fr cur_sum = fr_zero(); for (auto &v : f_table) cur_sum += v; pf.claimed_sum = cur_sum;
    auto cur = f_table;
    for (size_t round=0; round<n; ++round){
        Fr g0=fr_zero(), g1=fr_zero();
        for (size_t i=0;i<cur.size();i+=2){ g0 += cur[i]; g1 += cur[i+1]; }
        pf.rounds.push_back({g0, g1 - g0});
        tr.absorb_fr(g0); tr.absorb_fr(g1);
        Fr r = tr.challenge();
        cur = fold_mle(cur, r);
        Fr ns = fr_zero(); for (auto &x : cur) ns += x; cur_sum = ns;
    }
    return pf;
}
static bool sumcheck_verify(const SumcheckProof& pf, Transcript& tr, Fr claimed_sum){
    Fr cur_sum = claimed_sum;
    for (size_t round=0; round<pf.n_vars; ++round){
        const auto &rd = pf.rounds[round];
        Fr g0 = rd.g0, g1 = rd.g0 + rd.g1;
        if (g0 + g1 != cur_sum) return false;
        tr.absorb_fr(g0); tr.absorb_fr(g1);
        Fr r = tr.challenge();
        Fr g_eval = g0 + rd.g1 * r;
        cur_sum = g_eval;
    }
    return true;
}

/*----------------------------- VM -------------------------------------------*/
enum Op : uint8_t { OP_PUSH=0, OP_ADD=1, OP_MUL=2, OP_SUB=3, OP_AND=4, OP_OR=5, OP_HALT=255 };
struct Instr { Op op; int64_t imm; };
struct TraceRow {
    uint32_t pc; Fr opcode,x,y,z,is_halt; int64_t x_raw,y_raw,z_raw;
};
static bool program_uses_logic(const std::vector<Instr>& prog){
    for (auto &ins : prog) if (ins.op==OP_AND || ins.op==OP_OR) return true; return false;
}
static std::vector<TraceRow> run_vm(const std::vector<Instr>& prog, const std::vector<int64_t>& inputs){
    std::vector<int64_t> stack = inputs; std::vector<TraceRow> trace; uint32_t pc=0;
    for(;;){
        Op op = (pc < prog.size()) ? prog[pc].op : OP_HALT;
        TraceRow row; row.pc=pc; row.opcode=fr_from_u64(op);
        int64_t a = stack.size()>=1? stack.back():0, b = stack.size()>=2? stack[stack.size()-2]:0;
        row.x=fr_from_u64((uint64_t)a); row.y=fr_from_u64((uint64_t)b);
        row.x_raw=a; row.y_raw=b; row.is_halt=fr_from_u64(op==OP_HALT?1:0); int64_t z=0;
        if (op==OP_PUSH){ stack.push_back(prog[pc].imm); z=prog[pc].imm; pc++; }
        else if (op==OP_ADD){ auto xx=stack.back(); stack.pop_back(); stack.back()+=xx; z=stack.back(); pc++; }
        else if (op==OP_SUB){ auto xx=stack.back(); stack.pop_back(); stack.back()-=xx; z=stack.back(); pc++; }
        else if (op==OP_MUL){ auto xx=stack.back(); stack.pop_back(); stack.back()*=xx; z=stack.back(); pc++; }
        else if (op==OP_AND){ auto xx=stack.back(); stack.pop_back(); auto yy=stack.back(); stack.pop_back();
                              int64_t res = ((xx & 0xF) & (yy & 0xF)) & 0xF; stack.push_back(res); z=res; pc++; }
        else if (op==OP_OR){  auto xx=stack.back(); stack.pop_back(); auto yy=stack.back(); stack.pop_back();
                              int64_t res = ((xx & 0xF) | (yy & 0xF)) & 0xF; stack.push_back(res); z=res; pc++; }
        else if (op==OP_HALT){ z = stack.size()? stack.back():0; row.z=fr_from_u64((uint64_t)z); row.z_raw=z; trace.push_back(row); break; }
        else throw std::runtime_error("unknown opcode");
        row.z=fr_from_u64((uint64_t)z); row.z_raw=z; trace.push_back(row); if (pc>=prog.size()) break;
    }
    return trace;
}

/*----------------------------- Code bytes / SHA ------------------------------*/
static std::vector<uint8_t> serialize_program_bytes(const std::vector<Instr>& prog){
    std::vector<uint8_t> ser;
    for (auto &ins : prog){ ser.push_back((uint8_t)ins.op); for (int i=7;i>=0;i--) ser.push_back((ins.imm>>(8*i)) & 0xff); }
    return ser;
}
static std::array<uint8_t,32> commit_code_sha(const std::vector<Instr>& prog){
    return SHA256::hash_bytes(serialize_program_bytes(prog));
}
static Poly code_poly_from_bytes_monomial(const std::vector<uint8_t>& bytes){
    Poly p; p.c.resize(bytes.size()); for (size_t i=0;i<bytes.size();i++) p.c[i]=fr_from_u64(bytes[i]); poly_normalize(p); return p;
}

/*----------------------------- Public objects (stage 1/3) -------------------*/
struct CodeCommit {
    std::array<uint8_t,32> code_sha;
    G1 code_comm_kzg_base;   // Published base commitment (fixed per program/version)
};
struct PublicInstance {
    std::array<uint8_t,32> domain_tag;    // verifier-chosen 32 bytes (fresh per verification)
    std::array<uint8_t,32> input_sha;     // SHA-256 of public inputs (or any public data)
    bool bind_expected_output = false;    // optional enforcement
    uint64_t expected_output   = 0;
};

static std::array<uint8_t,32> hash_inputs_i64(const std::vector<int64_t>& in){
    std::vector<uint8_t> bytes; bytes.reserve(in.size()*8);
    for (int64_t v : in) for (int i=7;i>=0;i--) bytes.push_back(uint8_t((uint64_t)v>>(i*8)));
    return SHA256::hash_bytes(bytes);
}

/*----------------------------- LUT loader (prover only) ---------------------*/
static Poly load_poly_from_file(const std::string& path){
    std::ifstream in(path); if (!in) throw std::runtime_error("cannot open LUT file: " + path);
    size_t n; in >> n; Poly p; p.c.resize(n);
    for (size_t i=0;i<n;i++){ std::string s; in >> s; p.c[i].setStr(s, 16); }
    poly_normalize(p); return p;
}

/*----------------------------- Index derivation -----------------------------*/
static std::vector<size_t> derive_indices(const std::array<uint8_t,32>& seed, size_t domain, size_t k){
    std::vector<size_t> out; out.reserve(k);
    std::array<uint8_t,32> cur = seed;
    while (out.size() < k){
        for (uint32_t c=0;c<4 && out.size()<k;c++){
            std::vector<uint8_t> v(cur.begin(), cur.end());
            v.push_back((c>>24)&0xff); v.push_back((c>>16)&0xff); v.push_back((c>>8)&0xff); v.push_back(c&0xff);
            auto d = SHA256::hash_bytes(v); uint64_t x=0; for (int i=0;i<8;i++) x=(x<<8)|d[i];
            out.push_back((size_t)(x % domain)); cur = d;
        }
    }
    return out;
}

/*----------------------------- Blinding polynomial --------------------------*/
// Public blinding polynomial b(X) derived only from domain_tag.
// Degree kept small (8) to keep SRS needs modest; adjust as you like.
static Poly blinding_poly_from_domain_tag(const std::array<uint8_t,32>& domain_tag, size_t d_b = 8)
{
    Poly b; b.c.resize(d_b + 1);
    for (size_t i=0;i<=d_b;i++){
        std::vector<uint8_t> m = {'c','o','d','e','-','b','l','i','n','d'};
        m.insert(m.end(), domain_tag.begin(), domain_tag.end());
        m.push_back(uint8_t(i & 0xff));
        auto h = SHA256::hash_bytes(m);
        uint64_t v=0; for (int k=0;k<8;k++) v = (v<<8) | h[k];
        b.c[i] = fr_from_u64(v);
    }
    poly_normalize(b);
    return b;
}

/*----------------------------- Proof structs --------------------------------*/
struct KZGOpening { size_t idx; Fr value; G1 witness; };
struct LUTOpening { size_t idx; Fr x_val,y_val,z_val,op_val; G1 x_wit,y_wit,z_wit,op_wit; };

struct RowOpening {
    size_t idx;         // i   (0 <= i < trace_len-1)
    Fr pc_i, pc_ip1;    G1 pc_wit_i, pc_wit_ip1;
    Fr op_i;    G1 op_wit_i;
    Fr x_i;     G1 x_wit_i;
    Fr y_i;     G1 y_wit_i;
    Fr z_i;     G1 z_wit_i;
    Fr h_i;     G1 h_wit_i;
};

struct Proof {
    // Binding to CodeCommit + PublicInstance
    std::array<uint8_t,32> code_sha;
    G1 code_comm_kzg_sess;       // session-salted commitment (changes every run)
    std::array<uint8_t,32> domain_tag;
    std::array<uint8_t,32> input_sha;

    // Column commitments
    G1 pc_comm, op_comm, z_comm, x_comm, y_comm, h_comm;

    // Optional LUT commitments (present if AND/OR used)
    G1 lut_x_comm, lut_y_comm, lut_z_comm, lut_op_comm;

    // Protocol artifacts
    SumcheckProof sc;
    uint32_t trace_pow2;
    uint32_t trace_len;
    std::vector<KZGOpening> op_openings;
    std::vector<LUTOpening> lut_openings;
    std::vector<RowOpening> row_openings;

    // Public output
    uint64_t final_output;
};

/*----------------------------- Stage 1: code commit -------------------------*/
static CodeCommit commit_code(const KZGParams& pp, const std::vector<Instr>& prog){
    CodeCommit cc; cc.code_sha = commit_code_sha(prog);
    auto code_bytes = serialize_program_bytes(prog);
    Poly code_poly = code_poly_from_bytes_monomial(code_bytes);
    cc.code_comm_kzg_base = kzg_commit(pp, code_poly);
    return cc;
}

/*----------------------------- Stage 2: prover ------------------------------*/
static Proof prove(const KZGParams& pp,
                   const CodeCommit& cc,
                   const PublicInstance& inst,
                   const std::vector<Instr>& prog,
                   const std::vector<int64_t>& inputs,
                   size_t k_lookup = 4,
                   size_t k_rows_spot = 2)
{
    bool uses_logic = program_uses_logic(prog);

    // Run VM
    auto trace = run_vm(prog, inputs);
    size_t T = trace.size();
    size_t pow2 = 1; while (pow2 < T) pow2 <<= 1;

    // Trace columns (padded)
    std::vector<Fr> col_pc(pow2), col_op(pow2), col_z(pow2), col_h(pow2), col_x(pow2), col_y(pow2);
    for (size_t i=0;i<T;i++){
        col_pc[i]=fr_from_u64(trace[i].pc);
        col_op[i]=trace[i].opcode;
        col_z[i]=trace[i].z;
        col_h[i]=trace[i].is_halt;
        col_x[i]=trace[i].x;
        col_y[i]=trace[i].y;
    }
    for (size_t i=T;i<pow2;i++){
        col_pc[i].clear(); col_op[i].clear(); col_z[i].clear(); col_h[i].clear(); col_x[i].clear(); col_y[i].clear();
    }

    // Interpolate & commit
    Poly pc_poly = interpolate_on_range0(col_pc);
    Poly op_poly = interpolate_on_range0(col_op);
    Poly z_poly  = interpolate_on_range0(col_z);
    Poly x_poly  = interpolate_on_range0(col_x);
    Poly y_poly  = interpolate_on_range0(col_y);
    Poly h_poly  = interpolate_on_range0(col_h);

    G1 pc_comm = kzg_commit(pp, pc_poly);
    G1 op_comm = kzg_commit(pp, op_poly);
    G1 z_comm  = kzg_commit(pp, z_poly);
    G1 x_comm  = kzg_commit(pp, x_poly);
    G1 y_comm  = kzg_commit(pp, y_poly);
    G1 h_comm  = kzg_commit(pp, h_poly);

    // Transition constraint table f[i] = (pc[i+1] - (pc[i]+1)) * (1 - h[i])
    std::vector<Fr> f(pow2); for (auto &x : f) x.clear();
    Fr one = fr_one();
    for (size_t i=0;i+1<T;i++){
        Fr diff = col_pc[i+1] - (col_pc[i] + one);
        f[i] = diff * (one - col_h[i]);
    }

    // Load LUT (only if needed)
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
    } else {
        lut_x_comm.clear(); lut_y_comm.clear(); lut_z_comm.clear(); lut_op_comm.clear();
    }

    // Session-salted code commitment: C_sess = C_base + Commit(b(domain_tag))
    Poly bpoly = blinding_poly_from_domain_tag(inst.domain_tag);
    G1 B = kzg_commit(pp, bpoly);
    G1 C_sess = cc.code_comm_kzg_base;
    G1::add(C_sess, C_sess, B);

    // Transcript: bind PublicInstance then commitments
    Transcript tr;
    tr.absorb(inst.domain_tag);
    tr.absorb(inst.input_sha);
    tr.absorb(cc.code_sha);
    tr.absorb_g1(C_sess);
    tr.absorb_g1(pc_comm);
    tr.absorb_g1(op_comm);
    tr.absorb_g1(z_comm);
    tr.absorb_g1(x_comm);
    tr.absorb_g1(y_comm);
    tr.absorb_g1(h_comm);
    if (!lut_x_comm.isZero() || !lut_y_comm.isZero() || !lut_z_comm.isZero() || !lut_op_comm.isZero()){
        tr.absorb_g1(lut_x_comm); tr.absorb_g1(lut_y_comm); tr.absorb_g1(lut_z_comm); tr.absorb_g1(lut_op_comm);
    }

    // Sumcheck
    auto sc = sumcheck_prove(f, tr);

    // Sampled opcode openings
    auto seed = tr.squeeze();
    auto idxs = derive_indices(seed, pow2, k_lookup);
    std::vector<KZGOpening> op_openings;
    for (auto idx : idxs){
        Fr pt=fr_from_u64(idx); Fr val; G1 wit; kzg_open(pp, op_poly, pt, val, wit);
        op_openings.push_back({idx, val, wit});
    }

    // Semantic LUT openings (for all executed AND/OR rows)
    std::vector<LUTOpening> lut_openings;
    if (uses_logic){
        const size_t LUT_RANGE=16, LUT_PER_OP=256;
        for (size_t i=0;i<T;i++){
            uint64_t opv = fr_to_u64(trace[i].opcode);
            if (opv==OP_AND || opv==OP_OR){
                uint64_t xa=(uint64_t)(trace[i].x_raw & 0xF), yb=(uint64_t)(trace[i].y_raw & 0xF);
                size_t base = (opv==OP_AND)?0:LUT_PER_OP; size_t idx = base + xa*LUT_RANGE + yb;
                Fr pt=fr_from_u64(idx); Fr vx,vy,vz,vop; G1 wx,wy,wz,wop;
                kzg_open(pp, lut_x_poly, pt, vx, wx);
                kzg_open(pp, lut_y_poly, pt, vy, wy);
                kzg_open(pp, lut_z_poly, pt, vz, wz);
                kzg_open(pp, lut_op_poly, pt, vop, wop);
                lut_openings.push_back({idx, vx, vy, vz, vop, wx, wy, wz, wop});
            }
        }
    }

    // RANDOM-ROW SPOT CHECKS
    std::vector<uint8_t> rs_input(seed.begin(), seed.end()); rs_input.push_back(0x52);
    auto row_seed = SHA256::hash_bytes(rs_input);
    size_t domain_rows = (T>1)? (T-1) : 1;
    auto row_indices = derive_indices(row_seed, domain_rows, k_rows_spot);
    std::vector<RowOpening> row_openings; row_openings.reserve(row_indices.size());
    for (auto i : row_indices){
        RowOpening ro; ro.idx = i;
        Fr zi  = fr_from_u64(i);
        Fr zip = fr_from_u64(i+1);
        kzg_open(pp, pc_poly, zi,  ro.pc_i,    ro.pc_wit_i);
        kzg_open(pp, pc_poly, zip, ro.pc_ip1,  ro.pc_wit_ip1);
        kzg_open(pp, op_poly, zi, ro.op_i, ro.op_wit_i);
        kzg_open(pp, x_poly,  zi, ro.x_i,  ro.x_wit_i);
        kzg_open(pp, y_poly,  zi, ro.y_i,  ro.y_wit_i);
        kzg_open(pp, z_poly,  zi, ro.z_i,  ro.z_wit_i);
        kzg_open(pp, h_poly,  zi, ro.h_i,  ro.h_wit_i);
        row_openings.push_back(ro);
    }

    // Pack proof
    Proof prf;
    prf.code_sha           = cc.code_sha;
    prf.code_comm_kzg_sess = C_sess;
    prf.domain_tag         = inst.domain_tag;
    prf.input_sha          = inst.input_sha;
    prf.pc_comm=pc_comm; prf.op_comm=op_comm; prf.z_comm=z_comm;
    prf.x_comm=x_comm;   prf.y_comm=y_comm;   prf.h_comm=h_comm;
    prf.lut_x_comm = lut_x_comm; prf.lut_y_comm = lut_y_comm;
    prf.lut_z_comm = lut_z_comm; prf.lut_op_comm = lut_op_comm;
    prf.sc = sc; prf.trace_pow2 = (uint32_t)pow2; prf.trace_len = (uint32_t)T;
    prf.op_openings = std::move(op_openings);
    prf.lut_openings = std::move(lut_openings);
    prf.row_openings = std::move(row_openings);
    prf.final_output = (uint64_t)trace.back().z_raw;
    return prf;
}

/*----------------------------- Stage 3: verifier ----------------------------*/
static bool verify(const KZGParams& pp,
                   const CodeCommit& cc,
                   const PublicInstance& inst,
                   const Proof& proof,
                   size_t k_lookup = 4,
                   size_t k_rows_spot = 2,
                   std::string *reason_out = nullptr)
{
    auto fail = [&](const std::string& why){ if (reason_out) *reason_out = why; return false; };

    // Recompute session-salted code commitment C_sess_expected = C_base + Commit(b(tag))
    Poly bpoly = blinding_poly_from_domain_tag(inst.domain_tag);
    G1 B = kzg_commit(pp, bpoly);
    G1 C_sess_expected = cc.code_comm_kzg_base; G1::add(C_sess_expected, C_sess_expected, B);

    // Bind to published code (sha) and session-salted commitment
    if (proof.code_sha != cc.code_sha)                 return fail("code sha mismatch");
    if (proof.code_comm_kzg_sess != C_sess_expected)   return fail("code KZG (session) mismatch");

    // Anti-replay / instance binding
    if (proof.domain_tag != inst.domain_tag)           return fail("domain tag mismatch");
    if (proof.input_sha  != inst.input_sha)            return fail("input hash mismatch");
    if (inst.bind_expected_output && proof.final_output != inst.expected_output)
        return fail("unexpected output");

    if (proof.trace_len == 0 || proof.trace_pow2 == 0 || proof.trace_len > proof.trace_pow2)
        return fail("invalid trace sizes");

    // Rebuild transcript in same order
    Transcript tr;
    tr.absorb(proof.domain_tag);
    tr.absorb(proof.input_sha);
    tr.absorb(proof.code_sha);
    tr.absorb_g1(proof.code_comm_kzg_sess);
    tr.absorb_g1(proof.pc_comm);
    tr.absorb_g1(proof.op_comm);
    tr.absorb_g1(proof.z_comm);
    tr.absorb_g1(proof.x_comm);
    tr.absorb_g1(proof.y_comm);
    tr.absorb_g1(proof.h_comm);
    if (!proof.lut_x_comm.isZero() || !proof.lut_y_comm.isZero() || !proof.lut_z_comm.isZero() || !proof.lut_op_comm.isZero()){
        tr.absorb_g1(proof.lut_x_comm); tr.absorb_g1(proof.lut_y_comm); tr.absorb_g1(proof.lut_z_comm); tr.absorb_g1(proof.lut_op_comm);
    } else if (!proof.lut_openings.empty()){
        return fail("unexpected LUT openings");
    }

    // Sumcheck (global)
    if (!sumcheck_verify(proof.sc, tr, proof.sc.claimed_sum)) return fail("sumcheck failed");

    // Sampled opcode openings (global)
    auto seed = tr.squeeze();
    auto idxs = derive_indices(seed, proof.trace_pow2, k_lookup);
    if (idxs.size() != proof.op_openings.size()) return fail("opcode opening size mismatch");
    std::vector<uint64_t> allowed = { OP_PUSH, OP_ADD, OP_MUL, OP_SUB, OP_AND, OP_OR, OP_HALT };
    for (size_t i=0;i<idxs.size();i++){
        const auto &open = proof.op_openings[i];
        if (open.idx != idxs[i]) return fail("opcode opening idx mismatch");
        Fr z = fr_from_u64(open.idx);
        if (!kzg_verify(pp, proof.op_comm, z, open.value, open.witness)) return fail("opcode opening pairing fail");
        uint64_t v = fr_to_u64(open.value);
        bool ok=false; for (auto a: allowed) if (v==a){ ok=true; break; }
        if (!ok) return fail("opcode not allowed");
    }

    // LUT openings + semantics
    for (const auto &lo : proof.lut_openings){
        Fr pt=fr_from_u64(lo.idx);
        if (!kzg_verify(pp, proof.lut_x_comm, pt, lo.x_val, lo.x_wit)) return fail("lut x open");
        if (!kzg_verify(pp, proof.lut_y_comm, pt, lo.y_val, lo.y_wit)) return fail("lut y open");
        if (!kzg_verify(pp, proof.lut_z_comm, pt, lo.z_val, lo.z_wit)) return fail("lut z open");
        if (!kzg_verify(pp, proof.lut_op_comm, pt, lo.op_val, lo.op_wit)) return fail("lut op open");
        uint64_t xv=fr_to_u64(lo.x_val)&0xF, yv=fr_to_u64(lo.y_val)&0xF, zv=fr_to_u64(lo.z_val)&0xF, opv=fr_to_u64(lo.op_val);
        if (opv==OP_AND){ if (zv != ((xv & yv) & 0xF)) return fail("AND semantics"); }
        else if (opv==OP_OR){ if (zv != ((xv | yv) & 0xF)) return fail("OR semantics"); }
        else return fail("unexpected LUT op");
    }

    // RANDOM-ROW SPOT CHECKS
    std::vector<uint8_t> rs_input(seed.begin(), seed.end()); rs_input.push_back(0x52);
    auto row_seed = SHA256::hash_bytes(rs_input);
    size_t domain_rows = (proof.trace_len>1)? (proof.trace_len-1) : 1;
    auto row_indices = derive_indices(row_seed, domain_rows, k_rows_spot);

    if (proof.row_openings.size() != row_indices.size()) return fail("row openings size mismatch");

    Fr one = fr_one();
    for (size_t j=0;j<row_indices.size();j++){
        const auto &ro = proof.row_openings[j];
        if (ro.idx != row_indices[j]) return fail("row opening idx mismatch");

        Fr zi  = fr_from_u64(ro.idx);
        Fr zip = fr_from_u64(ro.idx+1);

        if (!kzg_verify(pp, proof.pc_comm, zi,  ro.pc_i,    ro.pc_wit_i))   return fail("pc[i] opening fail");
        if (!kzg_verify(pp, proof.pc_comm, zip, ro.pc_ip1,  ro.pc_wit_ip1)) return fail("pc[i+1] opening fail");
        if (!kzg_verify(pp, proof.op_comm, zi,  ro.op_i,    ro.op_wit_i))   return fail("op[i] opening fail");
        if (!kzg_verify(pp, proof.x_comm,  zi,  ro.x_i,     ro.x_wit_i))    return fail("x[i] opening fail");
        if (!kzg_verify(pp, proof.y_comm,  zi,  ro.y_i,     ro.y_wit_i))    return fail("y[i] opening fail");
        if (!kzg_verify(pp, proof.z_comm,  zi,  ro.z_i,     ro.z_wit_i))    return fail("z[i] opening fail");
        if (!kzg_verify(pp, proof.h_comm,  zi,  ro.h_i,     ro.h_wit_i))    return fail("h[i] opening fail");

        // local transition
        if (ro.h_i.isZero()){
            if (!(ro.pc_ip1 == (ro.pc_i + one))) return fail("pc local transition fail");
        }

        // local semantics
        uint64_t opv = fr_to_u64(ro.op_i);
        uint64_t xv  = fr_to_u64(ro.x_i);
        uint64_t yv  = fr_to_u64(ro.y_i);
        uint64_t zv  = fr_to_u64(ro.z_i);
        switch (opv){
            case OP_ADD: if (zv != (xv + yv)) return fail("ADD semantics"); break;
            case OP_SUB: if (zv != (yv - xv)) return fail("SUB semantics"); break;
            case OP_MUL: if (zv != (xv * yv)) return fail("MUL semantics"); break;
            case OP_AND: if ((zv & 0xF) != ((xv & 0xF) & (yv & 0xF))) return fail("AND semantics"); break;
            case OP_OR:  if ((zv & 0xF) != ((xv & 0xF) | (yv & 0xF))) return fail("OR semantics"); break;
            case OP_PUSH: /* Not enforcing imm without code-memory opens */ break;
            case OP_HALT: break;
            default: return fail("unexpected opcode in row check");
        }
    }

    return true;
}

/*----------------------------- Pretty printing ------------------------------*/
static void print_commitment(const CodeCommit& cc, const G1& c_sess){
    std::cout << "=== Commitment (Publisher + Session) ===\n";
    std::cout << "Code SHA256       : " << arr32_to_hex(cc.code_sha) << "\n";
    std::cout << "Code KZG Base (G1): " << g1_to_hex(cc.code_comm_kzg_base) << "\n";
    std::cout << "Code KZG Sess (G1): " << g1_to_hex(c_sess) << "  (changes per run)\n";
}
static void print_proof(const Proof& prf){
    std::cout << "=== Proof (key fields) ===\n";
    std::cout << "domain_tag        : " << arr32_to_hex(prf.domain_tag) << "\n";
    std::cout << "input_sha         : " << arr32_to_hex(prf.input_sha) << "\n";
    std::cout << "code_sha          : " << arr32_to_hex(prf.code_sha) << "\n";
    std::cout << "code KZG sess (G1): " << g1_to_hex(prf.code_comm_kzg_sess) << "\n";
    std::cout << "pc_comm (G1)      : " << g1_to_hex(prf.pc_comm) << "\n";
    std::cout << "op_comm (G1)      : " << g1_to_hex(prf.op_comm) << "\n";
    std::cout << "z_comm  (G1)      : " << g1_to_hex(prf.z_comm) << "\n";
    std::cout << "x_comm  (G1)      : " << g1_to_hex(prf.x_comm) << "\n";
    std::cout << "y_comm  (G1)      : " << g1_to_hex(prf.y_comm) << "\n";
    std::cout << "h_comm  (G1)      : " << g1_to_hex(prf.h_comm) << "\n";
    if (!prf.lut_x_comm.isZero() || !prf.lut_y_comm.isZero() || !prf.lut_z_comm.isZero() || !prf.lut_op_comm.isZero()){
        std::cout << "lut_x   (G1)      : " << g1_to_hex(prf.lut_x_comm) << "\n";
        std::cout << "lut_y   (G1)      : " << g1_to_hex(prf.lut_y_comm) << "\n";
        std::cout << "lut_z   (G1)      : " << g1_to_hex(prf.lut_z_comm) << "\n";
        std::cout << "lut_op  (G1)      : " << g1_to_hex(prf.lut_op_comm) << "\n";
    } else {
        std::cout << "No LUTs in this program.\n";
    }
    std::cout << "trace_len         : " << prf.trace_len << "\n";
    std::cout << "trace_pow2        : " << prf.trace_pow2 << "\n";
    std::cout << "sumcheck rounds   : " << prf.sc.n_vars << "\n";
    std::cout << "opcode openings   : " << prf.op_openings.size() << " indices\n";
    if (!prf.op_openings.empty()){
        std::cout << "  idx sample      :";
        for (size_t i=0;i<std::min<size_t>(prf.op_openings.size(), 4); i++)
            std::cout << " " << prf.op_openings[i].idx;
        std::cout << (prf.op_openings.size()>4 ? " ..." : "") << "\n";
    }
    std::cout << "row openings      : " << prf.row_openings.size() << " indices\n";
    if (!prf.row_openings.empty()){
        std::cout << "  row sample      :";
        for (size_t i=0;i<std::min<size_t>(prf.row_openings.size(), 4); i++)
            std::cout << " " << prf.row_openings[i].idx;
        std::cout << (prf.row_openings.size()>4 ? " ..." : "") << "\n";
    }
    std::cout << "final output      : " << prf.final_output << "\n";
}

/*----------------------------- Demo main ------------------------------------*/
int main(){
    try {
        mcl::bn::initPairing(mcl::BN254);

        // Example program: arithmetic + AND/OR + HALT
        std::vector<Instr> prog = {
            {OP_PUSH, 5}, {OP_PUSH, 7}, {OP_ADD, 0},
            {OP_PUSH, 2}, {OP_MUL, 0},          // stack = [24]
            {OP_PUSH, 0xF}, {OP_AND, 0},        // (24 & 0xF) = 8
            {OP_PUSH, 0x3}, {OP_OR,  0},        // (8 | 3) = 11
            {OP_HALT, 0}
        };
        std::vector<int64_t> inputs = {};

        // SRS
        auto pp = kzg_setup(4096); // generous upper bound

        // Stage 1: Publisher commits to code (base commitment)
        CodeCommit cc = commit_code(pp, prog);

        // PublicInstance (anti-replay)
        PublicInstance inst{};
        {
            std::vector<uint8_t> tag_src = {'s','e','s','s','i','o','n','-','v','3'};
            inst.domain_tag = SHA256::hash_bytes(tag_src);
        }
        inst.input_sha = hash_inputs_i64(inputs);
        inst.bind_expected_output = false;

        // Stage 2: Prover (creates session-salted commitment internally)
        Proof prf = prove(pp, cc, inst, prog, inputs, /*k_lookup=*/4, /*k_rows_spot=*/2);

        // Print commitments (base + session)
        std::cout << "\n";
        print_commitment(cc, prf.code_comm_kzg_sess);

        // Print proof summary
        std::cout << "\n";
        print_proof(prf);
        std::cout << "\n";

        // Stage 3: Verifier
        std::string reason;
        bool ok = verify(pp, cc, inst, prf, /*k_lookup=*/4, /*k_rows_spot=*/2, &reason);

        std::cout << "Verify: " << (ok ? "ACCEPT" : "REJECT") << "\n";
        if (!ok) std::cout << "Reason: " << reason << "\n";
        std::cout << "Claimed output: " << prf.final_output << "\n";
        return ok ? 0 : 1;

    } catch (const std::exception& e){
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
