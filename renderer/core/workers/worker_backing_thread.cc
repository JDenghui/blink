// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/worker_backing_thread.h"

#include <memory>

#include "base/location.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_context_snapshot.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_gc_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_idle_task_runner.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_initializer.h"
#include "third_party/blink/renderer/core/inspector/worker_thread_debugger.h"
#include "third_party/blink/renderer/core/workers/worker_backing_thread_startup_data.h"
#include "third_party/blink/renderer/platform/bindings/v8_per_isolate_data.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/web_thread_supporting_gc.h"

namespace blink {

// Wrapper functions defined in WebKit.h
void MemoryPressureNotificationToWorkerThreadIsolates(
    v8::MemoryPressureLevel level) {
  WorkerBackingThread::MemoryPressureNotificationToWorkerThreadIsolates(level);
}

void SetRAILModeOnWorkerThreadIsolates(v8::RAILMode rail_mode) {
  WorkerBackingThread::SetRAILModeOnWorkerThreadIsolates(rail_mode);
}

static Mutex& IsolatesMutex() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(Mutex, mutex, ());
  return mutex;
}

static HashSet<v8::Isolate*>& Isolates() {
#if DCHECK_IS_ON()
  DCHECK(IsolatesMutex().Locked());
#endif
  static HashSet<v8::Isolate*>& isolates = *new HashSet<v8::Isolate*>();
  return isolates;
}

static void AddWorkerIsolate(v8::Isolate* isolate) {
  MutexLocker lock(IsolatesMutex());
  Isolates().insert(isolate);
}

static void RemoveWorkerIsolate(v8::Isolate* isolate) {
  MutexLocker lock(IsolatesMutex());
  Isolates().erase(isolate);
}

WorkerBackingThread::WorkerBackingThread(const WebThreadCreationParams& params,
                                         bool should_call_gc_on_shutdown)
    : backing_thread_(WebThreadSupportingGC::Create(params)),
      is_owning_thread_(true),
      should_call_gc_on_shutdown_(should_call_gc_on_shutdown) {}

WorkerBackingThread::WorkerBackingThread(WebThread* thread,
                                         bool should_call_gc_on_shutdown)
    : backing_thread_(WebThreadSupportingGC::CreateForThread(thread)),
      is_owning_thread_(false),
      should_call_gc_on_shutdown_(should_call_gc_on_shutdown) {}

WorkerBackingThread::~WorkerBackingThread() = default;

void WorkerBackingThread::InitializeOnBackingThread(
    const WorkerBackingThreadStartupData& startup_data) {
  DCHECK(backing_thread_->IsCurrentThread());
  backing_thread_->InitializeOnThread();

  DCHECK(!isolate_);
  isolate_ = V8PerIsolateData::Initialize(
      backing_thread_->PlatformThread().GetTaskRunner(),
      V8PerIsolateData::V8ContextSnapshotMode::kDontUseSnapshot);
  AddWorkerIsolate(isolate_);
  V8Initializer::InitializeWorker(isolate_);

  ThreadState::Current()->RegisterTraceDOMWrappers(
      isolate_, V8GCController::TraceDOMWrappers,
      ScriptWrappableMarkingVisitor::InvalidateDeadObjectsInMarkingDeque,
      ScriptWrappableMarkingVisitor::PerformCleanup);
  if (RuntimeEnabledFeatures::V8IdleTasksEnabled()) {
    V8PerIsolateData::EnableIdleTasks(
        isolate_, std::make_unique<V8IdleTaskRunner>(
                      BackingThread().PlatformThread().Scheduler()));
  }
  if (is_owning_thread_)
    Platform::Current()->DidStartWorkerThread();

  V8PerIsolateData::From(isolate_)->SetThreadDebugger(
      std::make_unique<WorkerThreadDebugger>(isolate_));

  // Optimize for memory usage instead of latency for the worker isolate.
  isolate_->IsolateInBackgroundNotification();

  if (startup_data.heap_limit_mode ==
      WorkerBackingThreadStartupData::HeapLimitMode::kIncreasedForDebugging) {
    isolate_->IncreaseHeapLimitForDebugging();
  }
  isolate_->SetAllowAtomicsWait(
      startup_data.atomics_wait_mode ==
      WorkerBackingThreadStartupData::AtomicsWaitMode::kAllow);
}

void WorkerBackingThread::ShutdownOnBackingThread() {
  DCHECK(backing_thread_->IsCurrentThread());
  if (is_owning_thread_)
    Platform::Current()->WillStopWorkerThread();

  V8PerIsolateData::WillBeDestroyed(isolate_);
  // TODO(yhirano): Remove this when https://crbug.com/v8/1428 is fixed.
  if (should_call_gc_on_shutdown_) {
    // This statement runs only in tests.
    V8GCController::CollectAllGarbageForTesting(isolate_);
  }
  backing_thread_->ShutdownOnThread();

  RemoveWorkerIsolate(isolate_);
  V8PerIsolateData::Destroy(isolate_);
  isolate_ = nullptr;
}

// static
void WorkerBackingThread::MemoryPressureNotificationToWorkerThreadIsolates(
    v8::MemoryPressureLevel level) {
  MutexLocker lock(IsolatesMutex());
  for (v8::Isolate* isolate : Isolates())
    isolate->MemoryPressureNotification(level);
}

// static
void WorkerBackingThread::SetRAILModeOnWorkerThreadIsolates(
    v8::RAILMode rail_mode) {
  MutexLocker lock(IsolatesMutex());
  for (v8::Isolate* isolate : Isolates())
    isolate->SetRAILMode(rail_mode);
}

}  // namespace blink
