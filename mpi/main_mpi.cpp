#include <mpi.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "tokenize.hpp"
#include "timer.hpp"

namespace {

struct Triplet {
    int doc;
    int term;
    int count;
};

std::string basename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

std::vector<std::string> blob_to_terms(const std::vector<char>& buf) {
    std::vector<std::string> terms;
    std::string cur;
    for (char c : buf) {
        if (c == '\0') {
            if (!cur.empty()) {
                terms.push_back(std::move(cur));
                cur.clear();
            }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) terms.push_back(std::move(cur));
    return terms;
}

std::vector<char> pack_terms(const std::vector<std::string>& terms) {
    size_t total = 0;
    for (const auto& t : terms) total += t.size() + 1;
    std::vector<char> blob;
    blob.reserve(total);
    for (const auto& t : terms) {
        blob.insert(blob.end(), t.begin(), t.end());
        blob.push_back('\0');
    }
    return blob;
}

void broadcast_files(std::vector<std::string>& files, int root) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int count = rank == root ? static_cast<int>(files.size()) : 0;
    MPI_Bcast(&count, 1, MPI_INT, root, MPI_COMM_WORLD);
    if (count == 0) {
        files.clear();
        return;
    }
    std::vector<char> blob;
    if (rank == root) blob = pack_terms(files);
    int blob_len = rank == root ? static_cast<int>(blob.size()) : 0;
    MPI_Bcast(&blob_len, 1, MPI_INT, root, MPI_COMM_WORLD);
    if (rank != root) blob.resize(static_cast<size_t>(blob_len));
    if (blob_len > 0) {
        MPI_Bcast(blob.data(), blob_len, MPI_CHAR, root, MPI_COMM_WORLD);
    }
    if (rank != root) files = blob_to_terms(blob);
}

std::pair<int, int> doc_range(int total_docs, int world_size, int world_rank) {
    int base = total_docs / world_size;
    int extra = total_docs % world_size;
    int start = world_rank * base + std::min(world_rank, extra);
    int count = base + (world_rank < extra ? 1 : 0);
    return {start, start + count};
}

std::unordered_map<std::string, int> bag_of_words(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto tokens = tokenize(text);
    std::unordered_map<std::string, int> dict;
    dict.reserve(tokens.size());
    for (const auto& w : tokens) dict[w]++;
    return dict;
}

}  // namespace

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    double t0 = now_sec();

    std::string outpath = "out/matriz.csv";
    std::vector<std::string> files;

    if (rank == 0) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--out" && i + 1 < argc) {
                outpath = argv[++i];
            } else {
                files.push_back(arg);
            }
        }
    }

    broadcast_files(files, 0);
    int total_docs = static_cast<int>(files.size());

    if (total_docs == 0) {
        if (rank == 0) std::cout << "time_sec=" << (now_sec() - t0) << "\n";
        MPI_Finalize();
        return 0;
    }

    auto [start_doc, end_doc] = doc_range(total_docs, size, rank);
    std::vector<int> my_doc_ids;
    std::vector<std::unordered_map<std::string, int>> my_dicts;
    for (int doc = start_doc; doc < end_doc; ++doc) {
        my_doc_ids.push_back(doc);
        my_dicts.push_back(bag_of_words(files[doc]));
    }

    std::vector<std::string> local_terms;
    size_t approx_terms = 0;
    for (const auto& dict : my_dicts) approx_terms += dict.size();
    local_terms.reserve(approx_terms);
    for (const auto& dict : my_dicts) {
        for (const auto& kv : dict) local_terms.push_back(kv.first);
    }
    std::sort(local_terms.begin(), local_terms.end());
    local_terms.erase(std::unique(local_terms.begin(), local_terms.end()), local_terms.end());

    auto local_vocab_blob = pack_terms(local_terms);
    int local_vocab_len = static_cast<int>(local_vocab_blob.size());
    std::vector<int> vocab_counts;
    if (rank == 0) vocab_counts.resize(size);
    MPI_Gather(&local_vocab_len, 1, MPI_INT,
               rank == 0 ? vocab_counts.data() : nullptr, 1, MPI_INT,
               0, MPI_COMM_WORLD);

    std::vector<int> vocab_displs;
    int total_vocab_chars = 0;
    std::vector<char> gathered_vocab;
    if (rank == 0) {
        vocab_displs.assign(size, 0);
        for (int r = 0; r < size; ++r) {
            if (r > 0) vocab_displs[r] = vocab_displs[r - 1] + vocab_counts[r - 1];
            total_vocab_chars += vocab_counts[r];
        }
        gathered_vocab.resize(static_cast<size_t>(total_vocab_chars));
    }

    MPI_Gatherv(local_vocab_blob.empty() ? nullptr : local_vocab_blob.data(),
                local_vocab_len, MPI_CHAR,
                rank == 0 ? (gathered_vocab.empty() ? nullptr : gathered_vocab.data()) : nullptr,
                rank == 0 ? vocab_counts.data() : nullptr,
                rank == 0 ? vocab_displs.data() : nullptr,
                MPI_CHAR, 0, MPI_COMM_WORLD);

    std::vector<std::string> vocab;
    std::vector<char> vocab_blob;
    if (rank == 0) {
        vocab = blob_to_terms(gathered_vocab);
        std::sort(vocab.begin(), vocab.end());
        vocab.erase(std::unique(vocab.begin(), vocab.end()), vocab.end());
        vocab_blob = pack_terms(vocab);
    }

    int vocab_blob_len = rank == 0 ? static_cast<int>(vocab_blob.size()) : 0;
    MPI_Bcast(&vocab_blob_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (vocab_blob_len > 0) {
        if (rank != 0) vocab_blob.resize(static_cast<size_t>(vocab_blob_len));
        MPI_Bcast(vocab_blob.data(), vocab_blob_len, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    if (rank != 0) vocab = blob_to_terms(vocab_blob);

    std::unordered_map<std::string, int> vidx;
    vidx.reserve(vocab.size() * 2 + 8);
    for (int j = 0; j < static_cast<int>(vocab.size()); ++j) vidx[vocab[j]] = j;

    std::vector<Triplet> local_entries;
    size_t sum_terms = 0;
    for (const auto& dict : my_dicts) sum_terms += dict.size();
    local_entries.reserve(sum_terms);
    for (size_t idx = 0; idx < my_dicts.size(); ++idx) {
        int doc_id = my_doc_ids[idx];
        for (const auto& kv : my_dicts[idx]) {
            auto it = vidx.find(kv.first);
            if (it != vidx.end()) {
                local_entries.push_back(Triplet{doc_id, it->second, kv.second});
            }
        }
    }

    std::vector<int> local_triplet_buf;
    local_triplet_buf.reserve(local_entries.size() * 3);
    for (const auto& entry : local_entries) {
        local_triplet_buf.push_back(entry.doc);
        local_triplet_buf.push_back(entry.term);
        local_triplet_buf.push_back(entry.count);
    }

    int local_triplet_count = static_cast<int>(local_entries.size());
    std::vector<int> triplet_counts;
    if (rank == 0) triplet_counts.resize(size);
    MPI_Gather(&local_triplet_count, 1, MPI_INT,
               rank == 0 ? triplet_counts.data() : nullptr, 1, MPI_INT,
               0, MPI_COMM_WORLD);

    std::vector<int> triplet_recvcounts;
    std::vector<int> triplet_displs;
    int total_triplets = 0;
    std::vector<int> gathered_triplets;
    if (rank == 0) {
        triplet_recvcounts.assign(size, 0);
        triplet_displs.assign(size, 0);
        int offset = 0;
        for (int r = 0; r < size; ++r) {
            triplet_recvcounts[r] = triplet_counts[r] * 3;
            triplet_displs[r] = offset;
            offset += triplet_recvcounts[r];
            total_triplets += triplet_counts[r];
        }
        gathered_triplets.resize(static_cast<size_t>(offset));
    }

    MPI_Gatherv(local_triplet_buf.empty() ? nullptr : local_triplet_buf.data(),
                static_cast<int>(local_triplet_buf.size()), MPI_INT,
                rank == 0 ? (gathered_triplets.empty() ? nullptr : gathered_triplets.data()) : nullptr,
                rank == 0 ? triplet_recvcounts.data() : nullptr,
                rank == 0 ? triplet_displs.data() : nullptr,
                MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::vector<std::string> docnames;
        docnames.reserve(total_docs);
        for (const auto& path : files) docnames.push_back(basename(path));

        std::vector<Triplet> all_entries;
        all_entries.reserve(static_cast<size_t>(total_triplets));
        for (int i = 0; i < total_triplets; ++i) {
            Triplet t{gathered_triplets[3 * i + 0],
                      gathered_triplets[3 * i + 1],
                      gathered_triplets[3 * i + 2]};
            all_entries.push_back(t);
        }
        std::sort(all_entries.begin(), all_entries.end(),
                  [](const Triplet& a, const Triplet& b) {
                      if (a.doc != b.doc) return a.doc < b.doc;
                      return a.term < b.term;
                  });

        std::ofstream out(outpath);
        out << "doc";
        for (const auto& term : vocab) out << "," << term;
        out << "\n";

        size_t idx = 0;
        for (int doc = 0; doc < total_docs; ++doc) {
            out << docnames[doc];
            int term_cursor = 0;
            while (idx < all_entries.size() && all_entries[idx].doc == doc) {
                int term_id = all_entries[idx].term;
                while (term_cursor < term_id) {
                    out << ",0";
                    ++term_cursor;
                }
                out << "," << all_entries[idx].count;
                ++idx;
                ++term_cursor;
            }
            while (term_cursor < static_cast<int>(vocab.size())) {
                out << ",0";
                ++term_cursor;
            }
            out << "\n";
        }

        double elapsed = now_sec() - t0;
        std::cout << "time_sec=" << elapsed << "\n";
    }

    MPI_Finalize();
    return 0;
}
