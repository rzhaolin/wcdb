/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef Path_hpp
#define Path_hpp

#include <string>

#pragma GCC visibility push(hidden)

namespace WCDB {

namespace Path {

std::string addExtention(const std::string &base, const std::string &extention);
std::string addComponent(const std::string &base, const std::string &component);
std::string getFileName(const std::string &base);
std::string getDirectoryName(const std::string &base);

#if __cplusplus > 201402L
#warning TODO \
std::filesystem::path is available since C++17
#endif
std::string normalize(const std::string &path);

} //namespace Path

} //namespace WCDB

#pragma GCC visibility pop

#endif /* Path_hpp */