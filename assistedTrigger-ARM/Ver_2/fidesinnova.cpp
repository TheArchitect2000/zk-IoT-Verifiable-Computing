// fidesinnova.cpp
//
// Production-oriented CLI for (1) code commitment, (2) proof via GDB instruction trace,
// and (3) verification that the proof binds to the commitment and is locally consistent,
// now with a real KZG polynomial commitment + opening (BN254 via mcl).
//
// FIX / NEW IN THIS VERSION
// -------------------------
// - Correct MI token handling: waitResult returns on N^done / N^error / N^running and on *stopped.
// - Normalizes MI send so commands are always "N-<cmd>" (avoids accidental "--").
// - Removes fragile quoting in MI commands (file-exec-and-symbols).
// - Adds robust debug breadcrumbs to trace MI init and stepping.
// - Uses 8-byte read/disasm window for variable-length safety.
// - SHA-256 via OpenSSL EVP (no 3.0 deprecation warnings).
// - MI "inst" text is unescaped ("\\t" -> tab, etc.) before mnemonic parse.
// - Skip logs show the parsed mnemonic for fast diagnosis.
// - **NEW:** KZG commitment of the trace polynomial, Fiat–Shamir point z, opening (y,pi), and verification.
//
// BUILD
// -----
// Dependencies:
//   - OpenSSL (for SHA-256 EVP)
//   - Herumi mcl (BN254)  https://github.com/herumi/mcl
//
// Install mcl (typical):
//   git clone --depth=1 https://github.com/herumi/mcl
//   cd mcl && mkdir build && cd build
//   cmake .. -DMCL_STATIC_LIB=ON -DMCL_USE_OPENSSL=ON
//   make -j && sudo make install
//
// Build this tool:
//   g++ -std=gnu++17 -O2 -g fidesinnova.cpp -lmcl -lcrypto -o fidesinnova
//
// USAGE
// -----
//   ./fidesinnova -c <program.s>
//   ./fidesinnova -p <program> [--steps N] [--debug] [--domain TAG]
//   ./fidesinnova -v <program.com> <program.prf>
//
// -----------------------------------------------------------------------------

#include <bits/stdc++.h>
#include <openssl/evp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <mcl/bn.hpp>

static bool g_debug = false;
static void dbg(const std::string& s){ if(g_debug) fprintf(stderr, "[DBG] %s\n", s.c_str()); }

// ----------------------------- utils -----------------------------------------
static std::string to_hex(const std::string& s){
    static const char* H="0123456789abcdef";
    std::string o; o.reserve(s.size()*2);
    for(unsigned char c: s){ o.push_back(H[c>>4]); o.push_back(H[c&0xf]); }
    return o;
}
static std::string hex_of_bytes(const std::vector<uint8_t>& v){
    static const char* H="0123456789abcdef";
    std::string o; o.reserve(v.size()*2);
    for(unsigned char c: v){ o.push_back(H[c>>4]); o.push_back(H[c&0xf]); }
    return o;
}
static std::vector<uint8_t> parse_hex_bytes(const std::string& h){
    std::vector<uint8_t> v; v.reserve(h.size()/2);
    auto hv=[&](char c)->int{
        if('0'<=c&&c<='9') return c-'0';
        if('a'<=c&&c<='f') return 10+(c-'a');
        if('A'<=c&&c<='F') return 10+(c-'A');
        return -1;
    };
    for(size_t i=0;i+1<h.size();i+=2){
        int hi=hv(h[i]), lo=hv(h[i+1]);
        if(hi<0||lo<0) break;
        v.push_back(uint8_t((hi<<4)|lo));
    }
    return v;
}
static std::string read_file(const std::string& path){
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss; ss<<f.rdbuf();
    return ss.str();
}
static bool write_file(const std::string& path, const std::string& data){
    std::ofstream f(path, std::ios::binary);
    if(!f) return false;
    f<<data;
    return true;
}
static std::string basename_no_ext(const std::string& p){
    auto slash = p.find_last_of('/');
    std::string b = (slash==std::string::npos) ? p : p.substr(slash+1);
    auto dot = b.find_last_of('.');
    return (dot==std::string::npos)? b : b.substr(0,dot);
}
static std::string random_hex(size_t nbytes){
    std::string s; s.resize(nbytes);
    std::ifstream ur("/dev/urandom", std::ios::binary); ur.read(&s[0], s.size());
    return to_hex(s);
}

// OpenSSL EVP-based SHA-256
static std::string sha256_bytes_raw(const void* data, size_t len){
    unsigned char h[EVP_MAX_MD_SIZE];
    unsigned int hlen=0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    if(len) EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, h, &hlen);
    EVP_MD_CTX_free(ctx);
    return to_hex(std::string(reinterpret_cast<char*>(h), hlen));
}
static std::string sha256_of_bytes(const std::vector<uint8_t>& v){
    return sha256_bytes_raw(v.data(), v.size());
}
static std::string sha256_of_string(const std::string& s){
    return sha256_bytes_raw(s.data(), s.size());
}

// --------------------------- Minimal JSON helpers -----------------------------
static std::string json_escape(const std::string& s){
    std::string o; o.reserve(s.size()+16);
    for(char c: s){
        switch(c){
            case '\\': o+="\\\\"; break;
            case '\"': o+="\\\""; break;
            case '\n': o+="\\n"; break;
            case '\r': o+="\\r"; break;
            case '\t': o+="\\t"; break;
            default: o.push_back(c); break;
        }
    }
    return o;
}
struct Json{
    static std::string kv(const std::string& k, const std::string& v, bool str=true){
        return "\""+json_escape(k)+"\":" + (str? ("\""+json_escape(v)+"\"") : v);
    }
};

// ---------------------------- Commitment format ------------------------------
struct Commitment {
    std::string code_sha;     // sha256 of .s (or your code bytes)
    std::string salt;         // per-publish randomness
    std::string session_hash; // sha256(code_sha || salt)

    std::string to_json() const {
        std::ostringstream o;
        o<<"{\n"
         <<"  "<<Json::kv("code_sha", code_sha)<<",\n"
         <<"  "<<Json::kv("salt", salt)<<",\n"
         <<"  "<<Json::kv("session_hash", session_hash)<<"\n"
         <<"}\n";
        return o.str();
    }
    static Commitment from_json(const std::string& s){
        Commitment c; auto get=[&](const char* key)->std::string{
            auto k = std::string("\"")+key+"\"";
            auto p = s.find(k); if(p==std::string::npos) return "";
            p = s.find(':', p); if(p==std::string::npos) return "";
            p++;
            while(p<s.size() && (s[p]==' '||s[p]=='\t')) p++;
            bool quoted = p<s.size() && s[p]=='\"'; if(quoted) p++;
            std::string v;
            while(p<s.size()){
                char ch=s[p++];
                if(quoted){ if(ch=='\"') break; v.push_back(ch); }
                else { if(ch==','||ch=='}'||ch=='\n') break; v.push_back(ch); }
            }
            return v;
        };
        c.code_sha = get("code_sha");
        c.salt = get("salt");
        c.session_hash = get("session_hash");
        return c;
    }
};

// ------------------------------ Proof format ---------------------------------
struct TraceRow{
    uint64_t pc=0;
    std::vector<uint8_t> bytes;
    std::string asm_text;
};
struct Proof{
    std::string exe_path;
    std::string domain_tag;
    std::string code_sha;
    uint64_t    steps=0;
    std::vector<TraceRow> rows;

    // KZG fields
    std::string kzg_commit_g1;  // hex-encoded G1 commitment
    std::string kzg_z_fr;       // Fr as hex string (base-16)
    std::string kzg_y_fr;       // Fr as hex string (base-16)
    std::string kzg_pi_g1;      // hex-encoded G1 opening
    std::string srs_hex;        // SRS saved text (hex lines)

    std::string to_json() const {
        std::ostringstream o;
        o<<"{\n"
         <<"  "<<Json::kv("exe_path", exe_path)<<",\n"
         <<"  "<<Json::kv("domain_tag", domain_tag)<<",\n"
         <<"  "<<Json::kv("code_sha", code_sha)<<",\n"
         <<"  "<<Json::kv("steps", std::to_string(steps), false)<<",\n"
         <<"  \"rows\":[\n";
        for(size_t i=0;i<rows.size();i++){
            const auto& r=rows[i];
            o<<"    {"
             <<Json::kv("pc", std::to_string(r.pc), false)<<","
             <<Json::kv("bytes", hex_of_bytes(r.bytes))<<","
             <<Json::kv("asm", r.asm_text)
             <<"}" << (i+1<rows.size()? ",":"") << "\n";
        }
        o<<"  ],\n"
         <<"  "<<Json::kv("kzg_commit_g1", kzg_commit_g1)<<",\n"
         <<"  "<<Json::kv("kzg_z_fr",      kzg_z_fr)<<",\n"
         <<"  "<<Json::kv("kzg_y_fr",      kzg_y_fr)<<",\n"
         <<"  "<<Json::kv("kzg_pi_g1",     kzg_pi_g1)<<",\n"
         <<"  "<<Json::kv("srs_hex",       srs_hex)<<"\n"
         <<"}\n";
        return o.str();
    }
    static Proof from_json(const std::string& s){
        Proof p;
        auto get=[&](const char* key)->std::string{
            auto k = std::string("\"")+key+"\"";
            auto pos = s.find(k); if(pos==std::string::npos) return "";
            pos = s.find(':', pos); if(pos==std::string::npos) return "";
            pos++;
            while(pos<s.size() && (s[pos]==' '||s[pos]=='\t')) pos++;
            bool quoted = pos<s.size() && s[pos]=='\"'; if(quoted) pos++;
            std::string v;
            while(pos<s.size()){
                char ch=s[pos++];
                if(quoted){ if(ch=='\"') break; v.push_back(ch); }
                else { if(ch==','||ch=='}'||ch=='\n') break; v.push_back(ch); }
            }
            return v;
        };
        p.exe_path   = get("exe_path");
        p.domain_tag = get("domain_tag");
        p.code_sha   = get("code_sha");
        // steps
        {
            auto k="\"steps\"";
            auto q=s.find(k);
            if(q!=std::string::npos){
                q=s.find(':',q); if(q!=std::string::npos){
                    q++;
                    while(q<s.size() && isspace((unsigned char)s[q])) q++;
                    uint64_t v=0;
                    while(q<s.size() && isdigit((unsigned char)s[q])){ v = v*10 + (s[q]-'0'); q++; }
                    p.steps=v;
                }
            }
        }
        // rows (simple parse of our own format)
        auto rpos = s.find("\"rows\"");
        if(rpos!=std::string::npos){
            rpos = s.find('[', rpos);
            auto rend = s.find(']', rpos);
            if(rpos!=std::string::npos && rend!=std::string::npos){
                std::string block = s.substr(rpos+1, rend-rpos-1);
                size_t cur=0;
                while(true){
                    auto lb = block.find('{', cur);
                    if(lb==std::string::npos) break;
                    auto rb = block.find('}', lb);
                    if(rb==std::string::npos) break;
                    std::string row = block.substr(lb+1, rb-lb-1);
                    TraceRow tr;
                    auto getr=[&](const char* key)->std::string{
                        auto k=std::string("\"")+key+"\"";
                        auto p2=row.find(k);
                        if(p2==std::string::npos) return "";
                        p2=row.find(':',p2); if(p2==std::string::npos) return "";
                        p2++;
                        while(p2<row.size() && isspace((unsigned char)row[p2])) p2++;
                        bool quoted = p2<row.size() && row[p2]=='\"'; if(quoted) p2++;
                        std::string v;
                        while(p2<row.size()){
                            char ch=row[p2++];
                            if(quoted){ if(ch=='\"') break; v.push_back(ch); }
                            else { if(ch==','||ch=='}') break; v.push_back(ch); }
                        }
                        return v;
                    };
                    { std::string pc = getr("pc"); tr.pc = pc.empty()?0:std::stoull(pc); }
                    tr.bytes = parse_hex_bytes(getr("bytes"));
                    tr.asm_text = getr("asm");
                    p.rows.push_back(std::move(tr));
                    cur = rb+1;
                }
            }
        }

        p.kzg_commit_g1 = get("kzg_commit_g1");
        p.kzg_z_fr      = get("kzg_z_fr");
        p.kzg_y_fr      = get("kzg_y_fr");
        p.kzg_pi_g1     = get("kzg_pi_g1");
        p.srs_hex       = get("srs_hex");
        return p;
    }
};

// ------------------------------- MI helpers -----------------------------------
static inline bool mi_ok(const std::string& s){
    return s.find("^done") != std::string::npos;
}
static inline bool mi_err(const std::string& s){
    return s.find("^error") != std::string::npos;
}
static inline bool mi_running(const std::string& s){
    return s.find("^running") != std::string::npos || s.find("running") != std::string::npos;
}
static std::string mi_unescape(std::string s){
    std::string o; o.reserve(s.size());
    for(size_t i=0; i<s.size(); ++i){
        if(s[i]=='\\' && i+1<s.size()){
            char c = s[++i];
            switch(c){
                case 't': o.push_back('\t'); break;
                case 'n': o.push_back('\n'); break;
                case 'r': o.push_back('\r'); break;
                case '"': o.push_back('"');  break;
                case '\\': o.push_back('\\'); break;
                default:   o.push_back(c);   break;
            }
        }else{
            o.push_back(s[i]);
        }
    }
    return o;
}

// ------------------------------- GDB/MI2 --------------------------------------
class GdbMi {
    FILE* in_=nullptr; FILE* out_=nullptr;
    bool alive_=false, stopped_=false;
    int token_=1;

    std::string readChunk(int fd, int ms){
        std::string out;
        for(;;){
            fd_set r; FD_ZERO(&r); FD_SET(fd,&r);
            struct timeval tv; tv.tv_sec=0; tv.tv_usec = ms*1000;
            int rc = select(fd+1, &r, nullptr, nullptr, &tv);
            if(rc<=0) break;
            char buf[4096];
            ssize_t n= ::read(fd, buf, sizeof(buf));
            if(n<=0) break;
            out.append(buf, buf+n);
            if((int)n < (int)sizeof(buf)) break;
        }
        return out;
    }

    // Return on tokened ^done/^error/^running OR when *stopped appears.
    std::string waitResult(int myTok, const std::string& issued){
        int fd = fileno(out_);
        std::string acc;
        const std::string tok = std::to_string(myTok);
        const std::string tdone = tok + "^done";
        const std::string terr  = tok + "^error";
        const std::string trun  = tok + "^running";

        for(;;){
            std::string chunk = readChunk(fd, 200);
            if(chunk.empty()){
                usleep(2000);
            }else{
                if(g_debug) dbg(std::string("MI<< ")+chunk);
                acc += chunk;
            }
            if(acc.find(tdone)!=std::string::npos ||
               acc.find(terr)!=std::string::npos  ||
               acc.find(trun)!=std::string::npos  ||
               acc.find("*stopped")!=std::string::npos ||
               acc.find("^done")!=std::string::npos ||
               acc.find("^error")!=std::string::npos ||
               acc.find("^running")!=std::string::npos){
                return acc;
            }
        }
    }

    std::string miDo(const std::string& cmd){
        int myTok = token_++;

        // Normalize: strip a leading '-' if the caller provided one.
        std::string norm = cmd;
        if (!norm.empty() && norm[0] == '-') norm.erase(0, 1);

        if(g_debug){ dbg("MI>> " + norm); }
        std::string issued = std::to_string(myTok) + "-" + norm + "\n";
        fputs(issued.c_str(), in_); fflush(in_);
        return waitResult(myTok, issued);
    }

    void readBanner(){
        int fd = fileno(out_);
        std::string acc;
        for(int i=0;i<40;i++){
            std::string c = readChunk(fd, 200);
            if(!c.empty()){
                if(g_debug) dbg(std::string("MI<< ")+c);
                acc += c;
                if(acc.find("(gdb)")!=std::string::npos) break;
            } else break;
        }
    }

    bool waitStopped(const char* tag){
        int fd = fileno(out_);
        for(int i=0;i<150;i++){
            std::string c = readChunk(fd, 120);
            if(!c.empty()){
                if(g_debug) dbg(std::string("MI<< ")+c);
                if(c.find("*stopped")!=std::string::npos) return true;
            } else {
                usleep(2000);
            }
        }
        if(g_debug) dbg(std::string("waitStopped timeout @ ")+tag);
        return false;
    }

public:
    explicit GdbMi(const std::string& exe, const std::string& args){
        int inpipe[2], outpipe[2];
        if(pipe(inpipe)||pipe(outpipe)) throw std::runtime_error("pipe");
        pid_t pid=fork(); if(pid<0) throw std::runtime_error("fork");
        if(pid==0){
            dup2(inpipe[0],0); dup2(outpipe[1],1); dup2(outpipe[1],2);
            close(inpipe[1]); close(outpipe[0]);
            execlp("gdb","gdb","-q","--interpreter=mi2",nullptr);
            _exit(127);
        }
        close(inpipe[0]); close(outpipe[1]);
        in_ = fdopen(inpipe[1], "w");
        out_= fdopen(outpipe[0], "r");
        if(!in_||!out_) throw std::runtime_error("fdopen");

        readBanner();
        dbg("init: banner read ✓");

        auto r = miDo("-gdb-set confirm off");         if(!mi_ok(r) && !mi_err(r)) throw std::runtime_error("gdb-set confirm off failed");
        dbg("init: confirm off ✓");
        r = miDo("-gdb-set pagination off");           if(!mi_ok(r) && !mi_err(r)) throw std::runtime_error("gdb-set pagination off failed");
        dbg("init: pagination off ✓");
        r = miDo("-gdb-set width 0");                  if(!mi_ok(r) && !mi_err(r)) throw std::runtime_error("gdb-set width 0 failed");
        dbg("init: width 0 ✓");

        r = miDo(std::string("-file-exec-and-symbols ") + exe); if(!mi_ok(r)) throw std::runtime_error("file-exec-and-symbols failed");
        dbg("init: file-exec set ✓");
        if(!args.empty()){
            r = miDo(std::string("-exec-arguments ") + args);   if(!mi_ok(r)) throw std::runtime_error("exec-arguments failed");
            dbg("init: args set ✓");
        }
        r = miDo("-gdb-set breakpoint pending on");    if(!mi_ok(r)) throw std::runtime_error("bp pending on failed");
        dbg("init: bp pending on ✓");

        r = miDo("-break-insert -f main");             dbg("init: break main issued");
        if(!mi_ok(r)){
            dbg("main unresolved; trying _start");
            r = miDo("-break-insert -t _start");       if(!mi_ok(r)) throw std::runtime_error("failed to set any startup breakpoint");
            dbg("init: break _start ✓");
        } else {
            dbg("init: break main ✓");
        }

        r = miDo("-exec-run");                         dbg("init: exec-run issued ✓");
        alive_ = true;

        if(!mi_running(r)){
            waitStopped("run-alt");                    dbg("init: stopped after run-alt ✓");
        } else {
            waitStopped("run");                        dbg("init: stopped after run ✓");
        }
        stopped_ = true;

        auto q = miDo("-data-list-register-names");    if(!mi_ok(q)) throw std::runtime_error("no regs list");
        dbg("init: got registers ✓");
    }

    ~GdbMi(){
        if(in_){ fputs("-gdb-exit\n", in_); fflush(in_); fclose(in_); }
        if(out_) fclose(out_);
    }

    bool stepi(){
        auto r = miDo("-exec-step-instruction");
        if(r.find("*stopped")==std::string::npos){
            if(!waitStopped("stepi")) return false;
        }
        return true;
    }

    bool read_pc_and_opcode(uint64_t& pc, std::vector<uint8_t>& bytes, std::string& asm_text){
        // Get PC (correctly parse hex after "0x")
        auto fr = miDo("-stack-info-frame");
        size_t a = fr.find("addr=\"0x");
        if (a == std::string::npos) return false;
        a = fr.find("0x", a);
        if (a == std::string::npos) return false;
        a += 2;
        size_t b = fr.find('"', a);
        if (b == std::string::npos) return false;
        std::string addr_hex = fr.substr(a, b - a);
        pc = std::strtoull(addr_hex.c_str(), nullptr, 16);

        // Read 8 bytes at PC
        auto rm = miDo("-data-read-memory-bytes $pc 8");
        size_t m = rm.find("contents=\"");
        if (m == std::string::npos) return false;
        m += 10;
        size_t d = rm.find('"', m);
        if (d == std::string::npos) return false;
        std::string hex = rm.substr(m, d - m);
        hex.erase(std::remove_if(hex.begin(), hex.end(), [](unsigned char c){ return std::isspace(c); }), hex.end());
        bytes = parse_hex_bytes(hex);

        // Disassemble exactly 1 instruction starting at $pc (try mixed then raw)
        auto grab_inst = [](const std::string& s)->std::string{
            size_t p = s.find("inst=\"");
            if (p == std::string::npos) return {};
            p += 6;
            size_t q = s.find('"', p);
            if (q == std::string::npos) return {};
            return s.substr(p, q - p);
        };

        std::string ds = miDo("-data-disassemble -s $pc -e $pc+8 -- 1");
        asm_text = grab_inst(ds);
        if (asm_text.empty()) { ds = miDo("-data-disassemble -s $pc -e $pc+4 -- 1"); asm_text = grab_inst(ds); }
        if (asm_text.empty()) { ds = miDo("-data-disassemble -s $pc -e $pc+8 -- 0"); asm_text = grab_inst(ds); }
        if (asm_text.empty()) { ds = miDo("-data-disassemble -s $pc -e $pc+4 -- 0"); asm_text = grab_inst(ds); }

        asm_text = mi_unescape(asm_text);
        return true;
    }
};

// ------------------------- permitted mnemonics --------------------------------
static bool allowed_mnemonic(const std::string& asm_line){
    std::string m;
    m.reserve(16);
    for(char ch : asm_line){
        if(ch==' ' || ch=='\t') break;
        m.push_back((char)std::tolower((unsigned char)ch));
    }
    while(!m.empty() && (m.back()==',' || m.back()==':')) m.pop_back();

    static const char* ok[] = {
        // data processing
        "nop","mov","movz","movn","add","sub","mul","cmp","and","orr","eor","lsl","lsr",
        // loads/stores
        "ldr","str","ldp","stp",
        // addressing
        "adr","adrp",
        // branches / calls / returns
        "bl","blr","ret","b","cbz","cbnz"
    };

    for(const char* k: ok){ if(m==k) return true; }
    if(m.rfind("b.", 0)==0) return true; // conditional branches
    return false;
}

// ---------------------------- KZG (BN254 via mcl) -----------------------------
namespace KZG {
using Fr = mcl::bn::Fr;
using G1 = mcl::bn::G1;
using G2 = mcl::bn::G2;
using GT = mcl::Fp12;

inline G1 genG1()
{
    G1 P;
    static const char tag[] = "fidesinnova-g1";
    mcl::bn::hashAndMapToG1(P, tag, sizeof(tag)-1);
    return P;
}
inline G2 genG2()
{
    G2 Q;
    static const char tag[] = "fidesinnova-g2";
    mcl::bn::hashAndMapToG2(Q, tag, sizeof(tag)-1);
    return Q;
}

struct SRS {
    std::vector<G1> g1_powers;
    G2 g2_1;
    G2 g2_tau;

    size_t size() const { return g1_powers.size(); }

    static SRS trustedSetup(size_t n){
        static bool inited=false;
        if(!inited){ mcl::bn::initPairing(); inited=true; }

        // Secret tau (demo only)
        Fr tau; tau.setByCSPRNG();

        SRS s;
        s.g1_powers.resize(n);

        // Generators (deterministic via hash)
        G1 g1 = genG1();
        G2 g2 = genG2();

        Fr pow = 1;
        for(size_t i=0;i<n;i++){
            G1 t; mcl::bn::G1::mul(t, g1, pow);
            s.g1_powers[i] = t;
            pow *= tau;
        }
        s.g2_1 = g2;
        mcl::bn::G2::mul(s.g2_tau, g2, tau);
        return s;
    }

    // --- serialization helpers (use iostream + mcl::IoSerialize)
    template<class P>
    static void serializePoint(const P& Pnt, std::string& out){
        std::ostringstream oss(std::ios::binary);
        oss << mcl::IoSerialize << Pnt;   // compact binary form
        out = oss.str();
    }

    template<class P>
    static void deserializePoint(P& outP, const std::string& hex){
        // hex -> binary
        std::string bin; bin.reserve(hex.size()/2);
        for(size_t i=0;i+1<hex.size(); i+=2){
            char t[3] = { hex[i], hex[i+1], 0 };
            bin.push_back(static_cast<char>(strtol(t, nullptr, 16)));
        }
        std::istringstream iss(std::string(bin.data(), bin.size()), std::ios::binary);
        iss >> mcl::IoSerialize >> outP;  // read compact binary form
        if(!iss) throw std::runtime_error("bad point");
    }

    static std::string bytesToHex(const std::string& s){
        std::ostringstream o; o<<std::hex<<std::setfill('0');
        for(unsigned char c: s) o<<std::setw(2)<<(unsigned)c;
        return o.str();
    }
    template<class P>
    static std::string pointToHex(const P& Pnt){
        std::string s; serializePoint(Pnt, s);
        return bytesToHex(s);
    }

    std::string saveHex() const {
        std::ostringstream o;
        o<<"n:"<<g1_powers.size()<<"\n";
        o<<"g2_1:"<<pointToHex(g2_1)<<"\n";
        o<<"g2_tau:"<<pointToHex(g2_tau)<<"\n";
        for(size_t i=0;i<g1_powers.size();++i){
            o<<"g1_"<<i<<":"<<pointToHex(g1_powers[i])<<"\n";
        }
        return o.str();
    }
    static SRS loadHex(const std::string& txt){
        static bool inited=false;
        if(!inited){ mcl::bn::initPairing(); inited=true; }

        auto line=[&](const std::string& key)->std::string{
            auto p = txt.find(key);
            if(p==std::string::npos) return "";
            auto nl = txt.find('\n', p);
            auto colon = txt.find(':', p);
            if(colon==std::string::npos) return "";
            return txt.substr(colon+1, (nl==std::string::npos?txt.size():nl)-(colon+1));
        };
        size_t n = (size_t)std::stoull(line("n:"));

        SRS s; s.g1_powers.resize(n);
        deserializePoint(s.g2_1,  line("g2_1:"));
        deserializePoint(s.g2_tau, line("g2_tau:"));
        for(size_t i=0;i<n;i++){
            std::string k = std::string("g1_")+std::to_string(i)+":";
            deserializePoint(s.g1_powers[i], line(k));
        }
        return s;
    }
};


inline G1 msm_g1(const std::vector<G1>& bases, const std::vector<Fr>& scalars){
    if(bases.size()!=scalars.size()) throw std::runtime_error("msm len mismatch");
    G1 acc; acc.clear();
    for(size_t i=0;i<bases.size();++i){
        if(scalars[i].isZero()) continue;
        G1 tmp; mcl::bn::G1::mul(tmp, bases[i], scalars[i]);
        mcl::bn::G1::add(acc, acc, tmp);
    }
    return acc;
}

inline G1 commit(const SRS& srs, const std::vector<Fr>& coeffs){
    if(coeffs.size() > srs.g1_powers.size()) throw std::runtime_error("poly degree exceeds SRS");
    return msm_g1(srs.g1_powers, coeffs);
}

// Horner evaluation
inline Fr evalPoly(const std::vector<Fr>& a, const Fr& z){
    Fr y = 0;
    for(size_t i=a.size(); i-->0;){
        y *= z;
        y += a[i];
    }
    return y;
}

// Synthetic division by (X - z): returns Q with deg-1, and y = P(z)
inline std::pair<std::vector<Fr>, Fr> divXminusZ(const std::vector<Fr>& a, const Fr& z){
    if(a.empty()) return {{}, Fr(0)};
    size_t n = a.size();
    std::vector<Fr> q(n-1);
    Fr rem = a.back();
    for(size_t i=n-1; i-->0;){
        q[i] = rem;
        rem = a[i] + rem * z;
    }
    Fr y = rem; // P(z)
    return {q, y};
}

struct Opening { Fr y; G1 pi; };

inline Opening open(const SRS& srs, const std::vector<Fr>& coeffs, const Fr& z){
    if(coeffs.empty()) return {Fr(0), G1()};
    auto [q, y] = divXminusZ(coeffs, z);
    if(q.size() > srs.g1_powers.size()) throw std::runtime_error("witness degree exceeds SRS");
    G1 pi = msm_g1(srs.g1_powers, q);
    return {y, pi};
}

inline bool verify(const SRS& srs, const G1& C, const Fr& z, const Fr& y, const G1& pi){
    G1 g1 = genG1(); // use same generator as SRS
    G1 yG1, Cminus;
    mcl::bn::G1::mul(yG1, g1, y);
    mcl::bn::G1::sub(Cminus, C, yG1);

    G2 zG2, tauMinusZ;
    mcl::bn::G2::mul(zG2, srs.g2_1, z);
    mcl::bn::G2::sub(tauMinusZ, srs.g2_tau, zG2);

    GT lhs, rhs;
    mcl::pairing(lhs, Cminus, srs.g2_1);   // <-- CHANGED
    mcl::pairing(rhs, pi,      tauMinusZ); // <-- CHANGED
    return lhs == rhs;
}

// encoders for JSON
inline std::string g1ToHex(const G1& P){
    std::string s; SRS::serializePoint(P, s);
    return SRS::bytesToHex(s);
}
inline std::string frToHex(const Fr& x){
    // Use base-16 canonical string (simple & portable)
    return x.getStr(16);
}
inline Fr frFromHex(const std::string& h){
    Fr x; x.setStr(h, 16);
    return x;
}
inline G1 g1FromHex(const std::string& h){
    G1 P; SRS::deserializePoint(P, h); return P;
}

} // namespace KZG

// --- Hash a row into Fr (SHA-256 -> reduce) ---
static KZG::Fr hash_row_to_fr(uint64_t pc, const std::vector<uint8_t>& bytes, const std::string& asm_text){
    std::string t;
    t.reserve(8 + bytes.size() + asm_text.size() + 16);
    for(int i=0;i<8;i++) t.push_back((char)((pc >> (8*i)) & 0xff));
    t.append((const char*)bytes.data(), bytes.size());
    t.push_back((char)0x1f);
    t.append(asm_text);
    std::string h = sha256_of_string(t); // hex(32 bytes)

    // Convert hex->32 bytes
    std::vector<uint8_t> b = parse_hex_bytes(h);
    // Interpret as little-endian limbs into Fr (setArray takes uint64_t*)
    uint64_t limbs[4] = {0,0,0,0};
    for(int i=0;i<32;i++){
        ((uint8_t*)limbs)[i] = b[i]; // little-endian fill
    }
    KZG::Fr x;
    x.setArray(limbs, 4); // reduced mod r internally
    return x;
}

// -------------------------------- COMMIT --------------------------------------
static int cmd_commit(const std::string& asm_path){
    auto bytes = read_file(asm_path);
    if(bytes.empty()){
        fprintf(stderr, "Error: could not read %s\n", asm_path.c_str());
        return 1;
    }
    Commitment c;
    c.code_sha = sha256_of_string(bytes);
    c.salt = random_hex(16);
    c.session_hash = sha256_of_string(c.code_sha + "|" + c.salt);

    std::string outname = basename_no_ext(asm_path) + ".com";
    if(!write_file(outname, c.to_json())){
        fprintf(stderr, "Error: write %s failed\n", outname.c_str());
        return 1;
    }
    printf("Commitment written to %s\n", outname.c_str());
    printf("  code_sha     : %s\n", c.code_sha.c_str());
    printf("  session_hash : %s\n", c.session_hash.c_str());
    return 0;
}

// -------------------------------- PROVE ---------------------------------------
static int cmd_prove(const std::string& exe, int steps, const std::string& domain_tag){
    std::string asm_guess = basename_no_ext(exe) + ".s";
    std::string code_sha = "";
    {
        auto s = read_file(asm_guess);
        if(!s.empty()) code_sha = sha256_of_string(s);
    }

    GdbMi g(exe, "");
    Proof prf;
    prf.exe_path  = exe;
    prf.domain_tag= domain_tag;
    prf.code_sha  = code_sha;
    prf.steps     = 0;

    for(int i=0;i<steps;i++){
        uint64_t pc=0; std::vector<uint8_t> bytes; std::string asm_text;
        if(!g.read_pc_and_opcode(pc, bytes, asm_text)){
            dbg("read_pc_and_opcode failed; stopping");
            break;
        }
        if(allowed_mnemonic(asm_text)){
            TraceRow r; r.pc=pc; r.bytes=bytes; r.asm_text=asm_text;
            if(g_debug){
                std::ostringstream oss;
                oss << "row pc=0x" << std::hex << pc
                    << " bytes=" << hex_of_bytes(bytes)
                    << " asm=" << asm_text;
                dbg(oss.str());
            }
            prf.rows.push_back(std::move(r));
        }else{
            if(g_debug){
                std::string m; m.reserve(16);
                for(char ch: asm_text){ if(ch==' '||ch=='\t') break; m.push_back((char)std::tolower((unsigned char)ch)); }
                dbg(std::string("skip unsupported (mnemonic='") + m + "'): " + asm_text);
            }
        }

        prf.steps++;
        if(!g.stepi()){
            dbg("stepi returned false; stopping");
            break;
        }
    }

    // ---- KZG: polynomial over Fr from rows ----
    std::vector<KZG::Fr> coeffs;
    coeffs.reserve(prf.rows.size());
    for(const auto& r : prf.rows){
        coeffs.push_back(hash_row_to_fr(r.pc, r.bytes, r.asm_text));
    }

    // SRS (demo): generate per proof. For production, load a shared SRS.
    KZG::SRS srs = KZG::SRS::trustedSetup(std::max<size_t>(coeffs.size(), 2));

    // Commit
    KZG::G1 C = KZG::commit(srs, coeffs);

    // Fiat–Shamir point z (bind domain_tag, code_sha, rows count, commitment bytes)
    std::string fs = "fidesinnova|kzg|" + prf.domain_tag + "|" + prf.code_sha + "|" + std::to_string(coeffs.size()) + "|";
    {
        std::string Cser; KZG::SRS::serializePoint(C, Cser);
        fs.append(Cser);
    }
    std::string hz = sha256_of_string(fs);
    std::vector<uint8_t> zb = parse_hex_bytes(hz);
    uint64_t limbs[4] = {0,0,0,0};
    for(int i=0;i<32;i++) ((uint8_t*)limbs)[i] = zb[i];
    KZG::Fr z; z.setArray(limbs, 4);

    // Open at z
    auto op = KZG::open(srs, coeffs, z);

    // Fill PRF fields
    prf.kzg_commit_g1 = KZG::g1ToHex(C);
    prf.kzg_z_fr      = KZG::frToHex(z);
    prf.kzg_y_fr      = KZG::frToHex(op.y);
    prf.kzg_pi_g1     = KZG::g1ToHex(op.pi);
    prf.srs_hex       = srs.saveHex();

    std::string outname = basename_no_ext(exe) + ".prf";
    if(!write_file(outname, prf.to_json())){
        fprintf(stderr, "Error: write %s failed\n", outname.c_str());
        return 1;
    }
    printf("Proof written to %s\n", outname.c_str());
    printf("  trace_len=%zu\n", prf.rows.size());
    return 0;
}

// -------------------------------- VERIFY --------------------------------------
static bool pc_progress_ok(const std::vector<TraceRow>& rows){
    if(rows.empty()) return true;
    for(size_t i=1;i<rows.size();i++){
        auto prev = rows[i-1].pc;
        auto cur  = rows[i].pc;
        if(cur==0) return false;
        if(cur==prev){ return false; }
    }
    return true;
}

static int cmd_verify(const std::string& com_path, const std::string& prf_path){
    auto com_s = read_file(com_path);
    if(com_s.empty()){ fprintf(stderr, "Error: could not read %s\n", com_path.c_str()); return 1; }
    auto prf_s = read_file(prf_path);
    if(prf_s.empty()){ fprintf(stderr, "Error: could not read %s\n", prf_path.c_str()); return 1; }

    Commitment c = Commitment::from_json(com_s);
    Proof       p = Proof::from_json(prf_s);

    if(!c.code_sha.empty() && !p.code_sha.empty() && c.code_sha != p.code_sha){
        fprintf(stderr, "Verify: REJECT (code_sha mismatch)\n");
        return 1;
    }
    if(!pc_progress_ok(p.rows)){
        fprintf(stderr, "Verify: REJECT (pc local transition fail)\n");
        return 1;
    }

    // Parse KZG fields
    if(p.srs_hex.empty()||p.kzg_commit_g1.empty()||p.kzg_z_fr.empty()||p.kzg_y_fr.empty()||p.kzg_pi_g1.empty()){
        fprintf(stderr, "Verify: REJECT (missing KZG fields)\n");
        return 1;
    }

    KZG::SRS srs = KZG::SRS::loadHex(p.srs_hex);
    KZG::G1 C    = KZG::g1FromHex(p.kzg_commit_g1);
    KZG::Fr z    = KZG::frFromHex(p.kzg_z_fr);
    KZG::Fr y    = KZG::frFromHex(p.kzg_y_fr);
    KZG::G1 pi   = KZG::g1FromHex(p.kzg_pi_g1);

    bool kzg_ok = KZG::verify(srs, C, z, y, pi);
    if(!kzg_ok){
        fprintf(stderr, "Verify: REJECT (KZG failed)\n");
        return 1;
    }

    printf("Verify: ACCEPT\n");
    return 0;
}

// --------------------------------- MAIN ---------------------------------------
int main(int argc, char** argv){
    if(argc<3){
        fprintf(stderr,
            "Usage:\n"
            "  ./fidesinnova -c <program.s>\n"
            "  ./fidesinnova -p <program> [--steps N] [--debug] [--domain TAG]\n"
            "  ./fidesinnova -v <program.com> <program.prf>\n");
        return 1;
    }
    std::string mode = argv[1];
    for(int i=1;i<argc;i++) if(std::string(argv[i])=="--debug") g_debug=true;

    if(mode=="-c"){
        std::string asm_path = argv[2];
        return cmd_commit(asm_path);

    } else if(mode=="-p"){
        std::string exe = argv[2];
        int steps = 500;
        std::string domain_tag = "default-domain";
        for(int i=3;i<argc;i++){
            std::string a = argv[i];
            if(a=="--steps" && i+1<argc){
                steps = std::stoi(argv[++i]);
            } else if(a=="--domain" && i+1<argc){
                domain_tag = argv[++i];
            }
        }
        try{
            return cmd_prove(exe, steps, domain_tag);
        }catch(const std::exception& e){
            fprintf(stderr, "Prover error: %s\n", e.what());
            return 2;
        }

    } else if(mode=="-v"){
        if(argc<4){
            fprintf(stderr, "Usage: ./fidesinnova -v <program.com> <program.prf>\n");
            return 1;
        }
        return cmd_verify(argv[2], argv[3]);

    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode.c_str());
        return 1;
    }
}
