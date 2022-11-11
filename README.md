# FlowCV Luxonis DepthAI Oak Camera Plugin

Adds Oak camera source input to [FlowCV](https://github.com/FlowCV-org/FlowCV)

---

### Build Instructions

_**Currently there is a issue with USB permissions on MacOS Monterey or newer so the camera will not initialize properly_ 

Prerequisites:

* Build and/or install [Luxonis DepthAI Core C++ SDK](https://github.com/luxonis/depthai-core)
* Clone [FlowCV](https://github.com/FlowCV-org/FlowCV) repo.
* See System specific build requirements for FlowCV [Building From Source](http://docs.flowcv.org/building_source/build_from_source.html)

Build Steps:
1. Clone this repo.
2. cd to the repo directory
3. Run the following commands:

```shell
mkdir Build
cd Build
cmake .. -DDEPTHAI_DIR=/path/to/depthai/install/path -DFlowCV_DIR=/path/to/FlowCV/FlowCV_SDK
make
```

