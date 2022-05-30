#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <windows.h>
#include <tchar.h>

#include <string>
#include <vector>
#include <limits>
#include <algorithm>

#include "std/tstring.hpp"
#include "std/tstdio.hpp"

#include "tacklelib/utility/platform.hpp"
#include "tacklelib/utility/type_traits.hpp"
#include "tacklelib/utility/addressof.hpp"
#include "tacklelib/utility/utility.hpp"
#include "tacklelib/utility/math.hpp"
#include "tacklelib/utility/locale.hpp"


#ifdef _UNICODE
#   define IF_UNICODE_(x, y) x
#else
#   define IF_UNICODE_(x, y) y
#endif

#define IF_UNICODE(x, y) IF_UNICODE_(x, y)


using uint_t = unsigned int;

namespace utils
{
    template<class Compare, class T, class ForwardIt>
    inline ForwardIt lower_bound(ForwardIt first, ForwardIt last, const T & value, Compare comp)
    {
        ForwardIt it;
        typename std::iterator_traits<ForwardIt>::difference_type count, step;
        count = std::distance(first, last);

        while (count > 0) {
            it = first;
            step = count / 2;
            std::advance(it, step);
            if (comp(*it, value)) {
                first = ++it;
                count -= step + 1;
            }
            else
                count = step;
        }
        return first;
    }

    template<class Compare, class T, class ForwardIt>
    inline ForwardIt upper_bound(ForwardIt first, ForwardIt last, const T & value, Compare comp)
    {
        ForwardIt it;
        typename std::iterator_traits<ForwardIt>::difference_type count, step;
        count = std::distance(first, last);

        while (count > 0) {
            it = first;
            step = count / 2;
            std::advance(it, step);
            if (!comp(value, *it)) {
                first = ++it;
                count -= step + 1;
            }
            else
                count = step;
        }
        return first;
    }

    template<typename T, typename Pred>
    inline typename std::vector<T>::iterator insert_sorted(std::vector<T> & vec, const T & value, Pred pred)
    {
        return vec.insert(utils::upper_bound(vec.begin(), vec.end(), value, pred), value);
    }
}

#endif
