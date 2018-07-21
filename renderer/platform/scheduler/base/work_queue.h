// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_WORK_QUEUE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_WORK_QUEUE_H_

#include <stddef.h>

#include <set>

#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "third_party/blink/renderer/platform/scheduler/base/enqueue_order.h"
#include "third_party/blink/renderer/platform/scheduler/base/intrusive_heap.h"
#include "third_party/blink/renderer/platform/scheduler/base/sequenced_task_source.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"

namespace blink {
namespace scheduler {
namespace internal {

class WorkQueueSets;

// This class keeps track of immediate and delayed tasks which are due to run
// now. It interfaces deeply with WorkQueueSets which keeps track of which queue
// (with a given priority) contains the oldest task.
//
// If a fence is inserted, WorkQueue behaves normally up until
// TakeTaskFromWorkQueue reaches or exceeds the fence.  At that point it the
// API subset used by WorkQueueSets pretends the WorkQueue is empty until the
// fence is removed.  This functionality is a primitive intended for use by
// throttling mechanisms.
class PLATFORM_EXPORT WorkQueue {
 public:
  using QueueType = SequencedTaskSource::WorkType;

  // Note |task_queue| can be null if queue_type is kNonNestable.
  WorkQueue(TaskQueueImpl* task_queue, const char* name, QueueType queue_type);
  ~WorkQueue();

  // Associates this work queue with the given work queue sets. This must be
  // called before any tasks can be inserted into this work queue.
  void AssignToWorkQueueSets(WorkQueueSets* work_queue_sets);

  // Assigns the current set index.
  void AssignSetIndex(size_t work_queue_set_index);

  void AsValueInto(base::TimeTicks now,
                   base::trace_event::TracedValue* state) const;

  // Returns true if the |tasks_| is empty. This method ignores any fences.
  bool Empty() const { return tasks_.empty(); }

  // If the |tasks_| isn't empty and a fence hasn't been reached,
  // |enqueue_order| gets set to the enqueue order of the front task and the
  // function returns true. Otherwise the function returns false.
  bool GetFrontTaskEnqueueOrder(EnqueueOrder* enqueue_order) const;

  // Returns the first task in this queue or null if the queue is empty. This
  // method ignores any fences.
  const TaskQueueImpl::Task* GetFrontTask() const;

  // Returns the last task in this queue or null if the queue is empty. This
  // method ignores any fences.
  const TaskQueueImpl::Task* GetBackTask() const;

  // Pushes the task onto the |tasks_| and if a fence hasn't been reached
  // it informs the WorkQueueSets if the head changed.
  void Push(TaskQueueImpl::Task task);

  // Pushes the task onto the front of the |tasks_| and if it's before any
  // fence it informs the WorkQueueSets the head changed. Use with caution this
  // API can easily lead to task starvation if misused.
  void PushNonNestableTaskToFront(TaskQueueImpl::Task task);

  // Reloads the empty |tasks_| with
  // |task_queue_->TakeImmediateIncomingQueue| and if a fence hasn't been
  // reached it informs the WorkQueueSets if the head changed.
  void ReloadEmptyImmediateQueue();

  size_t Size() const { return tasks_.size(); }

  // Pulls a task off the |tasks_| and informs the WorkQueueSets.  If the
  // task removed had an enqueue order >= the current fence then WorkQueue
  // pretends to be empty as far as the WorkQueueSets is concerned.
  TaskQueueImpl::Task TakeTaskFromWorkQueue();

  // Removes all canceled tasks from the head of the list. Returns true if any
  // tasks were removed.
  bool RemoveAllCanceledTasksFromFront();

  const char* name() const { return name_; }

  TaskQueueImpl* task_queue() const { return task_queue_; }

  WorkQueueSets* work_queue_sets() const { return work_queue_sets_; }

  size_t work_queue_set_index() const { return work_queue_set_index_; }

  HeapHandle heap_handle() const { return heap_handle_; }

  void set_heap_handle(HeapHandle handle) { heap_handle_ = handle; }

  QueueType queue_type() const { return queue_type_; }

  // Returns true if the front task in this queue has an older enqueue order
  // than the front task of |other_queue|. Both queue are assumed to be
  // non-empty. This method ignores any fences.
  bool ShouldRunBefore(const WorkQueue* other_queue) const;

  // Submit a fence. When TakeTaskFromWorkQueue encounters a task whose
  // enqueue_order is >= |fence| then the WorkQueue will start pretending to be.
  // empty.
  // Inserting a fence may supersede a previous one and unblock some tasks.
  // Returns true if any tasks where unblocked, returns false otherwise.
  bool InsertFence(EnqueueOrder fence);

  // Submit a fence without triggering a WorkQueueSets notification.
  // Caller must ensure that WorkQueueSets are properly updated.
  // This method should not be called when a fence is already present.
  void InsertFenceSilently(EnqueueOrder fence);

  // Removes any fences that where added and if WorkQueue was pretending to be
  // empty, then the real value is reported to WorkQueueSets. Returns true if
  // any tasks where unblocked.
  bool RemoveFence();

  // Returns true if any tasks are blocked by the fence. Returns true if the
  // queue is empty and fence has been set (i.e. future tasks would be blocked).
  // Otherwise returns false.
  bool BlockedByFence() const;

  // Test support function. This should not be used in production code.
  void PopTaskForTesting();

 private:
  bool InsertFenceImpl(EnqueueOrder fence);

  TaskQueueImpl::TaskDeque tasks_;
  WorkQueueSets* work_queue_sets_ = nullptr;  // NOT OWNED.
  TaskQueueImpl* const task_queue_;           // NOT OWNED.
  size_t work_queue_set_index_ = 0;
  HeapHandle heap_handle_;
  const char* const name_;
  EnqueueOrder fence_ = 0;
  const QueueType queue_type_;

  DISALLOW_COPY_AND_ASSIGN(WorkQueue);
};

}  // namespace internal
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_WORK_QUEUE_H_