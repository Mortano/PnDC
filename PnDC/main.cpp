
#include "Sorting.h"
#include "ParallelUtil.h"

#include <vector>
#include <random>
#include <numeric>
#include <iostream>
#include <numeric>
#include <chrono>
#include "RuntimeMeasurement.h"

auto RandomNumbers(size_t count)
{
   static std::random_device s_rnd;
   static std::uniform_int_distribution<size_t> s_distr(0, 1000);
   std::vector<size_t> ret(count, 0);
   std::generate(ret.begin(), ret.end(), [&]() { return s_distr(s_rnd); });
   return ret;
}

template<typename T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& vec)
{
   stream << "[";
   for(size_t idx = 0; idx < vec.size(); idx++)
   {
      if(idx != vec.size() - 1)
      {
         stream << vec[idx] << ", ";
      }
      else
      {
         stream << vec[idx];
      }
   }
   stream << "]";
   stream.flush();
   return stream;
}

std::ostream& operator<<(std::ostream& stream, const rt::RuntimeStats& stats)
{
   stream << "\tAvg.: [" << std::chrono::duration_cast<std::chrono::milliseconds>(stats._average).count() << " ms]\n";
   stream << "\tWorst: [" << std::chrono::duration_cast<std::chrono::milliseconds>(stats._worst).count() << " ms]\n";
   stream << "\tBest: [" << std::chrono::duration_cast<std::chrono::milliseconds>(stats._best).count() << " ms]\n";
   stream << "\tCold cache time: [" << std::chrono::duration_cast<std::chrono::milliseconds>(stats._coldCacheTime).count() << " ms]\n";
   stream.flush();
   return stream;
}

size_t ParallelSum(const std::vector<size_t>& numbers)
{
   return ParallelDivideAndConquer(
      numbers, 8,
      [](const std::vector<size_t>& vec, size_t chunks)
      {
         using Iter_t = decltype(vec.begin());
         _ASSERT(chunks > 1);
         auto elementsPerChunk = vec.size() / chunks;
         std::vector<std::pair<Iter_t, Iter_t>> ret;
         ret.reserve(chunks);
         for(size_t idx = 0; idx < chunks - 1; idx++)
         {
            ret.push_back(std::make_pair(vec.begin() + (idx*elementsPerChunk), vec.begin() + ((idx + 1) * elementsPerChunk)));
         }
         ret.push_back(std::make_pair(vec.begin() + (chunks - 1)*elementsPerChunk, vec.end()));
         return ret;
      },
      [](auto l, auto r) { return l + r; },
      [](auto elem) { return std::accumulate(elem.first, elem.second, 0); });
}

int main(int argc, char** argv)
{
   constexpr size_t NumberCount = 1'000'000;
   constexpr size_t Iterations = 100;
   auto rndNumbers = RandomNumbers(NumberCount);
   auto rndNumbersCopy = rndNumbers;
   
   auto sequentialSortStas = rt::CollectRuntimeStats([](auto& numbers)
      {
         SequentialSort(numbers.begin(), numbers.end());
      }, 
      [=]() { return RandomNumbers(NumberCount); },
      Iterations);
   auto parallelSortStats = rt::CollectRuntimeStats([&](auto& numbers)
      {
         ParallelSort<4>(numbers.begin(), numbers.end());
      }, 
      [=]() { return RandomNumbers(NumberCount); },
      Iterations);

   std::cout << "######## Sequential sort stats ########\n";
   std::cout << sequentialSortStas;
   std::cout << "######## Parallel sort stats ########\n";
   std::cout << parallelSortStats;

   getchar();

   return 0;
}