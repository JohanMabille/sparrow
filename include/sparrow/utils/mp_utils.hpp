// Copyright 2024 Man Group Operations Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <concepts>
#include <cstdint>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>

namespace sparrow::mpl
{

    /// Workaround to replace static_assert(false) in template code.
    /// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2593r1.html
    template <class... T>
    struct dependent_false : std::false_type
    {
    };

    template <class L, template <class...> class U>
    struct is_type_instance_of : std::false_type
    {
    };

    template <template <class...> class L, template <class...> class U, class... T>
        requires std::is_same_v<L<T...>, U<T...>>
    struct is_type_instance_of<L<T...>, U> : std::true_type
    {
    };

    /// `true` if `T` is a concrete type template instanciation of `U` which is a type template.
    /// Example: is_type_instance_of_v< std::vector<int>, std::vector > == true
    template <class T, template <class...> class U>
    constexpr bool is_type_instance_of_v = is_type_instance_of<T, U>::value;

    /// A sequence of types, used for meta-programming operations.
    template <class... T>
    struct typelist
    {
    };

    /// Appends the given types to a typelist.
    ///
    /// This function takes a typelist and additional types as arguments, and returns a new typelist
    /// that contains all the types from the original typelist followed by the additional types.
    ///
    /// @tparam Ts... The types in the original typelist.
    /// @tparam Us... The additional types to be appended.
    /// @param typelist<Ts...> The original typelist.
    /// @param Us... The additional types.
    /// @return A new typelist containing all the types from the original typelist followed by the additional
    /// types.
    template <class... Ts, class... Us>
        requires(!is_type_instance_of_v<Us, typelist> && ...)
    consteval auto append(typelist<Ts...>, Us...)
    {
        return typelist<Ts..., Us...>{};
    }

    /// Appends two typelists together.
    ///
    /// This function takes two typelists as input and returns a new typelist that contains all the types from
    /// both input typelists.
    ///
    /// @tparam Ts... The types in the first typelist.
    /// @tparam Us... The types in the second typelist.
    /// @param list1 The first typelist.
    /// @param list2 The second typelist.
    /// @return A new typelist that contains all the types from both input typelists.
    template <class... Ts, class... Us>
    consteval auto append(typelist<Ts...>, typelist<Us...>)  // TODO: Handle several typelists
    {
        return typelist<Ts..., Us...>{};
    }


    /// Appends one or more types or typelist to a given TypeList.
    ///
    /// This template alias takes a TypeList and one or more types or typelist as template arguments.
    /// It appends the types to the given TypeList and returns the resulting TypeList.
    ///
    /// @tparam TypeList The TypeList to which the types will be appended.
    /// @tparam Us The types or typelists to be appended to the TypeList.
    /// @return The resulting TypeList after appending the types.
    template <class TypeList, class... Us>
        requires mpl::is_type_instance_of_v<TypeList, typelist>
    using append_t = decltype(append(TypeList{}, Us{}...));

    /// @returns The count of types contained in a given `typelist`.
    template <class... T>
    constexpr std::size_t size(typelist<T...> = {})
    {
        return sizeof...(T);
    }

    /// Matches any type which is an instance of `typelist`.
    /// Examples:
    ///     static_assert(any_typelist< typelist<int, float, std::vector<int> > > == true);
    ///     static_assert(any_typelist< std::vector<int> > == false);
    template <typename TList>
    concept any_typelist = is_type_instance_of_v<TList, typelist>;

    namespace impl
    {
        template <class From, template <class...> class To>
        struct rename_impl;

        template <template <class...> class From, template <class...> class To, class... T>
        struct rename_impl<From<T...>, To>
        {
            using type = To<T...>;
        };
    }

    /// Changes the type of the list to the given type/
    /// Example:
    ///     static_assert(std::same_as<rename<typelist<int, float>, std::variant>, std::variant<int, float>>)
    template <class From, template <class...> class To>
    using rename = typename impl::rename_impl<From, To>::type;

    //////////////////////////////////////////////////
    //// Type-predicates /////////////////////////////

    /// Matches template types which can be used as type-wrappers for evaluation in type-predicates.
    template <template <class...> class W, class T>
    concept type_wrapper = std::same_as<W<T>, typelist<T>> or std::same_as<W<T>, std::type_identity_t<T>>;

    /// Matches template types that can be evaluated at compile-time similarly to `std::true/false_type`
    /// This makes possible to use predicates provided by the standard.
    template <template <class> class P, class T>
    concept ct_type_predicate = requires {
        { P<T>::value } -> std::convertible_to<bool>;
    };

    /// Matches types whose instance can be called with an object representing a type to evaluate it.
    /// The object passed to calling the predicate instance is either a typelist with one element or
    /// `std::type_identity_t<T>`. Basically a value wrapper for representing a type.
    /// This is useful to detect type predicates which allow being called like normal functions.
    template< class P, class T >
    concept callable_type_predicate = std::semiregular<std::decay_t<P>>
        and (
                requires(std::decay_t<P> predicate)
                {
                    { predicate(typelist<T>{}) } -> std::convertible_to<bool>;
                }
            or
                requires(std::decay_t<P> predicate)
                {
                    { predicate(std::type_identity_t<T>{}) } -> std::convertible_to<bool>;
                }
            )
        ;

    /// Evaluates the provided compile-time template class predicate P given a type T, if P is of a similar
    /// shape to `std::true/false_type`.
    template <class T, template <class> class P>
        requires ct_type_predicate<P, T>
    consteval bool evaluate(P<T>)
    {
        return P<T>::value;
    }

    /// Evaluates the provided compile-time template class predicate P given a type T, if P's instance is
    /// callable given a value type wrapper of T.
    template <class T, callable_type_predicate<T> P>
    [[nodiscard]] consteval bool evaluate(P predicate)
    {
        if constexpr (requires(std::decay_t<P> p) {
                          { p(typelist<T>{}) } -> std::same_as<bool>;
                      })
        {
            return predicate(typelist<T>{});
        }
        else
        {
            return predicate(std::type_identity_t<T>{});
        }
    }

    namespace predicate
    {

        /// Compile-time type predicate: `true` if the evaluated type is the same as `T`.
        /// Example:
        ///     static constexpr auto some_types = typelist<int, float>{}
        ///     static constexpr auto same_as_int = same_as<int>{};
        ///     static_assert( any_of(some_types, same_as_int) == true);
        ///     static_assert( all_of(some_types, same_as_int) == false);
        template <class T>
        struct same_as
        {
            template <template <class...> class W, class X>
                requires type_wrapper<W, X>
            consteval bool operator()(W<X>) const
            {
                return std::same_as<T, X>;
            }
        };
    }

    template <template <class> class P>
    struct ct_type_predicate_to_callable
    {
        template <template <class...> class W, class T>
            requires ct_type_predicate<P, T> and type_wrapper<W, T>
        consteval bool operator()(W<T>) const
        {
            return P<T>::value;
        }
    };

    /// @returns A callable type-predicate object which will return `P<T>::value` when called with `T` in a
    /// type-wrapper.
    template <template <class> class P>
    consteval auto as_predicate()
    {
        return ct_type_predicate_to_callable<P>{};
    };

    //////////////////////////////////////////////////
    //// Algorithms //////////////////////////////////

    /// Checks that at least one type in the provided list of is making the provided predicate return `true`.
    /// @returns 'true' if for at least one type T in the type list L,, `Predicate{}(typelist<T>) == true`.
    ///          `false` otherwise or if the list is empty.
    template <class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (callable_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval bool any_of(L<T...>, [[maybe_unused]] Predicate predicate = {})
    {
        return (evaluate<T>(predicate) || ... || false);
    }

    /// Checks that at least one type in the provided list of is making the provided predicate return `true`.
    /// @returns 'true' if for at least one type T in the type list L, `Predicate<T>::value == true`.
    ///          `false` otherwise or if the list is empty.
    template <template <class> class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (ct_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval bool any_of(L<T...> list)
    {
        return any_of(list, as_predicate<Predicate>());
    }

    /// Checks that every type in the provided list of is making the provide predicate return `true`.
    /// @returns `true` if for every type T in the type list L, `Predicate{}(typelist<T>) == true`
    ///          or if the list is empty; `false` otherwise.
    template <class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (callable_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval bool all_of(L<T...>, [[maybe_unused]] Predicate predicate)  // predicate is used
                                                                                        // but GCC does not
                                                                                        // see it, that's why
                                                                                        // we use
                                                                                        // [[maybe_unused]]
    {
        return (evaluate<T>(predicate) && ... && true);
    }

    /// Checks that every type in the provided list of is making the provide predicate return `true`.
    /// @returns `true` if for every type T in the type list L, `Predicate<T>::value == true`
    ///          or if the list is empty; `false` otherwise.
    template <template <class> class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (ct_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval bool all_of(L<T...> list)
    {
        return all_of(list, as_predicate<Predicate>());
    }

    /// @returns `true` if the provided type list contains `V`
    template <class V, any_typelist L>
    [[nodiscard]] consteval bool contains(L list)
    {
        return any_of(list, predicate::same_as<V>{});
    }

    /// @returns The index position in the first type in the provided type list `L` that matches the provided
    /// predicate,
    ///          or the size of the list if the matching type was not found.
    template <class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (callable_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval std::size_t find_if(L<T...>, Predicate predicate)
    {
        std::size_t idx = 0;
        auto check = [&](bool match_success)
        {
            if (match_success)
            {
                return true;
            }
            else
            {
                ++idx;
                return false;
            }
        };

        (check(evaluate<T>(predicate)) || ...);

        return idx;
    }

    /// @returns The index position in the first type in the provided type list `L` that matches the provided
    /// predicate,
    ///          or the size of the list if the matching type was not found.
    template <template <class> class Predicate, template <class...> class L, class... T>
        requires any_typelist<L<T...>> and (ct_type_predicate<Predicate, T> && ...)
    [[nodiscard]] consteval std::size_t find_if(L<T...> list)
    {
        return find_if(list, as_predicate<Predicate>());
    }

    /// @returns The index position in the type `TypeToFind` in the provided type list `L`,
    ///           or the size of the list if the matching type was not found.
    template <class TypeToFind, any_typelist L>
    [[nodiscard]] consteval std::size_t find(L list)
    {
        return find_if(list, predicate::same_as<TypeToFind>{});
    }

    namespace impl
    {
        template <template <class...> class F, class... L>
        struct transform_impl
        {
            static_assert(dependent_false<L...>::value, "transform can apply to typelist-like types only");
        };

        template <template <class...> class F, template <class...> class L, class... T>
        struct transform_impl<F, L<T...>>
        {
            using type = L<F<T>...>;
        };

        template <template <class...> class F, template <class...> class L1, class... T1, template <class...> class L2, class... T2>
        struct transform_impl<F, L1<T1...>, L2<T2...>>
        {
            using type = L1<F<T1, T2>...>;
        };
    }

    /// Applies the metafunction F to each tuple of elements in the typelists and returns the
    /// corresponding list
    /// Example:
    ///     transform<std::add_pointer_t, typelist<int, float> gives typelist<int*, float*>
    ///     transform<std::is_same, typelist<int, float>, typelist<int, int>> gives
    ///         typelist<std::is_same<int, int>, std::is_same<float, int>>
    template <template <class> class F, class... L>
    using transform = typename impl::transform_impl<F, L...>::type;

    namespace impl
    {
        // TODO: merge this implementation and the consteval contains function
        template <class L, class T>
        struct contains_impl;

        template <template <class...> class L, class T>
        struct contains_impl<L<>, T> : std::false_type
        {
        };

        template <template <class...> class L, class T, class... U>
        struct contains_impl<L<T, U...>, T> : std::true_type
        {
        };

        template <template <class...> class L, class V, class... U, class T>
        struct contains_impl<L<V, U...>, T> : contains_impl<L<U...>, T>
        {
        };

        template <class S1, class S2>
        struct merge_set_impl;

        template <template <class...> class L, class... T>
        struct merge_set_impl<L<T...>, L<>>
        {
            using type = L<T...>;
        };

        template <template <class...> class L, class... T, class U, class... V>
        struct merge_set_impl<L<T...>, L<U, V...>>
        {
            using first_arg = std::conditional_t<contains_impl<L<T...>, U>::value, L<T...>, L<T..., U>>;
            using type = typename merge_set_impl<first_arg, L<V...>>::type;
        };
    }

    /// Generates the union of two typelists L1 and L2 and removes all dupolicated types
    /// Example:
    ///    merge_set<typelist<int, float>, typelist<float, double> gives typelist<int, float, double>
    template <class L1, class L2>
    using merge_set = typename impl::merge_set_impl<L1, L2>::type;

    namespace impl
    {
        template <class L>
        struct unique_impl;

        template <template <class...> class L, class... T>
        struct unique_impl<L<T...>>
        {
            using type = merge_set<L<>, L<T...>>;
        };
    }

    /// Removes all duplicated types in the given typelist
    /// Example:
    ///     unique<typelist<int, float, double, float, int> gives typelist<int, float, double>
    template <class L>
    using unique = typename impl::unique_impl<L>::type;

    //////////////////////////////////////////////////
    //// Miscellaneous ///////////////////////////////

    template <class T>
    struct add_const_lvalue_reference : std::add_lvalue_reference<std::add_const_t<T>>
    {
    };

    template <class T>
    using add_const_lvalue_reference_t = typename add_const_lvalue_reference<T>::type;

    template <typename T, bool is_const>
    struct constify
    {
        using type = std::conditional_t<is_const, const T, T>;
    };

    template <typename T, bool is_const>
    struct constify<T&, is_const>
    {
        using type = std::conditional_t<is_const, const T&, T&>;
    };

    // `constify_t` is required since `std::add_const_t<T&>`
    // is not `const T&` but `T&`.
    template <class T, bool is_const>
    using constify_t = typename constify<T, is_const>::type;

    /// Computes the const reference type of T.
    ///
    /// @tparam T The const reference type of T.
    template <class T>
    using iter_const_reference_t = std::common_reference_t<const std::iter_value_t<T>&&, std::iter_reference_t<T>>;

    /// Represents a constant iterator.
    ///
    /// A constant iterator is an iterator that satisfies the following requirements:
    /// - It is an input iterator.
    /// - The reference type of the iterator is the same as the const reference type of the iterator.
    ///
    /// @tparam T The type of the iterator.
    template <class T>
    concept constant_iterator = std::input_iterator<T>
                                && std::same_as<iter_const_reference_t<T>, std::iter_reference_t<T>>;

    /// The constant_range concept is a refinement of range for which ranges::begin returns a constant
    /// iterator.
    ///
    /// A constant range is a range that satisfies the following conditions:
    /// - It is an input range.
    /// - Its iterator type is a constant iterator.
    ///
    /// @tparam T The type to be checked for constant range concept.
    template <class T>
    concept constant_range = std::ranges::input_range<T> && constant_iterator<std::ranges::iterator_t<T>>;

    /// Invokes undefined behavior. An implementation may use this to optimize impossible code branches
    /// away (typically, in optimized builds) or to trap them to prevent further execution (typically, in
    /// debug builds).
    ///
    /// @note Documentation and implementation come from https://en.cppreference.com/w/cpp/utility/unreachable
    [[noreturn]] inline void unreachable()
    {
        // Uses compiler specific extensions if possible.
        // Even if no extension is used, undefined behavior is still raised by
        // an empty function body and the noreturn attribute.
#if defined(_MSC_VER) && !defined(__clang__)  // MSVC
        __assume(false);
#else  // GCC, Clang
        __builtin_unreachable();
#endif
    }

    /// Matches range types whose elements are convertible to bool.
    template <class BoolRange>
    concept bool_convertible_range = std::ranges::range<BoolRange>
                                     && std::convertible_to<std::ranges::range_value_t<BoolRange>, bool>;

    /// Matches types that can be convertible to and assignable from bool. We do not use
    /// `std::convertible_to` because we don't want to impose an implicit conversion.
    template <class T>
    concept boolean_like = std::is_assignable_v<std::add_lvalue_reference_t<std::decay_t<T>>, bool>
                           and requires { static_cast<bool>(std::declval<T>()); };

    /// Matches range types From whose elements are convertible to elements of range type To.
    template <class From, class To>
    concept convertible_ranges = std::convertible_to<std::ranges::range_value_t<From>, std::ranges::range_value_t<To>>;

    // Matches any `unique_ptr` instance.
    template <typename T>
    concept unique_ptr = mpl::is_type_instance_of_v<std::remove_reference_t<T>, std::unique_ptr>;

    // Matches any `unique_ptr` or derived instance.
    template <typename T>
    concept unique_ptr_or_derived = unique_ptr<T>
                                    || std::derived_from<T, std::unique_ptr<typename T::element_type>>;

    // Matches any `shared_ptr` instance.
    template <typename T>
    concept shared_ptr = mpl::is_type_instance_of_v<std::remove_reference_t<T>, std::shared_ptr>;

    // Matches any `shared_ptr` or derived instance.
    template <typename T>
    concept shared_ptr_or_derived = shared_ptr<T>
                                    || std::derived_from<T, std::shared_ptr<typename T::element_type>>;

    // Matches any `unique_ptr` or `shared_ptr` instance.
    template <typename T>
    concept smart_ptr = unique_ptr<T> || shared_ptr<T>;

    // Matches any `unique_ptr` or `shared_ptr` or derived instance.
    template <typename T>
    concept smart_ptr_and_derived = shared_ptr_or_derived<T> || unique_ptr_or_derived<T>;

    // Matches any type that is testable
    template <class T>
    concept testable = requires(T t) { t ? true : false; };

    // Fails if the Qualifier is true for Y but not for T.
    template <typename T, typename Y, template <typename> typename Qualifier>
    concept T_matches_qualifier_if_Y_is = Qualifier<T>::value || !Qualifier<Y>::value;

    // helper class to exclude copy and move constructors beeing routed
    // to a constructor with variadic arguments / perfect forwarding
    template <class CLS, class... ARGS>
    struct excludes_copy_and_move_ctor
    {
        static constexpr bool value = true;
    };

    template <class CLS>
    struct excludes_copy_and_move_ctor<CLS>
    {
        static constexpr bool value = true;
    };

    template <class CLS, class T>
    struct excludes_copy_and_move_ctor<CLS, T>
    {
        static constexpr bool value = !std::is_same_v<CLS, std::remove_cvref_t<T>>;
    };

    template <class CLS, class... ARGS>
    constexpr bool excludes_copy_and_move_ctor_v = excludes_copy_and_move_ctor<CLS, ARGS...>::value;

    /**
     * Concept to check if an iterator is of a specific type.
     *
     * This concept ensures that the given iterator type `I` satisfies the
     * `std::input_iterator` concept and that the value type of the iterator
     * matches the specified type `T`.
     *
     * @tparam I The iterator type to be checked.
     * @tparam T The type that the iterator's value type should match.
     */
    template <typename I, typename T>
    concept iterator_of_type = std::input_iterator<I>
                               && std::same_as<typename std::iterator_traits<I>::value_type, T>;


    // todo...make smth better based on sizeof and is pod
    template <class T>
    concept char_like = std::same_as<T, char> || std::same_as<T, std::byte> || std::same_as<T, uint8_t>;

    // Concept for std::array
    template <typename T>
    concept std_array = requires {
        typename std::remove_cvref_t<T>::value_type;
        requires std::same_as<
            std::array<typename std::remove_cvref_t<T>::value_type, std::tuple_size<std::remove_cvref_t<T>>::value>,
            std::remove_cvref_t<T>>;
    };

    // Concept for fixed-size std::span
    template <typename T>
    concept fixed_size_span = requires {
        typename std::remove_cvref_t<T>::element_type;
        requires std::tuple_size_v<T> != std::dynamic_extent;
        requires std::same_as<
            std::span<typename std::remove_cvref_t<T>::element_type, std::remove_cvref_t<T>::extent>,
            std::remove_cvref_t<T>>;
    };
}
