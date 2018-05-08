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

#include <WCDB/BasicConfig.hpp>
#include <WCDB/CheckpointConfig.hpp>
#include <WCDB/CipherConfig.hpp>
#include <WCDB/MigrationConfig.hpp>
#include <WCDB/TokenizeConfig.hpp>
#include <WCDB/TraceConfig.hpp>

namespace WCDB {

static_assert((int) MigrationConfig::order > (int) TraceConfig::order, "");
static_assert((int) MigrationConfig::order > (int) BasicConfig::order, "");
static_assert((int) MigrationConfig::order > (int) CipherConfig::order, "");
static_assert((int) MigrationConfig::order > (int) TokenizeConfig::order, "");

std::shared_ptr<Config>
MigrationConfig::configWithSetting(MigrationSetting *setting)
{
    return std::shared_ptr<Config>(new MigrationConfig(setting));
}

const std::string &MigrationConfig::name()
{
    static const std::string s_name("migration");
    return s_name;
}

bool MigrationConfig::invoke(Handle *handle) const
{
    bool schemaChanged = false;
    if (!doAttachSchema(handle) || !doCreateView(handle, schemaChanged)) {
        return false;
    }
    return schemaChanged ? doAttachSchema(handle) : true;
}

bool MigrationConfig::doAttachSchema(Handle *handle) const
{
    SharedLockGuard lockGuard(m_setting->getSharedLock());
    auto attacheds = handle->getUnorderedAttachedSchemas();
    if (!attacheds.first) {
        return false;
    }

    auto &toDetachs = attacheds.second;
    for (const auto &schemas : m_setting->getSchemasForAttaching()) {
        auto iter = toDetachs.find(schemas.first);
        if (iter != toDetachs.end()) {
            //necessary
            toDetachs.erase(iter);
        } else {
            WCDB::StatementAttach statement = WCDB::StatementAttach()
                                                  .attach(schemas.second.first)
                                                  .as(schemas.first);
            if (!handle->execute(statement)) {
                return false;
            }
        }
    }

    const std::string &schemaPrefix = MigrationInfo::schemaPrefix();
    for (const auto &toDetach : toDetachs) {
        if (toDetach.compare(0, schemaPrefix.length(), schemaPrefix) != 0) {
            continue;
        }
        //detach unnecessary WCDBMigration_ schema
        WCDB::StatementDetach statement =
            WCDB::StatementDetach().detach(toDetach);
        if (!handle->execute(statement)) {
            return false;
        }
    }
    return true;
}

bool MigrationConfig::doCreateView(WCDB::Handle *handle,
                                   bool &schemaChanged) const
{
    schemaChanged = false;
    std::list<std::shared_ptr<MigrationInfo>> infos;
    {
        SharedLockGuard lockGuard(m_setting->getSharedLock());
        if (!handle->runTransaction([&infos, this](Handle *handle) -> bool {
                for (const auto &info : m_setting->getInfos()) {
                    auto pair =
                        handle->isTableExists(info.second->getSourceTable());
                    if (!pair.first) {
                        return false;
                    }
                    if (pair.second) {
                        //Create view
                        if (!handle->execute(
                                info.second
                                    ->getStatementForCreatingUnionedView())) {
                            return false;
                        }
                    } else {
                        infos.push_back(info.second);
                    }
                }
                return true;
            })) {
            return false;
        }
    }
    if (!infos.empty()) {
        LockGuard lockGuard(m_setting->getSharedLock());
        if (!handle->runTransaction([&infos](Handle *handle) {
                for (const auto &info : infos) {
                    if (!handle->execute(
                            info->getStatementForDroppingUnionedView())) {
                        return false;
                    }
                }
                return true;
            })) {
            return false;
        }
        for (const auto &info : infos) {
            schemaChanged =
                m_setting->markAsMigrated(info->targetTable) || schemaChanged;
        }
    }
    return true;
}

MigrationConfig::MigrationConfig(MigrationSetting *setting)
    : Config(MigrationConfig::name(), MigrationConfig::order)
    , m_setting(setting)
{
}

} //namespace WCDB