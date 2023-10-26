//
//  ThreadPool.cpp
//  Set-Finder
//
//  Created by JD del Alamo on 9/13/23.
//

#include "ThreadPool.hpp"

namespace ThreadPool {

PoolTask::PoolTask()
{
   status = PoolTaskStatus::NOT_STARTED;
   pthread_mutex_init(&statusMutex, NULL);
   pthread_cond_init(&statusCond, NULL);
}

PoolTask::~PoolTask()
{
   pthread_mutex_destroy(&statusMutex);
   pthread_cond_destroy(&statusCond);
}

void* startThread(void* arg)
{
   ThreadPool* instance = (ThreadPool*)arg;
   while (true) {
      // Get task if there is one
      PoolTask* task;
      pthread_mutex_lock(&instance->_queueMutex);
      while (instance->_queue.empty()) {
         pthread_cond_wait(&instance->_queueCond, &instance->_queueMutex);
      }

      task = instance->_queue.front();
      instance->_queue.pop();
      pthread_mutex_unlock(&instance->_queueMutex);

      if (task->threadCancelled) break;

      bool taskFailed = false;
      try {
         pthread_mutex_lock(&task->statusMutex);
         task->status = PoolTaskStatus::RUNNING;
         pthread_mutex_unlock(&task->statusMutex);
         task->func(task->arg);
      } catch (...) {
         // TODO: enhance this
         std::cout << "Error during task execution, failing task" << std::endl;
         pthread_mutex_lock(&task->statusMutex);
         task->status = PoolTaskStatus::FAILED;
         pthread_mutex_unlock(&task->statusMutex);
         taskFailed = true;
      }

      if (!taskFailed) {
         pthread_mutex_lock(&task->statusMutex);
         task->status = PoolTaskStatus::SUCCEEDED;
         pthread_mutex_unlock(&task->statusMutex);
      }

      pthread_cond_signal(&task->statusCond);
   }

   // Do any clean up required before thread exits
   pthread_exit(NULL);
}

ThreadPool::ThreadPool(
   int numThreads)
{
   _numThreads = numThreads;

   pthread_mutex_init(&_queueMutex, NULL);
   pthread_cond_init(&_queueCond, NULL);

   for (int i = 0; i < _numThreads; i++) {
      pthread_t thread;
      int ret = pthread_create(&thread, NULL, &startThread, this);
      if (ret != 0) {
         exit(EXIT_FAILURE);
      }
      _threads.push_back(thread);
   }
}

ThreadPool::~ThreadPool()
{
   /**
    * Replace queue with a new queue containing one "cancel" task
    * for every thread in the threadpool.
    */
   std::vector<PoolTask> tasks(_numThreads);
   std::queue<PoolTask* const> newQueue;
   for (int i = 0; i < _numThreads; i++) {
      PoolTask task;
      task.threadCancelled = true;
      tasks[i] = task;
      newQueue.push(&tasks[i]);
   }

   pthread_mutex_lock(&_queueMutex);
   _queue.swap(newQueue);
   pthread_mutex_unlock(&_queueMutex);
   pthread_cond_broadcast(&_queueCond);

   for (pthread_t thread : _threads) {
      int ret = pthread_join(thread, NULL);
      if (ret != 0) {
         std::cout << "Error cancelling thread " << thread << std::endl;
      }
   }

   pthread_mutex_destroy(&_queueMutex);
   pthread_cond_destroy(&_queueCond);
}

void
ThreadPool::enqueue(
   PoolTask* const task)
{
   pthread_mutex_lock(&_queueMutex);
   _queue.push(task);
   pthread_mutex_unlock(&_queueMutex);
   pthread_cond_broadcast(&_queueCond);
}

PoolTaskStatus
ThreadPool::waitForTask(
   PoolTask* const task) const
{
   PoolTaskStatus status;
   pthread_mutex_lock(&task->statusMutex);
   while (task->status != PoolTaskStatus::FAILED &&
            task->status != PoolTaskStatus::SUCCEEDED) {
      pthread_cond_wait(&task->statusCond, &task->statusMutex);
   }
   status = task->status;
   pthread_mutex_unlock(&task->statusMutex);

   return status;
}

/**
 * In order to maximize efficiency the elements in the container should
 * be split as evenly as possible between the threads.  To put the goal
 * formally:
 *
 * Partition n, where n = the number of elements in the container, such that
 * the difference between the largest and smallest partitions is at most 1.
 *
 * This can be achieved with the following logic:
 *
 * n = # of elements in the container
 * x = # of threads
 * m = floor(n / x) = Minimum partition size
 * p1 = # of partitions with size m
 * p2 = # of partitions with size m + 1
 *
 * If n <= x: p2 = 0, p1 = x
 * Else: p2 = n % x, p1 = x - p2
 *
 * This ensures that every thread will either have m elements or m + 1
 * elements.
 *
 * @param [in] n : # of elements
 * @param [in] x : # of threads
 *
 * @return # of partitions with size m + 1
 */
int
ThreadPool::partition(
   const int n,
   const int x)
{
   if (n <= x) return 0;
   return n % x;
}

} // namespace ThreadPool
