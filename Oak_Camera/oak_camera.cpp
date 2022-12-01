//
// Oak Camera Device Handler
//

#include "oak_camera.hpp"

oak_camera::oak_camera()
{
    active_dev_idx_ = 0;
    is_color_streaming_ = false;
    is_depth_streaming_ = false;
    is_color_enabled_ = false;
    is_depth_enabled_ = false;
    has_rgb_ = false;
    has_depth_ = false;
    is_init_ = false;
    reconfigure_ = false;
    change_props_ = false;
    init_ = false;
    init_idx_ = 0;

    RefreshDeviceList();
}

void oak_camera::RefreshDeviceList()
{
    camera_name_list_.clear();
    camera_name_list_.emplace_back("None");

    infos_.clear();
    infos_ = dai::Device::getAllAvailableDevices();
    if(infos_.empty())
        return;

    for(auto& info : infos_) {
        camera_name_list_.emplace_back(info.mxid);
    }
}

int oak_camera::GetDeviceCount()
{
    return (int)infos_.size();
}

const std::vector<std::string> &oak_camera::GetDeviceList()
{
    return camera_name_list_;
}

void oak_camera::InitCamera_()
{
    std::lock_guard<std::mutex> lck(io_mutex_);
    if (is_init_) {
        device.reset();
        pipeline.reset();
        queueNames.clear();
        is_color_streaming_ = false;
        is_depth_streaming_ = false;
    }

    is_init_ = false;
    oak_dev_name_ = "";
    has_rgb_ = false;
    has_depth_ = false;
    if (init_idx_ > 0 && init_idx_ < infos_.size() + 1) {
        depth_configs_.clear();
        color_configs_.clear();
        color_props_.clear();
        depth_props_.clear();
        pipeline = std::make_shared<dai::Pipeline>();
        active_dev_idx_ = init_idx_ - 1;
        oak_dev_serial_ = infos_[active_dev_idx_].mxid;
        if (pipeline != nullptr) {
            device = std::make_shared<dai::Device>(dai::OpenVINO::Version::VERSION_2021_4, infos_[active_dev_idx_], dai::UsbSpeed::SUPER);
        } else {
            std::cerr << "Error initializing Oak Camera Pipeline" << std::endl;
            active_dev_idx_ = 0;
            oak_dev_serial_ = "";
            return;
        }
        if (device == nullptr) {
            std::cerr << "Error initializing Oak Device" << std::endl;
            active_dev_idx_ = 0;
            oak_dev_serial_ = "";
            return;
        }

        // Connect to device and start pipeline
        bool has_left = false;
        bool has_right = false;
        for(auto& sensor : device->getCameraSensorNames()) {
            if (sensor.first == dai::CameraBoardSocket::LEFT)
                has_left = true;
            else if (sensor.first == dai::CameraBoardSocket::RIGHT)
                has_right = true;
            else if (sensor.first == dai::CameraBoardSocket::RGB)
                has_rgb_ = true;
        }
        if (has_left && has_right) {
            has_depth_ = true;
            // Add common depth configurations
            depth_configs_.emplace_back(StreamConfig{"1280 x 800", "Depth", dai::CameraBoardSocket::AUTO,
                                                     (int)dai::MonoCameraProperties::SensorResolution::THE_800_P,
                                                     1280, 800, false, 1, 1, {120, 60, 30, 15}, 0});
            depth_configs_.emplace_back(StreamConfig{"1280 x 720", "Depth", dai::CameraBoardSocket::AUTO,
                                                     (int)dai::MonoCameraProperties::SensorResolution::THE_720_P,
                                                     1280, 720, false, 1, 1, {120, 60, 30, 15}, 0});
            depth_configs_.emplace_back(StreamConfig{"640 x 480", "Depth", dai::CameraBoardSocket::AUTO,
                                                     (int)dai::MonoCameraProperties::SensorResolution::THE_480_P,
                                                     640, 480, false, 1, 1, {120, 60, 30, 15}, 0});
            depth_configs_.emplace_back(StreamConfig{"640 x 400", "Depth", dai::CameraBoardSocket::AUTO,
                                                     (int)dai::MonoCameraProperties::SensorResolution::THE_400_P,
                                                     640, 400, false, 1, 1, {120, 60, 30, 15}, 0});
            // Add Depth Property Controls
            depth_props_["Preset"] = {0, {0, 2, 0, 1.0f},
                                        {"High Accuracy", "High Density"},
                                        true, false};
        }
        if (has_rgb_) {
            // Add common RGB configurations
            color_configs_.emplace_back(StreamConfig{"3840 x 2160", "RGB", dai::CameraBoardSocket::RGB,
                                                     (int)dai::ColorCameraProperties::SensorResolution::THE_4_K,
                                                     3840, 2160, false, 1, 1, {60, 30, 15}, 0});
            color_configs_.emplace_back(StreamConfig{"1920 x 1080", "RGB", dai::CameraBoardSocket::RGB,
                                                     (int)dai::ColorCameraProperties::SensorResolution::THE_1080_P,
                                                     1920, 1080, false, 1, 1, {60, 30, 15}, 0});
            color_configs_.emplace_back(StreamConfig{"1280 x 720", "RGB", dai::CameraBoardSocket::RGB,
                                                     (int)dai::ColorCameraProperties::SensorResolution::THE_1080_P,
                                                     1280, 720, true, 2, 3, {60, 30, 15}, 0});
            color_configs_.emplace_back(StreamConfig{"960 x 540", "RGB", dai::CameraBoardSocket::RGB,
                                                     (int)dai::ColorCameraProperties::SensorResolution::THE_1080_P,
                                                     960, 540, true, 1, 2, {60, 30, 15}, 0});
            color_configs_.emplace_back(StreamConfig{"640 x 360", "RGB", dai::CameraBoardSocket::RGB,
                                                     (int)dai::ColorCameraProperties::SensorResolution::THE_1080_P,
                                                     640, 360, true, 1, 3, {60, 30, 15}, 0});
            // Add Camera Property Controls
            color_props_["Brightness"] = {0, {-10, 10, 0, 0.25f}, {}, false, false};
            color_props_["Contrast"] = {0, {-10, 10, 0, 0.25f}, {}, false, false};
            color_props_["Saturation"] = {0, {-10, 10, 0, 0.25f}, {}, false, false};
            color_props_["Sharpness"] = {0, {0, 4, 0, 0.1f}, {}, false, false};
            color_props_["Auto_Exposure"] = {(int)true, {0, 1, (int)true, 0.1f}, {}, false, false};
            color_props_["Exposure"] = {20000, {1, 33000, 20000, 500.0f}, {}, false, false};
            color_props_["ISO"] = {800, {100, 1600, 800, 50.0f}, {}, false, false};
            color_props_["White_Balance_Mode"] = {1, {0, 8, 1, 1.0f},
                                                  {"Off", "Auto", "Incandescent", "Fluorescent",
                                                   "Warm Fluorescent", "Daylight", "Cloudy Daylight",
                                                   "Twilight", "Shade"},
                                                   true, false};
            color_props_["White_Balance"] = {4000, {200, 12000, 4000, 1000.0f}, {}, false, false};
            color_props_["Focus_Mode"] = {1, {0, 6, 1, 1.0f},
                                                  {"Off", "Auto", "Macro", "Continuous Video",
                                                   "Continuous Picture", "EDOF"},
                                                  true, false};
            color_props_["Focus_Pos"] = {150, {0, 255, 150, 3.0f}, {}, false, false};
        }

        if (device->isEepromAvailable()) {
            auto eeprom = device->readCalibration2().getEepromData();
            oak_dev_name_ = eeprom.boardName;
        }
        else
            oak_dev_name_ = "Oak";

        is_init_ = true;
    }
    init_ = false;
}

void oak_camera::InitCamera(int index, bool immediate)
{
    if (immediate) {
        init_idx_ = index;
        InitCamera_();
    }
    else {
        init_idx_ = index;
        init_ = true;
    }
}

void oak_camera::ReconfigureDevice_()
{
    std::lock_guard<std::mutex> lck(io_mutex_);
    if (is_init_) {
        if (is_color_streaming_ || is_depth_streaming_) {
            device.reset();
            pipeline.reset();
            pipeline = std::make_shared<dai::Pipeline>();
            device = std::make_shared<dai::Device>(dai::OpenVINO::Version::VERSION_2021_4, infos_[active_dev_idx_], dai::UsbSpeed::SUPER);
        }
        is_color_streaming_ = false;
        is_depth_streaming_ = false;
        queueNames.clear();
        if (is_color_enabled_ || is_depth_enabled_) {
            if (is_color_enabled_) {
                rgb_intrinsics_.clear();
                camRgb = pipeline->create<dai::node::ColorCamera>();
                rgbOut = pipeline->create<dai::node::XLinkOut>();
                controlIn = pipeline->create<dai::node::XLinkIn>();
                rgbOut->setStreamName(active_color_cfg_.str_stream_name);
                queueNames.emplace_back(active_color_cfg_.str_stream_name);
                controlIn->setStreamName("control");
                camRgb->setBoardSocket(dai::CameraBoardSocket::RGB);
                if (active_color_cfg_.ispScale) {
                    camRgb->setResolution((dai::ColorCameraProperties::SensorResolution)active_color_cfg_.res_prop);
                    camRgb->setIspScale(active_color_cfg_.numerator, active_color_cfg_.denominator);
                }
                else {
                    camRgb->setResolution((dai::ColorCameraProperties::SensorResolution)active_color_cfg_.res_prop);
                }
                camRgb->setFps((float)active_color_cfg_.fps_list.at(active_color_cfg_.fps_idx));
                camRgb->isp.link(rgbOut->input);
                controlIn->out.link(camRgb->inputControl);
                is_color_streaming_ = true;
            }
            if (is_depth_enabled_) {
                depth_intrinsics_.clear();
                left = pipeline->create<dai::node::MonoCamera>();
                right = pipeline->create<dai::node::MonoCamera>();
                stereo = pipeline->create<dai::node::StereoDepth>();
                depthOut = pipeline->create<dai::node::XLinkOut>();
                depthOut->setStreamName(active_depth_cfg_.str_stream_name);
                queueNames.emplace_back(active_depth_cfg_.str_stream_name);
                left->setResolution((dai::MonoCameraProperties::SensorResolution)active_depth_cfg_.res_prop);
                left->setBoardSocket(dai::CameraBoardSocket::LEFT);
                left->setFps((float)active_depth_cfg_.fps_list.at(active_depth_cfg_.fps_idx));
                right->setResolution((dai::MonoCameraProperties::SensorResolution)active_depth_cfg_.res_prop);
                right->setBoardSocket(dai::CameraBoardSocket::RIGHT);
                right->setFps((float)active_depth_cfg_.fps_list.at(active_depth_cfg_.fps_idx));
                if (depth_props_["Preset"].value == 0)
                    stereo->setDefaultProfilePreset(dai::node::StereoDepth::PresetMode::HIGH_ACCURACY);
                else if (depth_props_["Preset"].value == 1)
                    stereo->setDefaultProfilePreset(dai::node::StereoDepth::PresetMode::HIGH_DENSITY);
                stereo->setLeftRightCheck(true);
                if (is_color_enabled_)
                    stereo->setDepthAlign(dai::CameraBoardSocket::RGB);
                left->out.link(stereo->left);
                right->out.link(stereo->right);
                stereo->depth.link(depthOut->input);
                is_depth_streaming_ = true;
            }

            if (!device->isPipelineRunning()) {
                device->startPipeline(*pipeline);
            }
            // Sets queues size and behavior
            for(const auto& name : queueNames) {
                device->getOutputQueue(name, 4, true);
            }
            if (is_color_enabled_) {
                controlQueue = device->getInputQueue("control");
            }
            SetAllRgbControls();
            reconfigure_ = false;
        }
    }
}

bool oak_camera::EnableStream(StreamConfig& config, bool immediate)
{
    if (immediate) {
        if (config.stream_type == dai::CameraBoardSocket::RGB) {
            std::lock_guard<std::mutex> lck(io_mutex_);
            active_color_cfg_ = config;
            is_color_enabled_ = true;
            ReconfigureDevice_();
            return true;
        } else if (config.stream_type == dai::CameraBoardSocket::AUTO) {
            std::lock_guard<std::mutex> lck(io_mutex_);
            active_depth_cfg_ = config;
            is_depth_enabled_ = true;
            ReconfigureDevice_();
            return true;
        }
    }
    else {
        if (config.stream_type == dai::CameraBoardSocket::RGB) {
            std::lock_guard<std::mutex> lck(io_mutex_);
            active_color_cfg_ = config;
            is_color_enabled_ = true;
            reconfigure_ = true;
            return true;
        } else if (config.stream_type == dai::CameraBoardSocket::AUTO) {
            std::lock_guard<std::mutex> lck(io_mutex_);
            active_depth_cfg_ = config;
            is_depth_enabled_ = true;
            reconfigure_ = true;
            return true;
        }
    }

    return false;
}

void oak_camera::DisableStream(dai::CameraBoardSocket stream)
{
    if (stream == dai::CameraBoardSocket::RGB) {
        is_color_enabled_ = false;
        reconfigure_ = true;
    }
    else if (stream == dai::CameraBoardSocket::AUTO) {
        is_depth_enabled_ = false;
        reconfigure_ = true;
    }
}

void oak_camera::ProcessStreams()
{
    if (init_)
        InitCamera_();

    if (reconfigure_)
        ReconfigureDevice_();

    if (change_props_)
        ChangeProperties_();

    if (is_init_) {
        if (is_color_streaming_ || is_depth_streaming_) {
            if(rgb_intrinsics_.empty() || depth_intrinsics_.empty())
                UpdateCalibData_();
            meta_data_.clear();
            nlohmann::json jMeta;
            nlohmann::json intrinsic;
            meta_data_["data_type"] = "metadata";

            std::unordered_map<std::string, std::shared_ptr<dai::ImgFrame>> latestPacket;
            for (const auto &name: queueNames) {
                device->getQueueEvent(name);
                auto packets = device->getOutputQueue(name)->tryGetAll<dai::ImgFrame>();
                auto count = packets.size();
                if (count > 0) {
                    latestPacket[name] = packets[count - 1];
                }
            }
            for (const auto &name: queueNames) {
                if (latestPacket.find(name) != latestPacket.end()) {
                    if (name == active_depth_cfg_.str_stream_name && is_depth_enabled_) {
                        depth_frame_ = latestPacket[name]->getCvFrame();
                        nlohmann::json depth_frame;
                        nlohmann::json refDepth;
                        nlohmann::json depth_int;
                        int width = right->getResolutionWidth();
                        int height = right->getResolutionHeight();
                        if (is_color_streaming_) {
                            width = camRgb->getIspWidth();
                            height = camRgb->getIspHeight();
                        }
                        refDepth["w"] = width;
                        refDepth["h"] = height;
                        meta_data_["depth_frame"] = refDepth;
                        depth_frame["fps"] = right->getFps();
                        depth_frame["frame_num"] = latestPacket[name]->getSequenceNum();
                        depth_frame["timestamp"] = latestPacket[name]->getTimestamp().time_since_epoch().count();
                        jMeta["depth_frame"] = depth_frame;
                        depth_int["width"] = width;
                        depth_int["height"] = height;
                        depth_int["fx"] = depth_intrinsics_[0][0];
                        depth_int["fy"] = depth_intrinsics_[1][1];
                        depth_int["ppx"] = depth_intrinsics_[0][2];
                        depth_int["ppy"] = depth_intrinsics_[1][2];
                        intrinsic["depth"] = depth_int;
                    }
                    else if (name == active_color_cfg_.str_stream_name && is_color_enabled_) {
                        color_frame_ = latestPacket[name]->getCvFrame();
                        nlohmann::json ref;
                        ref["w"] = camRgb->getIspWidth();
                        ref["h"] = camRgb->getIspHeight();
                        meta_data_["ref_frame"] = ref;
                        nlohmann::json color_frame;
                        if (!rgb_intrinsics_.empty()) {
                            color_frame["fps"] = camRgb->getFps();
                            color_frame["frame_num"] = latestPacket[name]->getSequenceNum();
                            color_frame["timestamp"] = latestPacket[name]->getTimestamp().time_since_epoch().count();
                            jMeta["color_frame"] = color_frame;
                            nlohmann::json color_int;
                            color_int["width"] = camRgb->getIspWidth();
                            color_int["height"] = camRgb->getIspHeight();
                            color_int["fx"] = rgb_intrinsics_[0][0];
                            color_int["fy"] = rgb_intrinsics_[1][1];
                            color_int["ppx"] = rgb_intrinsics_[0][2];
                            color_int["ppy"] = rgb_intrinsics_[1][2];
                            intrinsic["color"] = color_int;
                        }
                    }
                }
            }
            if (!intrinsic.empty())
                jMeta["intrinsics"] = intrinsic;
            if (!jMeta.empty())
                meta_data_["data"].emplace_back(jMeta);
        }
    }
}

void oak_camera::UpdateCalibData_()
{
    if (is_depth_streaming_) {
        int width = right->getResolutionWidth();
        int height = right->getResolutionHeight();
        if (is_color_streaming_) {
            width = camRgb->getIspWidth();
            height = camRgb->getIspHeight();
        }
        depth_intrinsics_ = device->readCalibration2().getCameraIntrinsics(dai::CameraBoardSocket::RIGHT, width, height);
    }

    if (is_color_streaming_) {
        int width = camRgb->getIspWidth();
        int height = camRgb->getIspHeight();
        rgb_intrinsics_ = device->readCalibration2().getCameraIntrinsics(dai::CameraBoardSocket::RGB, width, height);
    }
}

cv::Mat &oak_camera::GetFrame(dai::CameraBoardSocket stream)
{
    if (stream == dai::CameraBoardSocket::AUTO)
        return depth_frame_;

    return color_frame_;
}

std::vector<StreamConfig> *oak_camera::GetStreamConfigList(dai::CameraBoardSocket stream_type)
{
    if (stream_type == dai::CameraBoardSocket::AUTO)
        return &depth_configs_;

    return &color_configs_;
}

bool oak_camera::IsInit()
{
    return is_init_;
}

std::map<std::string, Property> *oak_camera::GetPropertyList(dai::CameraBoardSocket stream_type)
{
    if (stream_type == dai::CameraBoardSocket::RGB)
        return &color_props_;
    else if (stream_type == dai::CameraBoardSocket::AUTO)
        return &depth_props_;

    return nullptr;
}

void oak_camera::SetAllRgbControls()
{
    if (is_init_ && is_color_enabled_ && is_color_streaming_) {
        dai::CameraControl ctrl;

        ctrl.setBrightness(color_props_["Brightness"].value);
        ctrl.setContrast(color_props_["Contrast"].value);
        ctrl.setSaturation(color_props_["Saturation"].value);
        ctrl.setSharpness(color_props_["Sharpness"].value);

        if (color_props_["Auto_Exposure"].value)
            ctrl.setAutoExposureEnable();
        else
            ctrl.setManualExposure(color_props_["Exposure"].value, color_props_["ISO"].value);

        if (color_props_["White_Balance_Mode"].value == 0)
            ctrl.setManualWhiteBalance(color_props_["White_Balance"].value);
        else
            ctrl.setAutoWhiteBalanceMode((dai::CameraControl::AutoWhiteBalanceMode)(color_props_["White_Balance_Mode"].value));

        ctrl.setAutoFocusMode((dai::CameraControl::AutoFocusMode)color_props_["Focus_Mode"].value);
        if (color_props_["Focus_Mode"].value == 0)
            ctrl.setManualFocus(color_props_["Focus_Pos"].value);
        else
            ctrl.setAutoFocusTrigger();

        controlQueue->send(ctrl);
    }
}

void oak_camera::SetProperty(dai::CameraBoardSocket stream_type, std::string prop_name, bool immediate_mode)
{
    if (stream_type == dai::CameraBoardSocket::RGB) {
        if (is_init_ && is_color_enabled_ && is_color_streaming_) {
            if (immediate_mode) {
                if (color_props_.find(prop_name) != color_props_.end()) {
                    dai::CameraControl ctrl;
                    if (prop_name == "Brightness")
                        ctrl.setBrightness(color_props_[prop_name].value);
                    else if (prop_name == "Contrast")
                        ctrl.setContrast(color_props_[prop_name].value);
                    else if (prop_name == "Saturation")
                        ctrl.setSaturation(color_props_[prop_name].value);
                    else if (prop_name == "Sharpness")
                        ctrl.setSharpness(color_props_[prop_name].value);
                    else if (prop_name == "Exposure") {
                        color_props_["Auto_Exposure"].value = (int)false;
                        ctrl.setManualExposure(color_props_["Exposure"].value, color_props_["ISO"].value);
                    }
                    else if (prop_name == "ISO") {
                        color_props_["Auto_Exposure"].value = (int)false;
                        ctrl.setManualExposure(color_props_["Exposure"].value, color_props_["ISO"].value);
                    }
                    else if (prop_name == "Auto_Exposure") {
                        if ((bool)color_props_[prop_name].value)
                            ctrl.setAutoExposureEnable();
                        else
                            ctrl.setManualExposure(color_props_["Exposure"].value, color_props_["ISO"].value);
                    }
                    else if (prop_name == "White_Balance_Mode") {
                        if (color_props_[prop_name].value == 0)
                            ctrl.setManualWhiteBalance(color_props_["White_Balance"].value);
                        else
                            ctrl.setAutoWhiteBalanceMode((dai::CameraControl::AutoWhiteBalanceMode)(color_props_[prop_name].value));
                    }
                    else if (prop_name == "White_Balance") {
                        if (color_props_["White_Balance_Mode"].value == 0)
                            ctrl.setManualWhiteBalance(color_props_["White_Balance"].value);
                    }
                    else if (prop_name == "Focus_Mode") {
                        ctrl.setAutoFocusMode((dai::CameraControl::AutoFocusMode) color_props_[prop_name].value);
                        if (color_props_[prop_name].value == 0)
                            ctrl.setManualFocus(color_props_["Focus_Pos"].value);
                        else
                            ctrl.setAutoFocusTrigger();
                    }
                    else if (prop_name == "Focus_Pos") {
                        if (color_props_["Focus_Mode"].value == 0)
                            ctrl.setManualFocus(color_props_["Focus_Pos"].value);
                    }
                    controlQueue->send(ctrl);
                }
            } else {
                if (color_props_.find(prop_name) != color_props_.end()) {
                    color_props_[prop_name].has_changed = true;
                    change_props_ = true;
                }
            }
        }
    }
    else if (stream_type == dai::CameraBoardSocket::AUTO) {
        if (is_init_ && is_depth_enabled_ && is_depth_streaming_) {
            if (depth_props_.find(prop_name) != depth_props_.end()) {
                if (prop_name == "Preset") {
                    if (depth_props_[prop_name].value == 0)
                        stereo->setDefaultProfilePreset(dai::node::StereoDepth::PresetMode::HIGH_ACCURACY);
                    else if (depth_props_[prop_name].value == 1)
                        stereo->setDefaultProfilePreset(dai::node::StereoDepth::PresetMode::HIGH_DENSITY);
                }
            }
        }
    }
}

int oak_camera::GetProperty(dai::CameraBoardSocket stream_type, std::string& prop_name)
{
    if (stream_type == dai::CameraBoardSocket::RGB) {
        if (color_props_.find(prop_name) != color_props_.end())
            return color_props_[prop_name].value;
    }
    else if (stream_type == dai::CameraBoardSocket::AUTO) {
        if (depth_props_.find(prop_name) != depth_props_.end())
            return depth_props_[prop_name].value;
    }

    return 0;
}

void oak_camera::ChangeProperties_()
{
    bool set_exp = false;
    bool set_auto_exp = false;

    for (auto &prop : color_props_) {
        if (prop.second.has_changed) {
            if (prop.first == "Auto_Exposure")
                set_auto_exp = true;
            else if (prop.first == "Exposure")
                set_exp = true;
            SetProperty(dai::CameraBoardSocket::RGB, prop.first, true);
            prop.second.has_changed = false;
        }
    }
    if (set_auto_exp && set_exp) {
        color_props_["Auto_Exposure"].value = (int)true;
        SetProperty(dai::CameraBoardSocket::RGB, "Auto_Exposure", true);
    }

    change_props_ = false;
}

void oak_camera::ResetProperties(dai::CameraBoardSocket stream_type)
{
    if (stream_type == dai::CameraBoardSocket::RGB) {
        for (auto &prop: color_props_) {
            prop.second.value = prop.second.range.def;
            prop.second.has_changed = true;
        }
    }
    change_props_ = true;
}

std::string oak_camera::GetDeviceSerial(int index)
{
    if (index > 0 && index < infos_.size() + 1)
        return infos_.at(index -1).mxid;

    return "";
}


nlohmann::json &oak_camera::GetMetaData()
{
    return meta_data_;
}

bool oak_camera::IsReconfiguring() const
{
    return reconfigure_;
}

bool oak_camera::HasColor() const
{
    return has_rgb_;
}

bool oak_camera::HasDepth() const
{
    return has_depth_;
}

std::string oak_camera::GetDeviceName(int index)
{
    return oak_dev_name_;
}


