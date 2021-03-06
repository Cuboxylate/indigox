/*! \file common.hpp
 */

#ifndef INDIGOX_UTILS_COMMON_HPP
#define INDIGOX_UTILS_COMMON_HPP

#include <array>
#include <bitset>
#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>

// forward definitions of serilisation stuff
namespace cereal {
  class access;
  class PortableBinaryInputArchive;
  class PortableBinaryOutputArchive;
  class JSONInputArchive;
  class JSONOutputArchive;
  template <class T> class construct;
} // namespace cereal

//! utils namespace for useful utility functions
namespace indigox::utils {
  enum class Option {
    Yes = 1,
    No = Yes << 1,
    Auto = Yes << 2,
    Default = Yes << 3,
    All = Yes << 4,
    Some = Yes << 5,
    None = Yes << 6
  };

  inline Option operator|(Option l, Option r) {
    using under = std::underlying_type<Option>::type;
    return static_cast<Option>(static_cast<under>(l) | static_cast<under>(r));
  }

  inline Option operator&(Option l, Option r) {
    using under = std::underlying_type<Option>::type;
    return static_cast<Option>(static_cast<under>(l) & static_cast<under>(r));
  }

  /*! \brief Convert a string to upper case.
   *  \param s the string to convert.
   *  \return the uppercase version of s. */
  std::string ToUpper(const std::string &s);

  /*! \brief Convert a string to lower case.
   *  \param s the string to convert.
   *  \return the lower case version of s. */
  std::string ToLower(const std::string &s);

  /*! \brief Convert a string to lower case with a single leading upper case.
   *  \details Acts on the string as a whole, ignoring any white space.
   *  \param s the string the convert.
   *  \return the lower case version of s with a leading upper case letter. */
  std::string ToUpperFirst(const std::string &s);

  /*! \brief Generate a random string.
   *  \details All upper and lower case letters are available.
   *  \param length the number of characters to generate.
   *  \param seed the seed for the random number generator. If
   *  \return the randomly generated string. */
  std::string GetRandomString(size_t length, size_t seed = 0);

  /*! \brief Check if a given shared_ptr<T> is in a weak_ptr<T> container.
   *  \details Checks all weak_ptr<T> instances in the iterator range to see
   *  if the reference the supplied shared_ptr<T>. If the supplied shared_ptr<T>
   *  is empty, no comparisons are made.
   *  \tparam T the element_type.
   *  \tparam __Iter the iterator type.
   *  \param b,e start and end of the iterator range.
   *  \param x the shared_ptr<T> instance to search for.
   *  \return the iterator position of the first found element. */
  template <typename T, typename __Iter>
  inline __Iter WeakContainsShared(__Iter b, __Iter e, std::shared_ptr<T> x) {
    static_assert(
        std::is_same<typename std::weak_ptr<T>,
                     typename std::iterator_traits<__Iter>::value_type>::value,
        "__Iter must be iterator over std::weak_ptr<T>.");
    if (!x) return e;
    return std::find_if(b, e,
                        [x](std::weak_ptr<T> _x) { return _x.lock() == x; });
  }

  /*! \brief Check if a shared_ptr is pointing to a null member.
   *  \details Types must implement a boolean operator which returns true if
   *  that instance is considered a null instance.
   *  \tparam T the stored type.
   *  \param t the shared_ptr<T> to check.
   *  \return if the shared_ptr references a null instance. */
  //  template<typename T>
  //  inline bool IsNull(std::shared_ptr<T> t) {
  //    return !(t && *t);
  //  }

} // namespace indigox::utils

  
#define b_cnt (uint32_t)Settings::BoolCount
#define i_cnt (uint32_t)Settings::IntCount
#define f_cnt (uint32_t)Settings::RealCount
#define p_cnt (uint32_t)param
  
#define DEFAULT_SETTINGS(...) \
private: \
  std::bitset<b_cnt> p_bool; \
  std::array<double, f_cnt - b_cnt - 2> p_num; \
  \
void ResetSettings() { \
  p_bool.reset(); \
  p_num.fill(0.); \
} \
  \
public: \
  bool GetBool(Settings param) { \
    if (p_cnt >= b_cnt) throw std::runtime_error("Not a boolean parameter"); \
    return p_bool.test(p_cnt); \
  } \
  void SetBool(Settings param, bool value) { \
    if (p_cnt >= b_cnt) throw std::runtime_error("Not a boolean parameter"); \
    p_bool[p_cnt] = value; \
  } \
  int64_t GetInt(Settings param) { \
    uint32_t offset = 1 + b_cnt; \
    if (p_cnt <= b_cnt || p_cnt >= i_cnt) throw std::runtime_error("Not an integer parameter"); \
    return int64_t(p_num[p_cnt - offset]); \
  } \
  void SetInt(Settings param, int64_t value) { \
    uint32_t offset = 1 + b_cnt; \
    if (p_cnt <= b_cnt || p_cnt >= i_cnt) throw std::runtime_error("Not an integer parameter"); \
    p_num[p_cnt - offset] = double(value); \
  } \
  double GetReal(Settings param) { \
    uint32_t offset =  2 + b_cnt; \
    if (p_cnt <= i_cnt || p_cnt >= f_cnt) throw std::runtime_error("Not a real parameter"); \
    return p_num[p_cnt - offset]; \
  } \
  void SetReal(Settings param, double value) { \
    uint32_t offset = 2 + b_cnt; \
    if (p_cnt <= i_cnt || p_cnt >= f_cnt) throw std::runtime_error("Not a real parameter"); \
    p_num[p_cnt - offset] = value; \
  } \
  void DefaultSettings()
  
#endif /* INDIGOX_UTILS_COMMON_HPP */
