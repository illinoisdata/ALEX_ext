#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace alex {
template<class T, class P>
class Pager {
public:
  typedef std::pair<T, P> V;

  virtual ~Pager() {}

  virtual size_t save_t(T* arr __attribute__((unused)), size_t n __attribute__((unused))) {
    throw std::logic_error("Not implemented: save_t");
  }

  virtual size_t save_p(P* arr __attribute__((unused)), size_t n __attribute__((unused))) {
    throw std::logic_error("Not implemented: save_p");
  }

  virtual size_t save_pair(V* arr __attribute__((unused)), size_t n __attribute__((unused))) {
    throw std::logic_error("Not implemented: save_pair");
  }

  virtual size_t save_u64(uint64_t* arr __attribute__((unused)), size_t n __attribute__((unused))) {
    throw std::logic_error("Not implemented: save_u64");
  }

  virtual T* load_t(size_t offset __attribute__((unused))) {
    throw std::logic_error("Not implemented: load_t");
  }

  virtual P* load_p(size_t offset __attribute__((unused))) {
    throw std::logic_error("Not implemented: load_p");
  }

  virtual V* load_pair(size_t offset __attribute__((unused))) {
    throw std::logic_error("Not implemented: load_pair");
  }

  virtual uint64_t* load_u64(size_t offset __attribute__((unused))) {
    throw std::logic_error("Not implemented: load_u64");
  }
};

template<class T, class P>
class WritePager : public Pager<T, P> {
private:
  std::ofstream file_;
  size_t current_size_;
public:
  typedef std::pair<T, P> V;

  explicit WritePager(std::string filename)
    : file_(filename, std::ios::trunc | std::ios::out | std::ios::binary),
      current_size_(0) {
    std::cerr << "WritePager: opened " << filename << std::endl;
  }
  
  virtual ~WritePager() {
    this->file_.close();
  }

  size_t save_t(T* arr, size_t n) override {
    return save_inner<T>(arr, n);
  }

  size_t save_p(P* arr, size_t n) override {
    return save_inner<P>(arr, n);
  }

  // size_t save_pair(V* arr, size_t n) override {
  //   return save_inner<V>(arr, n);
  // }

  size_t save_u64(uint64_t* arr, size_t n) override {
    return save_inner<uint64_t>(arr, n);
  }

  template<class K>
  size_t save_inner(K* arr, size_t n) {
    size_t offset = this->current_size_;
    this->file_.write(reinterpret_cast<const char*>(arr), n * sizeof(K));
    this->current_size_ += n * sizeof(K);
    // std::cout << "Saving " << n << " * " << sizeof(K) << ", first= " << arr[0] << ", cur= " << this->current_size_ << std::endl;
    return offset;
  }
};

template<class T, class P>
class ReadPager : public Pager<T, P> {
private:
  int fd_;
  size_t file_size_;
  void* begin_addr_;

public:
  typedef std::pair<T, P> V;

  explicit ReadPager(std::string filename) {
    // Open file
    int fd = open(filename.c_str(), O_RDWR);
    if (fd < 0) {
      std::cerr << "Error opening " << filename << std::endl;
      exit(1);
    }

    // Get file size for mapping
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
      std::cerr << "Error obtaining fstat" << std::endl;
      exit(1);
    }
    size_t file_size = sb.st_size;

    // Mmap to get begin address
    void* begin_addr = nullptr;
    if (file_size > 0) {
      begin_addr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
      if (begin_addr == MAP_FAILED) {
        std::cerr << "Error mmap data" << std::endl;
        exit(1);
      }
    } else {
      std::cerr << "Warning: mmap file is empty" << std::endl;
    }

    // Assign to class
    this->fd_ = fd;
    this->file_size_ = file_size;
    this->begin_addr_ = begin_addr;
    std::cerr << "ReadPager: fd_= " << fd_ << ", file_size_= " << file_size_ << ", begin_addr_= " << begin_addr_ << std::endl;
  }

  virtual ~ReadPager() {
    if (this->begin_addr_ != NULL) {
      munmap(this->begin_addr_, this->file_size_);
    }
    if (this->fd_ != -1) {
      close(this->fd_);
    }
  }

  virtual T* load_t(size_t offset) override {
    return load_inner<T>(offset);
  }

  virtual P* load_p(size_t offset) override {
    return load_inner<P>(offset);
  }

  // virtual V* load_pair(size_t offset) override {
  //   return load_inner<V>(offset);
  // }

  virtual uint64_t* load_u64(size_t offset) override {
    return load_inner<uint64_t>(offset);
  }

  template<class K>
  K* load_inner(size_t offset) {
    // Only arithmetic, no loading yet
    return reinterpret_cast<K*>((char*) this->begin_addr_ + offset);
  }
};
}  // namespace alex