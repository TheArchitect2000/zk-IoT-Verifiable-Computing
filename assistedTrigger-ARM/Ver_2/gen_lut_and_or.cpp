// gen_lut_and_or.cpp
//
// Generate the 4-bit AND/OR lookup-table polynomials and write them to disk.
// Each file is:
//   line 1 : number of coefficients (decimal)
//   line 2+: coefficient in hex (Fr::getStr(16))
//
// build:
//   g++ -std=gnu++17 -O2 gen_lut_and_or.cpp -I/usr/local/include -L/usr/local/lib -lmcl -o gen_lut_and_or
// run:
//   ./gen_lut_and_or
//
// it will create:
//   lut_and_or_x.txt
//   lut_and_or_y.txt
//   lut_and_or_z.txt
//   lut_and_or_op.txt

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <mcl/bn.hpp>

using namespace mcl::bn;

static mcl::Fr fr_from_u64(uint64_t v)
{
    mcl::Fr r;
    uint8_t buf[8];
    for (int i = 0; i < 8; i++) {
        buf[7 - i] = (uint8_t)((v >> (i * 8)) & 0xff);
    }
    r.setBigEndianMod(buf, 8);
    return r;
}

struct Poly {
    std::vector<mcl::Fr> c;
};

static void poly_normalize(Poly& p){
    while (!p.c.empty() && p.c.back().isZero()) p.c.pop_back();
}

// naive lagrange on x=0..n-1
static Poly interpolate_on_range0(const std::vector<mcl::Fr>& vals)
{
    size_t n = vals.size();
    Poly acc; acc.c = { mcl::Fr() }; acc.c[0].clear();
    mcl::Fr one = fr_from_u64(1);
    for (size_t i=0;i<n;i++){
        Poly numer; numer.c = { fr_from_u64(1) };
        mcl::Fr denom = fr_from_u64(1);
        mcl::Fr xi = fr_from_u64(i);
        for (size_t j=0;j<n;j++){
            if (j == i) continue;
            mcl::Fr fj = fr_from_u64(j);
            Poly term;
            mcl::Fr minus_j = mcl::Fr(); minus_j.clear(); minus_j -= fj;
            term.c = { minus_j, fr_from_u64(1) };
            // numer *= (x - j)
            Poly tmp;
            tmp.c.assign(numer.c.size() + 1, mcl::Fr());
            for (size_t a=0;a<numer.c.size();a++){
                tmp.c[a]   += numer.c[a] * term.c[0];
                tmp.c[a+1] += numer.c[a] * term.c[1];
            }
            numer = tmp;
            denom *= (xi - fj);
        }
        mcl::Fr denom_inv = one / denom;
        for (auto &cc : numer.c) cc *= denom_inv;
        // acc += numer * vals[i]
        Poly contrib; contrib.c.resize(numer.c.size());
        for (size_t k=0;k<numer.c.size();k++)
            contrib.c[k] = numer.c[k] * vals[i];
        if (contrib.c.size() > acc.c.size()) acc.c.resize(contrib.c.size());
        for (size_t k=0;k<contrib.c.size();k++) acc.c[k] += contrib.c[k];
    }
    poly_normalize(acc);
    return acc;
}

static void write_poly(const std::string& path, const Poly& p)
{
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot open " + path);
    out << p.c.size() << "\n";
    for (auto &coef : p.c){
        out << coef.getStr(16) << "\n";
    }
    out.close();
    std::cerr << "wrote " << path << " with " << p.c.size() << " coeffs\n";
}

int main()
{
    try {
        mcl::bn::initPairing(mcl::BN254);

        // 4-bit
        const size_t LUT_BITS       = 4;
        const size_t LUT_RANGE      = (1u << LUT_BITS);      // 16
        const size_t LUT_PER_OP     = LUT_RANGE * LUT_RANGE; // 256
        const size_t LUT_TOTAL_ROWS = LUT_PER_OP * 2;        // 512

        // columns
        std::vector<mcl::Fr> lut_x(LUT_TOTAL_ROWS), lut_y(LUT_TOTAL_ROWS),
                             lut_z(LUT_TOTAL_ROWS), lut_op(LUT_TOTAL_ROWS);

        const uint8_t OP_AND = 4;
        const uint8_t OP_OR  = 5;

        // AND block
        for (size_t a=0;a<LUT_RANGE;a++){
            for (size_t b=0;b<LUT_RANGE;b++){
                size_t idx = a*LUT_RANGE + b;
                lut_x[idx] = fr_from_u64(a);
                lut_y[idx] = fr_from_u64(b);
                lut_z[idx] = fr_from_u64((a & b) & 0xF);
                lut_op[idx] = fr_from_u64(OP_AND);
            }
        }
        // OR block
        for (size_t a=0;a<LUT_RANGE;a++){
            for (size_t b=0;b<LUT_RANGE;b++){
                size_t idx = LUT_PER_OP + a*LUT_RANGE + b;
                lut_x[idx] = fr_from_u64(a);
                lut_y[idx] = fr_from_u64(b);
                lut_z[idx] = fr_from_u64((a | b) & 0xF);
                lut_op[idx] = fr_from_u64(OP_OR);
            }
        }

        // interpolate
        Poly px = interpolate_on_range0(lut_x);
        Poly py = interpolate_on_range0(lut_y);
        Poly pz = interpolate_on_range0(lut_z);
        Poly pp = interpolate_on_range0(lut_op);

        // write
        write_poly("lut_and_or_x.txt", px);
        write_poly("lut_and_or_y.txt", py);
        write_poly("lut_and_or_z.txt", pz);
        write_poly("lut_and_or_op.txt", pp);

        return 0;
    } catch (const std::exception& e){
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
