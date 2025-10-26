// ---------------------------------------------------------------------------
// BUILD:  g++ -O2 -std=c++17 zkvm_lookup_arm64.cpp -o zkvm
// RUN:    ./zkvm trace.csv
//
// CSV format expected (one ADD/MOV/CMP per line):
//   step,pc,insn,rd,rn,rm,before_rd,before_rn,before_rm,after_rd
// Examples:
//   0,0x400804,ADD,X0,X1,X2,0,5,7,12
//   1,0x400808,MOV,X3,_,_,0,_,_,5
//   2,0x40080C,CMP,_,X4,X5,_,9,9,1   (after_rd is 1 if equal else 0, used for EQ)
//
// For ADD: we *prove* after_rd == before_rn + before_rm (mod 2^64) using nibble lookups.
// For MOV: we check after_rd equals source; for CMP/EQ we check equality bit.
// ---------------------------------------------------------------------------

#include <bits/stdc++.h>
using namespace std;

// ========================= Hash (SHA-256) ===============================
#include <openssl/sha.h>
static inline array<uint64_t,2> hash_bytes(const vector<uint8_t>& v){
    unsigned char d[32];
    SHA256(v.data(), v.size(), d);
    auto load64=[&](const unsigned char* p){ return
        (uint64_t)p[0] | ((uint64_t)p[1]<<8) | ((uint64_t)p[2]<<16) | ((uint64_t)p[3]<<24) |
        ((uint64_t)p[4]<<32) | ((uint64_t)p[5]<<40) | ((uint64_t)p[6]<<48) | ((uint64_t)p[7]<<56); };
    uint64_t a = load64(d), b = load64(d+8);
    return {a,b};
}
static inline array<uint64_t,2> hash_pair(const array<uint64_t,2>& A, const array<uint64_t,2>& B){
    vector<uint8_t> buf; buf.reserve(32);
    auto push64=[&](uint64_t x){ for(int i=0;i<8;i++) buf.push_back((x>>(i*8)) & 0xFF); };
    push64(A[0]); push64(A[1]); push64(B[0]); push64(B[1]);
    return hash_bytes(buf);
}

// ========================= Merkle tree ======================================
struct Merkle{
    vector<array<uint64_t,2>> leaves;
    vector<vector<array<uint64_t,2>>> levels;

    static array<uint64_t,2> leaf_hash(const vector<uint8_t>& bytes){ return hash_bytes(bytes); }

    void build(const vector<vector<uint8_t>>& rows){
        leaves.resize(rows.size());
        for(size_t i=0;i<rows.size();++i) leaves[i]=leaf_hash(rows[i]);
        levels.clear();
        levels.push_back(leaves);
        while(levels.back().size()>1){
            const auto &cur=levels.back();
            vector<array<uint64_t,2>> nxt; nxt.reserve((cur.size()+1)/2);
            for(size_t i=0;i<cur.size(); i+=2){
                if(i+1<cur.size()) nxt.push_back(hash_pair(cur[i],cur[i+1]));
                else nxt.push_back(hash_pair(cur[i],cur[i]));
            }
            levels.push_back(move(nxt));
        }
    }

    array<uint64_t,2> root() const{ return levels.empty()? array<uint64_t,2>{0,0}: levels.back()[0]; }

    vector<array<uint64_t,2>> open(size_t i) const{
        vector<array<uint64_t,2>> path; size_t idx=i;
        for(size_t h=0; h+1<levels.size(); ++h){
            const auto &cur=levels[h];
            size_t sib = (idx^1);
            if(sib>=cur.size()) path.push_back(cur[idx]); else path.push_back(cur[sib]);
            idx >>= 1;
        }
        return path;
    }

    static bool verify_open(const array<uint64_t,2>& leaf, const vector<array<uint64_t,2>>& path, size_t idx, const array<uint64_t,2>& root){
        array<uint64_t,2> h=leaf; size_t i=idx;
        for(const auto& sib: path){
            if((i&1)==0) h = hash_pair(h, sib); else h = hash_pair(sib, h);
            i >>= 1;
        }
        return h==root;
    }
};

// ========================= Transcript =======================================
struct Transcript{
    vector<uint8_t> st;
    void absorb_u64(uint64_t x){ for(int i=0;i<8;i++) st.push_back((x>>(i*8)) & 0xFF); }
    uint64_t squeeze_u64(){ auto h=hash_bytes(st); st.clear(); st.reserve(16);
        for(int i=0;i<8;i++) st.push_back((h[0]>>(i*8))&0xFF);
        for(int i=0;i<8;i++) st.push_back((h[1]>>(i*8))&0xFF);
        return h[0]^h[1]; }
};

// ========================= Fixed nibble add lookup table =====================
// Table entries are (a,b,cin,sum,cout) with a,b,sum in [0..15], cin,cout in {0,1}.
// We build this deterministically on both prover and verifier side and Merkle‑commit it.

struct NibbleTable{
    struct Row{ uint8_t a,b,cin,sum,cout; };
    vector<Row> rows;              // 512 rows
    vector<vector<uint8_t>> ser;   // serialized rows for Merkle
    Merkle merkle;

    static vector<uint8_t> ser_row(const Row& r){ return {r.a,r.b,r.cin,r.sum,r.cout}; }

    void build(){
        rows.clear(); rows.reserve(16*16*2);
        for(int a=0;a<16;a++) for(int b=0;b<16;b++) for(int cin=0;cin<2;cin++){
            int s = a + b + cin; Row r; r.a=a; r.b=b; r.cin=cin; r.sum = s & 0xF; r.cout = (s>>4)&1; rows.push_back(r);
        }
        ser.clear(); ser.reserve(rows.size());
        for(const auto& r: rows) ser.push_back(ser_row(r));
        merkle.build(ser);
    }

    array<uint64_t,2> root() const { return merkle.root(); }

    // Return index of row (a,b,cin,sum,cout) deterministically
    static size_t index_of(uint8_t a,uint8_t b,uint8_t cin,uint8_t sum,uint8_t cout){
        // The table is ordered lexicographically by a,b,cin
        int s = (a + b + cin); return (size_t)(a*32 + b*2 + cin);
        (void)sum; (void)cout; // implied by a,b,cin; included to bind leaf bytes
    }

    vector<array<uint64_t,2>> open(uint8_t a,uint8_t b,uint8_t cin,uint8_t sum,uint8_t cout) const{
        size_t idx = index_of(a,b,cin,sum,cout);
        auto leaf_bytes = ser[idx];
        (void)leaf_bytes; // just for clarity
        return merkle.open(idx);
    }

    static bool verify_open(uint8_t a,uint8_t b,uint8_t cin,uint8_t sum,uint8_t cout,
                            const vector<array<uint64_t,2>>& path, const array<uint64_t,2>& root){
        vector<uint8_t> bytes = ser_row({a,b,cin,sum,cout});
        auto leaf = Merkle::leaf_hash(bytes);
        size_t idx = index_of(a,b,cin,sum,cout);
        return Merkle::verify_open(leaf, path, idx, root);
    }
};

// ========================= ARM64 trace row ==================================
// We track only what we need for checks. For ADD Xd, Xn, Xm we store operands & result.

enum InsnKind : uint8_t { IK_ADD=1, IK_MOV=2, IK_CMP_EQ=3, IK_SUB=4, IK_AND=5, IK_ORR=6, IK_EOR=7, IK_LSL=8, IK_LSR=9 };

struct TraceRow{
    uint64_t step; uint64_t pc;
    InsnKind kind;
    // registers (names are informational only)
    string rd, rn, rm;
    uint64_t before_rd=0, before_rn=0, before_rm=0, after_rd=0;
};

static vector<uint8_t> ser_trace_row(const TraceRow& r){
    vector<uint8_t> v; v.reserve(8*6+3);
    auto push64=[&](uint64_t x){ for(int i=0;i<8;i++) v.push_back((x>>(i*8))&0xFF); };
    push64(r.step); push64(r.pc); v.push_back((uint8_t)r.kind);
    push64(r.before_rd); push64(r.before_rn); push64(r.before_rm); push64(r.after_rd);
    return v;
}

// ========================= Proof objects ====================================
struct AddNibbleOpen{ // per nibble
    uint8_t a,b,cin,sum,cout;               // the tuple
    vector<array<uint64_t,2>> auth_path;    // Merkle path into nibble table
};

struct Proof{
    array<uint64_t,2> trace_root;
    size_t trace_len=0;
    vector<size_t> sample_indices;          // Fiat–Shamir sampled steps (not 0)
    // For each sampled ADD row, include 16 nibble openings
    vector<vector<AddNibbleOpen>> add_openings; // aligned with sample_indices; empty for non‑ADD
    array<uint64_t,2> nibble_root;          // public constant (can be recomputed)
};

// ========================= Parser for CSV trace ==============================
static inline string trim(const string& s){ size_t a=0; while(a<s.size() && isspace((unsigned char)s[a])) a++; size_t b=s.size(); while(b>a && isspace((unsigned char)s[b-1])) b--; return s.substr(a,b-a); }

static bool parse_csv_line(const string& line, TraceRow& out){
    // step,pc,insn,rd,rn,rm,before_rd,before_rn,before_rm,after_rd
    vector<string> t; string cur; bool inq=false;
    for(char c: line){ if(c=='"') inq=!inq; else if(c==',' && !inq){ t.push_back(cur); cur.clear(); } else cur.push_back(c);} t.push_back(cur);
    if(t.size()<10) return false;
    TraceRow r; r.step = strtoull(t[0].c_str(),nullptr,10);
    r.pc = strtoull(t[1].c_str(),nullptr,0);
    string insn = trim(t[2]);
    for(auto &ch: insn) ch = toupper((unsigned char)ch);
    r.rd = trim(t[3]); r.rn = trim(t[4]); r.rm = trim(t[5]);
    auto parse_u64=[&](const string& s){ string u=trim(s); if(u=="_"||u=="") return 0ULL; return strtoull(u.c_str(),nullptr,0); };
    r.before_rd = parse_u64(t[6]); r.before_rn=parse_u64(t[7]); r.before_rm=parse_u64(t[8]); r.after_rd=parse_u64(t[9]);
    if(insn=="ADD") r.kind=IK_ADD;
    else if(insn=="SUB") r.kind=IK_SUB;
    else if(insn=="MOV") r.kind=IK_MOV;
    else if(insn=="CMP"||insn=="EQ") r.kind=IK_CMP_EQ;
    else if(insn=="AND") r.kind=IK_AND;
    else if(insn=="ORR"||insn=="OR") r.kind=IK_ORR;
    else if(insn=="EOR"||insn=="XOR") r.kind=IK_EOR;
    else if(insn=="LSL") r.kind=IK_LSL;
    else if(insn=="LSR") r.kind=IK_LSR;
    else return false;
    out=r; return true;
}

// ========================= Prover ===========================================
struct Prover{
    static Proof prove_from_trace(const vector<TraceRow>& trace, size_t num_queries=16){
        // 1) Commit to execution trace
        vector<vector<uint8_t>> ser_rows; ser_rows.reserve(trace.size());
        for(const auto& r: trace) ser_rows.push_back(ser_trace_row(r));
        Merkle trace_merkle; trace_merkle.build(ser_rows);

        // 2) Build fixed nibble table (CRS‑like)
        NibbleTable T; T.build();

        // 3) Fiat–Shamir sample indices (avoid 0 for predecessor; we only do single‑row checks here)
        Transcript tr; tr.absorb_u64(trace_merkle.root()[0]); tr.absorb_u64(trace_merkle.root()[1]); tr.absorb_u64(trace.size());
        vector<size_t> idxs; idxs.reserve(num_queries);
        size_t Tlen = trace.size(); if(Tlen==0) throw runtime_error("empty trace");
        for(size_t i=0;i<num_queries;i++){ size_t j = (size_t)(tr.squeeze_u64()%Tlen); if(j==0) j=1; idxs.push_back(j); }

        // 4) For sampled ADD rows, include 16 nibble lookups proving after == rn + rm mod 2^64
        vector<vector<AddNibbleOpen>> add_openings; add_openings.reserve(idxs.size());
        for(size_t j: idxs){
            const auto& r = trace[j];
            vector<AddNibbleOpen> opens;
            if(r.kind==IK_ADD){
                uint64_t a=r.before_rn, b=r.before_rm, out=r.after_rd; uint8_t cin=0;
                for(int k=0;k<16;k++){
                    uint8_t an=(a>>(4*k))&0xF, bn=(b>>(4*k))&0xF;
                    uint16_t s = an + bn + cin; uint8_t sum = s & 0xF; uint8_t cout=(s>>4)&1;
                    uint8_t outn = (out>>(4*k))&0xF; (void)outn; // verifier will check sum==out nibble
                    AddNibbleOpen one; one.a=an; one.b=bn; one.cin=cin; one.sum=sum; one.cout=cout; one.auth_path = T.open(an,bn,cin,sum,cout);
                    opens.push_back(move(one));
                    cin = cout;
                }
            }
            add_openings.push_back(move(opens));
        }

        Proof P; P.trace_root = trace_merkle.root(); P.trace_len = trace.size(); P.sample_indices = move(idxs); P.add_openings = move(add_openings); P.nibble_root = T.root();
        return P;
    }
};

// ========================= Verifier =========================================
static bool check_add_via_lookups(uint64_t a,uint64_t b,uint64_t out,
                                  const vector<AddNibbleOpen>& opens,
                                  const array<uint64_t,2>& nibble_root){
    if(opens.size()!=16) return false;
    uint8_t cin=0; for(int k=0;k<16;k++){
        const auto& o = opens[k];
        // Check the opening is valid against the fixed table root
        if(!NibbleTable::verify_open(o.a,o.b,o.cin,o.sum,o.cout, o.auth_path, nibble_root)) return false;
        // Check link to operands/outcome
        uint8_t an=(a>>(4*k))&0xF, bn=(b>>(4*k))&0xF, outn=(out>>(4*k))&0xF;
        if(o.a!=an || o.b!=bn || o.cin!=cin) return false;
        if(o.sum!=outn) return false; // sum nibble must match output nibble
        // propagate carry
        cin = o.cout;
    }
    return true;
}

struct Verifier{
    static bool verify(const Proof& P, const vector<TraceRow>& trace){
        // Rebuild trace Merkle to check root
        vector<vector<uint8_t>> ser_rows; ser_rows.reserve(trace.size());
        for(const auto& r: trace) ser_rows.push_back(ser_trace_row(r));
        Merkle trace_merkle; trace_merkle.build(ser_rows);
        if(trace_merkle.root()!=P.trace_root) return false;

        // Rebuild nibble table to get expected root
        NibbleTable T; T.build(); if(T.root()!=P.nibble_root) return false;

        if(P.sample_indices.size()!=P.add_openings.size()) return false;

        for(size_t k=0;k<P.sample_indices.size();++k){
            size_t j = P.sample_indices[k]; if(j>=trace.size()) return false;
            const auto& row = trace[j];
            switch(row.kind){
                case IK_ADD:
                    if(!check_add_via_lookups(row.before_rn,row.before_rm,row.after_rd, P.add_openings[k], P.nibble_root)) return false; break;
                case IK_SUB: {
                    // Two's complement: a + (~b) + 1
                    uint64_t twos = (~row.before_rm) + 1ULL;
                    uint64_t out = row.before_rn + twos; // mod 2^64
                    if(out != row.after_rd) return false; break; }
                case IK_MOV: if(row.after_rd != row.before_rn) return false; break;
                case IK_CMP_EQ: if(row.after_rd != (uint64_t)(row.before_rn==row.before_rm)) return false; break;
                case IK_AND: if(row.after_rd != (row.before_rn & row.before_rm)) return false; break;
                case IK_ORR: if(row.after_rd != (row.before_rn | row.before_rm)) return false; break;
                case IK_EOR: if(row.after_rd != (row.before_rn ^ row.before_rm)) return false; break;
                case IK_LSL: { uint64_t sh = row.before_rm & 63ULL; if(row.after_rd != (row.before_rn << sh)) return false; break; }
                case IK_LSR: { uint64_t sh = row.before_rm & 63ULL; if(row.after_rd != (row.before_rn >> sh)) return false; break; }
                default: return false;
            }
        }
        return true;
    }
};


// ========================= (Optional) SNARK path with libsnark (Groth16) ====
// Enable by compiling with -DUSE_LIBSNARK and linking libsnark & libff (BN128)
// This circuit proves, for all sampled ADD rows j in the Fiat–Shamir set, that
// after_rd[j] = before_rn[j] + before_rm[j] (mod 2^64)
// using a 16-nibble carry chain with boolean constraints — a *real* ZK proof.
// (It does not verify Merkle paths or do a lookup argument inside the circuit; it
// enforces the same relation as the lookup table algebraically.)
//
// Build example (Ubuntu):
// g++ -O2 -std=c++17 zkvm_lookup_arm64.cpp -DUSE_LIBSNARK \
// -I/usr/local/include -L/usr/local/lib \
// -lsnark -lff -lgmp -lprocps -lpthread -o zkvm
#ifdef USE_LIBSNARK
using ppT = libff::alt_bn128_pp; ppT::init_public_params();
GrothCircuit GC; // build circuit


// Allocate one circuit object per sampled row we want to prove
size_t add_samples=0, sub_samples=0, and_samples=0, orr_samples=0, eor_samples=0;
for(size_t idx=0; idx<proof.sample_indices.size(); ++idx){
const auto& row = trace[ proof.sample_indices[idx] ];
if(row.kind==IK_ADD){ GC.add_add(); add_samples++; }
else if(row.kind==IK_SUB){ GC.add_sub(); sub_samples++; }
else if(row.kind==IK_AND){ GC.add_logic(1); and_samples++; }
else if(row.kind==IK_ORR){ GC.add_logic(2); orr_samples++; }
else if(row.kind==IK_EOR){ GC.add_logic(3); eor_samples++; }
}


// Assign witnesses
size_t add_i=0, sub_i=0, and_i=0, orr_i=0, eor_i=0;
for(size_t k=0;k<proof.sample_indices.size();++k){
const auto& row = trace[ proof.sample_indices[k] ];
if(row.kind==IK_ADD){ auto &w = GC.adds[add_i++];
uint64_t a=row.before_rn, b=row.before_rm, out=row.after_rd; uint8_t cin=0;
for(int i=0;i<16;i++){
uint8_t an=(a>>(4*i))&0xF, bn=(b>>(4*i))&0xF; uint16_t s = an + bn + cin; uint8_t sn = s & 0xF; uint8_t cout=(s>>4)&1;
GC.pb.val(w.an[i]) = libff::Fr<ppT>(an);
GC.pb.val(w.bn[i]) = libff::Fr<ppT>(bn);
GC.pb.val(w.sn[i]) = libff::Fr<ppT>( (out>>(4*i)) & 0xF );
GC.pb.val(w.carry[i+1]) = libff::Fr<ppT>(cout);
cin = cout;
}
GC.pb.val(w.a)=libff::Fr<ppT>(a); GC.pb.val(w.b)=libff::Fr<ppT>(b); GC.pb.val(w.out)=libff::Fr<ppT>(out);
}
else if(row.kind==IK_SUB){ auto &w = GC.subs[sub_i++];
uint64_t a=row.before_rn, b=row.before_rm, out=row.after_rd; uint8_t cin=1;
for(int i=0;i<16;i++){
uint8_t an=(a>>(4*i))&0xF, bn=(b>>(4*i))&0xF; uint8_t bnot = 15 - bn; uint16_t s = an + bnot + cin; uint8_t sn = s & 0xF; uint8_t cout=(s>>4)&1;
GC.pb.val(w.an[i]) = libff::Fr<ppT>(an);
GC.pb.val(w.bn[i]) = libff::Fr<ppT>(bn);
GC.pb.val(w.sn[i]) = libff::Fr<ppT>( (out>>(4*i)) & 0xF );
GC.pb.val(w.carry[i+1]) = libff::Fr<ppT>(cout);
cin = cout;
}
GC.pb.val(w.a)=libff::Fr<ppT>(a); GC.pb.val(w.b)=libff::Fr<ppT>(b); GC.pb.val(w.out)=libff::Fr<ppT>(out);
}
else if(row.kind==IK_AND || row.kind==IK_ORR || row.kind==IK_EOR){
auto &w = (row.kind==IK_AND)? GC.logics[and_i++] : (row.kind==IK_ORR? GC.logics[orr_i++] : GC.logics[eor_i++]);
uint64_t a=row.before_rn, b=row.before_rm, out=row.after_rd;
for(int i=0;i<64;i++){
GC.pb.val(w.abits[i]) = libff::Fr<ppT>((a>>i)&1);
GC.pb.val(w.bbits[i]) = libff::Fr<ppT>((b>>i)&1);
GC.pb.val(w.sbits[i]) = libff::Fr<ppT>((out>>i)&1);
}
GC.pb.val(w.a)=libff::Fr<ppT>(a); GC.pb.val(w.b)=libff::Fr<ppT>(b); GC.pb.val(w.out)=libff::Fr<ppT>(out);
}