#include <fstream>
#include <iostream>

// include headers that implement a archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

template<class T, class P>
class InnerInfo {
private:
  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version __attribute__((unused))) const
  {
    ar & length;
    // TODO: save ID
    ar & t;
    ar & p;
  }
  template<class Archive>
  void load(Archive & ar, const unsigned int version __attribute__((unused)))
  {
    ar & length;
    this->fill_id();
    ar & t;
    ar & p;
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
  
  void fill_id() {
    if (this->id != nullptr) {
    delete[] this->id;
    }
    this->id = new char[length];
    for (size_t i = 0; i < length; ++i) {
    this->id[i] = 'a' + rand() % 26;
    }
    this->id[length - 1] = '\0';
  }
  
  size_t length;
  char* id;
  T t;
  P p;
public:
  InnerInfo() : InnerInfo(8) {};
  InnerInfo(int length) : length(length), id(nullptr) {
    fill_id();
  }
  InnerInfo(int length, T t, P p) : length(length), id(nullptr), t(t), p(p) {
    fill_id();
  }
  virtual ~InnerInfo() {
    delete[] this->id;
  }
  virtual void talk() const {
    std::cout << "length= " << length << ", id= " << id << ", t= " << t << ", p= " << p << std::endl;
  }
  virtual void do_something() { };
};

template<class T, class P>
class SpecialInfo : public InnerInfo<T, P> {
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version __attribute__((unused)))
  {
    ar & boost::serialization::base_object<InnerInfo<T, P>>(*this);
    ar & t2;
  }

  T t2;
public:
  SpecialInfo() : InnerInfo<T, P>() {};
  SpecialInfo(int length, T t, P p, T t2) : InnerInfo<T, P>( length, t, p), t2(t2) {}
  virtual ~SpecialInfo() {}
  void talk() const override {
    InnerInfo<T, P>::talk();
    std::cout << "t2= " << t2 << std::endl;
  }
  void do_something() override {
    std::cout << "Doing something" << std::endl;
  }
};

// BOOST_CLASS_EXPORT_GUID(SpecialInfo, "SpecialInfo")

// namespace boost {
// namespace archive {
// namespace detail {
// namespace {
// template<>
// struct init_guid< InnerInfo<T, float> > {
//     static ::boost::archive::detail::guid_initializer< InnerInfo<T, float> > const
//         & guid_initializer;
// };
// ::boost::archive::detail::guid_initializer< InnerInfo<T, float> > const &
//     ::boost::archive::detail::init_guid< InnerInfo<T, float> >::guid_initializer =
//         ::boost::serialization::singleton<
//             ::boost::archive::detail::guid_initializer< InnerInfo<T, float> >
//         >::get_mutable_instance().export_guid("InnerInfo<T, float>");
// }}}}

/////////////////////////////////////////////////////////////
// gps coordinate
//
// illustrates serialization for a simple type
//
class gps_position {
private:
  friend class boost::serialization::access;
  // When the class Archive corresponds to an output archive, the
  // & operator is defined similar to <<.  Likewise, when the class Archive
  // is a type of input archive the & operator is defined similar to >>.
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version __attribute__((unused)))
  {
    ar.template register_type<SpecialInfo<long, float>>();
    ar & degrees;
    ar & minutes;
    ar & seconds;
    // std::cout << "serialize" << std::endl;
    // if (inner != nullptr) {
    //   std::cout << "deleting inner" << std::endl;
    //   delete inner;
    // }
    // ar & special;
    ar & inner;
  }
  int degrees;
  int minutes;
  float seconds;
  // SpecialInfo<long, float> special;
  InnerInfo<long, float>* inner = nullptr;
public:
  gps_position() : gps_position(0, 0, 0, 42, 1.23, 43) {};
  gps_position(int d, int m, float s, long t, float p, long t2) : degrees(d), minutes(m), seconds(s)
  // , special(d, t, p, t2)
  {
    // inner = new InnerInfo(d, t, p);
    inner = new SpecialInfo(d, t, p, t2);
  }
  ~gps_position() {
    delete inner;
  }
  void talk() const {
    std::cout << "degrees= " << degrees << ", minutes= " << minutes << ", seconds= " << seconds << std::endl;
    // special.talk();
    inner->talk();
  }
};

int main() {
  // create and open a character archive for output
  std::ofstream ofs("filename");

  // create class instance
  gps_position g(35, 59, 24.567f, 456, 4.2, 789);

  // save data to archive
  {
    boost::archive::text_oarchive oa(ofs);
    // oa.register_type<SpecialInfo<long, float>>();
    // write class instance to archive
    oa << g;
    g.talk();
    // archive and stream closed when destructors are called
  }

  // ... some time later restore the class instance to its orginal state
  gps_position newg;
  {
    // create and open an archive for input
    std::ifstream ifs("filename");
    boost::archive::text_iarchive ia(ifs);
    // ia.register_type<SpecialInfo<long, float>>();
    // read class state from archive
    ia >> newg;
    newg.talk();
    // archive and stream closed when destructors are called
  }
  return 0;
}
