/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <app/util/config.h>
#include <static-supported-modes-manager.h>

using namespace std;
using namespace chip;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ModeSelect;
using chip::Protocols::InteractionModel::Status;

using ModeOptionStructType = Structs::ModeOptionStruct::Type;
using SemanticTag          = Structs::SemanticTagStruct::Type;
template <typename T>
using List               = app::DataModel::List<T>;
using storage_value_type = const ModeOptionStructType;
namespace {
Structs::ModeOptionStruct::Type buildModeOptionStruct(const char * label, uint8_t mode,
                                                      const List<const SemanticTag> & semanticTags)
{
    Structs::ModeOptionStruct::Type option;
    option.label        = CharSpan::fromCharString(label);
    option.mode         = mode;
    option.semanticTags = semanticTags;
    return option;
}
} // namespace

constexpr SemanticTag semanticTagsBlack[]     = { { .value = 0 } };
constexpr SemanticTag semanticTagsCappucino[] = { { .value = 0 } };
constexpr SemanticTag semanticTagsEspresso[]  = { { .value = 0 } };

// TODO: Configure your options for each endpoint
storage_value_type StaticSupportedModesManager::coffeeOptions[] = {
    buildModeOptionStruct("Black", 0, List<const SemanticTag>(semanticTagsBlack)),
    buildModeOptionStruct("Cappuccino", 4, List<const SemanticTag>(semanticTagsCappucino)),
    buildModeOptionStruct("Espresso", 7, List<const SemanticTag>(semanticTagsEspresso))
};
const StaticSupportedModesManager::EndpointSpanPair
    StaticSupportedModesManager::supportedOptionsByEndpoints[MATTER_DM_MODE_SELECT_CLUSTER_SERVER_ENDPOINT_COUNT] = {
        EndpointSpanPair(1, Span<storage_value_type>(StaticSupportedModesManager::coffeeOptions)) // Options for Endpoint 1
    };

const StaticSupportedModesManager StaticSupportedModesManager::instance = StaticSupportedModesManager();

SupportedModesManager::ModeOptionsProvider StaticSupportedModesManager::getModeOptionsProvider(EndpointId endpointId) const
{
    for (auto & endpointSpanPair : supportedOptionsByEndpoints)
    {
        if (endpointSpanPair.mEndpointId == endpointId)
        {
            return ModeOptionsProvider(endpointSpanPair.mSpan.data(), endpointSpanPair.mSpan.end());
        }
    }
    return ModeOptionsProvider(nullptr, nullptr);
}

Status StaticSupportedModesManager::getModeOptionByMode(unsigned short endpointId, unsigned char mode,
                                                        const ModeOptionStructType ** dataPtr) const
{
    auto modeOptionsProvider = this->getModeOptionsProvider(endpointId);
    if (modeOptionsProvider.begin() == nullptr)
    {
        return Status::UnsupportedCluster;
    }
    auto * begin = this->getModeOptionsProvider(endpointId).begin();
    auto * end   = this->getModeOptionsProvider(endpointId).end();

    for (auto * it = begin; it != end; ++it)
    {
        auto & modeOption = *it;
        if (modeOption.mode == mode)
        {
            *dataPtr = &modeOption;
            return Status::Success;
        }
    }
    ChipLogProgress(Zcl, "Cannot find the mode %u", mode);
    return Status::InvalidCommand;
}

const ModeSelect::SupportedModesManager * ModeSelect::getSupportedModesManager()
{
    return &StaticSupportedModesManager::instance;
}
