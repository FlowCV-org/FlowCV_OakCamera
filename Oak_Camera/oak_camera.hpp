//
// Oak Camera Device Handler
//

#ifndef FLOWCV_PLUGIN_OAK_CAMERA_HPP_
#define FLOWCV_PLUGIN_OAK_CAMERA_HPP_
#include <iostream>
#include <vector>
#include <string>
#include "opencv2/opencv.hpp"
#include "depthai/depthai.hpp"
#include <json.hpp>

struct OakRange
{
    int min;
    int max;
    int def;
    float step;
};

struct Property
{
    int value;
    OakRange range;
    std::vector<std::string> opt_list;
    bool is_list;
    bool has_changed;
};

struct StreamConfig
{
    std::string str_resolution;
    std::string str_stream_name;
    dai::CameraBoardSocket stream_type;
    int res_prop;
    int width;
    int height;
    bool ispScale;
    int numerator;
    int denominator;
    std::vector<int> fps_list;
    int fps_idx;
};

class oak_camera {
  public:
    oak_camera();
    void RefreshDeviceList();
    int GetDeviceCount();
    std::string GetDeviceSerial(int index);
    std::string GetDeviceName(int index);
    const std::vector<std::string> &GetDeviceList();
    void InitCamera(int index, bool immediate = false);
    bool IsInit();
    [[nodiscard]] bool IsReconfiguring() const;
    std::vector<StreamConfig> *GetStreamConfigList(dai::CameraBoardSocket stream_type);
    void SetAllRgbControls();
    std::map<std::string, Property> *GetPropertyList(dai::CameraBoardSocket stream_type);
    int GetProperty(dai::CameraBoardSocket stream_type, std::string& prop_name);
    void SetProperty(dai::CameraBoardSocket stream_type, std::string prop_name, bool immediate_mode = false);
    void ResetProperties(dai::CameraBoardSocket stream_type);
    bool EnableStream(StreamConfig& config, bool immediate = false);
    void DisableStream(dai::CameraBoardSocket stream);
    void ProcessStreams();
    cv::Mat &GetFrame(dai::CameraBoardSocket stream);
    nlohmann::json &GetMetaData();
    bool HasColor() const;
    bool HasDepth() const;

  protected:
    void InitCamera_();
    void ReconfigureDevice_();
    void ChangeProperties_();
    void UpdateCalibData_();

  private:
    std::mutex io_mutex_;
    int active_dev_idx_;
    std::shared_ptr<dai::Pipeline> pipeline;
    std::shared_ptr<dai::Device> device;
    std::vector<std::string> queueNames;
    std::shared_ptr<dai::node::ColorCamera> camRgb;
    std::shared_ptr<dai::node::XLinkIn> controlIn;
    std::shared_ptr<dai::DataInputQueue> controlQueue;
    std::shared_ptr<dai::node::MonoCamera> left;
    std::shared_ptr<dai::node::MonoCamera> right;
    std::shared_ptr<dai::node::StereoDepth> stereo;
    std::shared_ptr<dai::node::XLinkOut> rgbOut;
    std::shared_ptr<dai::node::XLinkOut> depthOut;
    std::vector<std::vector<float>> rgb_intrinsics_;
    std::vector<std::vector<float>> depth_intrinsics_;
    StreamConfig active_color_cfg_;
    StreamConfig active_depth_cfg_;
    cv::Mat color_frame_;
    cv::Mat depth_frame_;
    std::string oak_dev_serial_;
    std::string oak_dev_name_;
    std::vector<std::string> camera_name_list_;
    std::vector<dai::DeviceInfo> infos_;
    nlohmann::json meta_data_;
    int init_idx_;
    bool init_;
    bool has_rgb_;
    bool has_depth_;
    bool is_color_enabled_;
    bool is_depth_enabled_;
    bool is_color_streaming_;
    bool is_depth_streaming_;
    bool is_init_;
    bool reconfigure_;
    bool change_props_;
    std::vector<StreamConfig> color_configs_;
    std::map<std::string, Property> color_props_;
    std::vector<StreamConfig> depth_configs_;
    std::map<std::string, Property> depth_props_;

};

#endif //FLOWCV_PLUGIN_OAK_CAMERA_HPP_
