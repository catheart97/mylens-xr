#include "pch.h"

#include "My/Engine/ComponentManager.h"
#include "My/Engine/EntityManager.h"
#include "My/Engine/Game.h"

#include "My/Utility/MeshLoader.h"

#include "My/D3D11RenderSystem/D3D11RenderSystem.h"

#include "My/Math/GLMHelpers.h"

namespace My
{

namespace _implementation
{

using namespace My::Engine;
using namespace My::D3D11RenderSystem;

struct HololensGame : My::Engine::Game
{
    // Data //
private:
    const std::string _application_name;

    constexpr static XrFormFactor _form_factor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
    constexpr static XrViewConfigurationType _primary_view_config_type{
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
    constexpr static uint32_t _stereo_view_count =
        2; // PRIMARY_STEREO view configuration always has 2 views

    xr::InstanceHandle _instance;
    xr::SessionHandle _session;
    uint64_t _system_id{XR_NULL_SYSTEM_ID};
    xr::ExtensionDispatchTable _extensions;

    struct
    {
        bool DepthExtensionSupported{false};
        bool UnboundedRefSpaceSupported{false};
        bool SpatialAnchorSupported{false};
    } _optional_extensions;

    xr::SpaceHandle _app_space;
    XrReferenceSpaceType _app_space_type{};

    constexpr static uint32_t LeftSide = 0;
    constexpr static uint32_t RightSide = 1;
    std::array<XrPath, 2> _subaction_paths{};

    xr::ActionSetHandle _action_set;
    xr::ActionHandle _action_place;
    xr::ActionHandle _action_exit;
    xr::ActionHandle _action_pose;
    xr::ActionHandle _action_vibrate;

    XrEnvironmentBlendMode _environment_blend_mode{};
    xr::math::NearFar _near_far{};

    struct Hand
    {
        xr::SpaceHandle Space{};
    };

    Hand _hands[2];

    struct SwapchainD3D11
    {
        xr::SwapchainHandle Handle;
        DXGI_FORMAT Format{DXGI_FORMAT_UNKNOWN};
        uint32_t Width{0};
        uint32_t Height{0};
        uint32_t ArraySize{0};
        std::vector<XrSwapchainImageD3D11KHR> Images;
    };

    struct RenderResources
    {
        XrViewState ViewState{XR_TYPE_VIEW_STATE};
        std::vector<XrView> Views;
        std::vector<XrViewConfigurationView> ConfigViews;
        SwapchainD3D11 ColorSwapchain;
        SwapchainD3D11 DepthSwapchain;
        std::vector<XrCompositionLayerProjectionView> ProjectionLayerViews;
        std::vector<XrCompositionLayerDepthInfoKHR> DepthInfoViews;
    };

    std::unique_ptr<RenderResources> _render_resources{};

    bool _session_running{false};
    XrSessionState _session_state{XR_SESSION_STATE_UNKNOWN};

    bool _animate{true};

    ComponentManager _component_manager;
    EntityManager _entity_manager;
    My::D3D11RenderSystem::D3D11RenderSystem _render_system;

    EntityReference _object{0};
    bool _initialized{false};

    // Constructors/Methods //
public:
    HololensGame(std::string application_name = "MyLens")
        : _application_name(std::move(application_name))
    {}

    void Run() override
    {
        CreateInstance();
        CreateActions();

        bool request_restart = false;
        do
        {
            InitializeSystem();
            InitializeSession();

            InitializeScene();

            while (true)
            {
                bool exit_render_loop = false;
                ProcessEvents(&exit_render_loop, &request_restart);
                if (exit_render_loop)
                {
                    break;
                }

                if (_session_running)
                {
                    PollActions();
                    if (_initialized)
                    {
                        RenderFrame();
                    }
                }
                else
                {
                    // Throttle loop since xrWaitFrame won't be called.
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(250ms);
                }
            }

            if (request_restart)
            {
                PrepareSessionRestart();
            }
        } while (request_restart);
    }

private:
    float _phi{0.f};
    uint32_t _target_hand{0};
    void UpdateScene(XrFrameState & state)
    {
        if (_initialized)
        {
            if (_animate)
            {
                _phi -= .005f;
                _entity_manager[_object.Index].Rotation = My::Math::RollPitchYaw(0.f, _phi, 0.f);
            }
            else
            {
                XrSpaceLocation hand_location{XR_TYPE_SPACE_LOCATION};
                XrTime time = state.predictedDisplayTime;
                CHECK_XRCMD(xrLocateSpace(_hands[_target_hand].Space.Get(), _app_space.Get(), time,
                                          &hand_location));

                if (xr::math::Pose::IsPoseValid(hand_location))
                {
                    auto & pos = hand_location.pose.position;
                    _entity_manager[_object.Index].Position = glm::vec3(pos.x, pos.y, pos.z);
                }
            }
        }
    }

    void InitializeScene()
    {
        using namespace My;
        using namespace DirectX;

        _object = Utility::MeshLoader::Load(L"assets/objects/hololens.obj", //
                                            _entity_manager,                //
                                            _component_manager);

        auto light_entity = _entity_manager.CreateEntity();
        PointLightComponent light_component;
        light_component.Intensity = 100.f;
        _entity_manager[light_entity.Index].Position = glm::vec3(1, 0, 1);
        _component_manager.RegisterComponent(_entity_manager, light_entity, light_component);

        _initialized = true;
    }

    void CreateInstance()
    {
        CHECK(_instance.Get() == XR_NULL_HANDLE);

        // Build out the extensions to enable. Some extensions are required and some are optional.
        const std::vector<const char *> enabled_extensions = SelectExtensions();

        // Create the instance with enabled extensions.
        XrInstanceCreateInfo create_info{XR_TYPE_INSTANCE_CREATE_INFO};
        create_info.enabledExtensionCount = (uint32_t)enabled_extensions.size();
        create_info.enabledExtensionNames = enabled_extensions.data();

        create_info.applicationInfo = {"My", 1, "", 1, XR_CURRENT_API_VERSION};
        strcpy_s(create_info.applicationInfo.applicationName, _application_name.c_str());
        CHECK_XRCMD(xrCreateInstance(&create_info, _instance.Put()));

        _extensions.PopulateDispatchTable(_instance.Get());
    }

    std::vector<const char *> SelectExtensions()
    {
        // Fetch the list of extensions supported by the runtime.
        uint32_t extension_count;
        CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extension_count, nullptr));
        std::vector<XrExtensionProperties> extension_properties(extension_count,
                                                                {XR_TYPE_EXTENSION_PROPERTIES});
        CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(
            nullptr, extension_count, &extension_count, extension_properties.data()));

        std::vector<const char *> enabled_extensions;

        // Add a specific extension to the list of extensions to be enabled, if it is supported.
        auto EnableExtensionIfSupported = [&](const char * extension_name) {
            for (uint32_t i = 0; i < extension_count; i++)
            {
                if (strcmp(extension_properties[i].extensionName, extension_name) == 0)
                {
                    enabled_extensions.push_back(extension_name);
                    return true;
                }
            }
            return false;
        };

        // D3D11 extension is required for this sample, so check if it's supported.
        CHECK(EnableExtensionIfSupported(XR_KHR_D3D11_ENABLE_EXTENSION_NAME));

#if UWP
        // Require XR_EXT_win32_appcontainer_compatible extension when building in UWP context.
        CHECK(EnableExtensionIfSupported(XR_EXT_WIN32_APPCONTAINER_COMPATIBLE_EXTENSION_NAME));
#endif

        // Additional optional extensions for enhanced functionality. Track whether enabled in
        // _optionalExtensions.
        _optional_extensions.DepthExtensionSupported =
            EnableExtensionIfSupported(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
        _optional_extensions.UnboundedRefSpaceSupported =
            EnableExtensionIfSupported(XR_MSFT_UNBOUNDED_REFERENCE_SPACE_EXTENSION_NAME);
        _optional_extensions.SpatialAnchorSupported =
            EnableExtensionIfSupported(XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME);

        return enabled_extensions;
    }

    void CreateActions()
    {
        CHECK(_instance.Get() != XR_NULL_HANDLE);

        // Create an action set.
        {
            XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
            strcpy_s(actionSetInfo.actionSetName, "place_hologram_action_set");
            strcpy_s(actionSetInfo.localizedActionSetName, "Placement");
            CHECK_XRCMD(xrCreateActionSet(_instance.Get(), &actionSetInfo, _action_set.Put()));
        }

        // Create actions.
        {
            // Enable subaction path filtering for left or right hand.
            _subaction_paths[LeftSide] = GetXrPath("/user/hand/left");
            _subaction_paths[RightSide] = GetXrPath("/user/hand/right");

            // Create an input action to place a hologram.
            {
                XrActionCreateInfo action_info{XR_TYPE_ACTION_CREATE_INFO};
                action_info.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
                strcpy_s(action_info.actionName, "place_hologram");
                strcpy_s(action_info.localizedActionName, "Place Hologram");
                action_info.countSubactionPaths = (uint32_t)_subaction_paths.size();
                action_info.subactionPaths = _subaction_paths.data();
                CHECK_XRCMD(xrCreateAction(_action_set.Get(), &action_info, _action_place.Put()));
            }

            // Create an input action getting the left and right hand poses.
            {
                XrActionCreateInfo action_info{XR_TYPE_ACTION_CREATE_INFO};
                action_info.actionType = XR_ACTION_TYPE_POSE_INPUT;
                strcpy_s(action_info.actionName, "hand_pose");
                strcpy_s(action_info.localizedActionName, "Hand Pose");
                action_info.countSubactionPaths = (uint32_t)_subaction_paths.size();
                action_info.subactionPaths = _subaction_paths.data();
                CHECK_XRCMD(xrCreateAction(_action_set.Get(), &action_info, _action_pose.Put()));
            }

            // Create an output action for vibrating the left and right controller.
            {
                XrActionCreateInfo action_info{XR_TYPE_ACTION_CREATE_INFO};
                action_info.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
                strcpy_s(action_info.actionName, "vibrate");
                strcpy_s(action_info.localizedActionName, "Vibrate");
                action_info.countSubactionPaths = (uint32_t)_subaction_paths.size();
                action_info.subactionPaths = _subaction_paths.data();
                CHECK_XRCMD(xrCreateAction(_action_set.Get(), &action_info, _action_vibrate.Put()));
            }

            // Create an input action to exit the session.
            {
                XrActionCreateInfo action_info{XR_TYPE_ACTION_CREATE_INFO};
                action_info.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
                strcpy_s(action_info.actionName, "exit_session");
                strcpy_s(action_info.localizedActionName, "Exit session");
                action_info.countSubactionPaths = (uint32_t)_subaction_paths.size();
                action_info.subactionPaths = _subaction_paths.data();
                CHECK_XRCMD(xrCreateAction(_action_set.Get(), &action_info, _action_exit.Put()));
            }
        }

        // Set up suggested bindings for the simple_controller profile.
        {
            std::vector<XrActionSuggestedBinding> bindings;
            bindings.push_back(
                {_action_place.Get(), GetXrPath("/user/hand/right/input/select/click")});
            bindings.push_back(
                {_action_place.Get(), GetXrPath("/user/hand/left/input/select/click")});
            bindings.push_back({_action_pose.Get(), GetXrPath("/user/hand/right/input/grip/pose")});
            bindings.push_back({_action_pose.Get(), GetXrPath("/user/hand/left/input/grip/pose")});
            bindings.push_back(
                {_action_vibrate.Get(), GetXrPath("/user/hand/right/output/haptic")});
            bindings.push_back({_action_vibrate.Get(), GetXrPath("/user/hand/left/output/haptic")});
            bindings.push_back(
                {_action_exit.Get(), GetXrPath("/user/hand/right/input/menu/click")});
            bindings.push_back({_action_exit.Get(), GetXrPath("/user/hand/left/input/menu/click")});

            XrInteractionProfileSuggestedBinding suggestedBindings{
                XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile =
                GetXrPath("/interaction_profiles/khr/simple_controller");
            suggestedBindings.suggestedBindings = bindings.data();
            suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
            CHECK_XRCMD(xrSuggestInteractionProfileBindings(_instance.Get(), &suggestedBindings));
        }
    }

    void InitializeSystem()
    {
        CHECK(_instance.Get() != XR_NULL_HANDLE);
        CHECK(_system_id == XR_NULL_SYSTEM_ID);

        XrSystemGetInfo system_info{XR_TYPE_SYSTEM_GET_INFO};
        system_info.formFactor = _form_factor;
        while (true)
        {
            XrResult result = xrGetSystem(_instance.Get(), &system_info, &_system_id);
            if (SUCCEEDED(result))
            {
                break;
            }
            else if (result == XR_ERROR_FORM_FACTOR_UNAVAILABLE)
            {
                DEBUG_PRINT("No headset detected.  Trying again in one second...");
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1s);
            }
            else
            {
                CHECK_XRRESULT(result, "xrGetSystem");
            }
        };

        // Choose an environment blend mode.
        {
            // Query the list of supported environment blend modes for the current system.
            uint32_t count;
            CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(
                _instance.Get(), _system_id, _primary_view_config_type, 0, &count, nullptr));
            CHECK(count > 0); // A system must support at least one environment blend mode.

            std::vector<XrEnvironmentBlendMode> environmentBlendModes(count);
            CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(_instance.Get(), _system_id,
                                                         _primary_view_config_type, count, &count,
                                                         environmentBlendModes.data()));

            // This sample supports all modes, pick the system's preferred one.
            _environment_blend_mode = environmentBlendModes[0];
        }

        // Choosing a reasonable depth range can help improve hologram visual quality.
        // Use reversed-Z (near > far) for more uniform Z resolution.
        _near_far = {20.f, 0.1f};
    }

    void InitializeSession()
    {
        CHECK(_instance.Get() != XR_NULL_HANDLE);
        CHECK(_system_id != XR_NULL_SYSTEM_ID);

        CHECK(_session.Get() == XR_NULL_HANDLE);

        // Create the D3D11 device for the adapter associated with the system.
        XrGraphicsRequirementsD3D11KHR graphics_requirements{
            XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
        CHECK_XRCMD(_extensions.xrGetD3D11GraphicsRequirementsKHR(_instance.Get(), _system_id,
                                                                  &graphics_requirements));

        // Create a list of feature levels which are both supported by the OpenXR runtime and this
        // application.
        std::vector<D3D_FEATURE_LEVEL> feature_levels = {
            D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
        feature_levels.erase(std::remove_if(feature_levels.begin(), feature_levels.end(),
                                            [&](D3D_FEATURE_LEVEL fl) {
                                                return fl < graphics_requirements.minFeatureLevel;
                                            }),
                             feature_levels.end());
        CHECK_MSG(feature_levels.size() != 0, "Unsupported minimum feature level!");

        _render_system.Initialize(graphics_requirements.adapterLuid, feature_levels);
        ID3D11Device * device = _render_system.Device().get();

        XrGraphicsBindingD3D11KHR graphics_binding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
        graphics_binding.device = device;

        XrSessionCreateInfo create_info{XR_TYPE_SESSION_CREATE_INFO};
        create_info.next = &graphics_binding;
        create_info.systemId = _system_id;
        CHECK_XRCMD(xrCreateSession(_instance.Get(), &create_info, _session.Put()));

        XrSessionActionSetsAttachInfo attach_info{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        std::vector<XrActionSet> action_sets = {_action_set.Get()};
        attach_info.countActionSets = (uint32_t)action_sets.size();
        attach_info.actionSets = action_sets.data();
        CHECK_XRCMD(xrAttachSessionActionSets(_session.Get(), &attach_info));

        CreateSpaces();
        CreateSwapchains();
    }

    void CreateSpaces()
    {
        CHECK(_session.Get() != XR_NULL_HANDLE);

        // Create a app space to bridge interactions and all holograms.
        {
            if (_optional_extensions.UnboundedRefSpaceSupported)
            {
                // Unbounded reference space provides the best app space for world-scale
                // experiences.
                _app_space_type = XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;
            }
            else
            {
                // If running on a platform that does not support world-scale experiences, fall back
                // to local space.
                _app_space_type = XR_REFERENCE_SPACE_TYPE_LOCAL;
            }

            XrReferenceSpaceCreateInfo create_info{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
            create_info.referenceSpaceType = _app_space_type;
            create_info.poseInReferenceSpace = xr::math::Pose::Identity();
            CHECK_XRCMD(xrCreateReferenceSpace(_session.Get(), &create_info, _app_space.Put()));
        }

        // Create a space for each hand pointer pose.
        for (uint32_t side : {LeftSide, RightSide})
        {
            XrActionSpaceCreateInfo create_info{XR_TYPE_ACTION_SPACE_CREATE_INFO};
            create_info.action = _action_pose.Get();
            create_info.poseInActionSpace = xr::math::Pose::Identity();
            create_info.subactionPath = _subaction_paths[side];
            CHECK_XRCMD(
                xrCreateActionSpace(_session.Get(), &create_info, _hands[side].Space.Put()));
        }
    }

    std::tuple<DXGI_FORMAT, DXGI_FORMAT> SelectSwapchainPixelFormats()
    {
        CHECK(_session.Get() != XR_NULL_HANDLE);

        // Query the runtime's preferred swapchain formats.
        uint32_t swapchain_format_count;
        CHECK_XRCMD(
            xrEnumerateSwapchainFormats(_session.Get(), 0, &swapchain_format_count, nullptr));

        std::vector<int64_t> swapchain_formats(swapchain_format_count);
        CHECK_XRCMD(xrEnumerateSwapchainFormats(_session.Get(), (uint32_t)swapchain_formats.size(),
                                                &swapchain_format_count, swapchain_formats.data()));

        // Choose the first runtime-preferred format that this app supports.
        auto SelectPixelFormat =
            [](const std::vector<int64_t> & runtime_preferred_formats,
               const std::vector<DXGI_FORMAT> & application_supported_formats) {
                auto found = std::find_first_of(std::begin(runtime_preferred_formats),
                                                std::end(runtime_preferred_formats),
                                                std::begin(application_supported_formats),
                                                std::end(application_supported_formats));
                if (found == std::end(runtime_preferred_formats))
                {
                    THROW("No runtime swapchain format is supported.");
                }
                return (DXGI_FORMAT)*found;
            };

        DXGI_FORMAT color_swapchain_format =
            SelectPixelFormat(swapchain_formats, _render_system.SupportedColorFormats());
        DXGI_FORMAT depth_swapchain_format =
            SelectPixelFormat(swapchain_formats, _render_system.SupportedDepthFormats());

        return {color_swapchain_format, depth_swapchain_format};
    }

    void CreateSwapchains()
    {
        CHECK(_session.Get() != XR_NULL_HANDLE);
        CHECK(_render_resources == nullptr);

        _render_resources = std::make_unique<RenderResources>();

        // Read graphics properties for preferred swapchain length and logging.
        XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
        CHECK_XRCMD(xrGetSystemProperties(_instance.Get(), _system_id, &systemProperties));

        // Select color and depth swapchain pixel formats.
        const auto [colorSwapchainFormat, depth_swapchain_format] = SelectSwapchainPixelFormats();

        // Query and cache view configuration views.
        uint32_t view_count;
        CHECK_XRCMD(xrEnumerateViewConfigurationViews(
            _instance.Get(), _system_id, _primary_view_config_type, 0, &view_count, nullptr));
        CHECK(view_count == _stereo_view_count);

        _render_resources->ConfigViews.resize(view_count, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        CHECK_XRCMD(xrEnumerateViewConfigurationViews(
            _instance.Get(), _system_id, _primary_view_config_type, view_count, &view_count,
            _render_resources->ConfigViews.data()));

        // Using texture array for better performance, so requiring left/right views have identical
        // sizes.
        const XrViewConfigurationView & view = _render_resources->ConfigViews[0];
        CHECK(_render_resources->ConfigViews[0].recommendedImageRectWidth ==
              _render_resources->ConfigViews[1].recommendedImageRectWidth);
        CHECK(_render_resources->ConfigViews[0].recommendedImageRectHeight ==
              _render_resources->ConfigViews[1].recommendedImageRectHeight);
        CHECK(_render_resources->ConfigViews[0].recommendedSwapchainSampleCount ==
              _render_resources->ConfigViews[1].recommendedSwapchainSampleCount);

        // Use the system's recommended rendering parameters.
        const uint32_t image_rect_width = view.recommendedImageRectWidth;
        const uint32_t image_rect_height = view.recommendedImageRectHeight;
        const uint32_t swapchain_sample_count = view.recommendedSwapchainSampleCount;

        // Create swapchains with texture array for color and depth images.
        // The texture array has the size of viewCount, and they are rendered in a single pass using
        // VPRT.
        const uint32_t texture_array_size = view_count;
        _render_resources->ColorSwapchain = CreateSwapchainD3D11(
            _session.Get(), colorSwapchainFormat, image_rect_width, image_rect_height,
            texture_array_size, swapchain_sample_count, 0 /*createFlags*/,
            XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT);

        _render_resources->DepthSwapchain = CreateSwapchainD3D11(
            _session.Get(), depth_swapchain_format, image_rect_width, image_rect_height,
            texture_array_size, swapchain_sample_count, 0 /*createFlags*/,
            XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

        // Preallocate view buffers for xrLocateViews later inside frame loop.
        _render_resources->Views.resize(view_count, {XR_TYPE_VIEW});
    }

    struct SwapchainD3D11;
    SwapchainD3D11 CreateSwapchainD3D11(XrSession session, DXGI_FORMAT format, uint32_t width,
                                        uint32_t height, uint32_t array_size, uint32_t sample_count,
                                        XrSwapchainCreateFlags create_flags,
                                        XrSwapchainUsageFlags usage_flags)
    {
        SwapchainD3D11 swapchain;
        swapchain.Format = format;
        swapchain.Width = width;
        swapchain.Height = height;
        swapchain.ArraySize = array_size;

        XrSwapchainCreateInfo swapchain_create_info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapchain_create_info.arraySize = array_size;
        swapchain_create_info.format = format;
        swapchain_create_info.width = width;
        swapchain_create_info.height = height;
        swapchain_create_info.mipCount = 1;
        swapchain_create_info.faceCount = 1;
        swapchain_create_info.sampleCount = sample_count;
        swapchain_create_info.createFlags = create_flags;
        swapchain_create_info.usageFlags = usage_flags;

        CHECK_XRCMD(xrCreateSwapchain(session, &swapchain_create_info, swapchain.Handle.Put()));

        uint32_t chainLength;
        CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.Handle.Get(), 0, &chainLength, nullptr));

        swapchain.Images.resize(chainLength, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
        CHECK_XRCMD(xrEnumerateSwapchainImages(
            swapchain.Handle.Get(), (uint32_t)swapchain.Images.size(), &chainLength,
            reinterpret_cast<XrSwapchainImageBaseHeader *>(swapchain.Images.data())));

        return swapchain;
    }

    void ProcessEvents(bool * exit_render_loop, bool * request_restart)
    {
        *exit_render_loop = *request_restart = false;

        auto poll_event = [&](XrEventDataBuffer & event_data) -> bool {
            event_data.type = XR_TYPE_EVENT_DATA_BUFFER;
            event_data.next = nullptr;
            return CHECK_XRCMD(xrPollEvent(_instance.Get(), &event_data)) == XR_SUCCESS;
        };

        XrEventDataBuffer event_data{};
        while (poll_event(event_data))
        {
            switch (event_data.type)
            {
                case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                    *exit_render_loop = true;
                    *request_restart = false;
                    return;
                }
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                    const auto state_event =
                        *reinterpret_cast<const XrEventDataSessionStateChanged *>(&event_data);
                    CHECK(_session.Get() != XR_NULL_HANDLE &&
                          _session.Get() == state_event.session);
                    _session_state = state_event.state;
                    switch (_session_state)
                    {
                        case XR_SESSION_STATE_READY: {
                            CHECK(_session.Get() != XR_NULL_HANDLE);
                            XrSessionBeginInfo session_begin_info{XR_TYPE_SESSION_BEGIN_INFO};
                            session_begin_info.primaryViewConfigurationType =
                                _primary_view_config_type;
                            CHECK_XRCMD(xrBeginSession(_session.Get(), &session_begin_info));
                            _session_running = true;
                            break;
                        }
                        case XR_SESSION_STATE_STOPPING: {
                            _session_running = false;
                            CHECK_XRCMD(xrEndSession(_session.Get()));
                            break;
                        }
                        case XR_SESSION_STATE_EXITING: {
                            // Do not attempt to restart, because user closed this session.
                            *exit_render_loop = true;
                            *request_restart = false;
                            break;
                        }
                        case XR_SESSION_STATE_LOSS_PENDING: {
                            // Session was lost, so start over and poll for new systemId.
                            *exit_render_loop = true;
                            *request_restart = true;
                            break;
                        }
                    }
                    break;
                }
                case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                default: {
                    DEBUG_PRINT("Ignoring event type %d", event_data.type);
                    break;
                }
            }
        }
    }

    void PollActions()
    {
        // Get updated action states.
        std::vector<XrActiveActionSet> active_action_sets = {{_action_set.Get(), XR_NULL_PATH}};
        XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
        syncInfo.countActiveActionSets = (uint32_t)active_action_sets.size();
        syncInfo.activeActionSets = active_action_sets.data();
        CHECK_XRCMD(xrSyncActions(_session.Get(), &syncInfo));

        // Check the state of the actions for left and right hands separately.
        for (uint32_t side : {LeftSide, RightSide})
        {
            const XrPath subactionPath = _subaction_paths[side];

            // Apply a tiny vibration to the corresponding hand to indicate that action is detected.
            auto ApplyVibration = [this, subactionPath] {
                XrHapticActionInfo action_info{XR_TYPE_HAPTIC_ACTION_INFO};
                action_info.action = _action_vibrate.Get();
                action_info.subactionPath = subactionPath;

                XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
                vibration.amplitude = 0.5f;
                vibration.duration = XR_MIN_HAPTIC_DURATION;
                vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
                CHECK_XRCMD(xrApplyHapticFeedback(_session.Get(), &action_info,
                                                  (XrHapticBaseHeader *)&vibration));
            };

            XrActionStateBoolean place_action_value{XR_TYPE_ACTION_STATE_BOOLEAN};
            {
                XrActionStateGetInfo get_info{XR_TYPE_ACTION_STATE_GET_INFO};
                get_info.action = _action_place.Get();
                get_info.subactionPath = subactionPath;
                CHECK_XRCMD(
                    xrGetActionStateBoolean(_session.Get(), &get_info, &place_action_value));
            }

            // When select button is pressed, place the cube at the location of the corresponding
            // hand.
            if (place_action_value.isActive && place_action_value.changedSinceLastSync &&
                place_action_value.currentState)
            {
                // Use the pose at the historical time when the action happened to do the placement.
                const XrTime placement_time = place_action_value.lastChangeTime;

                // Locate the hand in the scene.
                XrSpaceLocation hand_location{XR_TYPE_SPACE_LOCATION};
                CHECK_XRCMD(xrLocateSpace(_hands[side].Space.Get(), _app_space.Get(),
                                          placement_time, &hand_location));

                // Ensure we have tracking before placing a cube in the scene, so that it stays
                // reliably at a physical location.
                if (!xr::math::Pose::IsPoseValid(hand_location))
                {
                    DEBUG_PRINT("Cube cannot be placed when positional tracking is lost.");
                }
                else
                {
                    // INPUT_TAP
                    _animate = !_animate;
                    _target_hand = side;
                }

                ApplyVibration();
            }

            // This sample, when menu button is released, requests to quit the session, and
            // therefore quit the application.
            {
                XrActionStateBoolean exitActionValue{XR_TYPE_ACTION_STATE_BOOLEAN};
                XrActionStateGetInfo get_info{XR_TYPE_ACTION_STATE_GET_INFO};
                get_info.action = _action_exit.Get();
                get_info.subactionPath = subactionPath;
                CHECK_XRCMD(xrGetActionStateBoolean(_session.Get(), &get_info, &exitActionValue));

                if (exitActionValue.isActive && exitActionValue.changedSinceLastSync &&
                    !exitActionValue.currentState)
                {
                    CHECK_XRCMD(xrRequestExitSession(_session.Get()));
                    ApplyVibration();
                }
            }
        }
    }

    void RenderFrame()
    {
        CHECK(_session.Get() != XR_NULL_HANDLE);

        XrFrameWaitInfo frame_wait_info{XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState frame_state{XR_TYPE_FRAME_STATE};
        CHECK_XRCMD(xrWaitFrame(_session.Get(), &frame_wait_info, &frame_state));

        UpdateScene(frame_state);

        XrFrameBeginInfo frame_begin_info{XR_TYPE_FRAME_BEGIN_INFO};
        CHECK_XRCMD(xrBeginFrame(_session.Get(), &frame_begin_info));

        // xrEndFrame can submit multiple layers. This sample submits one.
        std::vector<XrCompositionLayerBaseHeader *> layers;

        // The projection layer consists of projection layer views.
        XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};

        // Inform the runtime that the app's submitted alpha channel has valid data for use during
        // composition. The primary display on HoloLens has an additive environment blend mode. It
        // will ignore the alpha channel. However, mixed reality capture uses the alpha channel if
        // this bit is set to blend content with the environment.
        layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;

        // Only render when session is visible, otherwise submit zero layers.
        if (frame_state.shouldRender)
        {
            // First update the viewState and views using latest predicted display time.
            {
                XrViewLocateInfo view_locate_info{XR_TYPE_VIEW_LOCATE_INFO};
                view_locate_info.viewConfigurationType = _primary_view_config_type;
                view_locate_info.displayTime = frame_state.predictedDisplayTime;
                view_locate_info.space = _app_space.Get();

                // The output view count of xrLocateViews is always same as
                // xrEnumerateViewConfigurationViews. Therefore, Views can be preallocated and avoid
                // two call idiom here.
                uint32_t view_capacity_input = (uint32_t)_render_resources->Views.size();
                uint32_t view_count_output;
                CHECK_XRCMD(xrLocateViews(_session.Get(), &view_locate_info,
                                          &_render_resources->ViewState, view_capacity_input,
                                          &view_count_output, _render_resources->Views.data()));

                CHECK(view_count_output == view_capacity_input);
                CHECK(view_count_output == _render_resources->ConfigViews.size());
                CHECK(view_count_output == _render_resources->ColorSwapchain.ArraySize);
                CHECK(view_count_output == _render_resources->DepthSwapchain.ArraySize);
            }

            // Then, render projection layer into each view.
            if (RenderLayer(frame_state.predictedDisplayTime, layer))
            {
                layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&layer));
            }
        }

        // Submit the composition layers for the predicted display time.
        XrFrameEndInfo frame_end_info{XR_TYPE_FRAME_END_INFO};
        frame_end_info.displayTime = frame_state.predictedDisplayTime;
        frame_end_info.environmentBlendMode = _environment_blend_mode;
        frame_end_info.layerCount = (uint32_t)layers.size();
        frame_end_info.layers = layers.data();
        CHECK_XRCMD(xrEndFrame(_session.Get(), &frame_end_info));
    }

    uint32_t AcquireAndWaitForSwapchainImage(XrSwapchain handle)
    {
        uint32_t swapchainImageIndex;
        XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        CHECK_XRCMD(xrAcquireSwapchainImage(handle, &acquireInfo, &swapchainImageIndex));

        XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;
        CHECK_XRCMD(xrWaitSwapchainImage(handle, &waitInfo));

        return swapchainImageIndex;
    }

    void InitializeSpinningCube(XrTime predictedDisplayTime)
    {
        auto createReferenceSpace = [session =
                                         _session.Get()](XrReferenceSpaceType referenceSpaceType,
                                                         XrPosef poseInReferenceSpace) {
            xr::SpaceHandle space;
            XrReferenceSpaceCreateInfo create_info{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
            create_info.referenceSpaceType = referenceSpaceType;
            create_info.poseInReferenceSpace = poseInReferenceSpace;
            CHECK_XRCMD(xrCreateReferenceSpace(session, &create_info, space.Put()));
            return space;
        };
    }

    bool RenderLayer(XrTime predicted_display_time, XrCompositionLayerProjection & layer)
    {
        const uint32_t view_count = (uint32_t)_render_resources->ConfigViews.size();

        if (!xr::math::Pose::IsPoseValid(_render_resources->ViewState))
        {
            DEBUG_PRINT("xrLocateViews returned an invalid pose.");
            return false; // Skip rendering layers if view location is invalid
        }

        // Update holograms ?
        _render_resources->ProjectionLayerViews.resize(view_count);
        if (_optional_extensions.DepthExtensionSupported)
        {
            _render_resources->DepthInfoViews.resize(view_count);
        }

        // Swapchain is acquired, rendered to, and released together for all views as texture array
        const SwapchainD3D11 & color_swapchain = _render_resources->ColorSwapchain;
        const SwapchainD3D11 & depth_swapchain = _render_resources->DepthSwapchain;

        // Use the full size of the allocated swapchain image (could render smaller some frames to
        // hit framerate)
        const XrRect2Di imageRect = {
            {0, 0}, {(int32_t)color_swapchain.Width, (int32_t)color_swapchain.Height}};
        CHECK(color_swapchain.Width == depth_swapchain.Width);
        CHECK(color_swapchain.Height == depth_swapchain.Height);

        const uint32_t color_swapchain_image_index =
            AcquireAndWaitForSwapchainImage(color_swapchain.Handle.Get());
        const uint32_t depth_swapchain_image_index =
            AcquireAndWaitForSwapchainImage(depth_swapchain.Handle.Get());

        // Prepare rendering parameters of each view for swapchain texture arrays
        std::vector<xr::math::ViewProjection> viewProjections(view_count);
        for (uint32_t i = 0; i < view_count; i++)
        {
            viewProjections[i] = {_render_resources->Views[i].pose, _render_resources->Views[i].fov,
                                  _near_far};

            _render_resources->ProjectionLayerViews[i] = {
                XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
            _render_resources->ProjectionLayerViews[i].pose = _render_resources->Views[i].pose;
            _render_resources->ProjectionLayerViews[i].fov = _render_resources->Views[i].fov;
            _render_resources->ProjectionLayerViews[i].subImage.swapchain =
                color_swapchain.Handle.Get();
            _render_resources->ProjectionLayerViews[i].subImage.imageRect = imageRect;
            _render_resources->ProjectionLayerViews[i].subImage.imageArrayIndex = i;

            if (_optional_extensions.DepthExtensionSupported)
            {
                _render_resources->DepthInfoViews[i] = {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
                _render_resources->DepthInfoViews[i].minDepth = 0;
                _render_resources->DepthInfoViews[i].maxDepth = 1;
                _render_resources->DepthInfoViews[i].nearZ = _near_far.Near;
                _render_resources->DepthInfoViews[i].farZ = _near_far.Far;
                _render_resources->DepthInfoViews[i].subImage.swapchain =
                    depth_swapchain.Handle.Get();
                _render_resources->DepthInfoViews[i].subImage.imageRect = imageRect;
                _render_resources->DepthInfoViews[i].subImage.imageArrayIndex = i;

                // Chain depth info struct to the corresponding projection layer view's next pointer
                _render_resources->ProjectionLayerViews[i].next =
                    &_render_resources->DepthInfoViews[i];
            }
        }

        // For HoloLens additive display, best to clear render target with transparent black color
        // (0,0,0,0)
        constexpr DirectX::XMVECTORF32 opaqueColor = {0.184313729f, 0.309803933f, 0.309803933f,
                                                      1.000000000f};
        constexpr DirectX::XMVECTORF32 transparent = {0.000000000f, 0.000000000f, 0.000000000f,
                                                      0.000000000f};
        const DirectX::XMVECTORF32 renderTargetClearColor =
            (_environment_blend_mode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE) ? opaqueColor
                                                                          : transparent;
        _render_system.RenderXR(
            imageRect, renderTargetClearColor, viewProjections, color_swapchain.Format,
            color_swapchain.Images[color_swapchain_image_index].texture, depth_swapchain.Format,
            depth_swapchain.Images[depth_swapchain_image_index].texture, _entity_manager,
            _component_manager);

        XrSwapchainImageReleaseInfo release_info{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        CHECK_XRCMD(xrReleaseSwapchainImage(color_swapchain.Handle.Get(), &release_info));
        CHECK_XRCMD(xrReleaseSwapchainImage(depth_swapchain.Handle.Get(), &release_info));

        layer.space = _app_space.Get();
        layer.viewCount = (uint32_t)_render_resources->ProjectionLayerViews.size();
        layer.views = _render_resources->ProjectionLayerViews.data();
        return true;
    }

    void PrepareSessionRestart()
    {
        // TODO: Reset scene
        _render_resources.reset();
        _session.Reset();
        _system_id = XR_NULL_SYSTEM_ID;
    }

    constexpr bool IsSessionFocused() const { return _session_state == XR_SESSION_STATE_FOCUSED; }

    XrPath GetXrPath(const char * string) const
    {
        return xr::StringToPath(_instance.Get(), string);
    }
};
} // namespace _implementation

using _implementation::HololensGame;

} // namespace My

namespace My::Engine
{
std::unique_ptr<Game> CreateGame() { return std::make_unique<HololensGame>(); }
} // namespace My::Engine
