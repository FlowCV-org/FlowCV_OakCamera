//
// Plugin Oak Camera
//

#ifndef FLOWCV_PLUGIN_OAK_PLUGIN_HPP_
#define FLOWCV_PLUGIN_OAK_PLUGIN_HPP_
#include <DSPatch.h>
#include "FlowCV_Types.hpp"
#include "oak_camera.hpp"

namespace DSPatch::DSPatchables
{
namespace internal
{
class OakCamera;
}

class DLLEXPORT OakCamera final : public Component
{
  public:
    OakCamera();
    void UpdateGui(void *context, int interface) override;
    bool HasGui(int interface) override;
    std::string GetState() override;
    void SetState(std::string &&json_serialized) override;

  protected:
    void Process_( SignalBus const& inputs, SignalBus& outputs ) override;

  private:
    std::unique_ptr<internal::OakCamera> p;
    std::mutex io_mutex_;
    std::unique_ptr<oak_camera> camera_;
    int selected_camera_idx_;
    int color_cfg_idx_;
    int color_fps_idx_;
    int color_fps_;
    int depth_cfg_idx_;
    int depth_fps_idx_;
    int depth_fps_;
    bool enable_color_;
    bool enable_depth_;

};

EXPORT_PLUGIN( OakCamera )

}  // namespace DSPatch::DSPatchables

#endif //FLOWCV_PLUGIN_OAK_PLUGIN_HPP_
