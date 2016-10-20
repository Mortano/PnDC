#pragma once
#include <tuple>

namespace
{


   template<typename Func, typename... Args, size_t... Idx>
   void InvokeFromTupleImpl(Func&& func, std::tuple<Args...>&& args, std::index_sequence<Idx...>)
   {
      std::invoke(std::forward<Func>(func), std::get<Idx>(args)...);
   }

   template<typename Func, typename... Args, size_t... Idx>
   decltype(auto) InvokeFromTuple_WithReturnTypeImpl(Func&& func, std::tuple<Args...>&& args, std::index_sequence<Idx...>)
   {
      return std::invoke(std::forward<Func>(func), std::get<Idx>(args)...);
   }

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

//! \brief Invokes a function with a tuple that contains all function arguments
//! \param func Functor to invoke
//! \param args Arguments for the function as std::tuple
template<typename Func, typename... Args>
void InvokeFromTuple(Func&& func, std::tuple<Args...>&& args)
{
   InvokeFromTupleImpl(std::forward<Func>(func), std::move(args), std::index_sequence_for<Args...>());
}

//! \brief Invokes a function with a tuple that contains all function arguments and returns the result
//! \param func Functor to invoke
//! \param args Arguments for the function as std::tuple
template<typename Func, typename... Args>
decltype(auto) InvokeFromTuple_WithReturnType(Func&& func, std::tuple<Args...>&& args)
{
   return InvokeFromTuple_WithReturnTypeImpl(std::forward<Func>(func), std::move(args), std::index_sequence_for<Args...>());
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
