//
// Plugin Oak Camera
//

#include "oak_plugin.hpp"
#include "imgui.h"
#include "imgui_instance_helper.hpp"

using namespace DSPatch;
using namespace DSPatchables;

int32_t global_inst_counter = 0;

namespace DSPatch::DSPatchables::internal
{
class OakCamera
{
};
}  // namespace DSPatch

OakCamera::OakCamera()
    : Component( ProcessOrder::InOrder )
    , p( new internal::OakCamera() )
{
    // Name and Category
    SetComponentName_("Oak_Camera");
    SetComponentCategory_(Category::Category_Source);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInstanceCount(global_inst_counter);
    global_inst_counter++;

    // 0 inputs
    SetInputCount_( 0 );

    // 3 outputs
    SetOutputCount_( 3, {"rgb", "depth", "metadata"}, {IoType::Io_Type_CvMat, IoType::Io_Type_CvMat, IoType::Io_Type_JSON} );

    // Skip initial instance which is for plugin adding/checking
    if (global_inst_counter >= 2) {
        camera_ = std::make_unique<oak_camera>();
    }

    // Defaults
    selected_camera_idx_ = 0;
    color_cfg_idx_ = 2;
    color_fps_idx_ = 1;
    depth_cfg_idx_ = 2;
    depth_fps_idx_ = 2;
    color_fps_ = 30;
    depth_fps_ = 30;
    enable_color_ = false;
    enable_depth_ = false;

    // Enable
    SetEnabled(true);
}

void OakCamera::Process_( SignalBus const& inputs, SignalBus& outputs )
{
    std::lock_guard<std::mutex> lck(io_mutex_);
    camera_->ProcessStreams();
    if (!camera_->IsReconfiguring()) {
        if (!camera_->GetFrame(dai::CameraBoardSocket::RGB).empty()) {
            outputs.SetValue(0, camera_->GetFrame(dai::CameraBoardSocket::RGB));
        }
        if (!camera_->GetFrame(dai::CameraBoardSocket::AUTO).empty()) {
            outputs.SetValue(1, camera_->GetFrame(dai::CameraBoardSocket::AUTO));
        }
        if (!camera_->GetMetaData().empty())
            outputs.SetValue(2, camera_->GetMetaData());
    }
}

bool OakCamera::HasGui(int interface)
{
    // This is where you tell the system if your node has any of the following interfaces: Main, Control or Other
    if (interface == (int)FlowCV::GuiInterfaceType_Controls) {
        return true;
    }

    return false;
}

void OakCamera::UpdateGui(void *context, int interface)
{
    auto *imCurContext = (ImGuiContext *)context;
    ImGui::SetCurrentContext(imCurContext);

    if (interface == (int)FlowCV::GuiInterfaceType_Controls) {
        if (ImGui::Button(CreateControlString("Refresh Oak-D List", GetInstanceName()).c_str())) {
            camera_->RefreshDeviceList();
        }
        ImGui::Separator();
        auto cam_list = camera_->GetDeviceList();
        ImGui::SetNextItemWidth(150);
        if (ImGui::Combo(CreateControlString("Oak Cameras", GetInstanceName()).c_str(), &selected_camera_idx_, [](void* data, int idx, const char** out_text) {
            *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str();
            return true;
        }, (void*)&cam_list, (int)cam_list.size())) {
            enable_color_ = false;
            enable_depth_ = false;
            camera_->InitCamera(selected_camera_idx_);
        }
        ImGui::Separator();
        if (selected_camera_idx_ > 0 && camera_->IsInit()) {
            //
            // Common Section
            //
            ImGui::Text("Camera: %s", camera_->GetDeviceName(selected_camera_idx_).c_str());

            //
            // Color Section
            //
            if (camera_->HasColor()) {
                auto color_cfg_list = camera_->GetStreamConfigList(dai::CameraBoardSocket::RGB);
                if (ImGui::Checkbox(CreateControlString("Enable Color Sensor", GetInstanceName()).c_str(), &enable_color_)) {
                    color_cfg_list->at(color_cfg_idx_).fps_idx = color_fps_idx_;
                    if (enable_color_)
                        camera_->EnableStream(color_cfg_list->at(color_cfg_idx_));
                    else
                        camera_->DisableStream(dai::CameraBoardSocket::RGB);
                }
                ImGui::SetNextItemWidth(100);
                if (ImGui::Combo(CreateControlString("Color Resolution", GetInstanceName()).c_str(), &color_cfg_idx_, [](void *data, int idx, const char **out_text) {
                    *out_text = ((const std::vector<StreamConfig> *) data)->at(idx).str_resolution.c_str();
                    return true;
                }, (void *) color_cfg_list, (int) color_cfg_list->size())) {
                    for (int i = 0; i < color_cfg_list->at(color_cfg_idx_).fps_list.size(); i++) {
                        if (color_cfg_list->at(color_cfg_idx_).fps_list.at(i) == color_fps_) {
                            color_fps_idx_ = i;
                            break;
                        }
                    }
                    color_cfg_list->at(color_cfg_idx_).fps_idx = color_fps_idx_;
                    if (enable_color_) {
                        camera_->EnableStream(color_cfg_list->at(color_cfg_idx_));
                    }
                }
                ImGui::SetNextItemWidth(100);
                std::vector<std::string> cFpsList;
                for (int i = 0; i < color_cfg_list->at(color_cfg_idx_).fps_list.size(); i++) {
                    if (color_cfg_list->at(color_cfg_idx_).fps_list.at(i) == color_fps_)
                        color_fps_idx_ = i;
                    std::string fps = std::to_string(color_cfg_list->at(color_cfg_idx_).fps_list.at(i));
                    cFpsList.emplace_back(fps);
                }
                ImGui::SetNextItemWidth(100);
                if (ImGui::Combo(CreateControlString("Color FPS", GetInstanceName()).c_str(), &color_fps_idx_, [](void *data, int idx, const char **out_text) {
                    *out_text = ((const std::vector<std::string> *) data)->at(idx).c_str();
                    return true;
                }, (void *) &cFpsList, (int) cFpsList.size())) {
                    color_cfg_list->at(color_cfg_idx_).fps_idx = color_fps_idx_;
                    color_fps_ = color_cfg_list->at(color_cfg_idx_).fps_list.at(color_fps_idx_);
                    if (enable_color_) {
                        camera_->EnableStream(color_cfg_list->at(color_cfg_idx_));
                    }
                }
                if (enable_color_) {
                    if (ImGui::TreeNode("Color Controls")) {
                        auto color_props = camera_->GetPropertyList(dai::CameraBoardSocket::RGB);
                        if (ImGui::Button(CreateControlString("Restore Color Defaults", GetInstanceName()).c_str())) {
                            camera_->ResetProperties(dai::CameraBoardSocket::RGB);
                        }
                        ImGui::Separator();
                        for (auto &prop : *color_props) {
                            if (prop.second.range.min == 0 && prop.second.range.max == 1) { // Checkbox Control
                                bool tmpVal = (bool)prop.second.value;
                                ImGui::SetNextItemWidth(100);
                                if (ImGui::Checkbox(CreateControlString(prop.first.c_str(), GetInstanceName()).c_str(), &tmpVal)) {
                                    prop.second.value = (int)tmpVal;
                                    camera_->SetProperty(dai::CameraBoardSocket::RGB, prop.first);
                                }
                            }
                            else if (prop.second.is_list) { // Combo List
                                ImGui::SetNextItemWidth(150);
                                if (ImGui::Combo(CreateControlString(prop.first.c_str(), GetInstanceName()).c_str(),
                                                 &prop.second.value, [](void* data, int idx, const char** out_text) {
                                        *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str();
                                        return true;
                                    }, (void*)&prop.second.opt_list, (int)prop.second.opt_list.size())) {
                                    camera_->SetProperty(dai::CameraBoardSocket::RGB, prop.first);
                                }
                            }
                            else { // Int Drag Control
                                ImGui::SetNextItemWidth(100);
                                if (ImGui::DragInt(CreateControlString(prop.first.c_str(), GetInstanceName()).c_str(),
                                                   &prop.second.value, prop.second.range.step, prop.second.range.min, prop.second.range.max)) {
                                    if (prop.second.value < prop.second.range.min)
                                        prop.second.value = prop.second.range.min;
                                    else if (prop.second.value > prop.second.range.max)
                                        prop.second.value = prop.second.range.max;
                                    camera_->SetProperty(dai::CameraBoardSocket::RGB, prop.first);
                                }
                            }
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::Separator();
            }
            //
            // Depth Section
            //
            if (camera_->HasDepth()) {
                auto depth_cfg_list = camera_->GetStreamConfigList(dai::CameraBoardSocket::AUTO);
                if (ImGui::Checkbox(CreateControlString("Enable Depth Sensor", GetInstanceName()).c_str(), &enable_depth_)) {
                    depth_cfg_list->at(depth_cfg_idx_).fps_idx = depth_fps_idx_;
                    if (enable_depth_)
                        camera_->EnableStream(depth_cfg_list->at(depth_cfg_idx_));
                    else
                        camera_->DisableStream(dai::CameraBoardSocket::AUTO);
                }
                ImGui::SetNextItemWidth(100);
                if (ImGui::Combo(CreateControlString("Depth Resolution", GetInstanceName()).c_str(), &depth_cfg_idx_, [](void* data, int idx, const char** out_text) {
                    *out_text = ((const std::vector<StreamConfig>*)data)->at(idx).str_resolution.c_str();
                    return true;
                }, (void*)depth_cfg_list, (int)depth_cfg_list->size())) {
                    for (int i = 0; i < depth_cfg_list->at(depth_cfg_idx_).fps_list.size(); i++) {
                        if (depth_cfg_list->at(depth_cfg_idx_).fps_list.at(i) == depth_fps_) {
                            depth_fps_idx_ = i;
                            break;
                        }
                    }
                    depth_cfg_list->at(depth_cfg_idx_).fps_idx = depth_fps_idx_;
                    if (enable_depth_) {
                        camera_->EnableStream(depth_cfg_list->at(depth_cfg_idx_));
                    }
                }
                ImGui::SetNextItemWidth(100);
                std::vector<std::string> dFpsList;
                for (int i = 0; i < depth_cfg_list->at(depth_cfg_idx_).fps_list.size(); i++) {
                    if (depth_cfg_list->at(depth_cfg_idx_).fps_list.at(i) == depth_fps_)
                        depth_fps_idx_ = i;
                    std::string fps = std::to_string(depth_cfg_list->at(depth_cfg_idx_).fps_list.at(i));
                    dFpsList.emplace_back(fps);
                }
                ImGui::SetNextItemWidth(100);
                if (ImGui::Combo(CreateControlString("Depth FPS", GetInstanceName()).c_str(), &depth_fps_idx_, [](void* data, int idx, const char** out_text) {
                    *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str();
                    return true;
                }, (void*)&dFpsList, (int)dFpsList.size())) {
                    depth_cfg_list->at(depth_cfg_idx_).fps_idx = depth_fps_idx_;
                    depth_fps_ = depth_cfg_list->at(depth_cfg_idx_).fps_list.at(depth_fps_idx_);
                    if (enable_depth_) {
                        camera_->EnableStream(depth_cfg_list->at(depth_cfg_idx_));
                    }
                }
                if (enable_depth_) {
                    if (ImGui::TreeNode("Depth Controls")) {
                        auto depth_props = camera_->GetPropertyList(dai::CameraBoardSocket::AUTO);
                        for (auto &prop : *depth_props) {
                            if (prop.second.range.min == 0 && prop.second.range.max == 1) { // Checkbox Control
                                bool tmpVal = (bool)prop.second.value;
                                ImGui::SetNextItemWidth(100);
                                if (ImGui::Checkbox(CreateControlString(prop.first.c_str(), GetInstanceName()).c_str(), &tmpVal)) {
                                    prop.second.value = (int)tmpVal;
                                    camera_->SetProperty(dai::CameraBoardSocket::AUTO, prop.first);
                                }
                            }
                            else if (prop.second.is_list) { // Combo List
                                ImGui::SetNextItemWidth(150);
                                if (ImGui::Combo(CreateControlString(prop.first.c_str(), GetInstanceName()).c_str(),
                                                 &prop.second.value, [](void* data, int idx, const char** out_text) {
                                        *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str();
                                        return true;
                                    }, (void*)&prop.second.opt_list, (int)prop.second.opt_list.size())) {
                                    if (prop.first == "Preset") {
                                        camera_->EnableStream(depth_cfg_list->at(depth_cfg_idx_));
                                    }
                                    else {
                                        camera_->SetProperty(dai::CameraBoardSocket::AUTO, prop.first);
                                    }
                                }
                            }
                            else { // Int Drag Control
                                ImGui::SetNextItemWidth(100);
                                if (ImGui::DragInt(CreateControlString(prop.first.c_str(), GetInstanceName()).c_str(),
                                                   &prop.second.value, prop.second.range.step, prop.second.range.min, prop.second.range.max)) {
                                    if (prop.second.value < prop.second.range.min)
                                        prop.second.value = prop.second.range.min;
                                    else if (prop.second.value > prop.second.range.max)
                                        prop.second.value = prop.second.range.max;
                                    camera_->SetProperty(dai::CameraBoardSocket::AUTO, prop.first);
                                }
                            }
                        }
                        ImGui::TreePop();
                    }
                }
            }
        }
    }
}

std::string OakCamera::GetState()
{
    using namespace nlohmann;

    json state;

    if (selected_camera_idx_ > 0 && camera_->IsInit()) {
        state["cam_idx"] = selected_camera_idx_;
        state["oak_serial"] = camera_->GetDeviceSerial(selected_camera_idx_);
        state["color_enabled"] = enable_color_;
        if (enable_color_) {
            auto color_cfg_list = camera_->GetStreamConfigList(dai::CameraBoardSocket::RGB);
            auto color_props = camera_->GetPropertyList(dai::CameraBoardSocket::RGB);
            state["color_fps_idx"] = color_cfg_list->at(color_cfg_idx_).fps_idx;
            state["color_fps"] = color_cfg_list->at(color_cfg_idx_).fps_list.at(color_cfg_list->at(color_cfg_idx_).fps_idx);
            state["color_res_idx"] = color_cfg_idx_;
            nlohmann::json color_controls;
            for(const auto &prop : *color_props) {
                color_controls[prop.first] = prop.second.value;
            }
            state["color_controls"] = color_controls;
        }
        state["depth_enabled"] = enable_depth_;
        if (enable_depth_) {
            auto depth_cfg_list = camera_->GetStreamConfigList(dai::CameraBoardSocket::AUTO);
            auto depth_props = camera_->GetPropertyList(dai::CameraBoardSocket::AUTO);
            state["depth_fps_idx"] = depth_cfg_list->at(depth_cfg_idx_).fps_idx;
            state["depth_fps"] = depth_cfg_list->at(depth_cfg_idx_).fps_list.at(depth_cfg_list->at(depth_cfg_idx_).fps_idx);
            state["depth_res_idx"] = depth_cfg_idx_;
            nlohmann::json depth_controls;
            for(const auto &prop : *depth_props) {
                depth_controls[prop.first] = prop.second.value;
            }
            state["depth_controls"] = depth_controls;
            nlohmann::json depth_filtering;
        }
    }

    std::string stateSerialized = state.dump(4);

    return stateSerialized;
}

void OakCamera::SetState(std::string &&json_serialized)
{
    using namespace nlohmann;

    json state = json::parse(json_serialized);

    std::lock_guard<std::mutex> lck(io_mutex_);

    if (state.contains("cam_idx"))
        selected_camera_idx_ = state["cam_idx"].get<int>();
    if (selected_camera_idx_ > 0) {
        camera_->RefreshDeviceList();
        bool cam_exists = false;
        std::string saved_serial;
        if (state.contains("oak_serial"))
            saved_serial = state["oak_serial"].get<std::string>();

        if (!saved_serial.empty()) {
            std::string idx_serial = camera_->GetDeviceSerial(selected_camera_idx_);
            if (idx_serial != saved_serial) {
                // Attempt to find matching serial device
                for (int i = 0; i < camera_->GetDeviceCount(); i++) {
                    std::string check = camera_->GetDeviceSerial(i);
                    if (check == saved_serial) {
                        selected_camera_idx_ = i;
                        cam_exists = true;
                        break;
                    }
                }
            } else {
                cam_exists = true;
            }
            if (cam_exists) {
                camera_->InitCamera(selected_camera_idx_, true);
                if (state.contains("color_enabled"))
                    enable_color_ = state["color_enabled"].get<bool>();
                if (enable_color_) {
                    auto color_cfg_list = camera_->GetStreamConfigList(dai::CameraBoardSocket::RGB);
                    auto color_props = camera_->GetPropertyList(dai::CameraBoardSocket::RGB);
                    if (state.contains("color_res_idx"))
                        color_cfg_idx_ = state["color_res_idx"].get<int>();
                    if (state.contains("color_fps_idx"))
                        color_fps_idx_ = state["color_fps_idx"].get<int>();
                    if (state.contains("color_fps"))
                        color_fps_ = state["color_fps"].get<int>();
                    color_cfg_list->at(color_cfg_idx_).fps_idx = color_fps_idx_;
                    camera_->EnableStream(color_cfg_list->at(color_cfg_idx_));
                    if (state.contains("color_controls")) {
                        for (auto &prop : *color_props) {
                            if (state["color_controls"].contains(prop.first)) {
                                prop.second.value = state["color_controls"][prop.first].get<int>();
                                prop.second.has_changed = true;
                                camera_->SetProperty(dai::CameraBoardSocket::RGB, prop.first);
                            }
                        }
                        // Fix for Auto Enable Options
                        for (auto &prop : *color_props) {
                            if (prop.first == "Auto_Exposure") {
                                if (state["color_controls"].contains(prop.first)) {
                                    prop.second.value = state["color_controls"][prop.first].get<int>();
                                    if ((bool)prop.second.value) {
                                        camera_->SetProperty(dai::CameraBoardSocket::RGB, prop.first);
                                    }
                                    else {
                                        camera_->SetProperty(dai::CameraBoardSocket::RGB, "Exposure");
                                    }
                                }
                            }
                        }
                    }
                }
                if (state.contains("depth_enabled"))
                    enable_depth_ = state["depth_enabled"].get<bool>();
                if (enable_depth_) {
                    auto depth_cfg_list = camera_->GetStreamConfigList(dai::CameraBoardSocket::AUTO);
                    auto depth_props = camera_->GetPropertyList(dai::CameraBoardSocket::AUTO);
                    if (state.contains("depth_res_idx"))
                        depth_cfg_idx_ = state["depth_res_idx"].get<int>();
                    if (state.contains("depth_fps_idx"))
                        depth_fps_idx_ = state["depth_fps_idx"].get<int>();
                    if (state.contains("depth_fps"))
                        depth_fps_ = state["depth_fps"].get<int>();
                    depth_cfg_list->at(depth_cfg_idx_).fps_idx = depth_fps_idx_;
                    if (state.contains("depth_controls")) {
                        for (auto &prop : *depth_props) {
                            if (state["depth_controls"].contains(prop.first)) {
                                prop.second.value = state["depth_controls"][prop.first].get<int>();
                                camera_->SetProperty(dai::CameraBoardSocket::AUTO, prop.first);
                            }
                        }
                    }
                    camera_->EnableStream(depth_cfg_list->at(depth_cfg_idx_));
                }
            }
        }
        if (!cam_exists)
            selected_camera_idx_ = 0;
    }
}
