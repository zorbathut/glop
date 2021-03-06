#ifndef MOVINGWINDOW_H
#define MOVINGWINDOW_H

#include <assert.h>
#include <new>
#include <stdio.h>

template <class T>
class MovingWindow {
 public:
  MovingWindow() : size_(-1), first_index_(-1), data_(NULL) {
    
  }

  MovingWindow(int size, int start) : size_(size), first_index_(start) {
    assert(size >= 1);
    data_ = new T[size];
    for (int i = 0; i < size_; i++) {
      new (&data_[i]) T();
    }
  }
  MovingWindow(const MovingWindow& window) {
    size_ = window.size_;
    data_ = new T[size_];
    for (int i = 0; i < size_; i++) {
      new (&data_[i]) T(window.data_[i]);
    }
    first_index_ = window.first_index_;
  }
  MovingWindow& operator=(const MovingWindow& window) {
    size_ = window.size_;
    data_ = new T[size_];
    for (int i = 0; i < size_; i++) {
      new (&data_[i]) T(window.data_[i]);
    }
    first_index_ = window.first_index_;
  }

  ~MovingWindow() {
    delete[] data_;
    data_ = NULL;
  }

  T& operator[](int index) {
    if (index < first_index_) {
      printf("index %d < first_index_ %d\n", index, first_index_);
      assert(false);
    }
    if (index >= first_index_ + size_) {
      printf("index %d >= first_index_ + size_ %d\n", index, first_index_ + size_);
      assert(false);
    }
    return data_[((index % size_) + size_) % size_];
  }

  const T& operator[](int index) const {
    if (index < first_index_) {
      printf("index %d < first_index_ %d\n", index, first_index_);
      assert(false);
    }
    if (index >= first_index_ + size_) {
      printf("index %d >= first_index_ + size_ %d\n", index, first_index_ + size_);
      assert(false);
    }
    return data_[((index % size_) + size_) % size_];
  }

  void Advance() {
    data_[((first_index_ % size_) + size_) % size_].~T();

    // \todo jwills - Can't set to NULL because it might not be a pointer, but should we zero out
    // the memory there just to be safe?
//    data_[((first_index_ % size_) + size_) % size_] = NULL;
    first_index_++;
    new (&data_[((first_index_ % size_) + size_ - 1) % size_]) T();
  }

  int GetFirstIndex() const { return first_index_; }
  int GetLastIndex() const { return first_index_ + size_ - 1; }
  int size() const { return size_; }

 private:
  T* data_;
  int size_;

  /// The first available index in the window
  int first_index_;

// I initially made this private, is that a good idea?
//  MovingWindow();
};

#endif
