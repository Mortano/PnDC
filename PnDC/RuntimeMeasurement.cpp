#include "RuntimeMeasurement.h"
#include <vector>
#include <algorithm>
#include <numeric>

namespace rt
{
   RuntimeStats CollectRuntimeStats(const std::function<void()>& func, size_t repetitions)
   {
      if (!repetitions) return RuntimeStats{};

      std::vector<std::chrono::microseconds> runtimes;
      for(size_t idx = 0; idx < repetitions; idx++)
      {
         auto startTime = std::chrono::high_resolution_clock::now();
         func();
         auto endTime = std::chrono::high_resolution_clock::now();
         auto delta = endTime - startTime;
         runtimes.push_back(std::chrono::duration_cast<std::chrono::microseconds>(delta));
      }

      auto minmaxTime = std::minmax_element(runtimes.begin(), runtimes.end());
      auto avgTime = std::accumulate(runtimes.begin(), runtimes.end(), std::chrono::microseconds()) / repetitions;
      return{ runtimes.front(), *minmaxTime.first, avgTime, *minmaxTime.second };
   }
}
