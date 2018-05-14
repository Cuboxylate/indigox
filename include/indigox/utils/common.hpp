/*! \file common.hpp
 */

#ifndef INDIGOX_UTILS_COMMON_HPP
#define INDIGOX_UTILS_COMMON_HPP

#include <type_traits>

#include "numerics.hpp"

//! utils namespace for useful utility functions
namespace indigox::utils {
  
  /*! \brief Convert a string to upper case.
   *  \param s the string to convert.
   *  \return the uppercase version of s. */
  string_ ToUpper(const string_& s);
  
  /*! \brief Convert a string to lower case.
   *  \param s the string to convert.
   *  \return the lower case version of s. */
  string_ ToLower(const string_& s);
  
  /*! \brief Convert a string to lower case with a single leading upper case.
   *  \details Acts on the string as a whole, ignoring any white space.
   *  \param s the string the convert.
   *  \return the lower case version of s with a leading upper case letter. */
  string_ ToUpperFirst(const string_& s);
  
  /*! \brief Generate a random string.
   *  \details All upper and lower case letters are available.
   *  \param length the number of characters to generate.
   *  \param seed the seed for the random number generator. If 
   *  \return the randomly generated string. */
  string_ GetRandomString(size_ length, size_ seed = 0);
  
  /*! \brief Check if a given shared_ptr<T> is in a weak_ptr<T> container.
   *  \details Checks all weak_ptr<T> instances in the iterator range to see
   *  if the reference the supplied shared_ptr<T>. If the supplied shared_ptr<T>
   *  is empty, no comparisons are made.
   *  \tparam T the element_type.
   *  \tparam __Iter the iterator type.
   *  \param b,e start and end of the iterator range.
   *  \param x the shared_ptr<T> instance to search for.
   *  \return the iterator position of the first found element. */
  template<typename T, typename __Iter>
  inline __Iter WeakContainsShared(__Iter b, __Iter e, std::shared_ptr<T> x) {
    static_assert(std::is_same<
                  typename std::weak_ptr<T>,
                  typename std::iterator_traits<__Iter>::value_type>::value,
                  "__Iter must be iterator over std::weak_ptr<T>.");
    if (!x) return e;
    return std::find_if(b, e, [x](std::weak_ptr<T> _x) { return _x.lock() == x; });
  }
}  // namespace indigox::utils


#endif /* INDIGOX_UTILS_COMMON_HPP */
