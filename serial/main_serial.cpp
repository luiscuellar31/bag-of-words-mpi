#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "tokenize.hpp"
#include "csv.hpp"
#include "timer.hpp"

static std::string basename(const std::string& p) {
    size_t pos = p.find_last_of("/\\");
    return pos==std::string::npos? p : p.substr(pos+1);
}

int main(int argc, char** argv) {
    double t0 = now_sec();
    std::string outpath = "out/matriz.csv";
    std::vector<std::string> files;
    for (int i=1;i<argc;i++) {
        std::string a = argv[i];
        if (a == "--out" && i+1 < argc) { outpath = argv[++i]; }
        else { files.push_back(a); }
    }
    std::vector<std::string> docnames;
    std::vector<std::unordered_map<std::string,int>> dicts;
    for (auto& fp : files) {
        std::ifstream f(fp);
        std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        auto toks = tokenize(s);
        std::unordered_map<std::string,int> m;
        for (auto& w : toks) m[w]++;
        dicts.push_back(std::move(m));
        docnames.push_back(basename(fp));
    }
    std::vector<std::string> vocab;
    {
        std::unordered_map<std::string,int> seen;
        for (auto& m : dicts) for (auto& kv : m) seen.emplace(kv.first,0);
        vocab.reserve(seen.size());
        for (auto& kv : seen) vocab.push_back(kv.first);
        std::sort(vocab.begin(), vocab.end());
    }
    size_t D = dicts.size(), V = vocab.size();
    std::unordered_map<std::string,size_t> idx;
    for (size_t j=0;j<V;++j) idx[vocab[j]] = j;
    std::vector<std::vector<int>> M(D, std::vector<int>(V,0));
    for (size_t i=0;i<D;++i) {
        for (auto& kv : dicts[i]) {
            auto it = idx.find(kv.first);
            if (it!=idx.end()) M[i][it->second] = kv.second;
        }
    }
    write_csv(outpath, vocab, docnames, M);
    double t1 = now_sec();
    std::cout << "time_sec=" << (t1 - t0) << "\n";
    return 0;
}
