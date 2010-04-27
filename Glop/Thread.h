#ifndef GLOP_THREAD_H__
#define GLOP_THREAD_H__

// Includes
#include "Base.h"

// Class declarations
struct OsMutex;

// Thread class definition. This is the basic tool for threading. The user extends the Thread
// class and overloads the virtual function Run. Once Start is called, the new Run function is
// executed in a new thread. Join() can be used to wait for that thread to terminate.
class Thread {
 public:
  // Deletes this thread object. The thread must not be currently executing. Note: that calling
  // Join() here is insufficient since we would still delete the extending part of the class
  // before reaching this code.
  virtual ~Thread() {ASSERT(!is_running_);}

  // Begins executing this thread.
  void Start();

  // Returns whether the thread is currently executing.
  bool IsRunning() const {return is_running_;}

  // If the thread is currently executed, this requests that it stop. There is nothing requiring
  // a thread to honor this request, although it should if possible.
  void RequestStop() {is_stop_requested_ = true;}

  // Blocks until the thread finishes execution.
  void Join();

 protected:
  // Creates this thread object. It will not begin executing until Start is called.
  Thread(): is_stop_requested_(false), is_running_(false) {}

  // Returns is_stop_requested_ - for use within Run().
  bool IsStopRequested() const {return is_stop_requested_;}

  // Pure virtual function that is executed in the new thread. When this function returns, the
  // thread is considered finished.
  virtual void Run() = 0;

 private:
  static void StaticExecutor(void *thread_ptr);
  bool is_stop_requested_, is_running_;
  DISALLOW_EVIL_CONSTRUCTORS(Thread);
};

// Mutex class definition. This is a simple lock. At most one thread can have a single mutex
// acquired at any given time.
class Mutex {
 public:
  Mutex();
  ~Mutex();
  void Acquire();
  void Release();
 private:
  OsMutex *os_data_;
  DISALLOW_EVIL_CONSTRUCTORS(Mutex);
};

// MutexLock class definition. While in scope, a MutexLock keeps a mutex acquired. Once it goes
// out of scope, the mutex is released.
class MutexLock {
 public:
  MutexLock(Mutex *mutex): mutex_(mutex) {mutex_->Acquire();}
  ~MutexLock() {mutex_->Release();}
 private:
  Mutex *mutex_;
  DISALLOW_EVIL_CONSTRUCTORS(MutexLock);
};

// PCQueue class definition. This is a first-in first-out queue that safely supports a unique
// producer thread that can push data into the queue, and a unique consumer thread that can pop
// data out of the queue. A PCQueue has a fixed capacity specified in advance. A push blocks until
// it would avoid overfilling the queue. A pop blocks until there is data available to be popped.
// No mutexes are required to do these operations, which should make a PCQueue quite efficient.
class PCQueue {
 public:
  PCQueue(int capacity);
  ~PCQueue();
  int GetCapacity() const {return queue_length_ - 1;}
  int GetSize() const {return (push_pos_ - pop_pos_ + queue_length_) % queue_length_;}

  // Can only be called by a unique producer thread
  void PushData(const void *data, int size);
  void PushBool(bool data) {PushData(&data, sizeof(bool));}
  void PushChar(char data) {PushData(&data, sizeof(char));}
  void PushShort(short data) {PushData(&data, sizeof(short));}
  void PushInt(int data) {PushData(&data, sizeof(int));}
  void PushInt64(int64 data) {PushData(&data, sizeof(int64));}
  void PushFloat(float data) {PushData(&data, sizeof(float));}
  void PushDouble(double data) {PushData(&data, sizeof(double));}
  void PushPointer(void *data) {PushData(&data, sizeof(void*));}

  // Can only be called by a unique consumer thread
  void PopData(void *data, int size);
  bool PopBool() {bool data; PopData(&data, sizeof(bool)); return data;}
  char PopChar() {char data; PopData(&data, sizeof(char)); return data;}
  short PopShort() {short data; PopData(&data, sizeof(short)); return data;}
  int PopInt() {int data; PopData(&data, sizeof(int)); return data;}
  int64 PopInt64() {int64 data; PopData(&data, sizeof(int64)); return data;}
  float PopFloat() {float data; PopData(&data, sizeof(float)); return data;}
  double PopDouble() {double data; PopData(&data, sizeof(double)); return data;}
  void *PopPointer() {void *data; PopData(&data, sizeof(void*)); return data;}

 private:
  void *data_;
  int push_pos_, pop_pos_;
  int queue_length_;
  DISALLOW_EVIL_CONSTRUCTORS(PCQueue);
};

#endif // GLOP_THREAD_H__
