// A doubly linked list, similar to the STL list class with two differences:
//  - It manages its own memory, making it significantly faster than list with the basic allocator.
//  - It supports the notion of ids. These are similar to the integer lookups you get with vectors
//    or arrays, storing the index of an item in the List. This index will always remain valid as
//    long as the List is in existence.
//
// Usage: Lists are particularly intended for scenarios similar to the following:
//        - A 3d world that supports objects that can be added, deleted arbitrarily, or iterated
//          through by an external client.
//        In this scenario, it acts like a vector that can efficiently delete arbitrary elements.
//
// Performance: Sample run-times on a test suite doing both insertion and deletion:
//                vector=17376ms, list=5393ms, List=4376ms
//              Sample run-times of a test suite doing only insertion:
//                vector=418ms, list=1409ms, List=939ms
//              The main slowdown comes from the doubly-linked list structure, and from the fact
//              that iteration is done by indices instead of pointers (requiring an extra
//              dereference), which is required to support Ids.

#ifndef GLOP_LIST_H__
#define GLOP_LIST_H__

// Includes
#include <memory.h>
#include <iterator>
using namespace std;

// ListId class definition
class ListId {
 public: 
  template<class input_iterator> ListId(input_iterator it): value_(it.index_) {}
  ListId(int value = -1): value_(value) {}
  int value() const {return value_;}
  bool operator==(const ListId &rhs) const {return value_ == rhs.value_;}
  bool operator!=(const ListId &rhs) const {return value_ != rhs.value_;}
 private:
  friend class BaseList;
  int value_;
};

// List class definition
template <class T> class List {
 private:
  struct Node {
    T value;
    int prev, next;
  };
 public:
  // const_iterator subclass
  class const_iterator {
   public:
    // Typedefs used for various algorithms. difference_type is required for such things as
    // vector<int>(x.begin(), x.end()), even though the difference of iterators is not well
    // defined.
		typedef bidirectional_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef const T &reference;
    typedef const T *pointer;

    const_iterator(): nodes_(0) {}
    const T &operator*() const {return (*nodes_)[index_].value;}
    const T *operator->() const {return (*nodes_)[index_].value;}
    const_iterator& operator++() { // preincrement
      index_ = (*nodes_)[index_].next;
			return (*this);
    }
    const_iterator operator++(int) { // postincrement
			const_iterator temp = *this;
			++*this;
			return temp;
    }

    const_iterator& operator--() { // predecrement
      index_ = (*nodes_)[index_].prev;
			return (*this);
    }
    const_iterator operator--(int) { // postdecrement
			const_iterator temp = *this;
			--*this;
			return temp;
    }

    bool operator==(const const_iterator &rhs) const {
      return nodes_ == rhs.nodes_ && index_ == rhs.index_;
    }
    bool operator!=(const const_iterator &rhs) const {
      return index_ != rhs.index_ || nodes_ != rhs.nodes_;
    }
   private:
    friend class ListId;
    friend class List;
    const_iterator(Node *const* nodes, int index): nodes_(nodes), index_(index) {}
    Node *const* nodes_;
    int index_;
  };

  // iterator subclass
  class iterator {
   public:
    // See const_iterator
		typedef bidirectional_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef T &reference;
    typedef T *pointer;

    iterator(): nodes_(0) {}
    T &operator*() const {return (*nodes_)[index_].value;}
    T *operator->() const {return (*nodes_)[index_].value;}
    iterator& operator++() { // preincrement
      index_ = (*nodes_)[index_].next;
			return (*this);
    }
    iterator operator++(int) { // postincrement
			iterator temp = *this;
			++*this;
			return temp;
    }

    iterator& operator--() { // predecrement
      index_ = (*nodes_)[index_].prev;
			return (*this);
    }
    iterator operator--(int) { // postdecrement
			iterator temp = *this;
			--*this;
			return temp;
    }

    bool operator==(const iterator &rhs) const {
      return nodes_ == rhs.nodes_ && index_ == rhs.index_;
    }
    bool operator!=(const iterator &rhs) const {
      return index_ != rhs.index_ || nodes_ != rhs.nodes_;
    }
   private:
    friend class ListId;
    friend class List;
    iterator(Node **nodes, int index): nodes_(nodes), index_(index) {}
    Node **nodes_;
    int index_;
  };

  // Constructors
  List<T>() {Init(0);}
  List<T>(int n, const T &value) {
    Init(n);
    insert(begin(), n, value);
  }
  List<T>(const List<T> &rhs) {
    Init(rhs.size());
    insert(begin(), rhs.begin(), rhs.end());
  }
  template<class InputIterator> List<T>(InputIterator first, InputIterator last) {
    Init(0);
    insert(begin(), first, last);
  }
  const List<T> &operator=(const List<T> &rhs) {
    if (this == &rhs) return *this;
    clear();
    insert(begin(), rhs.begin(), rhs.end());
    return *this;
  }

  // Cleanup
  ~List<T>() {FreeData();}
  void clear() {
    FreeData();
    Init(0);
  }

  // Iterator constructors
  const_iterator begin() const {return const_iterator(&nodes_, nodes_[0].next);}
  iterator begin() {return iterator(&nodes_, nodes_[0].next);}
  const_iterator end() const {return const_iterator(&nodes_, 0);}
  iterator end() {return iterator(&nodes_, 0);}
  const_iterator next_to_end() const {return const_iterator(&nodes_, nodes_[0].prev);}
  iterator next_to_end() {return iterator(&nodes_, nodes_[0].prev);}
  const_iterator id_iterator(ListId i) const {return const_iterator(&nodes_, i.value());}
  iterator id_iterator(ListId i) {return iterator(&nodes_, i.value());}

  // Basic accessors
  bool empty() const {return size_ == 0;}
  int size() const {return size_;}
  const T &back() const {return nodes_[nodes_[0].prev].value;}
  T &back() {return nodes_[nodes_[0].prev].value;}
  const T &front() const {return nodes_[nodes_[0].next].value;}
  T &front() {return nodes_[nodes_[0].next].value;}
  const T &operator[](ListId id) const {return nodes_[id.value()].value;}
  T &operator[](ListId id) {return nodes_[id.value()].value;}

  // Basic mutators
  iterator insert(ListId pos, const T &value) {
    // Resize the list if necessary
    if (free_index_ == 0) {
      // Allocate memory
      int new_size = (size_ > 0? 2*size_ : 10);
      nodes_ = (Node*)realloc(nodes_, (new_size+1)*sizeof(Node));

      // Locate the free positions
      for (int i = size_+1; i < new_size; ++i)
        nodes_[i].next = i+1;
      nodes_[new_size].next = 0;
      free_index_ = size_+1;
    }

    // Insert this item and update the links
    int new_index = free_index_, next = pos.value(), prev = nodes_[next].prev;
    free_index_ = nodes_[free_index_].next;
    nodes_[new_index].next = next;
    nodes_[new_index].prev = prev;
    nodes_[next].prev = new_index; 
    nodes_[prev].next = new_index;
    new (&nodes_[new_index].value) T(value);
    ++size_;
    return iterator(&nodes_, new_index);
  }
  void insert(ListId pos, int n, const T &value) {
    for (int i = 0; i < n; i++)
      insert(pos, value);
  }
  template<class InputIterator> void insert(ListId pos,
    InputIterator first, InputIterator last) {
    for (InputIterator it = first; it != last; ++it)
      insert(pos, *it);
  }
  iterator push_back(const T &item) {
    return insert(end(), item);
  }
  iterator push_front(const T &item) {
    return insert(begin(), item);
  }
  iterator erase(ListId pos) {
    nodes_[pos.value()].value.~T();
    int next = nodes_[pos.value()].next;
    int prev = nodes_[pos.value()].prev;
    nodes_[next].prev = prev;
    nodes_[prev].next = next;
    nodes_[pos.value()].next = free_index_;
    free_index_ = pos.value();
    --size_;
    return iterator(&nodes_, next);
  }
  iterator erase(ListId first, ListId last) {
    iterator result;
    for (iterator it = first; it != last; ++it)
      result = erase(it);
    return result;
  }
  void pop_back() {
    erase(--end());
  }
  void pop_front() {
    erase(begin());
  }

 private:
  void Init(int num_items) {
    nodes_ = (Node*)malloc((num_items+1)*sizeof(Node));
    if (num_items > 0) {
      free_index_ = 1;
      for (int i = 1; i < num_items; ++i)
        nodes_[i].next = i+1;
      nodes_[num_items].next = 0;
    } else {
      free_index_ = 0;
    }
    nodes_[0].prev = nodes_[0].next = 0;
    size_ = 0;
  }

  void FreeData() {
    for (iterator it = begin(); it != end(); ++it)
      (*it).~T();
    free(nodes_);
  }

  Node *nodes_;
  int free_index_, size_;
};

#endif
