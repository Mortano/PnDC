#pragma once

#include <type_traits>
#include <algorithm>
#include <thread>
#include <condition_variable>
#include <vector>

#include "ParallelUtil.h"

template<typename Iter>
void SequentialSort(Iter iBegin, Iter iEnd)
{   
   std::sort(iBegin, iEnd);
}

namespace
{

   template<typename Iter>
   auto SplitRange(Iter begin, Iter end, size_t numChunks)
   {
      using Pair_t = decltype(std::make_pair(begin, begin + 1));
      auto dist = end - begin;
      auto rangeSize = dist / numChunks;
      std::vector<Pair_t> chunks;
      chunks.reserve(numChunks);
      for(size_t i = 0; i < numChunks - 1; i++)
      {
         chunks.push_back( std::make_pair(begin + i*rangeSize, begin + (i + 1)*rangeSize) );
      }
      chunks.push_back(std::make_pair(begin + (numChunks - 1)*rangeSize, end));
      return chunks;
   }

}

template<size_t Cores, typename Iter>
void ParallelSort(Iter begin, Iter end)
{
   static_assert(Cores > 1, "Parallel sort requires more than one core!");
   if (Cores > std::thread::hardware_concurrency()) throw std::exception("Machine has insufficient cores!");
   
   auto res = ParallelDivideAndConquer(
      std::make_pair(begin, end),
      Cores,
      [](auto pair, size_t chunks) { return SplitRange(pair.first, pair.second, chunks); },
      [](auto l, auto r)
      {
         _ASSERT(l.second == r.first);
         std::inplace_merge(l.first, l.second, r.second); 
         return std::make_pair(l.first, r.second);
      },
      [](auto pair) { std::sort(pair.first, pair.second); return pair; },
      ExecParallelFlags::MergeIsTrivial
   );
}

template<typename Iter>
void NaiveParallelSort(Iter begin, Iter end)
{
   static constexpr size_t Threshold = 1024;
   auto size = std::distance(begin, end);
   if(size <= Threshold)
   {
      //Range size is small, we can use an existing sorting algorithm here
      std::sort(begin, end);
      return;
   }
   auto mid = begin + (size / 2);
   ExecParallel(
      [&]() { NaiveParallelSort(begin, mid); },
      [&]() { NaiveParallelSort(mid, end); });
   std::inplace_merge(begin, mid, end);
}

//! \brief Parallel merge sort using the hand-written task system
template<typename Iter>
void TaskSystemParallelSort(Iter begin, Iter end)
{
   auto res = ParallelDivideAndConquer<true>(
      std::make_pair(begin, end),
      task::GetMaxConcurrency(),
      [](auto pair, size_t chunks) { return SplitRange(pair.first, pair.second, chunks); },
      [](auto l, auto r)
      {
         _ASSERT(l.second == r.first);
         std::inplace_merge(l.first, l.second, r.second);
         return std::make_pair(l.first, r.second);
      },
      [](auto pair) { std::sort(pair.first, pair.second); return pair; },
      ExecParallelFlags::MergeIsTrivial
      );
}