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

#include <chrono>

namespace sparrow::chrono
{
    using days = std::chrono::duration<int32_t, std::ratio<86400>>;      // 1 day = 86400 seconds
    using months = std::chrono::duration<int32_t, std::ratio<2629746>>;  // 1 month = 2629746 seconds
}
