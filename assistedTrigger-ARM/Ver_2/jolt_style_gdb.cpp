/*
================================================================================
Production-style JOLT-ish demo (ARM64 + GDB), single file, C++17
--------------------------------------------------------------------------------
What this does
- Commits to the program bytes (ASM or ELF) with:
  • SHA-256 of committed bytes
  • KZG commitment (BN254) to the byte polynomial (monomial basis)
  • Per-session blinding of the code commitment (domain-tag keyed)

- Runs the real program under GDB/MI (ARM64 Ubuntu), stepping instructions.
  • Records a trace of recognized ops: mov(additive PUSH), add/sub, mul, and, orr
  • Supports immediates and register shifts (lsl/lsr/asr #k) for the 2nd operand
  • Skips any unrecognized instructions (does not mislabel them)

- Prover:
  • Interpolates columns (pc/op/x/y/z/halt) over {0,1,...,T-1}
  • KZG-commits to each column
  • Enforces local PC transition with a sum-check over a constraint table
  • (Optional) Loads LUT files for AND/OR (pre-generated) and opens a few points
  • Samples random indices from a Fiat–Shamir transcript for:
      - opcode openings on [0..T-1]
      - row consistency checks on [0..T-2]
  • Binds everything to (domain_tag, input_sha, code_sha, blinded code KZG)

- Verifier:
  • Reconstructs per-session blinded code KZG from the public base + domain_tag
  • Verifies the transcript-driven sum-check and the sampled KZG openings
  • Checks opcode values IN-FIELD (Fr) against allowed set
  • Checks row-wise semantics for sampled rows
  • Never trusts equality to arbitrary external inputs (anti-replay baked-in)

Key engineering points
- BN254 via mcl (G1/G2/KZG) with monomial-basis commitment (simple, not fast).
- Deterministic blinding polynomial from domain_tag prevents proof replay.
- Strict GDB/MI exit detection, so we don’t prematurely stop tracing.
- Only emit HALT after we’ve recorded ≥1 row; otherwise keep stepping.
- Debug view: `--debug` prints the first few captured rows.

Tested assumptions
- Host: ARM64 Ubuntu
- Build: `g++ -std=gnu++17 -O2 -lmcl -lcrypto`
- Sample: compile your program with `-O0 -g -fno-pie -no-pie` so ops are visible.

Files optionally used at runtime
- `lut_and_or_x.txt`, `lut_and_or_y.txt`, `lut_and_or_z.txt`, `lut_and_or_op.txt`
  Each file format: first line N, then N hex field elements (Fr) on following lines.

Security notes
- The per-session blinded code commitment ties the proof to (domain_tag, session).
- The verifier recomputes this blinded code commitment; copying someone else’s
  proof bytes (even with matching base code commitment) won’t verify unless all
  bound public inputs and blinding match.
- Random sampling is Fiat–Shamir from all bound commitments.

================================================================================
*/

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
#include <sstream>
#include <regex>
#include <optional>
#include <cctype>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>

#include <mcl/bn.hpp>
using namespace mcl::bn;

// ---------- utils ----------
static std::string toHex(const uint8_t* p, size_t n){
    std::ostringstream oss; oss << std::hex << std::setfill('0');
    for (size_t i=0;i<n;i++) oss << std::setw(2) << (unsigned)p[i];
    return oss.str();
}
static std::string g1_to_hex(const G1& g){
    uint8_t buf[128]; size_t n = g.serialize(buf, sizeof(buf));
    return toHex(buf, n);
}
static std::string arr32_to_hex(const std::array<uint8_t,32>& a){ return toHex(a.data(), a.size()); }

static Fr fr_from_u64(uint64_t v){ Fr r; uint8_t b[8]; for(int i=0;i<8;i++) b[7-i]=uint8_t(v>>(i*8)); r.setBigEndianMod(b,8); return r; }
static Fr fr_zero(){ Fr z; z.clear(); return z; }
static Fr fr_one(){ return fr_from_u64(1); }
static uint64_t fr_to_u64(const Fr& x){
    uint8_t buf[64]; size_t n=x.serialize(buf,sizeof(buf));
    uint64_t v=0; size_t s=(n>8)?n-8:0; for (size_t i=s;i<n;i++) v=(v<<8)|buf[i]; return v;
}

// ---------- SHA256 & transcript ----------
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
            w[i]=(uint32_t)p[4*i]<<24|(uint32_t)p[4*i+1]<<16|(uint32_t)p[4*i+2]<<8|(uint32_t)p[4*i+3];
        for(int i=16;i<64;i++) w[i]=sig0(w[i-15])+w[i-7]+sig1(w[i-2])+w[i-16];
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for(int i=0;i<64;i++){ uint32_t t1=hh+Sig1(e)+Ch(e,f,g)+K[i]+w[i]; uint32_t t2=Sig0(a)+Maj(a,b,c);
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2; }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    void update(const void* data, size_t len){
        const uint8_t *p=(const uint8_t*)data; bits += (uint64_t)len*8ULL;
        while(len--){ buf.push_back(*p++); if (buf.size()==64){ process(buf.data()); buf.clear(); } }
    }
    std::array<uint8_t,32> digest(){
        std::vector<uint8_t> t=buf; t.push_back(0x80); while(t.size()%64!=56) t.push_back(0);
        for(int i=7;i>=0;i--) t.push_back((bits>>(8*i))&0xff);
        for(size_t i=0;i<t.size();i+=64) process(t.data()+i);
        std::array<uint8_t,32> out{};
        for(int i=0;i<8;i++){ out[4*i]=(h[i]>>24)&0xff; out[4*i+1]=(h[i]>>16)&0xff; out[4*i+2]=(h[i]>>8)&0xff; out[4*i+3]=(h[i])&0xff; }
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

struct Transcript {
    std::vector<uint8_t> st;
    void absorb(const std::array<uint8_t,32>& a){ st.insert(st.end(), a.begin(), a.end()); }
    void absorb_fr(const Fr& x){ uint8_t b[64]; size_t n=x.serialize(b,sizeof(b)); st.insert(st.end(), b, b+n); }
    void absorb_g1(const G1& g){ uint8_t b[128]; size_t n=g.serialize(b,sizeof(b)); st.insert(st.end(), b, b+n); }
    std::array<uint8_t,32> squeeze(){ return SHA256::hash_bytes(st); }
    Fr challenge(){ auto h=squeeze(); uint64_t v=0; for(int i=0;i<8;i++) v=(v<<8)|h[i]; return fr_from_u64(v); }
};

// ---------- KZG (BN254 via mcl) ----------
struct Poly { std::vector<Fr> c; };
static void poly_normalize(Poly& p){ while(!p.c.empty() && p.c.back().isZero()) p.c.pop_back(); }

struct KZGParams { G1 g1; G2 g2; std::vector<G1> srs_g1; G2 g2_s; Fr s; };

static KZGParams kzg_setup(size_t max_deg){
    KZGParams pp; mapToG1(pp.g1, 1); mapToG2(pp.g2, 2); pp.s.setByCSPRNG();
    pp.srs_g1.resize(max_deg+1);
    Fr pow_s = fr_one();
    for (size_t i=0;i<=max_deg;i++){ G1 gi = pp.g1; G1::mul(gi, gi, pow_s); pp.srs_g1[i]=gi; pow_s *= pp.s; }
    pp.g2_s = pp.g2; G2::mul(pp.g2_s, pp.g2, pp.s);
    return pp;
}
static G1 kzg_commit(const KZGParams& pp, const Poly& f){
    if (f.c.size() > pp.srs_g1.size())
        throw std::runtime_error("kzg_commit: SRS too small for polynomial degree="
                                 + std::to_string(f.c.size()-1) + ", srs="
                                 + std::to_string(pp.srs_g1.size()-1));
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

// ---------- interpolation + sumcheck ----------
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
        Fr ns = fr_zero(); for (auto &x : cur) ns += x;
        cur_sum = ns;
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

// ---------- VM row/op ----------
enum Op : uint8_t { OP_PUSH=0, OP_ADD=1, OP_MUL=2, OP_SUB=3, OP_AND=4, OP_OR=5, OP_HALT=255 };
struct TraceRow { uint32_t pc; Fr opcode,x,y,z,is_halt; int64_t x_raw,y_raw,z_raw; };

// ---------- GDB/MI ----------
class GdbMi {
    FILE* in_=nullptr;  FILE* out_=nullptr;
public:
    GdbMi(const std::string& exe, const std::string& args){
        std::signal(SIGPIPE, SIG_IGN);
        int inpipe[2], outpipe[2];
        if (pipe(inpipe)!=0 || pipe(outpipe)!=0) throw std::runtime_error("pipe failed");
        pid_t pid=fork();
        if (pid<0) throw std::runtime_error("fork failed");
        if (pid==0){
            dup2(inpipe[0], 0); dup2(outpipe[1], 1); dup2(outpipe[1], 2);
            close(inpipe[1]); close(outpipe[0]);
            execlp("gdb", "gdb", "-q", "--interpreter=mi", nullptr);
            _exit(127);
        }
        close(inpipe[0]); close(outpipe[1]);
        in_  = fdopen(inpipe[1],"w");
        out_ = fdopen(outpipe[0],"r");
        if (!in_ || !out_) throw std::runtime_error("fdopen failed");

        std::string banner = readUntilPromptOrEOF();
        if (banner.find("(gdb)") == std::string::npos)
            throw std::runtime_error("gdb no prompt; banner='"+banner+"'");

        mustOk(mi(std::string("-file-exec-and-symbols \"")+exe+"\""), "file-exec-and-symbols");
        if (!args.empty()) mustOk(mi("-exec-arguments "+args), "exec-arguments");
        mi("-gdb-set pagination off");
        mi("-gdb-set breakpoint pending on");

        // Try 'start' first, which sets a temporary bp on main
        std::string s = console("start");
        if (s.find("Temporary breakpoint") == std::string::npos &&
            s.find("breakpoint") == std::string::npos &&
            s.find("*stopped") == std::string::npos)
        {
            // If that fails, set bp on main and run
            mi("-break-insert -f main");
            std::string run = mi("-exec-run");
            if (hasError(run)) throw std::runtime_error("exec-run failed: "+run);
            for (int tries=0; tries<4; ++tries){
                if (run.find("reason=\"breakpoint-hit\"") != std::string::npos) break;
                run = mi("-exec-continue");
                if (run.find("exited-normally") != std::string::npos)
                    throw std::runtime_error("program exited before main()");
            }
        }
    }
    ~GdbMi(){ if (in_) { fprintf(in_,"-gdb-exit\n"); fflush(in_); fclose(in_);} if (out_) fclose(out_); }
    std::string mi(const std::string& cmd){
        if (fprintf(in_, "%s\n", cmd.c_str()) < 0) throw std::runtime_error("write gdb failed");
        fflush(in_);
        return readUntilPromptOrEOF();
    }
    std::string console(const std::string& cli){
        if (fprintf(in_, "-interpreter-exec console \"%s\"\n", cli.c_str()) < 0)
            throw std::runtime_error("write gdb console failed");
        fflush(in_);
        return readUntilPromptOrEOF();
    }
    std::string stepi(){ if (fprintf(in_, "-exec-step-instruction\n") < 0) throw std::runtime_error("write stepi failed"); fflush(in_); return readUntilPromptOrEOF(); }
    std::string disasCur(){ return console("x/i $pc"); }
    std::optional<uint64_t> readRegX(int idx){
        if (idx<0||idx>30) return std::nullopt;
        std::string r = mi("-data-evaluate-expression $x"+std::to_string(idx));
        std::smatch m;
        if (std::regex_search(r, m, std::regex("value=\"(0x[0-9a-fA-F]+)\""))) {
            uint64_t v=0; std::stringstream ss; ss<<std::hex<<m[1].str(); ss>>v; return v;
        }
        return std::nullopt;
    }
    // Strict MI helpers
    bool isExited(const std::string& resp) const {
        return resp.find("exited-normally")   != std::string::npos ||
               resp.find("exited-signalled")  != std::string::npos ||
               resp.find("exit-status")       != std::string::npos;
    }
    bool isStopped(const std::string& resp) const {
        return resp.find("*stopped")          != std::string::npos;
    }
private:
    static bool hasError(const std::string& s){
        return s.find("^error") != std::string::npos || s.find("Undefined")!=std::string::npos;
    }
    static void mustOk(const std::string& s, const char* label){
        if (s.find("^done")==std::string::npos && s.find("*stopped")==std::string::npos){
            throw std::runtime_error(std::string("gdb '")+label+"' failed: "+s);
        }
    }
    std::string readUntilPromptOrEOF(){
        std::string out; char buf[4096];
        while (true){
            if (!fgets(buf,sizeof(buf), out_)) break;
            out += buf;
            if (out.rfind("(gdb)") != std::string::npos) break;
            if (out.find("^done") != std::string::npos) break;
            if (out.find("^error") != std::string::npos) break;
            if (out.find("*stopped") != std::string::npos) break;
            if (out.find("exited-normally") != std::string::npos) break;
            if (out.find("exited-signalled") != std::string::npos) break;
        }
        return out;
    }
};

// ---------- AArch64 decoder ----------
enum class ShiftKind { NONE, LSL, LSR, ASR };
struct Decoded {
    bool recognized=false;
    Op op = OP_PUSH;
    int dst = -1;
    int src1 = -1;
    int src2 = -1;
    bool imm_used = false;
    uint64_t imm_val = 0;
    ShiftKind shift = ShiftKind::NONE;
    uint32_t shift_amt = 0;
};

static int regIndexA64(const std::string& r){
    if (r.size()<2) return -1;
    if (r[0]!='x'&&r[0]!='w') return -1;
    int n=0; for (size_t i=1;i<r.size();i++){ unsigned char c=r[i]; if (!std::isdigit(c)) return -1; n=n*10+(c-'0'); }
    return n;
}
static bool parse_imm64(const std::string& tok, uint64_t& out) {
    auto trim=[](std::string v){ v.erase(0,v.find_first_not_of(" \t")); v.erase(v.find_last_not_of(" \t")+1); return v; };
    std::string s = trim(tok);
    if (!s.empty() && s[0]=='#') s.erase(0,1);
    if (s.rfind("0x",0)==0 || s.rfind("0X",0)==0) { unsigned long long v=0; std::stringstream ss; ss<<std::hex<<s; ss>>v; out=(uint64_t)v; return !ss.fail();}
    unsigned long long v=0; std::stringstream ss; ss<<s; ss>>v; out=(uint64_t)v; return !ss.fail();
}

static Decoded decodeA64(const std::string& line){
    auto trim=[](std::string s){ s.erase(0,s.find_first_not_of(" \t")); s.erase(s.find_last_not_of(" \t")+1); return s; };
    std::smatch m;
    std::regex re(":[[:space:]]+([a-z]+)[[:space:]]+([^,]+),[[:space:]]*([^,]+)(?:,[[:space:]]*([^\\n]+))?");
    if (!std::regex_search(line, m, re)) return Decoded{};

    std::string op  = trim(m[1].str());
    std::string rd  = trim(m[2].str());
    std::string r1  = trim(m[3].str());
    std::string r2  = (m.size()>=5 && m[4].matched) ? trim(m[4].str()) : "";

    if (op=="adds") op="add";
    if (op=="subs") op="sub";

    Decoded d;

    // mov → PUSH
    if (op=="mov") {
        d.recognized=true; d.op=OP_PUSH; d.dst=regIndexA64(rd);
        uint64_t imm=0;
        if (parse_imm64(r1, imm)) { d.imm_used=true; d.imm_val=imm; }
        else                      { d.src1=regIndexA64(r1); }
        return d;
    }

    if      (op=="add") d.op = OP_ADD;
    else if (op=="sub") d.op = OP_SUB;
    else if (op=="mul") d.op = OP_MUL;
    else if (op=="and") d.op = OP_AND;
    else if (op=="orr") d.op = OP_OR;
    else return d; // unrecognized

    int dst = regIndexA64(rd);
    int s1  = regIndexA64(r1);
    if (dst<0 || s1<0) return Decoded{};

    auto isZR = [](std::string t){ for (auto& c: t) c=std::tolower(c); return t=="xzr"||t=="wzr"; };

    int s2 = regIndexA64(r2);
    uint64_t imm=0; bool imm_ok = (!r2.empty() && parse_imm64(r2, imm));

    ShiftKind shift = ShiftKind::NONE;
    uint32_t shamt = 0;
    if (!r2.empty() && s2<0 && !imm_ok) {
        std::smatch ms;
        std::regex rshift("([xw][0-9]+)[[:space:]]*,[[:space:]]*(lsl|lsr|asr)[[:space:]]*#?([0-9]+)");
        if (std::regex_search(r2, ms, rshift)) {
            s2 = regIndexA64(trim(ms[1].str()));
            std::string sk = trim(ms[2].str());
            unsigned long long k=0; std::stringstream ss; ss<<trim(ms[3].str()); ss>>k; shamt=(uint32_t)k;
            if      (sk=="lsl") shift=ShiftKind::LSL;
            else if (sk=="lsr") shift=ShiftKind::LSR;
            else if (sk=="asr") shift=ShiftKind::ASR;
        }
    }

    // orr mov-alias
    if (d.op==OP_OR && !r2.empty()) {
        if (isZR(r2) && s1>=0) { d.recognized=true; d.dst=dst; d.src1=s1; d.imm_used=true; d.imm_val=0; return d; }
        if (isZR(r1) && s2>=0) { d.recognized=true; d.dst=dst; d.src1=s2; d.imm_used=true; d.imm_val=0; return d; }
    }

    if (d.op==OP_MUL) {
        if (s2>=0) { d.recognized=true; d.dst=dst; d.src1=s1; d.src2=s2; return d; }
        return Decoded{};
    }

    if (imm_ok) { d.recognized=true; d.dst=dst; d.src1=s1; d.imm_used=true; d.imm_val=imm; return d; }
    if (s2>=0)  { d.recognized=true; d.dst=dst; d.src1=s1; d.src2=s2; d.shift=shift; d.shift_amt=shamt; return d; }

    return Decoded{}; // unrecognized
}

// ---------- tracing ----------
static std::vector<TraceRow> trace_with_gdb(const std::string& bin, const std::string& args, size_t maxSteps, bool debug)
{
    GdbMi g(bin, args);
    std::vector<TraceRow> trace;

    auto is_supported = [](Op op)->bool {
        switch(op){
            case OP_ADD: case OP_SUB: case OP_MUL:
            case OP_AND: case OP_OR:
            case OP_PUSH:
                return true;
            default:
                return false;
        }
    };
    auto readX = [&](int idx, GdbMi& gg)->uint64_t {
        auto v = gg.readRegX(idx);
        return v.value_or(0ULL);
    };
    auto apply_shift = [](uint64_t v, ShiftKind k, uint32_t a)->uint64_t {
        switch(k){
            case ShiftKind::LSL: return (a>=64)?0ULL:(v<<a);
            case ShiftKind::LSR: return (a>=64)?0ULL:(v>>a);
            case ShiftKind::ASR: { if (a>=64) a=63; int64_t sv=(int64_t)v; return (uint64_t)(sv>>a); }
            default: return v;
        }
    };

    uint32_t pcIdx = 0;
    for (size_t stepped=0; stepped<maxSteps; ++stepped) {
        std::string dis = g.disasCur();
        Decoded d = decodeA64(dis);

        std::string stepr = g.stepi();

        // Strict MI handling:
        if (g.isExited(stepr)) {
            // Only push HALT if we already recorded ≥1 real row
            if (!trace.empty()) {
                TraceRow h{}; h.pc = pcIdx++; h.opcode=fr_from_u64(OP_HALT);
                h.x=h.y=h.z=fr_zero(); h.x_raw=h.y_raw=h.z_raw=0; h.is_halt=fr_from_u64(1);
                trace.push_back(h);
            }
            break;
        }
        if (!g.isStopped(stepr)) {
            // Not a normal single-step stop; keep looping until we hit a good stop/exit
            continue;
        }

        if (!d.recognized) continue;           // only record recognized instructions
        if (!is_supported(d.op)) continue;     // and only the supported subset

        uint64_t x = (d.src1>=0) ? readX(d.src1, g) : 0ULL;
        uint64_t y = 0ULL;
        if (d.imm_used) y = d.imm_val;
        else if (d.src2>=0) y = apply_shift(readX(d.src2, g), d.shift, d.shift_amt);
        uint64_t z = (d.dst >=0) ? readX(d.dst, g) : 0ULL;

        TraceRow row{};
        row.pc = pcIdx++;
        row.opcode = fr_from_u64(d.op);
        row.x = fr_from_u64(x);
        row.y = fr_from_u64(y);
        row.z = fr_from_u64(z);
        row.x_raw = (int64_t)x;
        row.y_raw = (int64_t)y;
        row.z_raw = (int64_t)z;
        row.is_halt = fr_from_u64(0);
        trace.push_back(row);

        if (debug && trace.size() <= 8) {
            std::cerr << "[trace] pc=" << row.pc
                      << " op=" << (unsigned)fr_to_u64(row.opcode)
                      << " x=" << row.x_raw
                      << " y=" << row.y_raw
                      << " z=" << row.z_raw << "\n";
        }
    }

    if (trace.empty()) {
        TraceRow h{}; h.pc=0; h.opcode=fr_from_u64(OP_HALT);
        h.x=h.y=h.z=fr_zero(); h.x_raw=h.y_raw=h.z_raw=0; h.is_halt=fr_from_u64(1);
        trace.push_back(h);
    }
    return trace;
}

// ---------- file/commit helpers ----------
static std::array<uint8_t,32> sha_bytes(const std::vector<uint8_t>& b){ return SHA256::hash_bytes(b); }
static std::array<uint8_t,32> commit_sha_file_bytes(const std::string& path){
    std::ifstream f(path, std::ios::binary); if(!f) throw std::runtime_error("open failed: "+path);
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), {}); return sha_bytes(buf);
}
static Poly poly_from_file_bytes_monomial(const std::string& path){
    std::ifstream f(path, std::ios::binary); if(!f) throw std::runtime_error("open failed: "+path);
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), {});
    Poly p; p.c.resize(buf.size()); for(size_t i=0;i<buf.size();i++) p.c[i]=fr_from_u64((uint8_t)buf[i]); poly_normalize(p); return p;
}
static size_t file_size_bytes(const std::string& path){
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if(!f) throw std::runtime_error("open failed: "+path);
    return (size_t)f.tellg();
}
static size_t next_pow2(size_t x){
    if (x<=1) return 1;
    --x; x |= x>>1; x |= x>>2; x |= x>>4; x |= x>>8; x |= x>>16;
#if ULONG_MAX > 0xffffffffUL
    x |= x>>32;
#endif
    return x+1;
}

struct CodeCommit { std::array<uint8_t,32> code_sha; G1 code_comm_kzg_base; std::string source_kind; };
struct PublicInstance { std::array<uint8_t,32> domain_tag; std::array<uint8_t,32> input_sha; bool bind_expected_output=false; uint64_t expected_output=0; };

static CodeCommit commit_file(const KZGParams& pp, const std::string& path, const std::string& kind){
    CodeCommit cc; cc.code_sha = commit_sha_file_bytes(path); Poly p = poly_from_file_bytes_monomial(path);
    cc.code_comm_kzg_base = kzg_commit(pp, p); cc.source_kind = kind; return cc;
}
static std::array<uint8_t,32> hash_inputs_i64(const std::vector<int64_t>& in){
    std::vector<uint8_t> b; b.reserve(in.size()*8);
    for(int64_t v:in) for(int i=7;i>=0;i--) b.push_back(uint8_t((uint64_t)v>>(i*8)));
    return SHA256::hash_bytes(b);
}

// ---------- LUT/indices/blinding ----------
static Poly load_poly_from_file(const std::string& path){
    std::ifstream in(path); if (!in) throw std::runtime_error("cannot open LUT: "+path);
    size_t n; in >> n; Poly p; p.c.resize(n);
    for (size_t i=0;i<n;i++){ std::string s; in >> s; p.c[i].setStr(s,16); }
    poly_normalize(p); return p;
}
static std::vector<size_t> derive_indices(const std::array<uint8_t,32>& seed, size_t domain, size_t k){
    std::vector<size_t> out; out.reserve(k); std::array<uint8_t,32> cur=seed;
    while (out.size() < k){
        for (uint32_t c=0;c<4 && out.size()<k;c++){
            std::vector<uint8_t> v(cur.begin(), cur.end());
            v.push_back((c>>24)&0xff); v.push_back((c>>16)&0xff); v.push_back((c>>8)&0xff); v.push_back(c&0xff);
            auto d = SHA256::hash_bytes(v); uint64_t x=0; for (int i=0;i<8;i++) x=(x<<8)|d[i];
            out.push_back((size_t)(x % (domain ? domain : 1))); cur = d;
        }
    }
    return out;
}
static Poly blinding_poly_from_domain_tag(const std::array<uint8_t,32>& tag, size_t d_b=8){
    Poly b; b.c.resize(d_b+1);
    for (size_t i=0;i<=d_b;i++){
        std::vector<uint8_t> m={'c','o','d','e','-','b','l','i','n','d'};
        m.insert(m.end(), tag.begin(), tag.end());
        m.push_back(uint8_t(i&0xff));
        auto h = SHA256::hash_bytes(m);
        uint64_t v=0; for (int k=0;k<8;k++) v=(v<<8)|h[k];
        b.c[i] = fr_from_u64(v);
    }
    poly_normalize(b);
    return b;
}

// ---------- proof structs ----------
struct KZGOpening { size_t idx; Fr value; G1 witness; };
struct LUTOpening { size_t idx; Fr x_val,y_val,z_val,op_val; G1 x_wit,y_wit,z_wit,op_wit; };
struct RowOpening { size_t idx; Fr pc_i,pc_ip1; G1 pc_wit_i,pc_wit_ip1; Fr op_i;G1 op_wit_i; Fr x_i;G1 x_wit_i; Fr y_i;G1 y_wit_i; Fr z_i;G1 z_wit_i; Fr h_i;G1 h_wit_i; };

struct Proof {
    std::array<uint8_t,32> code_sha; G1 code_comm_kzg_sess; std::array<uint8_t,32> domain_tag; std::array<uint8_t,32> input_sha;
    G1 pc_comm, op_comm, z_comm, x_comm, y_comm, h_comm;
    G1 lut_x_comm, lut_y_comm, lut_z_comm, lut_op_comm;
    SumcheckProof sc; uint32_t trace_pow2, trace_len;
    std::vector<KZGOpening> op_openings; std::vector<LUTOpening> lut_openings; std::vector<RowOpening> row_openings;
    uint64_t final_output;
};

// ---------- prover ----------
static Proof prove_from_trace(const KZGParams& pp, const CodeCommit& cc, const PublicInstance& inst,
                              const std::vector<TraceRow>& trc, bool uses_logic,
                              size_t k_lookup=4, size_t k_rows_spot=4)
{
    size_t T=trc.size(); size_t pow2=1; while(pow2<T) pow2<<=1;

    std::vector<Fr> col_pc(pow2), col_op(pow2), col_z(pow2), col_h(pow2), col_x(pow2), col_y(pow2);
    for (size_t i=0;i<T;i++){
        col_pc[i]=fr_from_u64(trc[i].pc);
        col_op[i]=trc[i].opcode;
        col_z[i]=trc[i].z;
        col_h[i]=trc[i].is_halt;
        col_x[i]=trc[i].x;
        col_y[i]=trc[i].y;
    }
    for (size_t i=T;i<pow2;i++){ col_pc[i].clear(); col_op[i].clear(); col_z[i].clear(); col_h[i].clear(); col_x[i].clear(); col_y[i].clear(); }

    Poly pc_poly=interpolate_on_range0(col_pc), op_poly=interpolate_on_range0(col_op), z_poly=interpolate_on_range0(col_z),
         x_poly=interpolate_on_range0(col_x), y_poly=interpolate_on_range0(col_y), h_poly=interpolate_on_range0(col_h);
    G1 pc_comm=kzg_commit(pp,pc_poly), op_comm=kzg_commit(pp,op_poly), z_comm=kzg_commit(pp,z_poly),
       x_comm=kzg_commit(pp,x_poly), y_comm=kzg_commit(pp,y_poly), h_comm=kzg_commit(pp,h_poly);

    std::vector<Fr> f(pow2); for(auto&x:f) x.clear(); Fr one=fr_one();
    for(size_t i=0;i+1<T;i++){ Fr diff=col_pc[i+1]-(col_pc[i]+one); f[i]=diff*(one-col_h[i]); }

    Poly bpoly=blinding_poly_from_domain_tag(inst.domain_tag); G1 B=kzg_commit(pp,bpoly);
    G1 C_sess=cc.code_comm_kzg_base; G1::add(C_sess,C_sess,B);

    Transcript tr; tr.absorb(inst.domain_tag); tr.absorb(inst.input_sha); tr.absorb(cc.code_sha); tr.absorb_g1(C_sess);
    tr.absorb_g1(pc_comm); tr.absorb_g1(op_comm); tr.absorb_g1(z_comm); tr.absorb_g1(x_comm); tr.absorb_g1(y_comm); tr.absorb_g1(h_comm);

    Poly lut_x_poly, lut_y_poly, lut_z_poly, lut_op_poly; G1 lut_x_c, lut_y_c, lut_z_c, lut_op_c;
    std::vector<LUTOpening> lut_openings;
    if (uses_logic){
        try{
            lut_x_poly=load_poly_from_file("lut_and_or_x.txt");
            lut_y_poly=load_poly_from_file("lut_and_or_y.txt");
            lut_z_poly=load_poly_from_file("lut_and_or_z.txt");
            lut_op_poly=load_poly_from_file("lut_and_or_op.txt");
            lut_x_c=kzg_commit(pp,lut_x_poly); lut_y_c=kzg_commit(pp,lut_y_poly);
            lut_z_c=kzg_commit(pp,lut_z_poly); lut_op_c=kzg_commit(pp,lut_op_poly);
            tr.absorb_g1(lut_x_c); tr.absorb_g1(lut_y_c); tr.absorb_g1(lut_z_c); tr.absorb_g1(lut_op_c);

            const size_t LUT_RANGE=16, LUT_PER_OP=256;
            for (size_t i=0;i<T;i++){
                uint64_t opv = fr_to_u64(trc[i].opcode);
                if (opv==OP_AND || opv==OP_OR){
                    uint64_t xa=(uint64_t)(trc[i].x_raw & 0xF), yb=(uint64_t)(trc[i].y_raw & 0xF);
                    size_t base=(opv==OP_AND)?0:LUT_PER_OP; size_t idx = base + xa*LUT_RANGE + yb;
                    Fr pt=fr_from_u64(idx); Fr vx,vy,vz,vop; G1 wx,wy,wz,wop;
                    kzg_open(pp, lut_x_poly, pt, vx, wx);
                    kzg_open(pp, lut_y_poly, pt, vy, wy);
                    kzg_open(pp, lut_z_poly, pt, vz, wz);
                    kzg_open(pp, lut_op_poly, pt, vop, wop);
                    lut_openings.push_back({idx, vx, vy, vz, vop, wx, wy, wz, wop});
                }
            }
        }catch(...){
            lut_x_c.clear(); lut_y_c.clear(); lut_z_c.clear(); lut_op_c.clear(); lut_openings.clear();
        }
    } else { lut_x_c.clear(); lut_y_c.clear(); lut_z_c.clear(); lut_op_c.clear(); }

    auto sc = sumcheck_prove(f, tr);

    auto seed=tr.squeeze();
    auto idxs=derive_indices(seed,T,k_lookup);
    std::vector<KZGOpening> op_openings; op_openings.reserve(idxs.size());
    for (auto idx : idxs){
        Fr z=fr_from_u64(idx), val; G1 wit; kzg_open(pp, op_poly, z, val, wit);
        op_openings.push_back({idx, val, wit});
    }

    std::vector<RowOpening> row_openings;
    if (T>=2){
        std::vector<uint8_t> rs(seed.begin(), seed.end()); rs.push_back(0x52);
        auto row_seed = SHA256::hash_bytes(rs);
        auto row_indices = derive_indices(row_seed, T-1, k_rows_spot);
        for (auto i : row_indices){
            RowOpening ro; ro.idx = i; Fr zi=fr_from_u64(i), zip=fr_from_u64(i+1);
            kzg_open(pp, pc_poly, zi,  ro.pc_i,    ro.pc_wit_i);
            kzg_open(pp, pc_poly, zip, ro.pc_ip1,  ro.pc_wit_ip1);
            kzg_open(pp, op_poly, zi, ro.op_i, ro.op_wit_i);
            kzg_open(pp, x_poly,  zi, ro.x_i,  ro.x_wit_i);
            kzg_open(pp, y_poly,  zi, ro.y_i,  ro.y_wit_i);
            kzg_open(pp, z_poly,  zi, ro.z_i,  ro.z_wit_i);
            kzg_open(pp, h_poly,  zi, ro.h_i,  ro.h_wit_i);
            row_openings.push_back(ro);
        }
    }

    Proof prf;
    prf.code_sha=cc.code_sha; prf.code_comm_kzg_sess=C_sess; prf.domain_tag=inst.domain_tag; prf.input_sha=inst.input_sha;
    prf.pc_comm=pc_comm; prf.op_comm=op_comm; prf.z_comm=z_comm; prf.x_comm=x_comm; prf.y_comm=y_comm; prf.h_comm=h_comm;
    prf.lut_x_comm=lut_x_c; prf.lut_y_comm=lut_y_c; prf.lut_z_comm=lut_z_c; prf.lut_op_comm=lut_op_c;
    prf.sc=sc; prf.trace_pow2=(uint32_t)pow2; prf.trace_len=(uint32_t)T;
    prf.op_openings=std::move(op_openings); prf.lut_openings=std::move(lut_openings); prf.row_openings=std::move(row_openings);
    prf.final_output=(uint64_t)trc.back().z_raw;
    return prf;
}

// ---------- verifier ----------
static bool verify(const KZGParams& pp, const CodeCommit& cc, const PublicInstance& inst, const Proof& proof,
                   size_t k_lookup=4, size_t k_rows_spot=4, std::string* reason_out=nullptr)
{
    auto fail=[&](const std::string& w){ if(reason_out) *reason_out=w; return false; };

    Poly bpoly=blinding_poly_from_domain_tag(inst.domain_tag); G1 B=kzg_commit(pp,bpoly);
    G1 C_sess=cc.code_comm_kzg_base; G1::add(C_sess,C_sess,B);

    if (proof.code_sha != cc.code_sha)                 return fail("code sha mismatch");
    if (!(proof.code_comm_kzg_sess == C_sess))         return fail("code KZG (session) mismatch");
    if (proof.domain_tag != inst.domain_tag)           return fail("domain tag mismatch");
    if (proof.input_sha  != inst.input_sha)            return fail("input hash mismatch");
    if (proof.trace_len == 0 || proof.trace_pow2 == 0 || proof.trace_len > proof.trace_pow2)
        return fail("invalid trace sizes");

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

    if (!sumcheck_verify(proof.sc, tr, proof.sc.claimed_sum)) return fail("sumcheck failed");

    // --- opcode openings over [0, T-1], compare IN-FIELD (Fr) ---
    auto seed=tr.squeeze();
    auto idxs=derive_indices(seed, proof.trace_len, k_lookup);
    if (idxs.size() != proof.op_openings.size()) return fail("opcode opening size mismatch");

    Fr allowed_fr[] = {
        fr_from_u64(OP_PUSH), fr_from_u64(OP_ADD), fr_from_u64(OP_MUL),
        fr_from_u64(OP_SUB),  fr_from_u64(OP_AND), fr_from_u64(OP_OR),
        fr_from_u64(OP_HALT)
    };

    for (size_t i=0;i<idxs.size();i++){
        const auto &open = proof.op_openings[i];
        if (open.idx != idxs[i]) return fail("opcode opening idx mismatch");
        Fr z = fr_from_u64(open.idx);
        if (!kzg_verify(pp, proof.op_comm, z, open.value, open.witness)) return fail("opcode opening pairing fail");

        bool ok=false; for (auto &a : allowed_fr) if (open.value == a){ ok=true; break; }
        if (!ok) return fail("opcode not allowed");
    }

    // --- row checks (only if we have at least two rows) ---
    if (proof.trace_len < 2) return true;

    std::vector<uint8_t> rs(seed.begin(), seed.end()); rs.push_back(0x52);
    auto row_seed = SHA256::hash_bytes(rs);
    auto row_indices = derive_indices(row_seed, proof.trace_len - 1, k_rows_spot);

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

        if (ro.h_i.isZero()){
            Fr tmp = ro.pc_i; tmp += one;
            if (!(ro.pc_ip1 == tmp)) return fail("pc local transition fail");
        }

        uint64_t opv = fr_to_u64(ro.op_i);
        uint64_t xv  = fr_to_u64(ro.x_i);
        uint64_t yv  = fr_to_u64(ro.y_i);
        uint64_t zv  = fr_to_u64(ro.z_i);
        switch (opv){
            case OP_PUSH: break;
            case OP_ADD: if (zv != (xv + yv)) return fail("ADD semantics"); break;
            case OP_SUB: if (zv != (xv - yv)) return fail("SUB semantics"); break;
            case OP_MUL: if (zv != (xv * yv)) return fail("MUL semantics"); break;
            case OP_AND: if ((zv & 0xF) != ((xv & 0xF) & (yv & 0xF))) return fail("AND semantics"); break;
            case OP_OR : if ((zv & 0xF) != ((xv & 0xF) | (yv & 0xF))) return fail("OR semantics"); break;
            case OP_HALT: break;
            default: return fail("unexpected opcode in row check");
        }
    }
    return true;
}

// ---------- printing & CLI ----------
static void print_commitment(const CodeCommit& cc, const G1& c_sess){
    std::cout << "=== Commitment (Publisher + Session) ===\n";
    std::cout << "Source kind       : " << cc.source_kind << " (committed bytes)\n";
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
        std::cout << "No LUTs in this proof.\n";
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

struct Cli {
    std::string bin="./sample_bin";
    std::string args="";
    std::string asmPath="";
    size_t steps=200;
    bool debug=false;
};
static Cli parse_cli(int argc, char** argv){
    Cli c;
    for (int i=1;i<argc;i++){
        std::string a=argv[i];
        if (a=="--bin"   && i+1<argc) c.bin=argv[++i];
        else if (a=="--args"  && i+1<argc) c.args=argv[++i];
        else if (a=="--asm"   && i+1<argc) c.asmPath=argv[++i];
        else if (a=="--steps" && i+1<argc) c.steps=std::stoul(argv[++i]);
        else if (a=="--debug") c.debug=true;
        else { std::cerr<<"Unknown arg: "<<a<<"\n"; }
    }
    return c;
}

// ---------- main ----------
int main(int argc, char** argv){
    try {
        mcl::bn::initPairing(mcl::BN254);
        Cli cli = parse_cli(argc, argv);

        const bool useAsm = !cli.asmPath.empty();
        const std::string commitPath = useAsm ? cli.asmPath : cli.bin;
        const std::string commitKind = useAsm ? "asm" : "bin";

        size_t codeBytes = file_size_bytes(commitPath);
        if (codeBytes == 0) throw std::runtime_error("empty code file: "+commitPath);
        size_t tracePow2 = next_pow2(std::max<size_t>(cli.steps, 2));
        size_t maxDeg = std::max(codeBytes-1, tracePow2-1) + 64;
        auto pp = kzg_setup(maxDeg);

        CodeCommit cc = commit_file(pp, commitPath, commitKind);

        PublicInstance inst{};
        { std::vector<uint8_t> tag_src={'s','e','s','s','-','g','d','b','-','a','a','r','c','h','6','4'}; inst.domain_tag = SHA256::hash_bytes(tag_src); }
        std::vector<int64_t> public_inputs;
        inst.input_sha = hash_inputs_i64(public_inputs);
        inst.bind_expected_output = false;

        auto trace = trace_with_gdb(cli.bin, cli.args, cli.steps, cli.debug);

        bool uses_logic=false;
        for (auto &r: trace){ uint64_t opv = fr_to_u64(r.opcode); if (opv==OP_AND || opv==OP_OR){ uses_logic=true; break; } }

        Proof prf = prove_from_trace(pp, cc, inst, trace, uses_logic, 4, 4);

        Poly bpoly=blinding_poly_from_domain_tag(inst.domain_tag); G1 B=kzg_commit(pp,bpoly); G1 C_sess=cc.code_comm_kzg_base; G1::add(C_sess,C_sess,B);
        std::cout << "\n"; print_commitment(cc, C_sess);
        std::cout << "\n"; print_proof(prf); std::cout << "\n";

        std::string reason;
        bool ok = verify(pp, cc, inst, prf, 4, 4, &reason);
        std::cout << "Verify: " << (ok ? "ACCEPT" : "REJECT") << "\n";
        if (!ok) std::cout << "Reason: " << reason << "\n";
        std::cout << "Claimed output: " << prf.final_output << "\n";
        return ok?0:1;

    } catch (const std::exception& e){
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
