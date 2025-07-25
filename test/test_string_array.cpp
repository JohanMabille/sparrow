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

#include <cstdint>
#include <string>
#include <vector>

#include "sparrow/arrow_interface/arrow_array_schema_proxy.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/u8_buffer.hpp"
#include "sparrow/utils/nullable.hpp"
#include "sparrow/variable_size_binary_array.hpp"

#include "../test/external_array_data_creation.hpp"
#include "doctest/doctest.h"
#include "metadata_sample.hpp"
#include "test_utils.hpp"


using namespace std::literals;

namespace sparrow
{
    struct string_array_fixture
    {
        using layout_type = string_array;

        string_array_fixture()
            : m_arrow_proxy(create_arrow_proxy())
        {
        }

        arrow_proxy m_arrow_proxy;
        static constexpr size_t m_length = 10;
        static constexpr size_t m_offset = 1;
        static constexpr std::array<size_t, 2> m_false_bitmap{2, 5};

    private:

        static_assert(is_string_array_v<layout_type>);
        static_assert(std::same_as<layout_type::inner_value_type, std::string>);
        // static_assert(std::same_as<layout_type::inner_reference,
        // sparrow::variable_size_binary_reference<layout_type>>);
        static_assert(std::same_as<layout_type::inner_const_reference, std::string_view>);
        using const_value_iterator = layout_type::const_value_iterator;
        static_assert(std::same_as<const_value_iterator::value_type, std::string>);

        static_assert(std::same_as<const_value_iterator::reference, std::string_view>);

        arrow_proxy create_arrow_proxy()
        {
            ArrowSchema schema{};
            ArrowArray array{};
            const std::vector<size_t> false_bitmap{m_false_bitmap.begin(), m_false_bitmap.end()};
            test::fill_schema_and_array<std::string>(schema, array, m_length, m_offset, false_bitmap);
            return arrow_proxy{std::move(array), std::move(schema)};
        }
    };

    const std::vector<std::string> words{"hello", " ", "ugly", "", "world"};
    const std::unordered_set<std::size_t> where_nulls{2, 3};

    TEST_SUITE("string_array")
    {
        TEST_CASE("convenience")
        {
            SUBCASE("high-level")
            {
                string_array array(words, where_nulls, "name", metadata_sample_opt);

                CHECK_EQ(array.name(), "name");
                test_metadata(metadata_sample, *(array.metadata()));

                REQUIRE_EQ(array.size(), words.size());

                // check nulls
                CHECK_EQ(array[0].has_value(), true);
                CHECK_EQ(array[1].has_value(), true);
                CHECK_EQ(array[2].has_value(), false);
                CHECK_EQ(array[3].has_value(), false);
                CHECK_EQ(array[4].has_value(), true);

                // check values
                CHECK_EQ(array[0].value(), "hello");
                CHECK_EQ(array[1].value(), " ");
                CHECK_EQ(array[4].value(), "world");
            }
        }

        TEST_CASE_FIXTURE(string_array_fixture, "constructor")
        {
            SUBCASE("copy arrow_proxy")
            {
                CHECK_NOTHROW(layout_type(m_arrow_proxy));
            }

            SUBCASE("move arrow_proxy")
            {
                CHECK_NOTHROW(layout_type(std::move(m_arrow_proxy)));
            }

            SUBCASE("from u8_buffer, offset_buffer_type, validity_bitmap_input, name and metadata")
            {
                auto joined_words = std::ranges::views::join(words);
                auto size_range = words
                                  | std::views::transform(
                                      [](const auto& v)
                                      {
                                          return std::ranges::size(v);
                                      }
                                  );
                auto offset_buffer = layout_type::offset_from_sizes(size_range);
                u8_buffer<char> data_buffer(joined_words);
                CHECK_NOTHROW(
                    layout_type{std::move(data_buffer), std::move(offset_buffer), where_nulls, "name", metadata_sample_opt}
                );
            }

            SUBCASE("from values range, validity input, name and metadata")
            {
                CHECK_NOTHROW(layout_type{words, where_nulls, "name", metadata_sample_opt});
            }

            SUBCASE("from  values range, nullable, name and metadata")
            {
                CHECK_NOTHROW(layout_type{words, true, "name", metadata_sample_opt});
                CHECK_NOTHROW(layout_type{words, false, "name", metadata_sample_opt});
            }

            SUBCASE("from nullable range, name and metadata")
            {
                std::vector<nullable<std::string>> nullable_words;
                for (size_t i = 0; i < words.size(); ++i)
                {
                    if (where_nulls.contains(i))
                    {
                        nullable_words.emplace_back();
                    }
                    else
                    {
                        nullable_words.emplace_back(words[i]);
                    }
                }
                CHECK_NOTHROW(layout_type{nullable_words, "name", metadata_sample_opt});
            }
        }

        TEST_CASE_FIXTURE(string_array_fixture, "copy")
        {
            const layout_type ar(m_arrow_proxy);
            layout_type ar2(ar);
            CHECK_EQ(ar, ar2);

            layout_type ar3(std::move(m_arrow_proxy));
            ar3 = ar2;
            CHECK_EQ(ar2, ar3);
        }

        TEST_CASE_FIXTURE(string_array_fixture, "move")
        {
            layout_type ar(m_arrow_proxy);
            layout_type ar2(ar);
            layout_type ar3(std::move(ar));
            CHECK_EQ(ar2, ar3);

            layout_type ar4(std::move(m_arrow_proxy));
            ar4 = std::move(ar3);
            CHECK_EQ(ar2, ar4);
        }

        TEST_CASE_FIXTURE(string_array_fixture, "size")
        {
            const layout_type array(std::move(m_arrow_proxy));
            CHECK_EQ(array.size(), m_length - m_offset);
        }

        TEST_CASE_FIXTURE(string_array_fixture, "operator[]")
        {
            SUBCASE("const")
            {
                const layout_type array(std::move(m_arrow_proxy));
                REQUIRE_EQ(array.size(), m_length - m_offset);
                const auto cref0 = array[0];
                REQUIRE(cref0.has_value());
                CHECK_EQ(cref0.get(), "upon");
                const auto cref1 = array[1];
                REQUIRE_FALSE(cref1.has_value());
                const auto cref2 = array[2];
                REQUIRE(cref2.has_value());
                CHECK_EQ(cref2.get(), "time");
                const auto cref3 = array[3];
                REQUIRE(cref3.has_value());
                CHECK_EQ(cref3.get(), "I");
                const auto cref4 = array[4];
                REQUIRE_FALSE(cref4.has_value());
                const auto cref5 = array[5];
                REQUIRE(cref5.has_value());
                CHECK_EQ(cref5.get(), "writing");
                const auto cref6 = array[6];
                REQUIRE(cref6.has_value());
                CHECK_EQ(cref6.get(), "clean");
                const auto cref7 = array[7];
                REQUIRE(cref7.has_value());
                CHECK_EQ(cref7.get(), "code");
                const auto cref8 = array[8];
                REQUIRE(cref8.has_value());
                CHECK_EQ(cref8.get(), "now");
            }

            SUBCASE("mutable")
            {
                layout_type array(std::move(m_arrow_proxy));
                REQUIRE_EQ(array.size(), m_length - m_offset);
                auto ref0 = array[0];
                REQUIRE(ref0.has_value());
                CHECK_EQ(ref0.get(), "upon");
                auto ref1 = array[1];
                REQUIRE_FALSE(ref1.has_value());
                auto ref2 = array[2];
                REQUIRE(ref2.has_value());
                CHECK_EQ(ref2.get(), "time");
                auto ref3 = array[3];
                REQUIRE(ref3.has_value());
                CHECK_EQ(ref3.get(), "I");
                auto ref4 = array[4];
                REQUIRE_FALSE(ref4.has_value());
                auto ref5 = array[5];
                REQUIRE(ref5.has_value());
                CHECK_EQ(ref5.get(), "writing");
                auto ref6 = array[6];
                REQUIRE(ref6.has_value());
                CHECK_EQ(ref6.get(), "clean");
                auto ref7 = array[7];
                REQUIRE(ref7.has_value());
                CHECK_EQ(ref7.get(), "code");
                auto ref8 = array[8];
                REQUIRE(ref8.has_value());
                CHECK_EQ(ref8.get(), "now");

                array[6] = make_nullable<std::string>("fabulous");
                CHECK_EQ(ref6.get(), "fabulous");
                CHECK_EQ(ref7.get(), "code");
                CHECK_EQ(ref8.get(), "now");

                array[6] = make_nullable<std::string>("!");
                CHECK_EQ(ref6.get(), "!");
                CHECK_EQ(ref7.get(), "code");
                CHECK_EQ(ref8.get(), "now");
            }
        }

        TEST_CASE_FIXTURE(string_array_fixture, "value")
        {
            SUBCASE("const")
            {
                const layout_type array(std::move(m_arrow_proxy));
                CHECK_EQ(array.value(0), "upon");
                CHECK_EQ(array.value(1), "a");
                CHECK_EQ(array.value(2), "time");
                CHECK_EQ(array.value(3), "I");
                CHECK_EQ(array.value(4), "was");
                CHECK_EQ(array.value(5), "writing");
                CHECK_EQ(array.value(6), "clean");
            }

            SUBCASE("mutable")
            {
                layout_type array(std::move(m_arrow_proxy));
                CHECK_EQ(array.value(0), "upon");
                CHECK_EQ(array.value(1), "a");
                CHECK_EQ(array.value(2), "time");
                CHECK_EQ(array.value(3), "I");
                CHECK_EQ(array.value(4), "was");
                CHECK_EQ(array.value(5), "writing");
                CHECK_EQ(array.value(6), "clean");
                CHECK_EQ(array.value(7), "code");
                CHECK_EQ(array.value(8), "now");

                array.value(6) = "fabulous";
                CHECK_EQ(array.value(6), "fabulous");
                CHECK_EQ(array.value(7), "code");
                CHECK_EQ(array.value(8), "now");
                array.value(6) = "!";
                CHECK_EQ(array.value(6), "!");
                CHECK_EQ(array.value(7), "code");
                CHECK_EQ(array.value(8), "now");
            }
        }

        TEST_CASE_FIXTURE(string_array_fixture, "const_bitmap_iterator")
        {
            SUBCASE("ordering")
            {
                const layout_type array(std::move(m_arrow_proxy));
                const auto array_bitmap = array.bitmap();
                CHECK(array_bitmap.begin() < array_bitmap.end());
            }

            SUBCASE("equality")
            {
                const layout_type array(std::move(m_arrow_proxy));
                const auto array_bitmap = array.bitmap();

                layout_type::const_bitmap_iterator citer = array_bitmap.begin();
                CHECK_EQ(*citer, true);
                CHECK_EQ(*(++citer), false);
                CHECK_EQ(*(++citer), true);
                CHECK_EQ(*(++citer), true);
                CHECK_EQ(*(++citer), false);
                CHECK_EQ(*(++citer), true);
                CHECK_EQ(*(++citer), true);
                CHECK_EQ(*(++citer), true);
                CHECK_EQ(*(++citer), true);
            }
        }

        TEST_CASE_FIXTURE(string_array_fixture, "iterator")
        {
            SUBCASE("const")
            {
                const layout_type array(std::move(m_arrow_proxy));
                auto it = array.cbegin();

                REQUIRE(it->has_value());
                CHECK_EQ(it->value(), "upon");
                CHECK_EQ(*it, make_nullable(array[0].value()));
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), "a");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "time");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "I");
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), "was");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "writing");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "clean");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "code");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "now");
                ++it;

                CHECK_EQ(it, array.end());
            }

            SUBCASE("non const")
            {
                layout_type array(std::move(m_arrow_proxy));
                auto it = array.begin();

                REQUIRE(it->has_value());
                CHECK_EQ(it->value(), "upon");
                CHECK_EQ(*it, make_nullable(array[0].value()));
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), "a");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "time");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "I");
                ++it;

                CHECK_FALSE(it->has_value());
                CHECK_EQ(it->get(), "was");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "writing");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "clean");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "code");
                ++it;

                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "now");
                ++it;

                CHECK_EQ(it, array.end());

                --it;
                --it;
                *it = make_nullable<std::string>("fabulous");
                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "fabulous");
                ++it;
                REQUIRE(it->has_value());
                CHECK_EQ(it->get(), "now");
            }
        }

        TEST_CASE_FIXTURE(string_array_fixture, "value_iterator")
        {
            SUBCASE("const")
            {
                SUBCASE("ordering")
                {
                    const layout_type array(std::move(m_arrow_proxy));
                    const auto values = array.values();
                    CHECK(values.begin() < values.end());
                }

                SUBCASE("equality")
                {
                    const layout_type array(std::move(m_arrow_proxy));
                    const auto values = array.values();
                    auto iter = values.begin();
                    CHECK_EQ(*iter, "upon");
                    CHECK_EQ(*(++iter), "a");
                    CHECK_EQ(*(++iter), "time");
                    CHECK_EQ(*(++iter), "I");
                    CHECK_EQ(*(++iter), "was");
                    CHECK_EQ(*(++iter), "writing");
                    CHECK_EQ(*(++iter), "clean");
                    CHECK_EQ(*(++iter), "code");
                    CHECK_EQ(*(++iter), "now");
                    CHECK_EQ(++iter, values.end());
                }
            }

            SUBCASE("non const")
            {
                SUBCASE("ordering")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    auto values = array.values();
                    CHECK(values.begin() < values.end());
                }

                SUBCASE("equality")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    auto values = array.values();
                    auto iter = values.begin();
                    CHECK_EQ(*iter, "upon");
                    CHECK_EQ(*(++iter), "a");
                    CHECK_EQ(*(++iter), "time");
                    CHECK_EQ(*(++iter), "I");
                    CHECK_EQ(*(++iter), "was");
                    CHECK_EQ(*(++iter), "writing");
                    CHECK_EQ(*(++iter), "clean");
                    CHECK_EQ(*(++iter), "code");
                    CHECK_EQ(*(++iter), "now");
                    CHECK_EQ(++iter, values.end());
                }
            }
        }

        TEST_CASE_FIXTURE(string_array_fixture, "resize")
        {
            SUBCASE("smaller")
            {
                layout_type array(std::move(m_arrow_proxy));
                REQUIRE_EQ(array.size(), m_length - m_offset);
                array.resize(3, make_nullable<std::string>("test"));
                REQUIRE_EQ(array.size(), 3);
                CHECK_EQ(array.value(0), "upon");
                CHECK_EQ(array.value(1), "a");
                CHECK_EQ(array.value(2), "time");
            }

            SUBCASE("bigger")
            {
                layout_type array(std::move(m_arrow_proxy));
                REQUIRE_EQ(array.size(), m_length - m_offset);
                array.resize(12, make_nullable<std::string>("test"));
                REQUIRE_EQ(array.size(), 12);
                CHECK_EQ(array.value(0), "upon");
                CHECK_EQ(array.value(1), "a");
                CHECK_EQ(array.value(2), "time");
                CHECK_EQ(array.value(3), "I");
                CHECK_EQ(array.value(4), "was");
                CHECK_EQ(array.value(5), "writing");
                CHECK_EQ(array.value(6), "clean");
                CHECK_EQ(array.value(7), "code");
                CHECK_EQ(array.value(8), "now");
                CHECK_EQ(array.value(9), "test");
                CHECK_EQ(array.value(10), "test");
                CHECK_EQ(array.value(11), "test");
            }
        }

        TEST_CASE_FIXTURE(string_array_fixture, "insert")
        {
            const std::string to_insert = "insert";

            SUBCASE("with pos and value")
            {
                SUBCASE("at the beginning")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin();
                    const auto iter = array.insert(pos, make_nullable(to_insert));
                    CHECK_EQ(iter, array.begin());
                    REQUIRE_EQ(array.size(), 10);
                    CHECK_EQ(array.value(0), to_insert);
                    CHECK_EQ(array.value(1), "upon");
                    CHECK_EQ(array.value(2), "a");
                    CHECK_EQ(array.value(3), "time");
                    CHECK_EQ(array.value(4), "I");
                    CHECK_EQ(array.value(5), "was");
                    CHECK_EQ(array.value(6), "writing");
                    CHECK_EQ(array.value(7), "clean");
                    CHECK_EQ(array.value(8), "code");
                }

                SUBCASE("in the middle")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin() + 3;
                    const auto iter = array.insert(pos, make_nullable(to_insert));
                    CHECK_EQ(iter, array.begin() + 3);
                    REQUIRE_EQ(array.size(), 10);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), to_insert);
                    CHECK_EQ(array.value(4), "I");
                    CHECK_EQ(array.value(5), "was");
                    CHECK_EQ(array.value(6), "writing");
                    CHECK_EQ(array.value(7), "clean");
                    CHECK_EQ(array.value(8), "code");
                }

                SUBCASE("at the end")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cend();
                    const auto iter = array.insert(pos, make_nullable(to_insert));
                    CHECK_EQ(iter, array.end() - 1);
                    REQUIRE_EQ(array.size(), 10);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), "I");
                    CHECK_EQ(array.value(4), "was");
                    CHECK_EQ(array.value(5), "writing");
                    CHECK_EQ(array.value(6), "clean");
                    CHECK_EQ(array.value(7), "code");
                    CHECK_EQ(array.value(8), "now");
                    CHECK_EQ(array.value(9), to_insert);
                }
            }

            SUBCASE("with pos, value and count")
            {
                SUBCASE("at the beginning")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin();
                    const auto iter = array.insert(pos, make_nullable(to_insert), 3);
                    CHECK_EQ(iter, array.begin());
                    REQUIRE_EQ(array.size(), 12);
                    CHECK_EQ(array.value(0), to_insert);
                    CHECK_EQ(array.value(1), to_insert);
                    CHECK_EQ(array.value(2), to_insert);
                    CHECK_EQ(array.value(3), "upon");
                    CHECK_EQ(array.value(4), "a");
                    CHECK_EQ(array.value(5), "time");
                    CHECK_EQ(array.value(6), "I");
                    CHECK_EQ(array.value(7), "was");
                    CHECK_EQ(array.value(8), "writing");
                    CHECK_EQ(array.value(9), "clean");
                    CHECK_EQ(array.value(10), "code");
                    CHECK_EQ(array.value(11), "now");
                }

                SUBCASE("in the middle")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin() + 3;
                    const auto iter = array.insert(pos, make_nullable(to_insert), 3);
                    CHECK_EQ(iter, array.begin() + 3);
                    REQUIRE_EQ(array.size(), 12);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), to_insert);
                    CHECK_EQ(array.value(4), to_insert);
                    CHECK_EQ(array.value(5), to_insert);
                    CHECK_EQ(array.value(6), "I");
                    CHECK_EQ(array.value(7), "was");
                    CHECK_EQ(array.value(8), "writing");
                    CHECK_EQ(array.value(9), "clean");
                    CHECK_EQ(array.value(10), "code");
                    CHECK_EQ(array.value(11), "now");
                }

                SUBCASE("at the end")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cend();
                    const auto iter = array.insert(pos, make_nullable(to_insert), 3);
                    REQUIRE_EQ(array.size(), 12);
                    CHECK_EQ(iter, array.end() - 3);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), "I");
                    CHECK_EQ(array.value(4), "was");
                    CHECK_EQ(array.value(5), "writing");
                    CHECK_EQ(array.value(6), "clean");
                    CHECK_EQ(array.value(7), "code");
                    CHECK_EQ(array.value(8), "now");
                    CHECK_EQ(array.value(9), to_insert);
                    CHECK_EQ(array.value(10), to_insert);
                    CHECK_EQ(array.value(11), to_insert);
                }
            }

            SUBCASE("with pos and range")
            {
                const std::array<nullable<std::string>, 3> new_values{"!", "once", "!"};

                SUBCASE("at the beginning")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin();
                    const auto iter = array.insert(pos, new_values);
                    CHECK_EQ(iter, array.begin());
                    REQUIRE_EQ(array.size(), 12);
                    CHECK_EQ(array.value(0), "!");
                    CHECK_EQ(array.value(1), "once");
                    CHECK_EQ(array.value(2), "!");
                    CHECK_EQ(array.value(3), "upon");
                    CHECK_EQ(array.value(4), "a");
                    CHECK_EQ(array.value(5), "time");
                    CHECK_EQ(array.value(6), "I");
                    CHECK_EQ(array.value(7), "was");
                    CHECK_EQ(array.value(8), "writing");
                    CHECK_EQ(array.value(9), "clean");
                    CHECK_EQ(array.value(10), "code");
                    CHECK_EQ(array.value(11), "now");
                }

                SUBCASE("in the middle")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin() + 3;
                    const auto iter = array.insert(pos, new_values);
                    CHECK_EQ(iter, array.begin() + 3);
                    REQUIRE_EQ(array.size(), 12);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), "!");
                    CHECK_EQ(array.value(4), "once");
                    CHECK_EQ(array.value(5), "!");
                    CHECK_EQ(array.value(6), "I");
                    CHECK_EQ(array.value(7), "was");
                    CHECK_EQ(array.value(8), "writing");
                    CHECK_EQ(array.value(9), "clean");
                    CHECK_EQ(array.value(10), "code");
                }

                SUBCASE("at the end")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cend();
                    const auto iter = array.insert(pos, new_values);
                    REQUIRE_EQ(array.size(), 12);
                    CHECK_EQ(iter, array.end() - 3);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), "I");
                    CHECK_EQ(array.value(4), "was");
                    CHECK_EQ(array.value(5), "writing");
                    CHECK_EQ(array.value(6), "clean");
                    CHECK_EQ(array.value(7), "code");
                    CHECK_EQ(array.value(8), "now");
                    CHECK_EQ(array.value(9), "!");
                    CHECK_EQ(array.value(10), "once");
                    CHECK_EQ(array.value(11), "!");
                }
            }

            SUBCASE("with pos and initializer list")
            {
                SUBCASE("at the beginning")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin();
                    const auto iter = array.insert(
                        pos,
                        {make_nullable<std::string>("!"),
                         make_nullable<std::string>("once"),
                         make_nullable<std::string>("!")}
                    );
                    CHECK_EQ(iter, array.begin());
                    REQUIRE_EQ(array.size(), 12);
                    CHECK_EQ(array.value(0), "!");
                    CHECK_EQ(array.value(1), "once");
                    CHECK_EQ(array.value(2), "!");
                    CHECK_EQ(array.value(3), "upon");
                    CHECK_EQ(array.value(4), "a");
                    CHECK_EQ(array.value(5), "time");
                    CHECK_EQ(array.value(6), "I");
                    CHECK_EQ(array.value(7), "was");
                    CHECK_EQ(array.value(8), "writing");
                    CHECK_EQ(array.value(9), "clean");
                    CHECK_EQ(array.value(10), "code");
                }

                SUBCASE("in the middle")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin() + 3;
                    const auto iter = array.insert(
                        pos,
                        {make_nullable<std::string>("!"),
                         make_nullable<std::string>("once"),
                         make_nullable<std::string>("!")}
                    );
                    CHECK_EQ(iter, array.begin() + 3);
                    REQUIRE_EQ(array.size(), 12);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), "!");
                    CHECK_EQ(array.value(4), "once");
                    CHECK_EQ(array.value(5), "!");
                    CHECK_EQ(array.value(6), "I");
                    CHECK_EQ(array.value(7), "was");
                    CHECK_EQ(array.value(8), "writing");
                    CHECK_EQ(array.value(9), "clean");
                    CHECK_EQ(array.value(10), "code");
                }

                SUBCASE("at the end")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cend();
                    const auto iter = array.insert(
                        pos,
                        {make_nullable<std::string>("!"),
                         make_nullable<std::string>("once"),
                         make_nullable<std::string>("!")}
                    );
                    REQUIRE_EQ(array.size(), 12);
                    CHECK_EQ(iter, array.end() - 3);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), "I");
                    CHECK_EQ(array.value(4), "was");
                    CHECK_EQ(array.value(5), "writing");
                    CHECK_EQ(array.value(6), "clean");
                    CHECK_EQ(array.value(7), "code");
                    CHECK_EQ(array.value(8), "now");
                    CHECK_EQ(array.value(9), "!");
                    CHECK_EQ(array.value(10), "once");
                    CHECK_EQ(array.value(11), "!");
                }
            }
        }

        TEST_CASE_FIXTURE(string_array_fixture, "erase")
        {
            SUBCASE("with pos")
            {
                SUBCASE("at the beginning")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin();
                    const auto iter = array.erase(pos);
                    CHECK_EQ(iter, array.begin());
                    REQUIRE_EQ(array.size(), 8);
                    CHECK_EQ(array.value(0), "a");
                    CHECK_EQ(array.value(1), "time");
                    CHECK_EQ(array.value(2), "I");
                    CHECK_EQ(array.value(3), "was");
                    CHECK_EQ(array.value(4), "writing");
                    CHECK_EQ(array.value(5), "clean");
                    CHECK_EQ(array.value(6), "code");
                    CHECK_EQ(array.value(7), "now");
                }

                SUBCASE("in the middle")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin() + 2;
                    const auto iter = array.erase(pos);
                    CHECK_EQ(iter, array.begin() + 2);
                    REQUIRE_EQ(array.size(), 8);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "I");
                    CHECK_EQ(array.value(3), "was");
                    CHECK_EQ(array.value(4), "writing");
                    CHECK_EQ(array.value(5), "clean");
                    CHECK_EQ(array.value(6), "code");
                    CHECK_EQ(array.value(7), "now");
                }

                SUBCASE("at the end")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = std::prev(array.cend());
                    const auto iter = array.erase(pos);
                    CHECK_EQ(iter, array.end());
                    REQUIRE_EQ(array.size(), 8);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), "I");
                    CHECK_EQ(array.value(4), "was");
                    CHECK_EQ(array.value(5), "writing");
                    CHECK_EQ(array.value(6), "clean");
                    CHECK_EQ(array.value(7), "code");
                }
            }

            SUBCASE("with iterators")
            {
                SUBCASE("at the beginning")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin();
                    const auto iter = array.erase(pos, pos + 3);
                    CHECK_EQ(iter, array.begin());
                    REQUIRE_EQ(array.size(), 6);
                    CHECK_EQ(array.value(0), "I");
                    CHECK_EQ(array.value(1), "was");
                    CHECK_EQ(array.value(2), "writing");
                    CHECK_EQ(array.value(3), "clean");
                    CHECK_EQ(array.value(4), "code");
                    CHECK_EQ(array.value(5), "now");
                }

                SUBCASE("in the middle")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = array.cbegin() + 3;
                    const auto iter = array.erase(pos, pos + 3);
                    CHECK_EQ(iter, array.begin() + 3);
                    REQUIRE_EQ(array.size(), 6);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), "clean");
                    CHECK_EQ(array.value(4), "code");
                    CHECK_EQ(array.value(5), "now");
                }

                SUBCASE("at the end")
                {
                    layout_type array(std::move(m_arrow_proxy));
                    CHECK_EQ(array.size(), 9);
                    const auto pos = std::prev(array.cend());
                    const auto iter = array.erase(pos, array.cend());
                    CHECK_EQ(iter, array.end());
                    REQUIRE_EQ(array.size(), 8);
                    CHECK_EQ(array.value(0), "upon");
                    CHECK_EQ(array.value(1), "a");
                    CHECK_EQ(array.value(2), "time");
                    CHECK_EQ(array.value(3), "I");
                    CHECK_EQ(array.value(4), "was");
                    CHECK_EQ(array.value(5), "writing");
                    CHECK_EQ(array.value(6), "clean");
                    CHECK_EQ(array.value(7), "code");
                }
            }
        }

        TEST_CASE_FIXTURE(string_array_fixture, "push_back")
        {
            layout_type array(std::move(m_arrow_proxy));
            CHECK_EQ(array.size(), 9);
            array.push_back(make_nullable<std::string>("!"));
            REQUIRE_EQ(array.size(), 10);
            CHECK_EQ(array.value(0), "upon");
            CHECK_EQ(array.value(1), "a");
            CHECK_EQ(array.value(2), "time");
            CHECK_EQ(array.value(3), "I");
            CHECK_EQ(array.value(4), "was");
            CHECK_EQ(array.value(5), "writing");
            CHECK_EQ(array.value(6), "clean");
            CHECK_EQ(array.value(7), "code");
            CHECK_EQ(array.value(8), "now");
            CHECK_EQ(array.value(9), "!");
        }

        TEST_CASE_FIXTURE(string_array_fixture, "pop_back")
        {
            layout_type array(std::move(m_arrow_proxy));
            CHECK_EQ(array.size(), 9);
            array.pop_back();
            REQUIRE_EQ(array.size(), 8);
            CHECK_EQ(array.value(0), "upon");
            CHECK_EQ(array.value(1), "a");
            CHECK_EQ(array.value(2), "time");
            CHECK_EQ(array.value(3), "I");
            CHECK_EQ(array.value(4), "was");
            CHECK_EQ(array.value(5), "writing");
            CHECK_EQ(array.value(6), "clean");
            CHECK_EQ(array.value(7), "code");
        }

        TEST_CASE_FIXTURE(string_array_fixture, "zero_null_values")
        {
            layout_type array(std::move(m_arrow_proxy));
            CHECK_EQ(array.size(), 9);
            array.zero_null_values();
            for (size_t i = 0; i < array.size(); ++i)
            {
                if (!array[i].has_value())
                {
                    CHECK_EQ(array[i].get(), std::string{});
                }
            }
        }

#if defined(__cpp_lib_format)
        TEST_CASE_FIXTURE(string_array_fixture, "formatting")
        {
            const layout_type array(std::move(m_arrow_proxy));
            const std::string formatted = std::format("{}", array);
            constexpr std::string_view
                expected = "String [name=test | size=9] <upon, null, time, I, null, writing, clean, code, now>";
            CHECK_EQ(formatted, expected);
        }
#endif
    }
}
