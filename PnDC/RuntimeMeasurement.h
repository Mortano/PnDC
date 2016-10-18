#pragma once

#include <chrono>
#include <functional>
#include <algorithm>
#include <numeric>
#include <vector>

namespace rt
{
   
   struct RuntimeStats
   {
      std::chrono::microseconds _coldCacheTime;
      std::chrono::microseconds _best, _average, _worst;
   };

   //! \brief Collects runtime statistics for the given function by executing it repeatedly
   //! \param func Function to profile
   //! \param repetitions Number of repetitions
   //! \returns Statistics for the function
   RuntimeStats CollectRuntimeStats(const std::function<void()>& func, size_t repetitions);

   //! \brief Collects runtime statistics for the given function by executing it repeatedly. Calls an initialization function before
   //!        each iteration that is NOT profiled
   //! \param func Function to profile
   //! \param init Initialization function
   //! \param repetitions Number of repetitions
   //! \returns Statistics for the function
   template<typename Func, typename InitFunc>
   RuntimeStats CollectRuntimeStats(Func&& func, InitFunc&& init, size_t repetitions)
   {
      if (!repetitions) return RuntimeStats{};

      std::vector<std::chrono::microseconds> runtimes;
      for (size_t idx = 0; idx < repetitions; idx++)
      {
         auto&& funcArg = init();
         auto startTime = std::chrono::high_resolution_clock::now();
         func(funcArg);
         auto endTime = std::chrono::high_resolution_clock::now();
         auto delta = endTime - startTime;
         runtimes.push_back(std::chrono::duration_cast<std::chrono::microseconds>(delta));
      }

      auto minmaxTime = std::minmax_element(runtimes.begin(), runtimes.end());
      auto avgTime = std::accumulate(runtimes.begin(), runtimes.end(), std::chrono::microseconds()) / repetitions;
      return{ runtimes.front(), *minmaxTime.first, avgTime, *minmaxTime.second };
   }

}
