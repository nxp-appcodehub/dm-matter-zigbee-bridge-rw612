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

#include "ClustersCommandHandlers.h"

// On/Off : 6
void OnOffClusterCommandHandler::InvokeCommand(CommandHandlerInterface::HandlerContext& ctxt)
{
    using namespace chip::app::Clusters::OnOff;
    std::string cmd;

    switch (ctxt.mRequestPath.mCommandId) {
    case Commands::Off::Id: {
        Commands::Off::DecodableType data;
        cmd = "Off";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            BridgedOnOff(ctxt.mRequestPath.mEndpointId,0);
        }
    } break;
    case Commands::On::Id: {
        Commands::On::DecodableType data;
        cmd = "On";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            BridgedOnOff(ctxt.mRequestPath.mEndpointId,1);
        }
    } break;
    case Commands::Toggle::Id: {
        Commands::Toggle::DecodableType data;
        cmd = "Toggle";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            BridgedOnOff(ctxt.mRequestPath.mEndpointId,2);
        }
    } break;
    }

    if (!cmd.empty()) {
        ctxt.mCommandHandler.AddStatus(ctxt.mRequestPath, Protocols::InteractionModel::Status::Success);
    } else {
        ctxt.mCommandHandler.AddStatus(ctxt.mRequestPath, Protocols::InteractionModel::Status::UnsupportedCommand);
    }
    ctxt.SetCommandHandled();
}

// Level Control : 8
void LevelControlClusterCommandHandler::InvokeCommand(CommandHandlerInterface::HandlerContext& ctxt)
{
    using namespace chip::app::Clusters::LevelControl;

    std::string cmd;

    switch (ctxt.mRequestPath.mCommandId) {
    case Commands::MoveToLevel::Id: {
        Commands::MoveToLevel::DecodableType data;
        cmd = "MoveToLevel"; // "MoveToLevel";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            BridgedLevelControl(ctxt.mRequestPath.mEndpointId, data.level, data.transitionTime.Value());
        }
    } break;
    case Commands::Move::Id: {
        Commands::Move::DecodableType data;
        cmd = "Move"; // "Move";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
        }
    } break;
    case Commands::Step::Id: {
        Commands::Step::DecodableType data;
        cmd = "Step"; // "Step";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
        }
    } break;
    case Commands::Stop::Id: {
        Commands::Stop::DecodableType data;
        cmd = "Stop"; // "Stop";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
        }
    } break;
    case Commands::MoveToLevelWithOnOff::Id: {
        Commands::MoveToLevelWithOnOff::DecodableType data;
        cmd = "MoveToLevelWithOnOff"; // "MoveToLevelWithOnOff";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
        }
    } break;
    case Commands::MoveWithOnOff::Id: {
        Commands::MoveWithOnOff::DecodableType data;
        cmd = "MoveWithOnOff"; // "MoveWithOnOff";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
        }
    } break;
    case Commands::StepWithOnOff::Id: {
        Commands::StepWithOnOff::DecodableType data;
        cmd = "StepWithOnOff"; // "StepWithOnOff";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
        }
    } break;
    case Commands::StopWithOnOff::Id: {
        Commands::StopWithOnOff::DecodableType data;
        cmd = "StopWithOnOff"; // "StopWithOnOff";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
        }
    } break;
    }

    if (!cmd.empty()) {
        ctxt.mCommandHandler.AddStatus(ctxt.mRequestPath, Protocols::InteractionModel::Status::Success);
    } else {
        ctxt.mCommandHandler.AddStatus(ctxt.mRequestPath, Protocols::InteractionModel::Status::UnsupportedCommand);
    }
    ctxt.SetCommandHandled();
}

// Color Control : 768
void ColorControlClusterCommandHandler::InvokeCommand(CommandHandlerInterface::HandlerContext& ctxt)
{
    using namespace chip::app::Clusters::ColorControl;
    std::string cmd;

    switch (ctxt.mRequestPath.mCommandId) {
    case Commands::MoveToHue::Id: {
        Commands::MoveToHue::DecodableType data;
        cmd = "MoveToHue"; // "MoveToHue";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            PRINTF("\n ### Move to Hue : Hue=0x%x,Dir=%d,TransTime=%d,EP=%d", data.hue, data.direction,data. transitionTime, ctxt.mRequestPath.mEndpointId);
            BridgedMoveToHue(ctxt.mRequestPath.mEndpointId, data.hue, (uint8_t)(data.direction), data.transitionTime);
        }
    } break;
    case Commands::MoveHue::Id: {
        Commands::MoveHue::DecodableType data;
        cmd = "MoveHue"; // "MoveHue";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
           
        }
    } break;
    case Commands::StepHue::Id: {
        Commands::StepHue::DecodableType data;
        cmd = "StepHue"; // "StepHue";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::MoveToSaturation::Id: {
        Commands::MoveToSaturation::DecodableType data;
        cmd = "MoveToSaturation"; // "MoveToSaturation";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            PRINTF("\n ### Move to Saturation : Sat=0x%x,TransTime=%d,EP=%d\n",data.saturation, data.transitionTime, ctxt.mRequestPath.mEndpointId);
            BridgedMoveToSaturation(ctxt.mRequestPath.mEndpointId, data.saturation, data.transitionTime);
        }
    } break;
    case Commands::MoveSaturation::Id: {
        Commands::MoveSaturation::DecodableType data;
        cmd = "MoveSaturation"; // "MoveSaturation";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::StepSaturation::Id: {
        Commands::StepSaturation::DecodableType data;
        cmd = "StepSaturation"; // "StepSaturation";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::MoveToHueAndSaturation::Id: {
        Commands::MoveToHueAndSaturation::DecodableType data;
        cmd = "MoveToHueAndSaturation"; // "MoveToHueAndSaturation";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
           
        }
    } break;
    case Commands::MoveToColor::Id: {
        Commands::MoveToColor::DecodableType data;
        cmd = "MoveToColor"; // "MoveToColor";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            PRINTF("\n ### Move to Color : X=%d,Y=%d,TransTime=%d,EP=%d\n", data.colorX,data.colorY, data.transitionTime, ctxt.mRequestPath.mEndpointId);
            BridgedMoveToColor(ctxt.mRequestPath.mEndpointId, data.colorX, data.colorY, data.transitionTime);
        }
    } break;
    case Commands::MoveColor::Id: {
        Commands::MoveColor::DecodableType data;
        cmd = "MoveColor"; // "MoveColor";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::StepColor::Id: {
        Commands::StepColor::DecodableType data;
        cmd = "StepColor"; // "StepColor";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::MoveToColorTemperature::Id: {
        Commands::MoveToColorTemperature::DecodableType data;
        cmd = "MoveToColorTemperature"; // "MoveToColorTemperature";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            PRINTF("\n ### Move to Temperature : Temp=0x%x,TransTime=%d,EP=%d\n", data.colorTemperatureMireds, data.transitionTime, ctxt.mRequestPath.mEndpointId);
            BridgedMoveToColorTemperature(ctxt.mRequestPath.mEndpointId, data.colorTemperatureMireds, data.transitionTime);
        }
    } break;
    case Commands::EnhancedMoveToHue::Id: {
        Commands::EnhancedMoveToHue::DecodableType data;
        cmd = "EnhancedMoveToHue"; // "EnhancedMoveToHue";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::EnhancedMoveHue::Id: {
        Commands::EnhancedMoveHue::DecodableType data;
        cmd = "EnhancedMoveHue"; // "EnhancedMoveHue";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::EnhancedStepHue::Id: {
        Commands::EnhancedStepHue::DecodableType data;
        cmd = "EnhancedStepHue"; // "EnhancedStepHue";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::EnhancedMoveToHueAndSaturation::Id: {
        Commands::EnhancedMoveToHueAndSaturation::DecodableType data;
        cmd = "EnhancedMoveToHueAndSaturation"; // "EnhancedMoveToHueAndSaturation";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::ColorLoopSet::Id: {
        Commands::ColorLoopSet::DecodableType data;
        cmd = "ColorLoopSet"; // "ColorLoopSet";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::StopMoveStep::Id: {
        Commands::StopMoveStep::DecodableType data;
        cmd = "StopMoveStep"; // "StopMoveStep";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    case Commands::MoveColorTemperature::Id: {
        Commands::MoveColorTemperature::DecodableType data;
        cmd = "MoveColorTemperature"; // "MoveColorTemperature";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
           
        }
    } break;
    case Commands::StepColorTemperature::Id: {
        Commands::StepColorTemperature::DecodableType data;
        cmd = "StepColorTemperature"; // "StepColorTemperature";
        if (DataModel::Decode(ctxt.GetReader(), data) == CHIP_NO_ERROR) {
            
        }
    } break;
    }

    if (!cmd.empty()) {
        ctxt.mCommandHandler.AddStatus(ctxt.mRequestPath, Protocols::InteractionModel::Status::Success);
    } else {
        ctxt.mCommandHandler.AddStatus(ctxt.mRequestPath, Protocols::InteractionModel::Status::UnsupportedCommand);
    }
    ctxt.SetCommandHandled();
}