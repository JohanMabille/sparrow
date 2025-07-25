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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <memory>
#include <variant>

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/utils/memory.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    namespace detail
    {
        // Helper struct to allow overloading on the type of ARRAY
        // to get the data_type for an array. This is needed since
        // some arrays (for instance run_length_encoded_array)
        // do not have a inner_value_type, therefore we specialize
        // this in their respecitve headers.
        template <class ARRAY>
        struct get_data_type_from_array;

        template <class ARRAY>
        struct is_dictionary_encoded_array
        {
            [[nodiscard]] static constexpr bool get() noexcept
            {
                return false;
            }
        };
    }

    /**
     * Base class for array type erasure
     */
    class array_wrapper
    {
    public:

        using wrapper_ptr = std::unique_ptr<array_wrapper>;

        virtual ~array_wrapper() = default;

        array_wrapper(array_wrapper&&) = delete;
        array_wrapper& operator=(const array_wrapper&) = delete;
        array_wrapper& operator=(array_wrapper&&) = delete;

        [[nodiscard]] wrapper_ptr clone() const;

        [[nodiscard]] constexpr enum data_type data_type() const noexcept;
        [[nodiscard]] constexpr bool is_dictionary() const;

        [[nodiscard]] constexpr arrow_proxy& get_arrow_proxy();
        [[nodiscard]] constexpr const arrow_proxy& get_arrow_proxy() const;

    protected:

        constexpr array_wrapper(enum data_type dt);
        constexpr array_wrapper(const array_wrapper&) = default;

    private:

        enum data_type m_data_type;
        [[nodiscard]] virtual bool is_dictionary_impl() const = 0;
        [[nodiscard]] virtual arrow_proxy& get_arrow_proxy_impl() = 0;
        [[nodiscard]] virtual const arrow_proxy& get_arrow_proxy_impl() const = 0;
        [[nodiscard]] virtual wrapper_ptr clone_impl() const = 0;
    };

    template <class T>
    class array_wrapper_impl : public array_wrapper
    {
    public:

        array_wrapper_impl(T&& ar);
        array_wrapper_impl(T* ar);
        array_wrapper_impl(std::shared_ptr<T> ar);

        ~array_wrapper_impl() override = default;

        T& get_wrapped();
        const T& get_wrapped() const;

    private:

        using wrapper_ptr = array_wrapper::wrapper_ptr;

        [[nodiscard]] constexpr enum data_type get_data_type() const noexcept;

        constexpr array_wrapper_impl(const array_wrapper_impl&);
        [[nodiscard]] constexpr bool is_dictionary_impl() const noexcept override;
        [[nodiscard]] constexpr arrow_proxy& get_arrow_proxy_impl() override;
        [[nodiscard]] constexpr const arrow_proxy& get_arrow_proxy_impl() const override;
        [[nodiscard]] wrapper_ptr clone_impl() const override;

        using storage_type = std::variant<value_ptr<T>, std::shared_ptr<T>, T*>;
        storage_type m_storage;
        T* p_array;
    };

    template <class T>
    T& unwrap_array(array_wrapper&);

    template <class T>
    const T& unwrap_array(const array_wrapper&);

    /********************************
     * array_wrapper implementation *
     ********************************/

    inline auto array_wrapper::clone() const -> wrapper_ptr
    {
        return clone_impl();
    }

    constexpr enum data_type array_wrapper::data_type() const noexcept
    {
        return m_data_type;
    }

    constexpr bool array_wrapper::is_dictionary() const
    {
        return is_dictionary_impl();
    }

    constexpr arrow_proxy& array_wrapper::get_arrow_proxy()
    {
        return get_arrow_proxy_impl();
    }

    constexpr const arrow_proxy& array_wrapper::get_arrow_proxy() const
    {
        return get_arrow_proxy_impl();
    }

    constexpr array_wrapper::array_wrapper(enum data_type dt)
        : m_data_type(dt)
    {
    }

    /*************************************
     * array_wrapper_impl implementation *
     *************************************/

    template <class T>
    array_wrapper_impl<T>::array_wrapper_impl(T&& ar)
        : array_wrapper(get_data_type())
        , m_storage(value_ptr<T>(std::move(ar)))
        , p_array(std::get<value_ptr<T>>(m_storage).get())
    {
    }

    template <class T>
    array_wrapper_impl<T>::array_wrapper_impl(T* ar)
        : array_wrapper(get_data_type())
        , m_storage(ar)
        , p_array(ar)
    {
    }

    template <class T>
    array_wrapper_impl<T>::array_wrapper_impl(std::shared_ptr<T> ar)
        : array_wrapper(get_data_type())
        , m_storage(std::move(ar))
        , p_array(std::get<std::shared_ptr<T>>(m_storage).get())
    {
    }

    template <class T>
    T& array_wrapper_impl<T>::get_wrapped()
    {
        return *p_array;
    }

    template <class T>
    const T& array_wrapper_impl<T>::get_wrapped() const
    {
        return *p_array;
    }

    template <class T>
    constexpr enum data_type array_wrapper_impl<T>::get_data_type() const noexcept
    {
        return detail::get_data_type_from_array<T>::get();
    }

    template <class T>
    constexpr array_wrapper_impl<T>::array_wrapper_impl(const array_wrapper_impl& rhs)
        : array_wrapper(rhs)
        , m_storage(value_ptr<T>(T(rhs.get_wrapped())))
        , p_array(
              std::visit(
                  [](auto&& arg)
                  {
                      using U = std::decay_t<decltype(arg)>;
                      if constexpr (std::is_same_v<U, T*>)
                      {
                          return arg;
                      }
                      else
                      {
                          return arg.get();
                      }
                  },
                  m_storage
              )
          )  // Always deep copy
    {
    }

    template <class T>
    constexpr bool array_wrapper_impl<T>::is_dictionary_impl() const noexcept
    {
        return detail::is_dictionary_encoded_array<T>::get();
    }

    template <class T>
    constexpr arrow_proxy& array_wrapper_impl<T>::get_arrow_proxy_impl()
    {
        return detail::array_access::get_arrow_proxy(*p_array);
    }

    template <class T>
    constexpr const arrow_proxy& array_wrapper_impl<T>::get_arrow_proxy_impl() const
    {
        return detail::array_access::get_arrow_proxy(*p_array);
    }

    template <class T>
    auto array_wrapper_impl<T>::clone_impl() const -> wrapper_ptr
    {
        return wrapper_ptr{new array_wrapper_impl<T>(*this)};
    }

    template <class T>
    T& unwrap_array(array_wrapper& ar)
    {
        return static_cast<array_wrapper_impl<T>&>(ar).get_wrapped();
    }

    template <class T>
    const T& unwrap_array(const array_wrapper& ar)
    {
        return static_cast<const array_wrapper_impl<T>&>(ar).get_wrapped();
    }
}
