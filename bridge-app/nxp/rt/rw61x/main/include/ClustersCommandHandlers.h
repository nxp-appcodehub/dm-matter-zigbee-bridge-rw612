/*
 *    Copyright (c) 2025 Project CHIP Authors
 *    All rights reserved.
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

#pragma once

#include <app/CommandHandlerInterfaceRegistry.h>
#include "CHIPProjectAppConfig.h"

#include <app/app-platform/ContentApp.h>
#include <app/app-platform/ContentAppPlatform.h>
#include <protocols/interaction_model/StatusCode.h>
#include <app-common/zap-generated/attribute-type.h>

#include "BridgeMgr.h"
#include "zcb.h"

using namespace chip;
using namespace chip::app;
class OnOffClusterCommandHandler : public chip::app::CommandHandlerInterface {
public:
    OnOffClusterCommandHandler(chip::ClusterId id) : chip::app::CommandHandlerInterface(chip::Optional<chip::EndpointId>::Missing(), id)
    {
        chip::app::CommandHandlerInterfaceRegistry::Instance().RegisterCommandHandler(this);
    }
    void InvokeCommand(chip::app::CommandHandlerInterface::HandlerContext& HandlerContext) override;
};

class LevelControlClusterCommandHandler : public chip::app::CommandHandlerInterface {
public:
    LevelControlClusterCommandHandler(chip::ClusterId id) : chip::app::CommandHandlerInterface(chip::Optional<chip::EndpointId>::Missing(), id)
    {
        chip::app::CommandHandlerInterfaceRegistry::Instance().RegisterCommandHandler(this);
    }
    void InvokeCommand(chip::app::CommandHandlerInterface::HandlerContext& HandlerContext) override;
};

class ColorControlClusterCommandHandler : public chip::app::CommandHandlerInterface {
public:
    ColorControlClusterCommandHandler(chip::ClusterId id) : chip::app::CommandHandlerInterface(chip::Optional<chip::EndpointId>::Missing(), id)
    {
        chip::app::CommandHandlerInterfaceRegistry::Instance().RegisterCommandHandler(this);
    }
    void InvokeCommand(chip::app::CommandHandlerInterface::HandlerContext& HandlerContext) override;
};