// Includes
#include "Thread.h"
#include "Os.h"
#include "System.h"

// Thread
// ======

void Thread::Start() {
  // Note that we need to set is_running_ right away in case the user calls Join() before we
  // switch threads
  ASSERT(!is_running_);
  is_stop_requested_ = false;
  is_running_ = true;
  Os::StartThread(StaticExecutor, this);
}

void Thread::Join() {
  while (is_running_)
    system()->Sleep();
}

void Thread::StaticExecutor(void *thread_ptr) {
  Thread *thread = (Thread*)thread_ptr;
  thread->Run();
  thread->is_running_ = false;
}

// Mutex
// =====

Mutex::Mutex(): os_data_(Os::NewMutex()) {}
Mutex::~Mutex() {Os::DeleteMutex(os_data_);}
void Mutex::Acquire() {Os::AcquireMutex(os_data_);}
void Mutex::Release() {Os::ReleaseMutex(os_data_);}

// PCQueue
// =======

PCQueue::PCQueue(int capacity)
: data_(malloc(capacity + 1)),
  queue_length_(capacity + 1), // push_pos - pop_pos must vary between 0 and capacity inclusive
  push_pos_(0),
  pop_pos_(0) {}

PCQueue::~PCQueue() {
  free(data_);
}

void PCQueue::PushData(const void *data, int size) {
  // Wait until data is freed up. Note that this is safe since: we are the only producer thread
  // which means GetSize() can only go down, and hence the safety condition can never
  // asynchronously become false if it is ever true.
  while (GetSize() + size > GetCapacity())
    system()->Sleep();

  // Copy the data, watching out for data that spills beyond the queue end. We update push_pos_ at
  // the end so the data is not popped prematurely.
  if (push_pos_ + size > queue_length_) {
    int size1 = queue_length_ - push_pos_;
    memcpy((char*)data_ + push_pos_, data, size1);
    memcpy(data_, (char*)data + size1, size - size1);
  } else {
    memcpy((char*)data_ + push_pos_, data, size);
  }
  push_pos_ = (push_pos_ + size) % queue_length_;
}

void PCQueue::PopData(void *data, int size) {
  // See PushData
  while (GetSize() < size)
    system()->Sleep();
  if (pop_pos_ + size > queue_length_) {
    int size1 = queue_length_ - pop_pos_;
    memcpy(data, (char*)data_ + pop_pos_, size1);
    memcpy((char*)data + size1, data_, size - size1);
  } else {
    memcpy(data, (char*)data_ + pop_pos_, size);
  }
  pop_pos_ = (pop_pos_ + size) % queue_length_;
}
