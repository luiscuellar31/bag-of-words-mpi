#pragma once
#include <string>
#include <vector>

void write_csv(const std::string& outpath,
               const std::vector<std::string>& vocab,
               const std::vector<std::string>& docnames,
               const std::vector<std::vector<int>>& M);

