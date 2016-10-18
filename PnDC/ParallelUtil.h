#pragma once

#include <thread>
#include <future>
#include <algorithm>
#include <iterator>

#include "MathUtil.h"
#include "TupleUtil.h"

namespace
{
   template<typename T>
   void AwaitAllFutures(const std::vector<std::future<T>>& futures)
   {
      for (auto& f : futures) f.wait();
   }

   template<typename T>
   auto AggregateAllFutures(std::vector<std::future<T>>& futures)
   {
      using Result_t = decltype(futures[0].get());
      std::vector<Result_t> results;
      results.reserve(futures.size());
      std::transform(futures.begin(), futures.end(), std::back_inserter(results), [](auto& f) { return f.get(); });
      return results;
   }

   //! \brief Performs a binary fold operation on the given range. This takes pairs of consecutive elements
   //!        and folds them using the given fold function, then stores the results consecutively starting
   //!        from the beginning of the range
   //! \param begin Start of the range
   //! \param end End of the range
   //! \param fold Fold function
   //! \returns End of the range after fold
   template<typename RndIter, typename Func>
   RndIter BinaryFold(RndIter begin, RndIter end, Func fold)
   {
      auto count = std::distance(begin, end);
      _ASSERT(math::IsEven(count));
      for (size_t idx = 0; idx < count; idx += 2)
      {
         auto writeIdx = idx / 2;
         *(begin + writeIdx) = fold(*(begin + idx), *(begin + idx + 1));
      }
      return begin + (count / 2);
   }

   //! \brief Performs a parllel binary fold operation on the given range. This takes pairs of consecutive elements
   //!        and folds them using the given fold function, then stores the results consecutively starting
   //!        from the beginning of the range
   //! \param begin Start of the range
   //! \param end End of the range
   //! \param fold Fold function
   //! \returns End of the range after fold
   template<typename RndIter, typename Func>
   RndIter ParallelBinaryFold(RndIter begin, RndIter end, Func fold)
   {
      auto count = std::distance(begin, end);
      _ASSERT(math::IsEven(count));
      std::vector<std::future<void>> futures;
      futures.reserve(count / 2);
      for (size_t idx = 0; idx < count; idx += 2)
      {
         auto writeIdx = idx / 2;
         futures.push_back(std::async(std::launch::async, [=]()
         {
            *(begin + writeIdx) = fold(*(begin + idx), *(begin + idx + 1));
         }));
      }
      AwaitAllFutures(futures);
      return begin + (count / 2);
   }

   template<typename... Func, size_t... Idx>
   void ExecParallelImpl(std::tuple<Func...> functors, std::index_sequence<Idx...>)
   {
      auto futures = TransformTuple(functors, [](auto func) { return std::async(std::launch::async, func); });
      TupleForEach(futures, [](auto& future) { future.wait(); });
   }
}

enum class ExecParallelFlags
{
   None = 0,
   MergeIsTrivial = 1
};

constexpr bool operator&(ExecParallelFlags l, ExecParallelFlags r)
{
   using Underlying_t = std::underlying_type_t<ExecParallelFlags>;
   return (static_cast<Underlying_t>(l) & static_cast<Underlying_t>(r)) != 0;
}

//! \brief Runs a parallel divide-and-conquer style algorithm. The algorithm first splits a piece of data into 
//!        separate chunks, then runs a base algorithm on each of these pieces of data in parallel. Then, a parallel
//!        merge step is performed which essentially does pair-wise merging of the results of the base algorithm until
//!        all data is aggregated in a single final data set, which is returned.
//! \param data Root piece of data for the parallel algorithm
//! \param subtasks Number of chunks that the data should be split into. This is effectively the 'parallelity' of the algorithm
//! \param splitFunc A functor that takes an element of type 'Elem' and a size_t indicating into how many chunks the data should be split
//! \param mergeFunc A functor that performs a merge operation on two data chunks 
//! \param rootTask The root task to be executed for the split data chunks
//! \param flags Optional flags for the execution
template<
   typename Elem,
   typename Split,
   typename Merge,
   typename RootTask
>
auto ParallelDivideAndConquer(
   Elem&& data,
   size_t subtasks,
   Split splitFunc,
   Merge mergeFunc,
   RootTask rootTask,
   const ExecParallelFlags flags = ExecParallelFlags::None)
{
   if (subtasks % 2 != 0) throw std::exception("Currently only an even number of subtasks is supported!");

   auto dataChunks = splitFunc(data, subtasks);
   using RootFuture_t = decltype(std::async(std::launch::async, rootTask, *std::begin(dataChunks)));
   std::vector<RootFuture_t> rootFutures;
   rootFutures.reserve(subtasks);

   for (auto&& chunk : dataChunks)
   {
      rootFutures.push_back(std::async(std::launch::async, rootTask, chunk));
   }

   auto results = AggregateAllFutures(rootFutures);
   auto resultsBegin = results.begin();
   auto resultsEnd = results.end();

   //Now start a pair-wise merge
   if (flags & ExecParallelFlags::MergeIsTrivial)
   {
      while (std::distance(resultsBegin, resultsEnd) > 1)
      {
         resultsEnd = BinaryFold(resultsBegin, resultsEnd, mergeFunc);
      }
   }
   else
   {
      while (std::distance(resultsBegin, resultsEnd) > 1)
      {
         resultsEnd = ParallelBinaryFold(resultsBegin, resultsEnd, mergeFunc);
      }
   }


   return std::move(results[0]);
}

//! \brief Executes a range of functions in parallel
//! \param functors Functions
template<typename... Func>
void ExecParallel(Func&&... functors)
{
   ExecParallelImpl(std::forward_as_tuple(functors...), std::index_sequence_for<Func...>());
}