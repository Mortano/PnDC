#include "TaskSystem.h"

#include <condition_variable>
#include <Windows.h>

namespace
{
   void JoinAll(std::vector<std::thread>& threads)
   {
      for (auto& thread : threads) thread.join();
   }
}

namespace task
{
   
   std::vector<std::thread> s_threads;
   ConcurrentQueue<std::unique_ptr<ITask>> s_tasks;
   std::atomic_bool s_runTasks;

   std::condition_variable s_taskAwait;
   std::mutex s_taskAwaitLock;

   void ThreadFunc()
   {
      while(s_runTasks)
      {
         std::unique_ptr<ITask> task;

         {
            std::unique_lock<std::mutex> lock(s_taskAwaitLock);
            s_taskAwait.wait(lock);
            if (s_tasks.IsEmpty()) continue;
            task = s_tasks.Dequeue();
         }

         task->Run();
      }
   }

   void impl::AddTaskImpl(std::unique_ptr<ITask> task)
   {
      s_tasks.Enqueue(std::move(task));
      std::unique_lock<std::mutex> lock(s_taskAwaitLock);
      s_taskAwait.notify_one();
   }

   void Initialize()
   {
      s_runTasks = true;

      auto maxThreads = std::thread::hardware_concurrency();
      s_threads.reserve(maxThreads);
      for(size_t i = 0; i < maxThreads; i++)
      {
         s_threads.emplace_back(ThreadFunc);
         auto handle = s_threads[i].native_handle();
         SetThreadAffinityMask(handle, static_cast<DWORD_PTR>(1ull << i));
      }
   }

   void Shutdown()
   {
      s_runTasks = false;
      {
         std::unique_lock<std::mutex> lock(s_taskAwaitLock);
         s_taskAwait.notify_all();
      }
      JoinAll(s_threads);
   }

   size_t GetMaxConcurrency()
   {
      return std::thread::hardware_concurrency();
   }
}
