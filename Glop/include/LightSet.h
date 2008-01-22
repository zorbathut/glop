// A container that supports the following operations:
//   - Add an element, returning a unique id in O(1) time.
//   - Look up an element with a given unique id in O(1) time.
//   - Remove an element with a given unique id in O(1) time.
//   - Iterate through the elements in the order they were added, getting each
//     element in O(1) time.
//
// The underlying data structure is essentially a doubly linked list, but the class does its own
// memory management, similar to vector, so as to minimize the number of new and delete calls.
//
// Templating is used to allow any kind of data to be stored while maintaining a reasonable
// interface. However, to minimize code bloat, this just delegates to BaseLightSet for much of
// the work.
//
// LightSetId is defined in Base.h

#ifndef GLOP_LIGHT_SET_H
#define GLOP_LIGHT_SET_H

// Includes
#include "Base.h"
#include <new>

// BaseLightSet class definition - for internal use only.
class BaseLightSet {
 public:
  BaseLightSet(int data_size);
  ~BaseLightSet();
  
  // Mutators
  LightSetId InsertItem(LightSetId prev_id, void **item_address);
  LightSetId RemoveItem(LightSetId id);
  
  // Accessors
  int GetSize() const {return size_;}
  const void *GetData(LightSetId id) const {return ((char*)data_) + data_size_ * id;}
  LightSetId GetFirstId() const {return first_id_;}
  LightSetId GetLastId() const {return last_id_;}
  LightSetId GetNextId(LightSetId id) const {
    return (id == 0? first_id_ : nodes_[id].next_id);
  }
  LightSetId GetPrevId(LightSetId id) const {
    return (id == 0? last_id_ : nodes_[id].prev_id);
  }
  
 private:
  struct LightSetNode {
    LightSetId prev_id, next_id;
  };
  LightSetNode *nodes_;
  void *data_;
  LightSetId *free_ids_;
  
  LightSetId first_id_, last_id_;
  int size_, capacity_, data_size_;
  DISALLOW_EVIL_CONSTRUCTORS(BaseLightSet);
};

// LightSet class definition
template <class T> class LightSet {
 public:
  // Construction and destruction
  LightSet<T>() : data_(sizeof(T)) {}
  ~LightSet<T>() {Clear();}
  
  // Assignment/copying - deliberately avoids the lightweight =, == notation
  void Copy(const LightSet<T> &rhs) const {
    if (this == &rhs)
      return;
    Clear();
    for (LightSetId id = rhs.GetFirstId(); id != 0; id = rhs.GetNextId())
      InsertItem(rhs[id]);
  }
  bool IsEqual(const LightSet<T> &rhs) const {
    if (GetSize() != rhs.GetSize())
      return false;
    LightSetId id1 = GetFirstId(), id2 = rhs.getFirstId();
    for (int i = 0; i < rhs.GetSize(); i++) {
      if ( (*this)[id1] != rhs[id2])
        return false; 
      id1 = GetNextId(id1);
      id2 = rhs.GetNextId(id2);
    }
    return true;
  }
  
  // Mutators
  LightSetId InsertItem(const T &item) {return InsertItem(item, data_.GetLastId());}
  LightSetId InsertItem(const T &item, LightSetId prev_id) {
    void *item_address;
    LightSetId result = data_.InsertItem(prev_id, &item_address);
    new ((void*)item_address) T(item);
    return result;
  }
  LightSetId RemoveItem(LightSetId id) {
    ((T*)data_.GetData(id))->~T();
    return data_.RemoveItem(id);
  }
  void Clear() {
    while (data_.GetSize())
      RemoveItem(data_.GetFirstId());
  }
  
  // Accessors
  int GetSize() const {return data_.GetSize();}
  const T &operator[](LightSetId id) const {return *((T*)data_.GetData(id));}
  T &operator[](LightSetId id) {return *((T*)data_.GetData(id));}
  LightSetId GetFirstId() const {return data_.GetFirstId();}
  LightSetId GetLastId() const {return data_.GetLastId();}
  LightSetId GetNextId(LightSetId id) const {return data_.GetNextId(id);}
  LightSetId GetPrevId(LightSetId id) const {return data_.GetPrevId(id);}

  // A convenience lookup method. Runs in linear time.
  LightSetId Find(const T &item) const {
    for (LightSetId id = data_.GetFirstId(); id != 0; id = data_.GetNextId(id))
      if ( (*this)[id] == item)
        return id;
    return 0;
  }
  
 private:
  BaseLightSet data_;
  DISALLOW_EVIL_CONSTRUCTORS(LightSet);
};

#endif // GLOP_LIGHT_SET_H__
