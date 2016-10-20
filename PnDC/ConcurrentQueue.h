#pragma once

#include <queue>
#include <mutex>

//! \brief A concurrent queue that can be accessed from multiple threads
template<typename T>
class ConcurrentQueue
{
public:
   T Dequeue()
   {
      std::lock_guard<std::mutex> guard(_lock);
      auto ret = std::move( _queue.front() );
      _queue.pop();
      return ret;
   }

   void Enqueue(const T& elem)
   {
      std::lock_guard<std::mutex> guard(_lock);
      _queue.push(elem);
   }

   void Enqueue(T&& elem)
   {
      std::lock_guard<std::mutex> guard(_lock);
      _queue.push(std::move(elem));
   }

   bool IsEmpty() const
   {
      std::lock_guard<std::mutex> guard(_lock);
      return _queue.empty();
   }
private:
   std::queue<T> _queue;
   mutable std::mutex _lock;
};