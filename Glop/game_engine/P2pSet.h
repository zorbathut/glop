#ifndef P2P_SET_H__
#define P2P_SET_H__

// Includes
#include <Glop/source/Base.h>
#include <map>
using namespace std;

template <class T>
void SerializeToString(const T &t, string *data) {
  t.SerializeToString(data);
}

template <class T>
void ParseFromString(const string &data, T *t) {
  t->ParseFromString(data);
}

template <class S, class T>
void SerializeToString(const pair<S,T> &p, string *data) {
  string s, t;
  SerializeToString(p.first, &s);
  SerializeToString(p.second, &t);
  data->resize(4);
  ((int*)data->data())[0] = (int)s.size();
  (*data) += s;
  (*data) += t;
}

template <class S, class T>
void ParseFromString(const string &data, pair<S,T> *p) {
  int s_len = ((int*)data.data())[0];
  ParseFromString(data.substr(4, s_len), &p->first);
  ParseFromString(data.substr(s_len+4), &p->second);
}

#include <Glop/source/List.h>

// Ids
typedef ListId P2pSetIndex;
struct P2pSetId {
  P2pSetId(): computer(0), local_id(0) {}
  P2pSetId(int _computer, int _local_id): computer(_computer), local_id(_local_id) {}
  P2pSetId(const P2pSetId &rhs): computer(rhs.computer), local_id(rhs.local_id) {}

  int computer;
  int local_id;

  bool operator<(const P2pSetId &rhs) const {
    return computer < rhs.computer ||
           (computer == rhs.computer && local_id < rhs.local_id);
  }
  bool operator==(const P2pSetId &rhs) const {
    return computer == rhs.computer && local_id == rhs.local_id;
  }
  bool operator!=(const P2pSetId &rhs) const {
    return computer != rhs.computer || local_id != rhs.local_id;
  }
  void SerializeToString(string *data) const {
    data->resize(8);
    ((int*)data->data())[0] = computer;
    ((int*)data->data())[1] = local_id;
  }
  void ParseFromString(const string &data) {
    computer = ((int*)data.data())[0];
    local_id = ((int*)data.data())[1];
  }
};

// P2pSet class definition
template <class T> class P2pSet {
 public:
  // iterator subclass
  class iterator {
   public:
    // See const_iterator
		typedef bidirectional_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef T &reference;
    typedef T *pointer;

    iterator() {}
    T &operator*() {return it_->second;}
    T *operator->() {return &(it_->second);}
    const P2pSetId &id() const {return it_->first;}
    iterator& operator++() { // preincrement
      ++it_;
			return (*this);
    }
    iterator operator++(int) { // postincrement
			iterator temp = *this;
			++it_;
			return temp;
    }

    iterator& operator--() { // predecrement
      --it_;
			return (*this);
    }
    iterator operator--(int) { // postdecrement
			iterator temp = *this;
			--it_;
			return temp;
    }

    bool operator==(const iterator &rhs) const {
      return it_ == rhs.it_;
    }
    bool operator!=(const iterator &rhs) const {
      return it_ != rhs.it_;
    }
   private:
    friend class ListId;
    friend class P2pSet;
    int index() const {return ListId(it_).value();}
    iterator(const typename List<pair<P2pSetId, T> >::iterator &it): it_(it) {}
    typename List<pair<P2pSetId, T> >::iterator it_;
  };

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

    const_iterator() {}
    const_iterator(const iterator &it): it_(it.it_) {}
    const T &operator*() const {return it_->second;}
    const T *operator->() const {return &(it_->second);}
    const P2pSetId &id() const {return it_->first;}
    const_iterator& operator++() { // preincrement
      ++it_;
			return (*this);
    }
    const_iterator operator++(int) { // postincrement
			const_iterator temp = *this;
			++it_;
			return temp;
    }

    const_iterator& operator--() { // predecrement
      --it_;
			return (*this);
    }
    const_iterator operator--(int) { // postdecrement
			const_iterator temp = *this;
			--it_;
			return temp;
    }

    bool operator==(const const_iterator &rhs) const {
      return it_ == rhs.it_;
    }
    bool operator!=(const const_iterator &rhs) const {
      return it_ != rhs.it_;
    }
   private:
    friend class ListId;
    friend class P2pSet;
    int index() const {return ListId(it_).value();}
    const_iterator(const typename List<pair<P2pSetId, T> >::const_iterator &it): it_(it) {}
    typename List<pair<P2pSetId, T> >::const_iterator it_;
  };

  // Construction / destruction
  P2pSet<T>() {}
  P2pSet<T>(const P2pSet<T> &rhs): map_(rhs.map_), list_(rhs.list_) {}
  void clear() {
    map_.clear();
    list_.clear();
  }

  // Iteration
  const_iterator begin() const {return const_iterator(list_.begin());}
  iterator begin() {return iterator(list_.begin());}
  const_iterator end() const {return const_iterator(list_.end());}
  iterator end() {return iterator(list_.end());}
  P2pSetIndex next(P2pSetIndex i) const {return ++iterator_at(i);}
  P2pSetIndex prev(P2pSetIndex i) const {return --iterator_at(i);}

  // Basic accessors
  bool empty() const {return list_.empty();}
  int size() const {return list_.size();}
  const_iterator iterator_at(P2pSetIndex i) const {return const_iterator(list_.iterator_at(i));}
  iterator iterator_at(P2pSetIndex i) {return iterator(list_.iterator_at(i));}
  const T &operator[](P2pSetIndex i) const {return list_[i].second;}
  T &operator[](P2pSetIndex i) {return list_[i].second;}

  // P2pSetId lookups
  int count(const P2pSetId &id) const {return (int)map_.count(id);}
  const_iterator find(const P2pSetId &id) const {
    map<P2pSetId, P2pSetIndex>::const_iterator it = map_.find(id);
    if (it == map_.end())
      return end();
    else
      return const_iterator(list_.iterator_at(it->second));
  }
  iterator find(const P2pSetId &id) {
    map<P2pSetId, P2pSetIndex>::const_iterator it = map_.find(id);
    if (it == map_.end())
      return end();
    else
      return iterator(list_.iterator_at(it->second));
  }

  // Basic mutators
  iterator push_back(const P2pSetId &id, const T &value) {
    ASSERT(!map_.count(id));
    typename List<pair<P2pSetId, T> >::iterator it = list_.push_back(make_pair(id, value));
    map_[id] = P2pSetIndex(it);
    return iterator(it);
  }
  iterator push_front(const P2pSetId &id, const T &value) {
    ASSERT(!map_.count(id));
    typename List<pair<P2pSetId, T> >::iterator it = list_.push_front(make_pair(id, value));
    map_[id] = P2pSetIndex(it);
    return iterator(it);
  }
  iterator erase(P2pSetIndex i) {
    map_.erase(list_[i].first);
    return iterator(list_.erase(i));
  }
  iterator erase(const P2pSetId &id) {
    return erase(find(id));
  }
  iterator erase(P2pSetIndex first, P2pSetIndex last) {
    iterator result;
    for (iterator it = first; it != last; ++it)
      result = erase(it);
    return result;
  }

  void SerializeToString(string *data) const {
    list_.SerializeToString(data);
  }

  void ParseFromString(const string &data) {
    list_.ParseFromString(data);
    map_.clear();
    typename List<pair<P2pSetId, T> >::iterator it;
    for (it = list_.begin(); it != list_.end(); it++) {
      map_[it->first] = it;
    }
  }

  

 private:
  map<P2pSetId, P2pSetIndex> map_;
  List<pair<P2pSetId, T> > list_;
};

#endif  // P2P_SET_H__
