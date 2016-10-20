#pragma once
#include <thread>
#include <vector>
#include <future>

#include "ConcurrentQueue.h"

namespace task
{

   class ITask
   {
   public:
      virtual ~ITask() {}
      virtual void Run() = 0;
   };

   template<typename _Task>
   class Task : public ITask
   {
   public:
      explicit Task(_Task&& t) :
         _task(std::forward<_Task>(t)) {}

      void Run() override { _task(); }
   protected:
      _Task _task;
   };

   template<typename _Task, typename Result>
   class AwaitableTask : public ITask
   {
   public:
      explicit AwaitableTask(_Task&& t) :
         _task(std::forward<_Task>(t)) {}

      std::future<Result> GetFuture()
      {
         return _promise.get_future();
      }

      void Run() override
      {
         _promise.set_value(std::invoke(_task));
      }
   private:
      _Task _task;
      std::promise<Result> _promise;
   };

   template<typename _Task>
   class AwaitableTask<_Task, void> : public ITask
   {
   public:
      explicit AwaitableTask(_Task&& t) :
         _task(std::forward<_Task>(t)) {}

      std::future<void> GetFuture()
      {
         return _promise.get_future();
      }

      void Run() override
      {
         std::invoke(_task);
         _promise.set_value();
      }
   private:
      _Task _task;
      std::promise<void> _promise;
   };

   namespace impl
   {
      void AddTaskImpl(std::unique_ptr<ITask> task);
   }

   namespace
   {      
      template<typename Ret, typename Func>
      decltype(auto) MakeUniqueHelper(Func&& func)
      {
         return std::make_unique<AwaitableTask<Func, Ret>>(std::forward<Func>(func));
      }

      template<typename Ret>
      struct MakeAwaitableTaskHelper
      {
         template<
            typename Func,
            typename... Args
         >
         static decltype(auto) MakeTask(Func&& func, Args&&... args)
         {            
            return MakeUniqueHelper<Ret>(
                  [func,
                  args = std::make_tuple(std::forward<Args>(args)...)]() mutable
               {
                  return InvokeFromTuple_WithReturnType(func, std::move(args));
               });
         }
      };

      template<>
      struct MakeAwaitableTaskHelper<void>
      {
         template<
            typename Func,
            typename... Args
         >
         static decltype(auto) MakeTask(Func&& func, Args&&... args)
         {
            return MakeUniqueHelper<void>(
               [func,
               args = std::make_tuple(std::forward<Args>(args)...)]() mutable
            {
               InvokeFromTuple(func, std::move(args));
            });
         }
      };
   }

   void Initialize();
   void Shutdown();

   //! \brief Adds a new task to the task system
   //! \param taskFunc The task function that shall be executed
   template<typename TaskFunc, typename... Args>
   void AddTask(TaskFunc&& taskFunc, Args&&... args)
   {
      auto task = std::make_unique<Task<TaskFunc>>(
         [taskFunc = std::forward<TaskFunc>(taskFunc),
         args = std::make_tuple(std::forward<Args>(args)...)]() mutable
      {
         InvokeFromTuple(std::forward<TaskFunc>(taskFunc), std::move(args));
      });
      impl::AddTaskImpl(std::move(task));
   }

   //! \brief Adds a new awaitable task to the task system
   //! \param taskFunc The task function that shall be executed
   //! \returns Future object that stores the result of the task
   template<typename TaskFunc, typename... Args>
   decltype(auto) AddAwaitableTask(TaskFunc taskFunc, Args&&... args)
   {
      using Result_t = decltype(taskFunc(std::forward<Args>(args)...));
      auto task = MakeAwaitableTaskHelper<Result_t>::MakeTask(taskFunc, std::forward<Args>(args)...);
      auto future = task->GetFuture();
      impl::AddTaskImpl(std::move(task));
      return future;
   }

   //! \brief Returns the maximum number of parallel tasks that can be run in this task system
   size_t GetMaxConcurrency();

}