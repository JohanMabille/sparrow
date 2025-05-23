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

#include <string>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <filesystem>

#include "sparrow/c_data_integration/c_data_integration.hpp"
#include "sparrow/c_interface.hpp"

#include "better_junit_reporter.hpp"
#include "doctest/doctest.h"


const std::filesystem::path json_files_path = JSON_FILES_PATH;

const std::vector<std::filesystem::path> json_to_test = {
    json_files_path / "primitive-empty.json",
    json_files_path / "primitive.json",
};

TEST_SUITE("c_data_integration")
{
    TEST_CASE("ExportSchemaFromJson")
    {
        for (const auto& json : json_to_test)
        {
            SUBCASE(json.filename().string().c_str())
            {
                ArrowSchema schema;
                const auto error = sparrow_CDataIntegration_ExportSchemaFromJson(json.string().c_str(), &schema);
                if (error != nullptr)
                {
                    CHECK_EQ(std::string_view(error), std::string_view());
                }
            }
        }
    }

    TEST_CASE("ImportSchemaAndCompareToJson")
    {
        for (const auto& json : json_to_test)
        {
            SUBCASE(json.filename().string().c_str())
            {
                ArrowSchema schema;
                auto error = sparrow_CDataIntegration_ExportSchemaFromJson(json.string().c_str(), &schema);
                if (error != nullptr)
                {
                    CHECK_EQ(std::string_view(error), std::string_view());
                }
                error = sparrow_CDataIntegration_ImportSchemaAndCompareToJson(json.string().c_str(), &schema);
                if (error != nullptr)
                {
                    CHECK_EQ(std::string_view(error), std::string_view());
                }
            }
        }
    }

    TEST_CASE("ExportBatchFromJson")
    {
        for (const auto& json : json_to_test)
        {
            SUBCASE(json.filename().string().c_str())
            {
                ArrowArray array;
                const auto error = sparrow_CDataIntegration_ExportBatchFromJson(json.string().c_str(), 0, &array);
                if (error != nullptr)
                {
                    CHECK_EQ(std::string_view(error), std::string_view());
                }
            }
        }
    }

    TEST_CASE("ImportBatchAndCompareToJson")
    {
        for (const auto& json : json_to_test)
        {
            SUBCASE(json.filename().string().c_str())
            {
                ArrowArray array;
                auto error = sparrow_CDataIntegration_ExportBatchFromJson(json.string().c_str(), 0, &array);
                if (error != nullptr)
                {
                    CHECK_EQ(std::string_view(error), std::string_view());
                }
                error = sparrow_CDataIntegration_ImportBatchAndCompareToJson(json.string().c_str(), 0, &array);
                if (error != nullptr)
                {
                    CHECK_EQ(std::string_view(error), std::string_view());
                }
            }
        }
    }
}
