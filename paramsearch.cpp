#include <random>
#include <vector>
#include <tuple>
#include <limits>
#include <iostream>
#include <algorithm>

#include "lzhb_common.hpp"
#include "lzhb3sa.hpp"

double computeAverageHeight(const std::vector<lzhb::PhraseC>& phrases) {
  std::vector<int> heights;

  for (uint32_t i = 0; i < phrases.size(); i++) {
    const auto& p = phrases[i];

    // Literal
    if (p.len == 1) {
      heights.push_back(0);
      continue;
    }

    auto phrase_start = heights.size();
    for (uint32_t j = 0; j < p.len - 1; j++) {
      if(p.src + j >= phrase_start) {
        heights.push_back(heights[p.src + j]); // Self-Reference
      }
      else heights.push_back(heights[p.src + j] + 1);
    }

    // appended literal
    heights.push_back(0);
  }

  long double totalHeight = 0.0;
  int maxHeight = 0;
  for (auto h : heights) {
    totalHeight += h;
    if (h > maxHeight) {
      maxHeight = h;
    }
  }

  double avgHeight = totalHeight / heights.size();
  return avgHeight;
}


double costFunctionWithParams(uInt len, uInt sumh, uInt height_bound, const lzhb3sa::CostParams& p) {
    double cost = p.ALPHA * (sumh / (len * height_bound)) +
                  p.BETA * std::log(len) +
                  p.GAMMA / len;
    return cost;
}

// Random sampling framework
void randomParameterSearch(const std::string& s,
                           uInt height_bound,
                           uInt threshold,
                           int samples) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> alpha_dist(-1.0, 2.0);
    std::uniform_real_distribution<double> beta_dist(-2.0, 1.0);
    std::uniform_real_distribution<double> gamma_dist(-1.0, 1.0);

    double best_score = std::numeric_limits<double>::max();
    lzhb3sa::CostParams best_params;
    double best_avg_height = std::numeric_limits<double>::max();
    double best_parse_size = std::numeric_limits<double>::max();

    for (int i = 0; i < samples; i++) {
        lzhb3sa::CostParams params{alpha_dist(rng), beta_dist(rng), gamma_dist(rng)};

        // Wrap cost function to use these parameters
        auto costWrapper = [&params](uInt len, uInt sumh, uInt height_bound) {
            return costFunctionWithParams(len, sumh, height_bound, params);
        };
        std::cout << "Testing parameters: ALPHA=" << params.ALPHA
                  << ", BETA=" << params.BETA
                  << ", GAMMA=" << params.GAMMA << "\n";
        auto phrases = lzhb3sa::parseGreedierC(s, height_bound, threshold, params);

        double avg_height = computeAverageHeight(phrases);
        double parse_size = static_cast<double>(phrases.size());

        // Simple combined score: lower is better
        double score = avg_height + 0.1 * parse_size;

        if (score < best_score) {
            best_score = score;
            best_params = params;
            std::cout << "New best: ALPHA=" << best_params.ALPHA
                      << ", BETA=" << best_params.BETA
                      << ", GAMMA=" << best_params.GAMMA
                      << " -> avg_height=" << avg_height
                      << ", parse_size=" << parse_size << "\n";
            best_avg_height = avg_height;
            best_parse_size = parse_size;
        }
    }

    std::cout << "Best parameters found:\n"
              << "ALPHA=" << best_params.ALPHA
              << ", BETA=" << best_params.BETA
              << ", GAMMA=" << best_params.GAMMA << "\n"
              << " -> avg_height=" << best_avg_height
              << ", parse_size=" << best_parse_size << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <height_bound> <threshold> [samples]\n";
        return 1;
    }

    std::string input_file = argv[1];
    uInt height_bound = std::stoull(argv[2]);
    uInt threshold = std::stoull(argv[3]);
    int samples = (argc >= 5) ? std::stoi(argv[4]) : 100;

    std::string s = lzhb::fileread(input_file);

    randomParameterSearch(s, height_bound, threshold, samples);

    return 0;
}