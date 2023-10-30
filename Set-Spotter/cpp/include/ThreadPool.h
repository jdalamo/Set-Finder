//
//  ThreadPool.hpp
//  Set-Spotter
//
//  Created by JD del Alamo on 9/9/23.
//

#pragma once

#include <pthread.h>
#include <vector>
#include <queue>
#include <iostream>

namespace ThreadPool {

const int DEFAULT_NUM_THREADS = 3;

template <typename T>
struct PoolTaskArg {
   typename T::iterator start;
   typename T::iterator end;
};

enum class PoolTaskStatus {
   NOT_STARTED,
   RUNNING,
   FAILED,
   SUCCEEDED
};

class PoolTask {
public:
   PoolTask();
   ~PoolTask();

   void(*func)(void*);
   void* arg;
   bool threadCancelled = false;
   PoolTaskStatus status;
   pthread_mutex_t statusMutex;
   pthread_cond_t statusCond;
};

class ThreadPool {
public:
   ThreadPool(int numThreads=DEFAULT_NUM_THREADS);
   ~ThreadPool();

   void enqueue(PoolTask* const task);

   template <typename T>
   void parallelize(
      void(*targetFn)(void*),
      T& container,
      std::function<PoolTaskArg<T>*()> getArgFn);

   PoolTaskStatus waitForTask(PoolTask* const task) const;

   static int partition(const int n, const int x);

   friend void* startThread(void* arg);

private:
   int _numThreads;
   std::vector<pthread_t> _threads;
   std::queue<PoolTask* const> _queue;
   pthread_mutex_t _queueMutex;
   pthread_cond_t _queueCond;
};

template <typename T>
void
ThreadPool::parallelize(
   void(*targetFn)(void*),
   T& container,
   std::function<PoolTaskArg<T>*()> getArgFn)
{
   const int numElements = container.size();
   const int partitionSize = numElements <= _numThreads ? 1 :
      numElements / _numThreads;
   const int bigPartitionSize = partitionSize + 1;
   const int numBigPartitions = partition(numElements, _numThreads);
   std::vector<PoolTask*> tasks;
   int elementIndex = 0;
   int threadIndex = 0;
   try {
      while (elementIndex < numElements && threadIndex < _numThreads) {
         int start;
         int end;
         if (threadIndex < numBigPartitions) {
            start = threadIndex * bigPartitionSize;
            end = (threadIndex + 1) * bigPartitionSize;
         } else {
            start = threadIndex * partitionSize + numBigPartitions;
            end = (threadIndex + 1) * partitionSize + numBigPartitions;
         }

         PoolTask* task = new PoolTask;
         task->func = targetFn;
         PoolTaskArg<T>* arg = getArgFn();
         arg->start = container.begin() + start;
         arg->end = container.begin() + end;
         task->arg = arg;
         tasks.push_back(task);

         enqueue(task);

         elementIndex++;
         threadIndex++;
      }
   } catch (...) {
      // TODO: enhance this
      std::cout << "Exception occurred" << std::endl;
   }

   for (PoolTask* task : tasks) {
      PoolTaskStatus status = waitForTask(task);

      if (status == PoolTaskStatus::FAILED) {
         throw std::runtime_error("Task failed");
      }

      // Task succeeded, clean up resources
      PoolTaskArg<T>* tArgPtr = (PoolTaskArg<T>*)task->arg;
      delete tArgPtr;
      delete task;
   }
}

} // namespace ThreadPool
