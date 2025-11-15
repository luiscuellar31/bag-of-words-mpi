#include "csv.hpp"
#include <fstream>

void write_csv(const std::string& outpath,
               const std::vector<std::string>& vocab,
               const std::vector<std::string>& docnames,
               const std::vector<std::vector<int>>& M) {
    std::ofstream f(outpath);
    f << "doc";
    for (const auto& t : vocab) f << "," << t;
    f << "\n";
    for (size_t i = 0; i < docnames.size(); ++i) {
        f << docnames[i];
        for (size_t j = 0; j < vocab.size(); ++j) f << "," << M[i][j];
        f << "\n";
    }
}
