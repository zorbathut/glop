// Includes
#include "../include/LightSet.h"
#include <memory.h>

// Light-weight constructor. No extra memory is allocated until it is needed.
BaseLightSet::BaseLightSet(int data_size)
: nodes_(0),
  data_(0),
  free_ids_(0),
  first_id_(0),
  last_id_(0),
  size_(0),
  capacity_(0),
  data_size_(data_size) {}

// Destructor.
BaseLightSet::~BaseLightSet() {
  if (capacity_) {
    free(nodes_);
    free(data_);
    free(free_ids_);
  }
}

// Prepares to insert an item into the light set after the item with the given id.
// All links are set up but the item data itself is not set. Instead, the address of where
// it should be placed is returned, so the caller can do it properly with a constructor.
LightSetId BaseLightSet::InsertItem(LightSetId prev_id, void **item_address) {
  // Resize the list if necessary
  if (size_ == capacity_) {
    // Determine the new capacity
    int old_capacity = capacity_;
    if (capacity_ > 0)
      capacity_ *= 2;
    else
      capacity_ = 10;
    
    // Reallocate memory.
    // An extra spot is allocated for data_ and free_id_'s since ids range from 1
    // to capacity_.
    nodes_ = (LightSetNode*)realloc(nodes_, (capacity_+1)*sizeof(LightSetNode));
    data_ = (void*)realloc(data_, (capacity_+1)*data_size_);
    free_ids_ = (LightSetId*)realloc(free_ids_, capacity_*sizeof(LightSetId));
    
    // Locate the free positions
    for (int i = 0; i < capacity_ - old_capacity; i++)
      free_ids_[capacity_ - old_capacity - i - 1] = old_capacity + i + 1;
  }
  
  // Insert this item and update the links
  LightSetId new_id = free_ids_[capacity_ - 1 - size_];
  nodes_[new_id].next_id = GetNextId(prev_id);
  nodes_[new_id].prev_id = prev_id;
  if (nodes_[new_id].prev_id != 0)
    nodes_[nodes_[new_id].prev_id].next_id = new_id;
  else
    first_id_ = new_id;
  if (nodes_[new_id].next_id != 0)
    nodes_[nodes_[new_id].next_id].prev_id = new_id;
  else
    last_id_ = new_id;
  *item_address = (void*)GetData(new_id);
  size_++;
  return new_id;
}

// Removes the item with the given id from the light set. The id of the previous item
// in the list is returned.
LightSetId BaseLightSet::RemoveItem(LightSetId item_id) {
  LightSetId next_id = nodes_[item_id].next_id;
  LightSetId prev_id = nodes_[item_id].prev_id;
  
  // Fix the next guy's links
  if (next_id != 0)
    nodes_[next_id].prev_id = prev_id;
  else
    last_id_ = prev_id;
  
  // Fix the previous guy's links
  if (prev_id != 0)
    nodes_[prev_id].next_id = next_id;
  else
    first_id_ = next_id;
  
  // Reclaim the space
  free_ids_[capacity_ - size_] = item_id;
  size_--;
  return prev_id;
}
