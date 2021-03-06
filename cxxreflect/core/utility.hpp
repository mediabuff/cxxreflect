
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_UTILITY_HPP_
#define CXXREFLECT_CORE_UTILITY_HPP_

#include "cxxreflect/core/diagnostic.hpp"





// NOTE WELL:  We use macros here for performance.  These overloads can also be introduced via a
// base class template, however, Visual C++ will not fully perform Empty-Base Optimization (EBO)
// in the presence of multiple inheritance. This has a disasterous effect on many other critical
// optimizations (for example, member function inlining is hindered due to the additional offset
// computations that are required).  Thus, we use macros to generate the overloads.

// Generators for comparison operators, based on == and < operators that must be user-defined.
#define CXXREFLECT_GENERATE_EQUALITY_OPERATORS(T)                                        \
    friend auto operator!=(T const& lhs, T const& rhs) -> bool { return !(lhs == rhs); }

#define CXXREFLECT_GENERATE_RELATIONAL_OPERATORS(T)                                      \
    friend auto operator> (T const& lhs, T const& rhs) -> bool { return   rhs <  lhs ; } \
    friend auto operator<=(T const& lhs, T const& rhs) -> bool { return !(rhs <  lhs); } \
    friend auto operator>=(T const& lhs, T const& rhs) -> bool { return !(lhs <  rhs); }

#define CXXREFLECT_GENERATE_COMPARISON_OPERATORS(T)                                      \
    CXXREFLECT_GENERATE_EQUALITY_OPERATORS(T)                                            \
    CXXREFLECT_GENERATE_RELATIONAL_OPERATORS(T)

// Generators for addition and subtraction operators for types that are pointer-like (i.e. types
// that use indices or pointers of some kind to point to elements in a sequence).  These are a
// bit flakey and we abuse them for types that don't support all of them, but they work well.
#define CXXREFLECT_GENERATE_ADDITION_OPERATORS(the_type, get_value, difference, opt_t)       \
    the_type& operator++()    { ++(*this).get_value; return *this;              }            \
    the_type  operator++(int) { auto const it(*this); ++*this; return it;       }            \
                                                                                             \
    the_type& operator+=(difference const n)                                                 \
    {                                                                                        \
        typedef opt_t std::remove_reference<decltype((*this).get_value)>::type ValueType;    \
        (*this).get_value = static_cast<ValueType>((*this).get_value + n);                   \
        return *this;                                                                        \
    }                                                                                        \
                                                                                             \
    friend the_type operator+(the_type it, difference n) { return it += n; }                 \
    friend the_type operator+(difference n, the_type it) { return it += n; }

#define CXXREFLECT_GENERATE_SUBTRACTION_OPERATORS(the_type, get_value, difference, opt_t)    \
    the_type& operator--()    { --(*this).get_value; return *this;              }            \
    the_type  operator--(int) { auto const it(*this); --*this; return it;       }            \
                                                                                             \
    the_type& operator-=(difference const n)                                                 \
    {                                                                                        \
        typedef opt_t std::remove_reference<decltype((*this).get_value)>::type ValueType;    \
        (*this).get_value = static_cast<ValueType>((*this).get_value - n);                   \
        return *this;                                                                        \
    }                                                                                        \
                                                                                             \
    friend the_type operator-(the_type it, difference n) { return it -= n; }                 \
                                                                                             \
    friend difference operator-(the_type lhs, the_type rhs)                                  \
    {                                                                                        \
        return static_cast<difference>(lhs.get_value - rhs.get_value);                       \
    }

#define CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS(type, get_value, difference)          \
    CXXREFLECT_GENERATE_ADDITION_OPERATORS(type, get_value, difference,)                         \
    CXXREFLECT_GENERATE_SUBTRACTION_OPERATORS(type, get_value, difference,)

#define CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS_TEMPLATE(type, get_value, difference) \
    CXXREFLECT_GENERATE_ADDITION_OPERATORS(type, get_value, difference, typename)                \
    CXXREFLECT_GENERATE_SUBTRACTION_OPERATORS(type, get_value, difference, typename)

// Generates nonmember begin/end pairs for the named type so we don't need 'using std::begin'.  Note
// that we use templates to work around inane name lookup behavior.
/*#define CXXREFLECT_GENERATE_NONMEMBER_BEGIN_END(x_type)                           \
    template <typename T>                                                         \
    friend auto begin(T&& r)                                                      \
        -> typename std::enable_if<                                               \
            std::is_same<typename std::decay<T>::type, x_type>::value,            \
            decltype(r.begin())                                                   \
        >::type                                                                   \
    {                                                                             \
        return r.begin();                                                         \
    }                                                                             \
                                                                                  \
    template <typename T>                                                         \
    friend auto end(T&& r)                                                        \
        -> typename std::enable_if<                                               \
            std::is_same<typename std::decay<T>::type, x_type>::value,            \
            decltype(r.end())                                                     \
        >::type                                                                   \
    {                                                                             \
        return r.end();                                                           \
    }
*/

// Generator for the safe-bool operator
#define CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(type)                                  \
private:                                                                                \
                                                                                        \
    typedef void (type::*faux_bool_type)() const;                                       \
                                                                                        \
    void this_type_does_not_support_comparisons() const { }                             \
                                                                                        \
public:                                                                                 \
                                                                                        \
    operator faux_bool_type() const                                                     \
    {                                                                                   \
        return !(*this).operator!()                                                     \
            ? &type::this_type_does_not_support_comparisons                             \
            : nullptr;                                                                  \
    }





// A static assertion whose message is simply the condition that failed.  This is useful pretty much
// everywhere, so we define it here.
#define CXXREFLECT_STATIC_ASSERT(...) static_assert((__VA_ARGS__), # __VA_ARGS__)





namespace cxxreflect { namespace core { namespace detail {

    template <typename T>
    struct is_iterator_impl
    {
        // Note:  This only works for class-type iterators that define the category as member
        // typedefs.  We handle pointers through another specialization of the is_iterator trait.
        // We make no attempt to handle class type iterators that only specialize iterator_traits
        // and do not define the category as a member typedef.
        template <typename U>
        static auto f(int, typename U::iterator_category* = nullptr) -> std::true_type;

        template <typename U>
        static auto f(unsigned long long) -> std::false_type;

        typedef decltype(f<T>(0)) type;

        enum { value = type::value };
    };





    template <typename T, size_type Shift>
    struct log_base_2_impl;

    template <typename T>
    struct log_base_2_impl<T, 0>
    {
        static auto compute(T) -> size_type
        {
            return static_cast<size_type>(-1);
        }
    };

    template <typename T, size_type Shift>
    struct log_base_2_impl
    {
        static_assert(std::is_unsigned<T>::value, "value type must be an unsigned integral type");

        static auto compute(T const value) -> size_type
        {
            static const char table[256] = 
            {
                #define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
                -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,

                LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
                LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)

                #undef LT
            };

            size_type const shifted_value(value >> (Shift - 8));
            if (shifted_value != 0)
                return table[shifted_value];

            return log_base_2_impl<T, Shift - 8>::compute(value);
        }
    };

} } }

namespace cxxreflect { namespace core {

    using std::begin;
    using std::end;

    /// Represents a range of elements in an array
    template <typename T>
    class array_range
    {
    public:

        typedef T           value_type;
        typedef T*          pointer;

        array_range()
            : _begin(nullptr), _end(nullptr)
        {
        }

        array_range(pointer const first, pointer const last)
            : _begin(first), _end(last)
        {
            assert_initialized(*this);
        }

        /// Converting constructor to convert `array_range<T>` to `array_range<cv T>`
        template <typename U>
        array_range(array_range<U> const& other,
              typename std::enable_if<
                  std::is_same<
                      typename std::remove_cv<T>::type,
                      typename std::remove_cv<U>::type
                  >::value
              >::type* = nullptr)
            : _begin(other.is_initialized() ? other.begin() : nullptr),
              _end  (other.is_initialized() ? other.end()   : nullptr)
        {
        }

        auto begin() const -> pointer         { return _begin; }
        auto end()   const -> pointer         { return _end;   }

        auto empty() const -> bool
        {
            // Note that we do not assert initialized here.  If we are not initialized, we are empty
            // which is okey dokey.
            return _begin == _end;
        }

        auto size()  const -> core::size_type
        {
            assert_initialized(*this);
            return static_cast<core::size_type>(_end - _begin);
        }
        

        auto is_initialized() const -> bool
        {
            return _begin != nullptr && _end != nullptr;
        }

    private:

        pointer _begin;
        pointer _end;
    };

    typedef array_range<byte const> const_byte_range;





    /// A checked pointer wrapper that throws `logic_error` if it is dereferenced when nullptr
    ///
    /// Similar to `value_initialized<T>`, `checked_pointer<T>` always initializes its value, so it
    /// may be safely used as the type of a data member in a class.
    template <typename T>
    class checked_pointer
    {
    public:

        typedef T        value_type;
        typedef T &      reference;
        typedef T const& const_reference;
        typedef T *      pointer;
        typedef T const* const_pointer;

        checked_pointer()
            : _value()
        {
        }

        explicit checked_pointer(pointer const value)
            : _value(value)
        {
        }

        auto operator*() -> reference
        {
            assert_not_null(_value);
            return *_value;
        }

        auto operator*() const -> const_reference
        {
            assert_not_null(_value);
            return *_value;
        }

        auto operator->() -> pointer
        {
            assert_not_null(_value);
            return _value;
        }

        auto operator->() const -> const_pointer
        {
            assert_not_null(_value);
            return _value;
        }

        auto get()       -> pointer&       { return _value; }
        auto get() const -> const_pointer  { return _value; }
        
        auto reset() -> void { _value = nullptr; }

        auto is_initialized() const -> bool { return _value != nullptr; }

        friend auto operator==(checked_pointer const& lhs, checked_pointer const& rhs) -> bool
        {
            return std::equal_to<pointer>()(lhs._value, rhs._value);
        }

        friend auto operator<(checked_pointer const& lhs, checked_pointer const& rhs) -> bool
        {
            return std::less<pointer>()(lhs._value, rhs._value);
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(checked_pointer)

    private:

        pointer _value;
    };





    template <typename T>
    class constructor_forwarder
    {
    public:

        auto operator()() const -> T 
        {
            return T();
        }

        template <typename P0>
        auto operator()(P0&& p0) const -> T
        {
            return T(std::forward<P0>(p0));
        }

        template <typename P0, typename P1>
        auto operator()(P0&& p0, P1&& p1) const -> T
        {
            return T(std::forward<P0>(p0), std::forward<P1>(p1));
        }
    };

    template <typename T>
    class internal_constructor_forwarder
    {
    public:

        auto operator()() const -> T 
        {
            return T(internal_key());
        }

        template <typename P0>
        auto operator()(P0&& p0) const -> T
        {
            return T(std::forward<P0>(p0), internal_key());
        }

        template <typename P0, typename P1>
        auto operator()(P0&& p0, P1&& p1) const -> T
        {
            return T(std::forward<P0>(p0), std::forward<P1>(p1), internal_key());
        }
    };





    typedef std::array<byte, 20> sha1_hash;





    template <typename T>
    class integer_converter
    {
    public:

        integer_converter(T const value)
            : _value(value)
        {
        }

        template <typename U>
        operator U() const
        {
            #if CXXREFLECT_COMPILER == CXXREFLECT_COMPILER_VISUALCPP
            #    pragma warning(push)
            #    pragma warning(disable: 4244)
            #endif

            // TODO check_range<T, U>(_value);
            return static_cast<U>(_value);

            #if CXXREFLECT_COMPILER == CXXREFLECT_COMPILER_VISUALCPP
            #    pragma warning(pop)
            #endif
        }

    private:

        #if CXXREFLECT_COMPILER == CXXREFLECT_COMPILER_VISUALCPP
        #    pragma warning(push)
        #    pragma warning(disable: 4018)
        #endif

        template <typename T0, typename T1>
        static typename std::enable_if<
            std::is_unsigned<T0>::value && std::is_unsigned<T1>::value, void
        >::type
        check_range(T0 const value)
        {
            assert_true([&]
            {
                return std::numeric_limits<T1>::max() >= value;
            });
        }

        template <typename T0, typename T1>
        static typename std::enable_if<
            !std::is_unsigned<T0>::value || !std::is_unsigned<T1>::value, void
        >::type
        check_range(T0 const value)
        {
            assert_true([&]
            {
                return std::numeric_limits<T1>::max() >= value
                    && std::numeric_limits<T1>::min() <= value;
            });
        }

        #if CXXREFLECT_COMPILER == CXXREFLECT_COMPILER_VISUALCPP
        #    pragma warning(pop)
        #endif

        T _value;
    };

    template <typename T>
    typename std::enable_if<
        std::is_integral<T>::value, integer_converter<T>
    >::type convert_integer(T const x)
    {
        return integer_converter<T>(x);
    }





    /// Utility type that is default-constructible and implicitly convertible to any type
    struct default_value
    {
        template <typename T>
        operator T() const { return T(); }
    };





    /// The identity metafunction that maps a type to itself
    template <typename T>
    struct identity
    {
        typedef T type;
    };





    /// A transformer function object that returns its argument
    struct identity_transformer
    {
        template <typename T>
        auto operator()(T&& x) const -> T { return x; }
    };





    template <typename Target, typename Source>
    auto implicit_cast(Source&& source) -> Target
    {
        return std::forward<Source>(source);
    }





    // These macros allow containers of unique_ptr<incomplete_type> by moving the definition of the
    // deleter's operator() into a .cpp file where the type is complete.  We cannot use the default
    // deleter because its operator() may be instantiated by member functions of a container other
    // than the destructor.  We have no control over which functions may be instantiated, so we
    // simply create custom deleter types for full control.
    //
    // Use CXXREFLECT_DECLARE_INCOMPLETE_DELETE once in one header to define the deleter type.  The
    // deleter is named 'name' and deletes an object of type 'type'.
    //
    // Then use CXXREFLECT_DEFINE_INCOMPLETE_DELETE in one source (.cpp) file in which the 'type'
    // is complete.
    #define CXXREFLECT_DECLARE_INCOMPLETE_DELETE(name, type) \
        struct name                                          \
        {                                                    \
            auto operator()(type* p) const -> void;          \
        }

    #define CXXREFLECT_DEFINE_INCOMPLETE_DELETE(name, type)  \
        auto name::operator()(type* const p) const -> void   \
        {                                                    \
            delete p;                                        \
        }





    /// Access key for internal members
    class internal_key
    {
    };





    template <typename T>
    struct is_iterator
    {
        enum { value = detail::is_iterator_impl<T>::value };
    };

    template <typename T>
    struct is_iterator<T*>
    {
        enum { value = true };
    };





    /// A linear allocator for arrays of elements
    ///
    /// We do a lot of allocation of arrays, where the lifetimes of the arrays are bound to the
    /// lifetime of another known object.  This very simple linear allocator allocates blocks of
    /// memory and services allocation requests for arrays.  For a canonical example of using this
    /// allocator, see its use for storing UTF-16 converted strings from the metadata database.
    ///
    /// The arrays are not destroyed until the allocator is destroyed.  No reclamation of allocated
    /// storage is attempted.
    template <typename T, size_type BlockSize>
    class linear_array_allocator
    {
    public:

        typedef size_type      size_type;
        typedef T              value_type;
        typedef T*             pointer;
        typedef array_range<T> range;

        enum { block_size = BlockSize };

        linear_array_allocator()
        {
        }

        linear_array_allocator(linear_array_allocator&& other)
            : _blocks(std::move(other._blocks)),
              _current(std::move(other._current))
        {
            other._current = block_iterator();
        }

        auto operator=(linear_array_allocator&& other) -> linear_array_allocator&
        {
            swap(other);
            return *this;
        }

        auto swap(linear_array_allocator& other) -> void
        {
            std::swap(other._blocks,  _blocks);
            std::swap(other._current, _current);
        }

        auto allocate(core::size_type const n) -> range
        {
            ensure_available(n);

            range const r(&*_current, &*_current + n);
            _current += n;
            return r;
        }

    private:

        typedef std::array<value_type, block_size> block;
        typedef typename block::iterator           block_iterator;
        typedef std::unique_ptr<block>             block_pointer;
        typedef std::vector<block_pointer>         block_sequence;
        
        auto ensure_available(size_type const n) -> void
        {
            if (n > block_size)
                throw runtime_error(L"attempted to allocate too large an array");

            if (_blocks.size() > 0)
            {
                if (core::distance(_current, end(*_blocks.back())) >= n)
                    return;
            }

            // Note:  We expressly leave the new block uninitialized, for performance.  There is
            // no reason to initialize it here; the caller will initialize it with its own data.
            block_pointer new_block(new block);
            _blocks.push_back(std::move(new_block));
            _current = begin(*_blocks.back());
        }

        block_sequence _blocks;
        block_iterator _current;
    };





    /// Computes the log base 2 of an unsigned integer
    ///
    /// Complements of http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogLookup
    template <typename T>
    auto log_base_2(T const value) -> size_type
    {
        static_assert(std::is_unsigned<T>::value, "value type must be an unsigned integral type");

        return detail::log_base_2_impl<T, sizeof(T)>::compute(value);
    }





    template <typename T>
    auto make_unique() -> std::unique_ptr<T>
    {
        std::unique_ptr<T> p(new T());
        return p;
    }

    template <typename T, typename P0>
    auto make_unique(P0&& a0) -> std::unique_ptr<T>
    {
        std::unique_ptr<T> p(new T(std::forward<P0>(a0)));
        return p;
    }

    template <typename T, typename P0, typename P1>
    auto make_unique(P0&& a0, P1&& a1) -> std::unique_ptr<T>
    {
        std::unique_ptr<T> p(new T(std::forward<P0>(a0),
                                   std::forward<P1>(a1)));
        return p;
    }

    template <typename T, typename P0, typename P1, typename P2>
    auto make_unique(P0&& a0, P1&& a1, P2&& a2) -> std::unique_ptr<T>
    {
        std::unique_ptr<T> p(new T(std::forward<P0>(a0),
                                   std::forward<P1>(a1),
                                   std::forward<P2>(a2)));
        return p;
    }

    template <typename T, typename P0, typename P1, typename P2, typename P3>
    auto make_unique(P0&& a0, P1&& a1, P2&& a2, P3&& a3) -> std::unique_ptr<T>
    {
        std::unique_ptr<T> p(new T(std::forward<P0>(a0),
                                   std::forward<P1>(a1),
                                   std::forward<P2>(a2),
                                   std::forward<P3>(a3)));
        return p;
    }

    template <typename T, typename P0, typename P1, typename P2, typename P3, typename P4>
    auto make_unique(P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4) -> std::unique_ptr<T>
    {
        std::unique_ptr<T> p(new T(std::forward<P0>(a0),
                                   std::forward<P1>(a1),
                                   std::forward<P2>(a2),
                                   std::forward<P3>(a3),
                                   std::forward<P4>(a4)));
        return p;
    }

    template <typename D, typename T>
    auto make_unique_with_delete() -> std::unique_ptr<T, D>
    {
        std::unique_ptr<T, D> p(new T());
        return p;
    }

    template <typename D, typename T, typename P0>
    auto make_unique_with_delete(P0&& a0) -> std::unique_ptr<T, D>
    {
        std::unique_ptr<T, D> p(new T(std::forward<P0>(a0)));
        return p;
    }

    template <typename D, typename T, typename P0, typename P1>
    auto make_unique_with_delete(P0&& a0, P1&& a1) -> std::unique_ptr<T, D>
    {
        std::unique_ptr<T, D> p(new T(std::forward<P0>(a0),
                                      std::forward<P1>(a1)));
        return p;
    }

    template <typename D, typename T, typename P0, typename P1, typename P2>
    auto make_unique_with_delete(P0&& a0, P1&& a1, P2&& a2) -> std::unique_ptr<T, D>
    {
        std::unique_ptr<T, D> p(new T(std::forward<P0>(a0),
                                      std::forward<P1>(a1),
                                      std::forward<P2>(a2)));
        return p;
    }

    template <typename D, typename T, typename P0, typename P1, typename P2, typename P3>
    auto make_unique_with_delete(P0&& a0, P1&& a1, P2&& a2, P3&& a3) -> std::unique_ptr<T, D>
    {
        std::unique_ptr<T, D> p(new T(std::forward<P0>(a0),
                                      std::forward<P1>(a1),
                                      std::forward<P2>(a2),
                                      std::forward<P3>(a3)));
        return p;
    }

    template <typename D, typename T, typename P0, typename P1, typename P2, typename P3, typename P4>
    auto make_unique_with_delete(P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4) -> std::unique_ptr<T, D>
    {
        std::unique_ptr<T, D> p(new T(std::forward<P0>(a0),
                                      std::forward<P1>(a1),
                                      std::forward<P2>(a2),
                                      std::forward<P3>(a3),
                                      std::forward<P4>(a4)));
        return p;
    }

    template <typename T>
    auto make_unique_array(size_type const n) -> std::unique_ptr<T[]>
    {
        std::unique_ptr<T[]> p(new T[n]());

        return p;
    }





    template <typename T>
    class optional
    {
    public:

        typedef T value_type;

        optional()
            : _value(), _has_value(false)
        {
        }

        optional(value_type const& value)
            : _value(value), _has_value(true)
        {
        }

        auto has_value() const -> bool     { return _has_value; }
        auto value()           -> T&       { return _value;     }
        auto value()     const -> T const& { return _value;     }

    private:

        value_type _value;
        bool       _has_value;
    };





    /// Computes the hamming weight (pop count) of an unsigned integer
    ///
    /// Complements of http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    template <typename T>
    inline auto pop_count(T value) -> size_type
    {
        static_assert(std::is_unsigned<T>::value, "value type must be an unsigned integral type");

        value = value - ((value >> 1) & (T)~(T)0/3);
        value = (value & (T)~(T)0/15*3) + ((value >> 2) & (T)~(T)0/15*3);
        value = (value + (value >> 4)) & (T)~(T)0/255*15;
        value = (T)(value * ((T)~(T)0/255)) >> (sizeof(T) - 1) * 8;

        return static_cast<size_type>(value);
    }





    /// A scope-guard implementation that calls a function when it is destroyed
    ///
    /// This implementation is good enough for most uss, though because it uses `std::function` 
    /// (which in turn may use dynamic allocation), it may throw when it is constructed due to
    /// failed allocation.
    class scope_guard
    {
    public:

        typedef std::function<void()> function_type;

        explicit scope_guard(function_type f)
            : _f(std::move(f))
        {
        }

        ~scope_guard()
        {
            if (_f != nullptr)
                _f();
        }

        auto release() -> void
        {
            _f = nullptr;
        }

    private:

        function_type _f;
    };





    /// A uniquely-owned array
    class unique_byte_array
    {
    public:

        typedef std::function<void()> release_function;

        unique_byte_array() 
            : _first(nullptr), _last(nullptr)
        {
        }

        /// Constructs a new `unique_byte_array` from an array of bytes and optional release function
        ///
        /// The release function is optional:  if it is provided, it will be called when this object
        /// is destroyed  (unless it is moved-from).  If it is not provided, no cleanup will be done.
        /// The use case for no-cleanup is an array of bytes with static storage duration.
        unique_byte_array(const_byte_iterator const first, const_byte_iterator const last, release_function release = nullptr)
            : _first(first), _last(last), _release(std::move(release))
        {
        }

        unique_byte_array(unique_byte_array&& other)
            : _first(other._first), _last(other._last), _release(other._release)
        {
            other._release = nullptr;
        }

        auto operator=(unique_byte_array&& other) -> unique_byte_array&
        {
            std::swap(_first,   other._first  );
            std::swap(_last,    other._last   );
            std::swap(_release, other._release);

            return *this;
        }

        ~unique_byte_array()
        {
            if (_release != nullptr)
                _release();
        }

        auto begin() const -> const_byte_iterator { return _first; }
        auto end()   const -> const_byte_iterator { return _last;  }

        auto is_initialized() const -> bool
        {
            return _first != nullptr && _last != nullptr;
        }

    private:

        const_byte_iterator _first;
        const_byte_iterator _last;
        release_function    _release;
    };





    /// Value initialization wrapper
    ///
    /// This value initialization wrapper should be used for all data members that have POD type or
    /// which otherwise would not be correctly initialized implicitly.  This type ensures that the
    /// contained object is always initialized without having to do any explicit initialization.
    template <typename T>
    class value_initialized
    {
    public:

        typedef T        value_type;
        typedef T &      reference;
        typedef T const& const_reference;

        value_initialized()
            : _value()
        {
        }

        explicit value_initialized(const_reference value)
            : _value(value)
        {
        }

        auto get()       -> reference       { return _value; }
        auto get() const -> const_reference { return _value; }
        
        auto reset() -> void { _value = value_type(); }

        friend auto operator==(value_initialized const& lhs, value_initialized const& rhs) -> bool
        {
            return std::equal_to<value_type>()(lhs._value, rhs._value);
        }

        friend auto operator<(value_initialized const& lhs, value_initialized const& rhs) -> bool
        {
            return std::less<value_type>()(lhs._value, rhs._value);
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(value_initialized)

    private:

        value_type _value;
    };

} }

#endif
