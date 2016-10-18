#pragma once
#include <tuple>

namespace
{

   template<typename... Args>
   void Sink(Args&&...) {}

   template<typename... Args, typename Func, size_t... Idx>
   void TupleForEachImpl(const std::tuple<Args...>& tuple, Func&& functor, std::index_sequence<Idx...>)
   {
      int dummy[] = { 0, ((void)functor(std::get<Idx>(tuple)), 0) ... };
      //Sink(functor(std::get<Idx>(tuple))...);
   }

   template<typename... Args, typename Func, size_t... Idx>
   decltype(auto) TransformTupleImpl(const std::tuple<Args...>& tuple, Func&& functor, std::index_sequence<Idx...>)
   {
      return std::make_tuple(functor(std::get<Idx>(tuple))...);
   }
}

//! \brief Calls a function on each element of the given tuple
//! \param tuple Tuple
//! \param functor Function to call on tuple elements
template<typename... Args, typename Func>
void TupleForEach(const std::tuple<Args...>& tuple, Func&& functor)
{
   TupleForEachImpl(tuple, std::forward<Func>(functor), std::index_sequence_for<Args...>());
}

//! \brief Transforms all elements of the given tuple with the given functor and returns the result as a new tuple
//! \param tuple Tuple
//! \param functor Function that transforms tuple elements
//! \returns Transformed tuple
template<typename... Args, typename Func>
decltype(auto) TransformTuple(const std::tuple<Args...>& tuple, Func&& functor)
{
   return TransformTupleImpl(tuple, std::forward<Func>(functor), std::index_sequence_for<Args...>());
}
