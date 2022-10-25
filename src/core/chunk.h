#include "alex_base.h"

namespace alex {

template <class T, class P, class Compare = AlexCompare, class Alloc = std::allocator<std::pair<T, P>>>
class Chunk {
  // Dummy base class
public:
  Chunk() {}
  virtual ~Chunk() = default;
  virtual std::string name() const {  // trigger polymorphism
    return "Chunk";
  }

private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar __attribute__((unused)), const unsigned int version __attribute__((unused))) {
    // std::cout << "Chunk::serialize" << std::endl;
    return;
  }

public:
  virtual bool continue_load_from_pager(Pager<T, P>* pager __attribute__((unused))) {
    return false;
  }
};

template <class T, class P, class Compare = AlexCompare, class Alloc = std::allocator<std::pair<T, P>>>
class DataChunk : public Chunk<T, P, Compare, Alloc> {
  std::string name() const override {  // trigger polymorphism
    return "DataChunk";
  }
private:
  // Allocators
  typedef std::pair<T, P> V;
  typedef typename Alloc::template rebind<T>::other key_alloc_type;
  typedef typename Alloc::template rebind<P>::other payload_alloc_type;
  typedef typename Alloc::template rebind<V>::other value_alloc_type;
  typedef typename Alloc::template rebind<uint64_t>::other bitmap_alloc_type;
  key_alloc_type key_allocator() { return key_alloc_type(allocator_); }
  payload_alloc_type payload_allocator() {
    return payload_alloc_type(allocator_);
  }
  value_alloc_type value_allocator() { return value_alloc_type(allocator_); }
  bitmap_alloc_type bitmap_allocator() { return bitmap_alloc_type(allocator_); }

public:
  Pager<T, P>* pager_ = nullptr;
  bool has_pager_ = false;
  int data_capacity_ = 0;
  int bitmap_size_ = 0;
  uint64_t* bitmap_ = nullptr;
#if ALEX_DATA_NODE_SEP_ARRAYS
  T* key_slots_ = nullptr;
  P* payload_slots_ = nullptr;
#else
  V* data_slots_ = nullptr;
#endif

  const Compare& key_less_;
  const Alloc& allocator_;

  size_t key_slots_offset_ = 0;
  size_t payload_slots_offset_ = 0;
  size_t data_slots_offset_ = 0;
  size_t bitmap_offset_ = 0;

  DataChunk(const Compare& comp = Compare(),
            const Alloc& alloc = Alloc()) :
            key_less_(comp),
            allocator_(alloc) {}  // empty constructor
  virtual ~DataChunk() = default;
#if ALEX_DATA_NODE_SEP_ARRAYS
  DataChunk(Pager<T, P>* pager, int data_capacity, int bitmap_size,
            uint64_t* bitmap=nullptr, T* key_slots=nullptr, P* payload_slots=nullptr,
            const Compare& comp = Compare(), const Alloc& alloc = Alloc()) :
            pager_(pager),
            has_pager_(pager_ != nullptr),
            data_capacity_(data_capacity),
            bitmap_size_(bitmap_size),
            bitmap_(bitmap),
            key_slots_(key_slots),
            payload_slots_(payload_slots),
            key_less_(comp),
            allocator_(alloc) {}
#else
  DataChunk(Pager<T, P>* pager, int data_capacity, int bitmap_size,
            uint64_t* bitmap=nullptr, V* data_slots=nullptr,
            const Compare& comp = Compare(), const Alloc& alloc = Alloc()) :
            pager_(pager),
            has_pager_(pager_ != nullptr),
            data_capacity_(data_capacity),
            bitmap_size_(bitmap_size),
            bitmap_(bitmap),
            data_slots_(data_slots),
            key_less_(comp),
            allocator_(alloc) {}
#endif
private:
  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version __attribute__((unused))) const {
    ar & boost::serialization::base_object<Chunk<T, P>>(*this);
    // std::cout << "DataChunk::save" << std::endl;
    ar & has_pager_;
    if (!has_pager_) {
      // Serialize data arrays together with Alex
      #if ALEX_DATA_NODE_SEP_ARRAYS
      // Needs to iterate because key_slots_ and payload_slots_ are pointers.
      for (int i = 0; i < data_capacity_; ++i) {
        ar << key_slots_[i];
      }
      for (int i = 0; i < data_capacity_; ++i) {
        ar << payload_slots_[i];
      }
      #else
      // Needs to iterate because data_slots_ is a pointer.
      for (int i = 0; i < data_capacity_; ++i) {
        ar << data_slots_[i];
      }
      #endif
      for (int i = 0; i < bitmap_size_; ++i) {
        ar << bitmap_[i];
      }
    } else {
      // Serialize data arrays separately for lazy load
      // std::cout << "DataChunk::save::lazy" << std::endl;
      #if ALEX_DATA_NODE_SEP_ARRAYS
      size_t key_slots_offset = pager_->save_t(key_slots_, data_capacity_);
      ar << key_slots_offset;
      // std::cout << "DataChunk::save::lazy, key_slots_offset= " << key_slots_offset << std::endl;
      size_t payload_slots_offset = pager_->save_p(payload_slots_, data_capacity_);
      ar << payload_slots_offset;
      // std::cout << "DataChunk::save::lazy, payload_slots_offset= " << payload_slots_offset << std::endl;
      #else
      size_t data_slots_offset = pager_->save_v(data_slots_, data_capacity_);
      ar << data_slots_offset;
      // std::cout << "DataChunk::save::lazy, data_slots_offset= " << data_slots_offset << std::endl;
      #endif
      size_t bitmap_offset = pager_->save_u64(bitmap_, bitmap_size_);
      ar << bitmap_offset;
      // std::cout << "DataChunk::save::lazy, bitmap_offset= " << bitmap_offset << std::endl;
    }
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version __attribute__((unused))) {
    ar & boost::serialization::base_object<Chunk<T, P>>(*this);

    // std::cout << "DataChunk::load" << std::endl;
    ar & has_pager_;
    if (!has_pager_) {
      // Load data arrays from archive
      #if ALEX_DATA_NODE_SEP_ARRAYS
      // Needs to iterate because key_slots_ and payload_slots_ are pointers.
      key_slots_ =
          new (key_allocator().allocate(data_capacity_)) T[data_capacity_];
      payload_slots_ =
          new (payload_allocator().allocate(data_capacity_)) P[data_capacity_];
      for (int i = 0; i < data_capacity_; ++i) {
        ar >> key_slots_[i];
      }
      for (int i = 0; i < data_capacity_; ++i) {
        ar >> payload_slots_[i];
      }
      #else
      // Needs to iterate because data_slots_ is a pointer.
      data_slots_ =
          new (value_allocator().allocate(data_capacity_)) V[data_capacity];
      for (int i = 0; i < data_capacity_; ++i) {
        ar >> data_slots_[i];
      }
      #endif
      bitmap_ = new (bitmap_allocator().allocate(bitmap_size_))
        uint64_t[bitmap_size_]();  // initialize to all false
      for (int i = 0; i < bitmap_size_; ++i) {
        ar >> bitmap_[i];
      }
    } else {
      // Lazy load
      // std::cout << "DataChunk::load::lazy" << std::endl;
      #if ALEX_DATA_NODE_SEP_ARRAYS
      ar >> key_slots_offset_;
      ar >> payload_slots_offset_;
      #else
      ar >> data_slots_offset_;
      #endif
      ar >> bitmap_offset_;
    }
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()

public:
  bool continue_load_from_pager(Pager<T, P>* pager) override {
    // std::cout << "DataChunk::continue_load_from_pager" << std::endl;
    if (has_pager_ && pager != nullptr) {
      pager_ = pager;
      #if ALEX_DATA_NODE_SEP_ARRAYS
      key_slots_ = pager_->load_t(key_slots_offset_);
      // std::cout << "DataChunk::load::lazy, key_slots_offset_= " << key_slots_offset_ << ", key_slots_= " << key_slots_ << ", first= " << key_slots_[0] << std::endl;
      payload_slots_ = pager_->load_p(payload_slots_offset_);
      // std::cout << "DataChunk::load::lazy, payload_slots_offset_= " << payload_slots_offset_ << ", payload_slots_= " << payload_slots_ << ", first= " << payload_slots_[0] << std::endl;
      #else
      data_slots_ = pager_->load_v(data_slots_offset_);
      // std::cout << "DataChunk::load::lazy, data_slots_offset_= " << data_slots_offset_ << ", data_slots_= " << data_slots_ << ", first= " << data_slots_[0] << std::endl;
      #endif
      bitmap_ = pager_->load_u64(bitmap_offset_);
      // std::cout << "DataChunk::load::lazy, bitmap_offset_= " << bitmap_offset_ << ", bitmap_= " << bitmap_ << ", first= " << bitmap_[0] << std::endl;
      return true;
    }
    return false;
  }
};

}  // namespace alex