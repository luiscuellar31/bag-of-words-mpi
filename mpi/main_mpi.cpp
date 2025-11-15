#include <mpi.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "tokenize.hpp"
#include "csv.hpp"
#include "timer.hpp"

static std::string basename(const std::string& p) {
    size_t pos = p.find_last_of("/\\");
    return pos==std::string::npos? p : p.substr(pos+1);
}

static void send_strings(int dst, const std::vector<std::string>& v, int tag) {
    int n = (int)v.size();
    MPI_Send(&n, 1, MPI_INT, dst, tag, MPI_COMM_WORLD);
    for (auto& s : v) {
        int len = (int)s.size();
        MPI_Send(&len, 1, MPI_INT, dst, tag+1, MPI_COMM_WORLD);
        if (len) MPI_Send(s.data(), len, MPI_CHAR, dst, tag+2, MPI_COMM_WORLD);
    }
}

static std::vector<std::string> recv_strings(int src, int tag) {
    MPI_Status st;
    int n=0; MPI_Recv(&n, 1, MPI_INT, src, tag, MPI_COMM_WORLD, &st);
    std::vector<std::string> v; v.reserve(n);
    for (int i=0;i<n;++i) {
        int len=0; MPI_Recv(&len, 1, MPI_INT, src, tag+1, MPI_COMM_WORLD, &st);
        std::string s; s.resize(len);
        if (len) MPI_Recv(s.data(), len, MPI_CHAR, src, tag+2, MPI_COMM_WORLD, &st);
        v.push_back(std::move(s));
    }
    return v;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size; MPI_Comm_rank(MPI_COMM_WORLD, &rank); MPI_Comm_size(MPI_COMM_WORLD, &size);
    double t0 = now_sec();
    std::string outpath = "out/matriz.csv";
    std::vector<std::string> files_all;
    if (rank==0) {
        for (int i=1;i<argc;i++) {
            std::string a = argv[i];
            if (a == "--out" && i+1 < argc) { outpath = argv[++i]; }
            else { files_all.push_back(a); }
        }
    }
    int Nall = rank==0 ? (int)files_all.size() : 0;
    // difunde número de archivos
    MPI_Bcast(&Nall, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (Nall==0) {
        if (rank==0) std::cout << "time_sec=" << (now_sec()-t0) << "\n";
        MPI_Finalize();
        return 0;
    }
    std::vector<int> counts(size,0), offsets(size,0);
    int base = Nall / size, rem = Nall % size;
    for (int r=0;r<size;r++) counts[r] = base + (r < rem ? 1 : 0);
    for (int r=1;r<size;r++) offsets[r] = offsets[r-1] + counts[r-1];
    if (rank==0) {
        for (int r=1;r<size;r++) {
            std::vector<std::string> slice;
            slice.reserve(counts[r]);
            for (int i=0;i<counts[r];++i) slice.push_back(files_all[offsets[r]+i]);
            send_strings(r, slice, 100);
        }
    }
    std::vector<std::string> my_files;
    if (rank==0) {
        my_files.reserve(counts[0]);
        for (int i=0;i<counts[0];++i) my_files.push_back(files_all[offsets[0]+i]);
    } else {
        my_files = recv_strings(0, 100);
    }
    std::vector<std::string> my_docnames;
    std::vector<std::unordered_map<std::string,int>> my_dicts;
    for (auto& fp : my_files) {
        std::ifstream f(fp);
        std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        auto toks = tokenize(s);
        std::unordered_map<std::string,int> m;
        for (auto& w : toks) m[w]++;
        my_dicts.push_back(std::move(m));
        my_docnames.push_back(basename(fp));
    }
    std::unordered_set<std::string> my_terms;
    for (auto& m : my_dicts) for (auto& kv : m) my_terms.insert(kv.first);
    std::vector<std::string> my_terms_vec(my_terms.begin(), my_terms.end());
    std::string packed;
    if (rank==0) {
        std::unordered_set<std::string> all;
        for (auto& t : my_terms_vec) all.insert(t);
        for (int r=1;r<size;r++) {
            auto v = recv_strings(r, 200);
            for (auto& t : v) all.insert(t);
        }
        std::vector<std::string> vocab_tmp(all.begin(), all.end());
        std::sort(vocab_tmp.begin(), vocab_tmp.end());
        size_t total_len=0; for (auto& s : vocab_tmp) total_len += s.size()+1;
        packed.reserve(total_len);
        for (auto& s : vocab_tmp) { packed.append(s); packed.push_back('\n'); }
    } else {
        send_strings(0, my_terms_vec, 200);
    }
    int L = rank==0 ? (int)packed.size() : 0;
    MPI_Bcast(&L, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank!=0) packed.resize(L);
    if (L) MPI_Bcast(packed.data(), L, MPI_CHAR, 0, MPI_COMM_WORLD);
    std::vector<std::string> vocab;
    {
        std::string cur;
        for (char c : packed) {
            if (c=='\n') { if (!cur.empty()) { vocab.push_back(cur); cur.clear(); } }
            else cur.push_back(c);
        }
        if (!cur.empty()) vocab.push_back(cur);
    }
    std::unordered_map<std::string,int> vidx;
    for (int j=0;j<(int)vocab.size();++j) vidx[vocab[j]] = j;
    std::vector<int> my_flat;
    int V = (int)vocab.size();
    int Dloc = (int)my_dicts.size();
    my_flat.assign(Dloc*V, 0);
    for (int i=0;i<Dloc;++i) {
        for (auto& kv : my_dicts[i]) {
            auto it = vidx.find(kv.first);
            if (it!=vidx.end()) my_flat[i*V + it->second] = kv.second;
        }
    }
    int Dall = Nall;
    std::vector<int> recvcounts, displs;
    if (rank==0) {
        recvcounts.resize(size);
        displs.resize(size);
        for (int r=0;r<size;r++) recvcounts[r] = counts[r]*V;
        displs[0]=0; for (int r=1;r<size;r++) displs[r] = displs[r-1] + recvcounts[r-1];
    }
    std::vector<int> all_flat;
    if (rank==0) all_flat.assign(Dall*V, 0);
    MPI_Gatherv(my_flat.data(), Dloc*V, MPI_INT,
                rank==0? all_flat.data():nullptr,
                rank==0? recvcounts.data():nullptr,
                rank==0? displs.data():nullptr,
                MPI_INT, 0, MPI_COMM_WORLD);
    double t1 = now_sec();
    if (rank==0) {
        std::vector<std::string> docnames;
        docnames.reserve(Dall);
        for (auto& fp : files_all) docnames.push_back(basename(fp));
        std::vector<std::vector<int>> M;
        M.resize(Dall, std::vector<int>(V,0));
        for (int i=0;i<Dall;++i) for (int j=0;j<V;++j) M[i][j] = all_flat[i*V+j];
        write_csv(outpath, vocab, docnames, M);
        double parallel_sec = t1 - t0;
        std::cout << "time_sec=" << parallel_sec << "\n";
    }
    MPI_Finalize();
    return 0;
}
