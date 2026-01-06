#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <tuple>
#include <vector>

#include "lzhb3sa.hpp"
#include "lzhb_common.hpp"

#define CSV_HEADER \
  "INPUT,COST_FUNC,ALPHA,BETA,GAMMA,HEIGHT_BOUND,THRESHOLD,AVERAGE_HEIGHT,ORIGINAL_SIZE,PARSE_SIZE\n"
#define CURRENT_COST_FUNCTION "C2"

struct ParseResult {
  lzhb3sa::CostParams params;
  double average_height;
  uint64_t parse_size;
};

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
      if (p.src + j >= phrase_start) {
        heights.push_back(heights[p.src + j]);  // Self-Reference
      } else
        heights.push_back(heights[p.src + j] + 1);
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

// Random sampling framework
std::vector<ParseResult> randomParameterSearch(const std::string& s,
                                               uInt height_bound,
                                               uInt threshold, int samples,
                                               const std::string& input_file) {
  std::mt19937 rng(std::random_device{}());
  std::uniform_real_distribution<double> alpha_dist(0.5, 3.0);
  std::uniform_real_distribution<double> beta_dist(0.05, 0.5);
  std::uniform_real_distribution<double> gamma_dist(-1.0, 1.0);

  std::vector<ParseResult> results;
  double best_score = std::numeric_limits<double>::max();
  lzhb3sa::CostParams best_params;
  double best_avg_height = std::numeric_limits<double>::max();
  double best_parse_size = std::numeric_limits<double>::max();

  for (int i = 0; i < samples; i++) {
    lzhb3sa::CostParams params{alpha_dist(rng), beta_dist(rng),
                               gamma_dist(rng)};

    std::cout << "Testing parameters: ALPHA=" << params.ALPHA
              << ", BETA=" << params.BETA << ", GAMMA=" << params.GAMMA << "\n";
    auto phrases = lzhb3sa::parseGreedierC(s, height_bound, threshold, params);

    double avg_height = computeAverageHeight(phrases);
    uint64_t parse_size =
        static_cast<uint64_t>(phrases.size()) * sizeof(lzhb::PhraseC);

    auto result = ParseResult{params, avg_height, parse_size};

    std::ofstream csv_file("parameter_search_results.csv", std::ios::app);

    csv_file << input_file << "," << CURRENT_COST_FUNCTION << ","
         << params.ALPHA << "," << params.BETA << "," << params.GAMMA << ","
         << height_bound << "," << threshold << ","
         << result.average_height << "," << s.size() * sizeof(char) << ","
         << result.parse_size << "\n";

    csv_file.close();

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
            << "ALPHA=" << best_params.ALPHA << ", BETA=" << best_params.BETA
            << ", GAMMA=" << best_params.GAMMA << "\n"
            << " -> avg_height=" << best_avg_height
            << ", parse_size=" << best_parse_size << "\n";
  return results;
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " <input_file> <height_bound> <threshold> [samples]\n";
    return 1;
  }

  std::string input_file = argv[1];
  uInt height_bound = std::stoull(argv[2]);
  uInt threshold = std::stoull(argv[3]);
  int samples = (argc >= 5) ? std::stoi(argv[4]) : 100;

  std::string s = lzhb::fileread(input_file);
  // The string can be very large; consider using a substring for testing
  std::string test_str = s.substr(
      0, std::min<size_t>(s.size(), 10000000));  // First 10 million chars

  auto result = randomParameterSearch(test_str, height_bound, threshold,
                                      samples, input_file);

  return 0;
}