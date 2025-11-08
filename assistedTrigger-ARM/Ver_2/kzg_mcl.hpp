// kzg_mcl.hpp
// Minimal KZG over BN254 using Herumi mcl (https://github.com/herumi/mcl)
//
// Build deps (Ubuntu):
//   sudo apt-get install -y cmake g++ make
//   git clone --depth=1 https://github.com/herumi/mcl && cd mcl && mkdir build && cd build
//   cmake .. -DMCL_STATIC_LIB=ON -DMCL_USE_OPENSSL=ON && make -j
//   sudo make install
//
// Link flags:  -lmcl -lcrypto
//
// API summary:
//   KZG::SRS srs = KZG::SRS::trustedSetup(n);  // or SRS::load(path)
//   auto C = KZG::commit(srs, coeffs);
//   auto [y,pi] = KZG::open(srs, coeffs, z);
//   bool ok = KZG::verify(srs, C, z, y, pi);

#pragma once
#include <mcl/bn256.hpp>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

namespace KZG {

using Fr = mcl::bn::Fr;
using G1 = mcl::bn::G1;
using G2 = mcl::bn::G2;
using GT = mcl::bn::Fp12;

struct SRS {
    // [tau^i]G1 for i = 0..n-1
    std::vector<G1> g1_powers;
    // Needed by verifier: [1]G2 and [tau]G2
    G2 g2_1;
    G2 g2_tau;

    size_t size() const { return g1_powers.size(); }

    static SRS trustedSetup(size_t n){
        static bool inited=false;
        if(!inited){ mcl::bn::initPairing(); inited=true; }

        // secret tau (DO NOT USE IN PROD!) – for dev/demo we derive from /dev/urandom or std::random_device.
        std::random_device rd;
        std::mt19937_64 gen(rd());
        uint64_t r0 = gen(), r1 = gen(), r2 = gen(), r3 = gen();
        Fr tau; tau.setByCSPRNG();
        // Alternative manual seed:
        // tau.setByCSPRNG(); // uses OpenSSL if built with MCL_USE_OPENSSL

        SRS s;
        s.g1_powers.resize(n);

        // Generators
        G1 g1; mcl::bn::mapToG1(g1, 1); // standard generator
        G2 g2; mcl::bn::mapToG2(g2, 1);

        // g1_powers[i] = [tau^i]G1
        Fr pow = 1;
        for(size_t i=0;i<n;i++){
            mcl::bn::G1::mul(s.g1_powers[i], g1, pow);
            pow *= tau;
        }

        // g2_1 = [1]G2 ; g2_tau = [tau]G2
        s.g2_1 = g2;
        mcl::bn::G2::mul(s.g2_tau, g2, tau);
        return s;
    }

    // Serialize hex (uncompressed affine)
    static std::string toHex(const void* p, size_t n){
        const uint8_t* b = (const uint8_t*)p;
        std::ostringstream o; o<<std::hex<<std::setfill('0');
        for(size_t i=0;i<n;i++) o<<std::setw(2)<<(unsigned)b[i];
        return o.str();
    }
    template<class P>
    static std::string pointToHex(const P& Pnt){
        std::string s; serialize(Pnt, s);
        return toHex(s.data(), s.size());
    }
    template<class P>
    static void serialize(const P& Pnt, std::string& out){
        out.resize(128); // enough for uncompressed G1/G2 of BN254
        size_t written = mcl::bn::serialize(&out[0], out.size(), Pnt);
        out.resize(written);
    }
    template<class P>
    static void deserialize(P& out, const std::string& hex){
        std::string bin; bin.reserve(hex.size()/2);
        for(size_t i=0;i+1<hex.size(); i+=2){
            char h[3]={hex[i],hex[i+1],0};
            bin.push_back((char)strtol(h,nullptr,16));
        }
        if(mcl::bn::deserialize(out, bin.data(), bin.size())==0) throw std::runtime_error("bad point");
    }

    std::string saveHex() const {
        std::ostringstream o;
        // sizes
        o<<"n:"<<g1_powers.size()<<"\n";
        // G2
        o<<"g2_1:"<<pointToHex(g2_1)<<"\n";
        o<<"g2_tau:"<<pointToHex(g2_tau)<<"\n";
        // G1 powers
        for(size_t i=0;i<g1_powers.size();++i){
            o<<"g1_"<<i<<":"<<pointToHex(g1_powers[i])<<"\n";
        }
        return o.str();
    }
    static SRS loadHex(const std::string& txt){
        static bool inited=false;
        if(!inited){ mcl::bn::initPairing(); inited=true; }

        auto getLine=[&](const std::string& key)->std::string{
            auto p = txt.find(key);
            if(p==std::string::npos) return "";
            auto nl = txt.find('\n', p);
            auto colon = txt.find(':', p);
            if(colon==std::string::npos) return "";
            return txt.substr(colon+1, (nl==std::string::npos?txt.size():nl)-(colon+1));
        };
        size_t n = (size_t)std::stoull(getLine("n:"));

        SRS s; s.g1_powers.resize(n);
        deserialize(s.g2_1,  getLine("g2_1:"));
        deserialize(s.g2_tau, getLine("g2_tau:"));
        for(size_t i=0;i<n;i++){
            std::string k = std::string("g1_")+std::to_string(i)+":";
            std::string hx = getLine(k);
            deserialize(s.g1_powers[i], hx);
        }
        return s;
    }
};

// ---------------- helpers ----------------
inline Fr frFromBytes32(const uint8_t b[32]){
    Fr x;
    // setLittleEndian or setBigEndian. mcl uses little-endian setArray.
    x.setArray((const uint64_t*)b, 4); // 4*64 = 256 bits; reduction is modulo r internally
    return x;
}

// Horner evaluation P(z)
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
    Fr y = rem; // remainder == P(z)
    return {q, y};
}

// MSM over G1: \sum a_i * SRS[i]
inline G1 msm_g1(const std::vector<G1>& bases, const std::vector<Fr>& scalars){
    if(bases.size()!=scalars.size()) throw std::runtime_error("msm len mismatch");
    G1 acc; acc.clear(); // point at infinity
    for(size_t i=0;i<bases.size();++i){
        if(scalars[i].isZero()) continue;
        G1 tmp; mcl::bn::G1::mul(tmp, bases[i], scalars[i]);
        mcl::bn::G1::add(acc, acc, tmp);
    }
    return acc;
}

// Public KZG API
inline G1 commit(const SRS& srs, const std::vector<Fr>& coeffs){
    if(coeffs.size() > srs.g1_powers.size()) throw std::runtime_error("poly degree exceeds SRS");
    return msm_g1(srs.g1_powers, coeffs);
}

struct Opening { Fr y; G1 pi; };

inline Opening open(const SRS& srs, const std::vector<Fr>& coeffs, const Fr& z){
    if(coeffs.size() == 0) return {Fr(0), G1()};
    auto [q, y] = divXminusZ(coeffs, z);
    if(q.size() > srs.g1_powers.size()) throw std::runtime_error("witness degree exceeds SRS");
    G1 pi = msm_g1(srs.g1_powers, q);
    return {y, pi};
}

inline bool verify(const SRS& srs, const G1& C, const Fr& z, const Fr& y, const G1& pi){
    // Check: e(C - y*G1, G2)  ==  e(pi, [tau]G2 - z*[1]G2)
    G1 yG1; G1 Cminus;
    G2 rhsBase, zG2, tauMinusZ;

    // y*G1
    G1 g1; mcl::bn::mapToG1(g1, 1);
    mcl::bn::G1::mul(yG1, g1, y);
    // C - yG1
    mcl::bn::G1::sub(Cminus, C, yG1);

    // [tau]G2 - z*[1]G2
    mcl::bn::G2::mul(zG2, srs.g2_1, z);
    mcl::bn::G2::sub(tauMinusZ, srs.g2_tau, zG2);

    GT lhs = mcl::bn::pairing(Cminus, srs.g2_1);
    GT rhs = mcl::bn::pairing(pi,      tauMinusZ);
    return lhs == rhs;
}

// ----------------- encoding helpers for PRF JSON -----------------
inline std::string g1ToHex(const G1& P){
    std::string s; SRS::serialize(P, s);
    std::ostringstream o; o<<std::hex<<std::setfill('0');
    for(unsigned char c: s) o<<std::setw(2)<<(unsigned)c;
    return o.str();
}
inline std::string g2ToHex(const G2& P){
    std::string s; SRS::serialize(P, s);
    std::ostringstream o; o<<std::hex<<std::setfill('0');
    for(unsigned char c: s) o<<std::setw(2)<<(unsigned)c;
    return o.str();
}
inline std::string frToHex(const Fr& x){
    uint8_t b[32]={0};
    x.serialize(b, sizeof(b));
    std::ostringstream o; o<<std::hex<<std::setfill('0');
    for(int i=0;i<32;i++) o<<std::setw(2)<<(unsigned)b[i];
    return o.str();
}
inline Fr frFromHex(const std::string& h){
    std::string bin; bin.reserve(h.size()/2);
    for(size_t i=0;i+1<h.size(); i+=2){
        char t[3]={h[i],h[i+1],0};
        bin.push_back((char)strtol(t,nullptr,16));
    }
    Fr x; x.deserialize((const uint8_t*)bin.data(), bin.size());
    return x;
}
inline G1 g1FromHex(const std::string& h){ G1 P; SRS::deserialize(P,h); return P; }

} // namespace KZG
