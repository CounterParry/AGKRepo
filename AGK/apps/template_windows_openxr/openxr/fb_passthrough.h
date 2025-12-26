#pragma once
#include <openxr/openxr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XR_FB_PASSTHROUGH_EXTENSION_NAME "XR_FB_passthrough"

// Function pointer typedefs only
typedef XrResult (XRAPI_PTR *PFN_xrCreatePassthroughFB)(
    XrSession session,
    const XrPassthroughCreateInfoFB* createInfo,
    XrPassthroughFB* outPassthrough);

typedef XrResult (XRAPI_PTR *PFN_xrDestroyPassthroughFB)(
    XrPassthroughFB passthrough);

typedef XrResult (XRAPI_PTR *PFN_xrPassthroughStartFB)(
    XrPassthroughFB passthrough);

typedef XrResult (XRAPI_PTR *PFN_xrCreatePassthroughLayerFB)(
    XrSession session,
    const XrPassthroughLayerCreateInfoFB* createInfo,
    XrPassthroughLayerFB* outLayer);

typedef XrResult (XRAPI_PTR *PFN_xrDestroyPassthroughLayerFB)(
    XrPassthroughLayerFB layer);

#ifdef __cplusplus
}
#endif
