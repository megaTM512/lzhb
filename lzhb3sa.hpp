#ifndef __LZHB3SA_HPP__
#define __LZHB3SA_HPP__
#include <string>
#include <vector>

#include "lzhb_common.hpp"
#include <functional>
namespace lzhb3sa {
struct CostParams {
  double ALPHA;
  double BETA;
  double GAMMA;
};
CostParams defaultCostParams();
std::function<double(uInt, uInt, uInt)> makeCostFunctionWithParamsC1(
    const CostParams& params);
std::function<double(uInt, uInt, uInt)> makeCostFunctionWithParamsC2(
    const CostParams& params);
std::vector<lzhb::Phrase> parse(const std::string& s, uInt height_bound);
std::vector<lzhb::Phrase> parseGreedier(const std::string& s,
                                        uInt height_bound);
std::vector<lzhb::PhraseC> parseC(const std::string& s, uInt height_bound);
std::vector<lzhb::PhraseC> parseGreedierC(const std::string& s,
                                          uInt height_bound, uInt threshold, const CostParams& params, uInt costFunctionId);
}  // namespace lzhb3sa
#endif  // __LZHB3SA_HPP__