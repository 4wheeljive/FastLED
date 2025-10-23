#pragma once

/*
Provides eanble_if and is_derived for compilers before C++14.
*/

#include "fl/stdint.h"

#include "fl/move.h"
#include "fl/int.h"

namespace fl { // mandatory namespace to prevent name collision with
               // std::enable_if.

// Define integral_constant as base for true_type and false_type
// Using enum instead of static constexpr to avoid ODR-use issues in C++11
template <typename T, T v>
struct integral_constant {
    enum : T { value = v };
    using value_type = T;
    using type = integral_constant;
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
};

// Define true_type and false_type
using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;

// Define identity trait - prevents template argument deduction
template <typename T> struct identity {
    using type = T;
};

// Define add_rvalue_reference trait (remove_reference is already defined in move.h)
template <typename T> struct add_rvalue_reference {
    using type = T&&;
};

template <typename T> struct add_rvalue_reference<T&> {
    using type = T&;
};

// Define declval for use in SFINAE expressions
template <typename T>
typename add_rvalue_reference<T>::type declval() noexcept;

// Define enable_if for SFINAE
template <bool Condition, typename T = void> struct enable_if {};

// Specialization for true condition
template <typename T> struct enable_if<true, T> {
    using type = T;
};

// if enable_if<Condition, T> is true, then there will be a member type
// called type. Otherwise it will not exist. This is (ab)used to enable
// constructors and other functions based on template parameters. If there
// is no member type, then the compiler will not fail to bind to the target
// function or operation.
template <bool Condition, typename T = void>
using enable_if_t = typename enable_if<Condition, T>::type;

// Define is_base_of to check inheritance relationship
template <typename Base, typename Derived> struct is_base_of {
  private:
    typedef u8 yes;
    typedef u16 no;
    static yes test(Base *); // Matches if Derived is convertible to Base*
    static no test(...);     // Fallback if not convertible
    enum {
        kSizeDerived = sizeof(test(static_cast<Derived *>(nullptr))),
    };

  public:
    // Use enum instead of static constexpr to avoid ODR-use issues in C++11
    enum : bool { value = (kSizeDerived == sizeof(yes)) };
};

// Define is_base_of_v for compatibility with pre-C++14
// Replaced variable template with a constant static member
template <typename Base, typename Derived> struct is_base_of_v_helper {
    static constexpr bool value = is_base_of<Base, Derived>::value;
};

// Define is_same trait
template <typename T, typename U> struct is_same {
    enum : bool { value = false };
};

// Specialization for when T and U are the same type
template <typename T> struct is_same<T, T> {
    enum : bool { value = true };
};

// Define is_same_v for compatibility with variable templates
template <typename T, typename U> struct is_same_v_helper {
    static constexpr bool value = is_same<T, U>::value;
};


// Define conditional trait
template <bool B, typename T, typename F> struct conditional {
    using type = T;
};

template <typename T, typename F> struct conditional<false, T, F> {
    using type = F;
};

template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

// Define is_array trait
template <typename T> struct is_array {
    enum : bool { value = false };
};

template <typename T> struct is_array<T[]> {
    enum : bool { value = true };
};

template <typename T, fl::size N> struct is_array<T[N]> {
    enum : bool { value = true };
};

// Define remove_extent trait
template <typename T> struct remove_extent {
    using type = T;
};

template <typename T> struct remove_extent<T[]> {
    using type = T;
};

template <typename T, fl::size N> struct remove_extent<T[N]> {
    using type = T;
};

// Define is_function trait
template <typename T> struct is_function {
    enum : bool { value = false };
};

template <typename Ret, typename... Args> struct is_function<Ret(Args...)> {
    enum : bool { value = true };
};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) const> {
    enum : bool { value = true };
};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) volatile> {
    enum : bool { value = true };
};

template <typename Ret, typename... Args>
struct is_function<Ret(Args...) const volatile> {
    enum : bool { value = true };
};

// Define add_pointer trait
template <typename T> struct add_pointer {
    using type = T *;
};

template <typename T> struct add_pointer<T &> {
    using type = T *;
};

template <typename T> struct add_pointer<T &&> {
    using type = T *;
};

template <typename T> using add_pointer_t = typename add_pointer<T>::type;

// Define remove_const trait
template <typename T> struct remove_const {
    using type = T;
};

template <typename T> struct remove_const<const T> {
    using type = T;
};

// Define is_const trait
template <typename T> struct is_const {
    enum : bool { value = false };
};

template <typename T> struct is_const<const T> {
    enum : bool { value = true };
};

// Define is_lvalue_reference trait
template <typename T> struct is_lvalue_reference {
    enum : bool { value = false };
};

template <typename T> struct is_lvalue_reference<T &> {
    enum : bool { value = true };
};

// Define is_void trait
template <typename T> struct is_void {
    enum : bool { value = false };
};

template <> struct is_void<void> {
    enum : bool { value = true };
};

// Implementation of forward
template <typename T>
constexpr T &&forward(typename remove_reference<T>::type &t) noexcept {
    return static_cast<T &&>(t);
}

// Overload for rvalue references
template <typename T>
constexpr T &&forward(typename remove_reference<T>::type &&t) noexcept {
    static_assert(!is_lvalue_reference<T>::value,
                  "Cannot forward an rvalue as an lvalue");
    return static_cast<T &&>(t);
}

// Define remove_cv trait
template <typename T> struct remove_cv {
    using type = T;
};

template <typename T> struct remove_cv<const T> {
    using type = T;
};

template <typename T> struct remove_cv<volatile T> {
    using type = T;
};

template <typename T> struct remove_cv<const volatile T> {
    using type = T;
};

template <typename T> using remove_cv_t = typename remove_cv<T>::type;

// Define decay trait
template <typename T> struct decay {
  private:
    using U = typename remove_reference<T>::type;

  public:
    using type = typename conditional<
        is_array<U>::value, typename remove_extent<U>::type *,
        typename conditional<is_function<U>::value,
                             typename add_pointer<U>::type,
                             typename remove_cv<U>::type>::type>::type;
};

template <typename T> using decay_t = typename decay<T>::type;

// Define is_pod trait (basic implementation)
template <typename T> struct is_pod {
    enum : bool { value = false }; // Default to false for safety
};

// Specializations for fundamental types
template <> struct is_pod<bool> {
    enum : bool { value = true };
};
template <> struct is_pod<char> {
    enum : bool { value = true };
};
template <> struct is_pod<signed char> {
    enum : bool { value = true };
};
template <> struct is_pod<unsigned char> {
    enum : bool { value = true };
};
template <> struct is_pod<short> {
    enum : bool { value = true };
};
template <> struct is_pod<unsigned short> {
    enum : bool { value = true };
};
template <> struct is_pod<int> {
    enum : bool { value = true };
};
template <> struct is_pod<unsigned int> {
    enum : bool { value = true };
};
template <> struct is_pod<long> {
    enum : bool { value = true };
};
template <> struct is_pod<unsigned long> {
    enum : bool { value = true };
};
template <> struct is_pod<long long> {
    enum : bool { value = true };
};
template <> struct is_pod<unsigned long long> {
    enum : bool { value = true };
};
template <> struct is_pod<float> {
    enum : bool { value = true };
};
template <> struct is_pod<double> {
    enum : bool { value = true };
};
template <> struct is_pod<long double> {
    enum : bool { value = true };
};

// Helper struct for is_pod_v (similar to other _v helpers)
template <typename T> struct is_pod_v_helper {
    static constexpr bool value = is_pod<T>::value;
};

//----------------------------------------------------------------------------
// trait to detect pointer‑to‑member‑function
// must come before Function so SFINAE sees it
//----------------------------------------------------------------------------
template <typename T> struct is_member_function_pointer;
template <typename C, typename Ret, typename... A>
struct is_member_function_pointer<Ret (C::*)(A...)>;
template <typename C, typename Ret, typename... A>
struct is_member_function_pointer<Ret (C::*)(A...) const>;

template <typename T> struct is_member_function_pointer {
    enum : bool { value = false };
};

template <typename C, typename Ret, typename... A>
struct is_member_function_pointer<Ret (C::*)(A...)> {
    enum : bool { value = true };
};

template <typename C, typename Ret, typename... A>
struct is_member_function_pointer<Ret (C::*)(A...) const> {
    enum : bool { value = true };
};

//-------------------------------------------------------------------------------
// is_integral trait (built-in integer types only)
//-------------------------------------------------------------------------------
template <typename T> struct is_integral {
    enum : bool { value = false };
};
template <> struct is_integral<bool> {
    enum : bool { value = true };
};
template <> struct is_integral<char> {
    enum : bool { value = true };
};
template <> struct is_integral<signed char> {
    enum : bool { value = true };
};
template <> struct is_integral<unsigned char> {
    enum : bool { value = true };
};
template <> struct is_integral<short> {
    enum : bool { value = true };
};
template <> struct is_integral<unsigned short> {
    enum : bool { value = true };
};
template <> struct is_integral<int> {
    enum : bool { value = true };
};
template <> struct is_integral<unsigned int> {
    enum : bool { value = true };
};
template <> struct is_integral<long> {
    enum : bool { value = true };
};
template <> struct is_integral<unsigned long> {
    enum : bool { value = true };
};
template <> struct is_integral<long long> {
    enum : bool { value = true };
};
template <> struct is_integral<unsigned long long> {
    enum : bool { value = true };
};

template <typename T> struct is_integral<const T> {
    static constexpr bool value = is_integral<T>::value;
};

template <typename T> struct is_integral<volatile T> {
    static constexpr bool value = is_integral<T>::value;
};

template <typename T> struct is_integral<T &> {
    static constexpr bool value = is_integral<T>::value;
};

//-------------------------------------------------------------------------------
// is_floating_point trait
//-------------------------------------------------------------------------------
template <typename T> struct is_floating_point {
    enum : bool { value = false };
};
template <> struct is_floating_point<float> {
    enum : bool { value = true };
};
template <> struct is_floating_point<double> {
    enum : bool { value = true };
};
template <> struct is_floating_point<long double> {
    enum : bool { value = true };
};

template <typename T> struct is_floating_point<const T> {
    static constexpr bool value = is_floating_point<T>::value;
};

template <typename T> struct is_floating_point<volatile T> {
    static constexpr bool value = is_floating_point<T>::value;
};

template <typename T> struct is_floating_point<T &> {
    static constexpr bool value = is_floating_point<T>::value;
};

//-------------------------------------------------------------------------------
// is_signed trait
//-------------------------------------------------------------------------------
template <typename T> struct is_signed {
    enum : bool { value = false };
};
template <> struct is_signed<signed char> {
    enum : bool { value = true };
};
template <> struct is_signed<short> {
    enum : bool { value = true };
};
template <> struct is_signed<int> {
    enum : bool { value = true };
};
template <> struct is_signed<long> {
    enum : bool { value = true };
};
template <> struct is_signed<long long> {
    enum : bool { value = true };
};
template <> struct is_signed<float> {
    enum : bool { value = true };
};
template <> struct is_signed<double> {
    enum : bool { value = true };
};
template <> struct is_signed<long double> {
    enum : bool { value = true };
};
// Note: sized integer types (i8, i16, i32, int64_t) are typedefs
// for the basic types above, so they automatically inherit these specializations

//-------------------------------------------------------------------------------
// Type size ranking for promotion rules
//-------------------------------------------------------------------------------
template <typename T> struct type_rank {
    static constexpr int value = 0;
};
template <> struct type_rank<bool> {
    static constexpr int value = 1;
};
template <> struct type_rank<signed char> {
    static constexpr int value = 2;
};
template <> struct type_rank<unsigned char> {
    static constexpr int value = 2;
};
template <> struct type_rank<char> {
    static constexpr int value = 2;
};
template <> struct type_rank<short> {
    static constexpr int value = 3;
};
template <> struct type_rank<unsigned short> {
    static constexpr int value = 3;
};
template <> struct type_rank<int> {
    static constexpr int value = 4;
};
template <> struct type_rank<unsigned int> {
    static constexpr int value = 4;
};
template <> struct type_rank<long> {
    static constexpr int value = 5;
};
template <> struct type_rank<unsigned long> {
    static constexpr int value = 5;
};
template <> struct type_rank<long long> {
    static constexpr int value = 6;
};
template <> struct type_rank<unsigned long long> {
    static constexpr int value = 6;
};
template <> struct type_rank<float> {
    static constexpr int value = 10;
};
template <> struct type_rank<double> {
    static constexpr int value = 11;
};
template <> struct type_rank<long double> {
    static constexpr int value = 12;
};
// Note: sized integer types (i8, i16, i32, int64_t) are typedefs
// for the basic types above, so they automatically inherit these specializations

//-------------------------------------------------------------------------------
// Helper templates for integer type promotion logic
//-------------------------------------------------------------------------------

// Helper: Choose type based on size (larger wins)
template <typename T, typename U>
struct choose_by_size {
    using type = typename conditional<
        (sizeof(T) > sizeof(U)), T,
        typename conditional<
            (sizeof(U) > sizeof(T)), U,
            void  // same size - handled elsewhere
        >::type
    >::type;
};

// Helper: Choose type based on type rank when same size (higher rank wins)
template <typename T, typename U>
struct choose_by_rank {
    using type = typename conditional<
        (type_rank<T>::value > type_rank<U>::value), T,
        typename conditional<
            (type_rank<U>::value > type_rank<T>::value), U,
            void  // same rank - handled elsewhere
        >::type
    >::type;
};

// Helper: Choose type based on signedness when same size and rank (signed wins)
template <typename T, typename U>
struct choose_by_signedness {
    static constexpr bool t_signed = is_signed<T>::value;
    static constexpr bool u_signed = is_signed<U>::value;
    static constexpr bool mixed_signedness = (t_signed != u_signed);
    
    using type = typename conditional<
        mixed_signedness && t_signed, T,
        typename conditional<
            mixed_signedness && u_signed, U,
            T  // same signedness - just pick first
        >::type
    >::type;
};

// Helper: Main integer promotion logic dispatcher
template <typename T, typename U>
struct integer_promotion_impl {
    static constexpr bool same_size = (sizeof(T) == sizeof(U));
    static constexpr bool same_rank = (type_rank<T>::value == type_rank<U>::value);
    
    using by_size_result = typename choose_by_size<T, U>::type;
    using by_rank_result = typename choose_by_rank<T, U>::type;
    using by_signedness_result = typename choose_by_signedness<T, U>::type;
    
    using type = typename conditional<
        !same_size, by_size_result,
        typename conditional<
            same_size && !same_rank, by_rank_result,
            by_signedness_result  // same size and rank
        >::type
    >::type;
};

//-------------------------------------------------------------------------------
// Common type trait for type promotion - now much cleaner!
//-------------------------------------------------------------------------------

// Primary template - fallback
template <typename T, typename U, typename = void> struct common_type_impl {
    using type = T;
};

// Same type specialization - handles all cases where T == U
template <typename T> struct common_type_impl<T, T> { 
    using type = T; 
};

// Float/double specializations - only exist when T is numeric but not the same type, otherwise compilation fails
template <typename T> 
struct common_type_impl<T, float, typename enable_if<(is_integral<T>::value || is_floating_point<T>::value) && !is_same<T, float>::value>::type> { 
    using type = float;
};

template <typename T> 
struct common_type_impl<T, double, typename enable_if<(is_integral<T>::value || is_floating_point<T>::value) && !is_same<T, double>::value>::type> { 
    using type = double;
};

// Symmetric specializations - when first type is float/double and second is numeric but not the same type
template <typename T> 
struct common_type_impl<float, T, typename enable_if<(is_integral<T>::value || is_floating_point<T>::value) && !is_same<T, float>::value>::type> { 
    using type = float;
};

template <typename T> 
struct common_type_impl<double, T, typename enable_if<(is_integral<T>::value || is_floating_point<T>::value) && !is_same<T, double>::value>::type> { 
    using type = double;
};

// Explicitly forbid i8 and u8 combinations 
// No type member = clear compilation error when accessed
template <>
struct common_type_impl<i8, u8, void> {
    // Intentionally no 'type' member - will cause error: 
    // "no type named 'type' in 'struct fl::common_type_impl<signed char, unsigned char, void>'"
};

template <>
struct common_type_impl<u8, i8, void> {
    // Intentionally no 'type' member - will cause error:
    // "no type named 'type' in 'struct fl::common_type_impl<unsigned char, signed char, void>'"
};

// Generic integer promotion logic - now much cleaner!
template <typename T, typename U>
struct common_type_impl<T, U, typename enable_if<
    is_integral<T>::value && is_integral<U>::value &&
    !is_same<T, U>::value &&
    !((is_same<T, i8>::value && is_same<U, u8>::value) ||
      (is_same<T, u8>::value && is_same<U, i8>::value))
>::type> {
    using type = typename integer_promotion_impl<T, U>::type;
};

// Mixed floating point sizes - larger wins
template <> struct common_type_impl<float, double> { using type = double; };
template <> struct common_type_impl<double, float> { using type = double; };
template <> struct common_type_impl<float, long double> { using type = long double; };
template <> struct common_type_impl<long double, float> { using type = long double; };
template <> struct common_type_impl<double, long double> { using type = long double; };
template <> struct common_type_impl<long double, double> { using type = long double; };

template <typename T, typename U> struct common_type {
    using type = typename common_type_impl<T, U>::type;
};

template <typename T, typename U>
using common_type_t = typename common_type<T, U>::type;

// This uses template magic to maybe generate a type for the given condition. If
// that type doesn't exist then a type will fail to be generated, and the
// compiler will skip the consideration of a target function. This is useful for
// enabling template constructors that only become available if the class can be
// upcasted to the desired type.
//
// Example:
// This is an optional upcasting constructor for a Ref<T>. If the type U is not
// derived from T then the constructor will not be generated, and the compiler
// will skip it.
//
// template <typename U, typename = fl::is_derived<T, U>>
// Ref(const Ref<U>& refptr) : referent_(refptr.get());
template <typename Base, typename Derived>
using is_derived = enable_if_t<is_base_of<Base, Derived>::value>;

//-----------------------------------------------------------------------------
// detect whether T has a member void swap(T&)
//-----------------------------------------------------------------------------
template <typename T> struct has_member_swap {
  private:
    // must be 1 byte vs. >1 byte for sizeof test
    typedef u8 yes;
    typedef u16 no;

    // helper<U, &U::swap> is only well-formed if U::swap(T&) exists with that
    // signature
    template <typename U, void (U::*M)(U &)> struct helper {};

    // picks this overload if helper<U, &U::swap> is valid
    template <typename U> static yes test(helper<U, &U::swap> *);

    // fallback otherwise
    template <typename> static no test(...);

  public:
    static constexpr bool value = sizeof(test<T>(nullptr)) == sizeof(yes);
};

// primary template: dispatch on has_member_swap<T>::value
template <typename T, bool = has_member_swap<T>::value> struct swap_impl;

// POD case - now using move semantics for better performance
template <typename T> struct swap_impl<T, false> {
    static void apply(T &a, T &b) {
        T tmp = fl::move(a);
        a = fl::move(b);
        b = fl::move(tmp);
    }
};

// non‑POD case (requires T implements swap)
template <typename T> struct swap_impl<T, true> {
    static void apply(T &a, T &b) { a.swap(b); }
};

// single entry‑point
template <typename T> void swap(T &a, T &b) {
    // if T is a POD, use use a simple data copy swap.
    // if T is not a POD, use the T::Swap method.
    swap_impl<T>::apply(a, b);
}

template <typename T> void swap_by_copy(T &a, T &b) {
    // Force copy semantics (for cases where move might not be safe)
    T tmp = a;
    a = b;
    b = tmp;
}

// Container type checks.
template <typename T, typename... Types> struct contains_type;

template <typename T> struct contains_type<T> {
    enum : bool { value = false };
};

template <typename T, typename U, typename... Rest>
struct contains_type<T, U, Rest...> {
    static constexpr bool value =
        fl::is_same<T, U>::value || contains_type<T, Rest...>::value;
};

// Helper to get maximum size of types
template <typename... Types> struct max_size;

template <> struct max_size<> {
    enum : fl::size { value = 0 };
};

template <typename T, typename... Rest> struct max_size<T, Rest...> {
    static constexpr fl::size value = (sizeof(T) > max_size<Rest...>::value)
                                        ? sizeof(T)
                                        : max_size<Rest...>::value;
};

// Helper to get maximum alignment of types
template <typename... Types> struct max_align;

template <> struct max_align<> {
    enum : fl::size { value = 1 };
};

template <typename T, typename... Rest> struct max_align<T, Rest...> {
    static constexpr fl::size value = (alignof(T) > max_align<Rest...>::value)
                                        ? alignof(T)
                                        : max_align<Rest...>::value;
};

// alignment_of trait
template <typename T>
struct alignment_of {
    enum : fl::size { value = alignof(T) };
};



// C++11 requires out-of-class definitions for static constexpr members that are ODR-used
// These definitions are in src/fl/static_constexpr_defs.cpp to avoid duplicate symbols

// For comparison operators that return bool against pod data. The class obj
// will need to supply the comparison operator for the pod type. This example
// will show how to define a comparison operator for a class that can be
// compared against a pod type.
// Example:
//   FASTLED_DEFINE_POD_COMPARISON_OPERATOR(Myclass, >=) will allow MyClass to
//   be compared MyClass obj; return obj >= 0;
#define FASTLED_DEFINE_POD_COMPARISON_OPERATOR(CLASS, OP)                      \
    template <typename T, typename U>                                          \
    typename enable_if<                                                        \
        is_same<U, CLASS>::value && is_pod<T>::value, bool>::type             \
    operator OP(const T &pod, const CLASS &obj) {                              \
        return pod OP obj;                                                     \
    }                                                                          \
    template <typename T>                                                      \
    typename enable_if<is_pod<T>::value, bool>::type operator OP(             \
        const CLASS &obj, const T &pod) {                                      \
        return obj OP pod;                                                     \
    }

} // namespace fl
