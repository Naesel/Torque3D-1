//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "console/engineAPI.h"
#include "platform/input/openVR/openVRProvider.h"
#include "platform/input/openVR/openVRStageModelData.h"
#include "T3D/gameBase/gameConnection.h"
#include "math/mTransform.h"
#include "gfx/sim/cubemapData.h"

DECLARE_SCOPE(OpenVR);
IMPLEMENT_SCOPE(OpenVR, OpenVRProvider, , "");
ConsoleDoc(
   "@class OpenVRProvider\n"
   "@brief This class is the interface between TorqueScript and OpenVR.\n\n"
   "@ingroup OpenVR\n"
);

// Enum impls
ImplementEnumType(OpenVRTrackingResult,
   "\n\n"
   "@ingroup OpenVR")
{ vr::TrackingResult_Uninitialized, "None" },
{ vr::TrackingResult_Calibrating_InProgress, "Calibrating_InProgress" },
{ vr::TrackingResult_Calibrating_OutOfRange, "Calibrating_OutOfRange" },
{ vr::TrackingResult_Running_OK, "Running_Ok" },
{ vr::TrackingResult_Running_OutOfRange, "Running_OutOfRange" },
{ vr::TrackingResult_Fallback_RotationOnly, "Fallback_RotationOnly" },
EndImplementEnumType;

ImplementEnumType(OpenVRTrackingUniverseOrigin,
   "Identifies which style of tracking origin the application wants to use for the poses it is requesting.\n\n"
   "@ingroup OpenVR")
{ vr::TrackingUniverseSeated, "Seated", "Poses are provided relative to the seated zero pose" },
{ vr::TrackingUniverseStanding, "Standing", "Poses are provided relative to the safe bounds configured by the user" },
{ vr::TrackingUniverseRawAndUncalibrated, "RawAndUncalibrated", "Poses are provided in the coordinate system defined by the driver.  It has Y up and is unified for devices of the same driver. You usually don't want this one."},
EndImplementEnumType;

ImplementEnumType(OpenVRState,
   "Status of the overall system or tracked objects. .\n\n"
   "@ingroup OpenVR")
{ vr::VRState_Undefined, "Undefined" },
{ vr::VRState_Off, "Off" },
{ vr::VRState_Searching, "Searching" },
{ vr::VRState_Searching_Alert, "Searching_Alert" },
{ vr::VRState_Ready, "Ready" },
{ vr::VRState_Ready_Alert, "Ready_Alert" },
{ vr::VRState_NotReady, "NotReady" },
{ vr::VRState_Standby, "Standby" },
{ vr::VRState_Ready_Alert_Low, "Ready_Alert_Low" },
EndImplementEnumType;

ImplementEnumType(OpenVRTrackedDeviceClass,
   "Types of devices which are tracked .\n\n"
   "@ingroup OpenVR")
{ vr::TrackedDeviceClass_Invalid, "Invalid", "The ID was not valid" },
{ vr::TrackedDeviceClass_HMD, "HMD", "Head-Mounted Displays" },
{ vr::TrackedDeviceClass_Controller, "Controller", "Tracked controllers" },
{ vr::TrackedDeviceClass_GenericTracker, "GenericTracker", "Generic trackers, similar to controllers" },
{ vr::TrackedDeviceClass_TrackingReference, "TrackingReference", "Camera and base stations that serve as tracking reference points" },
{ vr::TrackedDeviceClass_DisplayRedirect, "Other", "Accessories that aren't necessarily tracked themselves, but may redirect video output from other tracked devices" },
EndImplementEnumType;

ImplementEnumType(OpenVRControllerAxisType,
   "Types of controller axes.\n\n"
   "@ingroup OpenVR")
{ vr::k_eControllerAxis_None, "None" },
{ vr::k_eControllerAxis_TrackPad, "TrackPad" },
{ vr::k_eControllerAxis_Joystick, "Joystick" },
{ vr::k_eControllerAxis_Trigger, "Trigger" },
EndImplementEnumType;

ImplementEnumType(OpenVRTrackedControllerRole,
   "Describes the specific role associated with a tracked device.\n\n"
   "@ingroup OpenVR")
{ vr::TrackedControllerRole_Invalid, "Invalid", "Invalid value for controller type" },
{ vr::TrackedControllerRole_LeftHand, "LeftHand", "Tracked device associated with the left hand"},
{ vr::TrackedControllerRole_RightHand, "RightHand", "Tracked device associated with the right hand"},
{ vr::TrackedControllerRole_OptOut, "OptOut", "Tracked device is opting out of left/right hand selection"},
{ vr::TrackedControllerRole_Treadmill, "Treadmill", "Tracked device is a treadmill or other locomotion device"},
{ vr::TrackedControllerRole_Stylus, "Stylus", "Tracked device is a stylus"},
EndImplementEnumType;

ImplementEnumType(OpenVRActionType,
   "Input action types that can be mapped by IVRInput.\n\n"
   "@ingroup OpenVR")
{ OpenVRActionType_Digital, "Digital" },
{ OpenVRActionType_Analog, "Analog" },
{ OpenVRActionType_Pose, "Pose" },
{ OpenVRActionType_Skeleton, "Skeleton" },
EndImplementEnumType;

DefineEngineStaticMethod(OpenVR, isHmdPresent, bool, (), ,
   "Returns true if there is an HMD attached. This check is as lightweight as possible and "
   "can be called outside of VR_Init/VR_Shutdown (OpenVR::setEnabled(true/false). It should "
   "be used when an application wants to know if initializing VR is a possibility but isn't "
   "ready to take that step yet.\n"
   "@ingroup OpenVR")
{
   return vr::VR_IsHmdPresent();
}

DefineEngineStaticMethod(OpenVR, getControllerModel, const char*, (S32 idx), ,
   "@brief Returns the model name for the device at the passed index.\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return "";
   }

   return OPENVR->getControllerModel(idx);
}

DefineEngineStaticMethod(OpenVR, isDeviceActive, bool, (), ,
   "@brief Used to determine if the OpenVR input device is active\n\n"

   "The OpenVR device is considered active when the library has been "
   "initialized and either a real of simulated HMD is present.\n\n"
   "@return True if the OpenVR input device is active.\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return false;
   }

   return OPENVR->getActive();
}


DefineEngineStaticMethod(OpenVR, setEnabled, bool, (bool value), ,
   "@brief Enable or disable OpenVR\n\n"
   "Enabling will initialize the vr interfaces, load the action manifest and begin "
   "polling for input. Disabling will stop input polling and call vr::VR_Shutdown().\n\n"
   "@return True if the OpenVR is enabled. False if it is disabled or there was an error enabling.\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return false;
   }

   return value ? OPENVR->enable() : OPENVR->disable();
}


DefineEngineStaticMethod(OpenVR, setHMDAsGameConnectionDisplayDevice, bool, (GameConnection* conn), ,
   "@brief Sets the first HMD to be a GameConnection's display device\n\n"
   "@param conn The GameConnection to set.\n"
   "@return True if the GameConnection display device was set.\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      Con::errorf("OpenVR::setHMDAsGameConnectionDisplayDevice(): No Open VR Device present.");
      return false;
   }

   if (!conn)
   {
      Con::errorf("OpenVR::setHMDAsGameConnectionDisplayDevice(): Invalid GameConnection.");
      return false;
   }

   conn->setDisplayDevice(OPENVR);
   return true;
}

DefineEngineStaticMethod(OpenVR, setRoomTracking, void, (bool roomTracking), (true),
   "@brief Sets the tracking universe for OpenVR\n\n"
   "If room tracking is true, the standing tracking universe is used and "
   "poses are provided relative to the safe bounds configured by the user.\n"
   "If room tracking is false, the seated tracking universe is used and "
   "Poses are provided relative to the seated zero pose.\n\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return;
   }

   OPENVR->setRoomTracking(roomTracking);
}

DefineEngineFunction(OpenVRIsCompiledIn, bool, (), , "")
{
   return true;
}

DefineEngineStaticMethod(OpenVR, orientUniverse, void, (TransformF txfm), ,
   "Sets the yaw of the tracking universe in the T3D world. "
   "Pitch and roll from the passed transform are ignored.\n"
   "@param txfm object transform to set.\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return;
   }

   if ( !txfm.hasRotation() )
      OPENVR->rotateUniverse(0.0f);
   else
      OPENVR->orientUniverse(txfm.getMatrix());
}

DefineEngineStaticMethod(OpenVR, rotateUniverse, void, (F32 yaw), (0.0f),
   "Sets the yaw of the tracking universe in the T3D world.\n"
   "@param yaw Tracking universe rotation about the z axis in radians.\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return;
   }

   OPENVR->rotateUniverse(yaw);
}

DefineEngineStaticMethod(OpenVR, isSteamVRDrawingControllers, bool, (), ,
   "Returns true if SteamVR is drawing controllers on top of the application. Applications "
   "should consider not drawing anything attached to the user's hands in this case.\n"
   "@ingroup OpenVR")
{
   return vr::VRSystem() && vr::VRSystem()->IsSteamVRDrawingControllers();
}

DefineEngineStaticMethod(OpenVR, getDevicePropertyString, String, (U32 deviceIdx, U32 propID), ,
   "Returns a device property string value.\n"
   "@param deviceIdx device to read property value for.\n"
   "@param propID The property id value from vr::ETrackedDeviceProperty. "
   "See: https://github.com/ValveSoftware/openvr/blob/ebdea152f8aac77e9a6db29682b81d762159df7e/headers/openvr.h#L229\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return String::EmptyString;
   }
   return OPENVR->getDevicePropertyString(deviceIdx, propID);
}

DefineEngineStaticMethod(OpenVR, getDevicePropertyBool, bool, (U32 deviceIdx, U32 propID), ,
   "Returns a device property boolean value.\n"
   "@param deviceIdx device to read property value for.\n"
   "@param propID The property id value from vr::ETrackedDeviceProperty. "
   "See: https://github.com/ValveSoftware/openvr/blob/ebdea152f8aac77e9a6db29682b81d762159df7e/headers/openvr.h#L229\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return false;
   }
   return OPENVR->getDevicePropertyBool(deviceIdx, propID);
}

DefineEngineStaticMethod(OpenVR, getDevicePropertyInt, S32, (U32 deviceIdx, U32 propID), ,
   "Returns a device property int32 value.\n"
   "@param deviceIdx device to read property value for.\n"
   "@param propID The property id value from vr::ETrackedDeviceProperty. "
   "See: https://github.com/ValveSoftware/openvr/blob/ebdea152f8aac77e9a6db29682b81d762159df7e/headers/openvr.h#L229\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return 0;
   }
   return OPENVR->getDevicePropertyInt(deviceIdx, propID);
}

DefineEngineStaticMethod(OpenVR, getDevicePropertyUInt, String, (U32 deviceIdx, U32 propID), ,
   "Returns a device property UInt64 value.\n"
   "@param deviceIdx device to read property value for.\n"
   "@param propID The property id value from vr::ETrackedDeviceProperty. "
   "See: https://github.com/ValveSoftware/openvr/blob/ebdea152f8aac77e9a6db29682b81d762159df7e/headers/openvr.h#L229\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return String::EmptyString;
   }
   return OPENVR->getDevicePropertyUInt(deviceIdx, propID);
}

DefineEngineStaticMethod(OpenVR, getDevicePropertyFloat, F32, (U32 deviceIdx, U32 propID), ,
   "Returns a device property floating point value.\n"
   "@param deviceIdx device to read property value for.\n"
   "@param propID The property id value from vr::ETrackedDeviceProperty. "
   "See: https://github.com/ValveSoftware/openvr/blob/ebdea152f8aac77e9a6db29682b81d762159df7e/headers/openvr.h#L229\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return 0.0f;
   }
   return OPENVR->getDevicePropertyFloat(deviceIdx, propID);
}

DefineEngineStaticMethod(OpenVR, getTrackedDeviceIndices, String, (OpenVRTrackedDeviceClass deviceClass), ,
   "Get a sorted array of device indices of a given class of tracked devices "
   "(e.g. controllers).  Devices are sorted right to left relative to the specified "
   "tracked device.\n"
   "@param deviceClass device class to obtain indices for. One of: HMD, Controller, GenericTracker, TrackingReference or Other.\n"
   "@return A space separated list of device indices.\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return String::EmptyString;
   }
   return OPENVR->getTrackedDeviceIndices(deviceClass);
}

DefineEngineStaticMethod(OpenVR, getDeviceClass, String, (U32 deviceIdx), ,
   "Returns the device class for the openvr device at index deviceIdx.\n"
   "@param deviceIdx device to read property value for.\n"
   "@return One of the OpenVRTrackedDeviceClass enumeration values. "
   "(Invalid, HMD, Controller, GenericTracker, TrackingReference or Other)\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return String::EmptyString;
   }
   return OPENVR->getDeviceClass(deviceIdx);
}

DefineEngineStaticMethod(OpenVR, getControllerAxisType, String, (U32 deviceIdx, U32 axisID), ,
   "Marked depreciated in openvr 1.0.15.\n"
   "Returns an openvr controller axis type.\n"
   "@param deviceIdx device to read property value for.\n"
   "@param axisID ID of the axis..\n"
   "@return One of the OpenVRControllerAxisType enumeration values. "
   "(None, TrackPad, Joystick or Trigger)\n"
   "@ingroup OpenVR")
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return String::EmptyString;
   }
   return OPENVR->getControllerAxisType(deviceIdx, axisID);
}

DefineEngineStaticMethod(OpenVR, setHMDTrackingHeight, void, (F32 hmdHeight), ,
   "Sets the tracking height offset for the hmd. Useful for mapping the standing "
   "tracking space to your character height.\n\n"
   "@param hmdHeight The upright HMD height.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRProvider>::instanceOrNull())
      OPENVR->mStandingHMDHeight = hmdHeight;
}

DefineEngineStaticMethod(OpenVR, getHMDTrackingHeight, F32, (), ,
   "Gets the current tracking height offset for the hmd.\n\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRProvider>::instanceOrNull())
      return OPENVR->mStandingHMDHeight;
   return 0.0f;
}

// Compositor Skinning
DefineEngineStaticMethod(OpenVR, setSkyboxOverride, bool, (CubemapData* cubemap), ,
   "Override the skybox used in the compositor (e.g. for during level loads when the app can't "
   "feed scene images fast enough).\n\n"
   "@param cubemap The cubemap to display as the compositor skybox.\n"
   "@returns true if the command was successful. false on error.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRProvider>::instanceOrNull() && cubemap)
      return OPENVR->setSkyboxOverride(cubemap);
   return false;
}

DefineEngineStaticMethod(OpenVR, clearSkyboxOverride, void, (), ,
   "Resets the compositor skybox back to defaults.\n\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRProvider>::instanceOrNull())
      OPENVR->clearSkyboxOverride();
}

DefineEngineStaticMethod(OpenVR, setStageOverride_Async, bool, (OpenVRStageModelData* modelData, TransformF transform), ,
   "Override the stage model used in the compositor to replace the grid. The render model "
   "and texture will be loaded asynchronously from disk and uploaded to the gpu by the runtime. "
   "Once ready for rendering, the onStageOverrideReady() callback will be called. Use "
   "fadeGrid() to reveal. Call clearStageOverride() to free the associated resources "
   "when finished.\n\n"
   "@param modelData An OpenVRStageModelData object initialized with the model path and render "
   "settings for the compositor stage.\n"
   "@see OpenVRStageModelData\n"
   "@param transform Position and orientation for the stage model relative to the tracking universe origin.\n"
   "@returns true if the command was successful. false on error.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRProvider>::instanceOrNull() && modelData)
      return OPENVR->setStageOverride_Async(modelData, transform.getMatrix());
   return false;
}

DefineEngineStaticMethod(OpenVR, clearStageOverride, void, (), ,
   "Resets the compositor stage to its default user specified setting.\n\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRProvider>::instanceOrNull())
      OPENVR->clearStageOverride();
}

DefineEngineStaticMethod(OpenVR, fadeGrid, void, (F32 fSeconds, bool bFadeGridIn), ,
   "Fade the Grid in or out over fSeconds.\n\n"
   "@param fSeconds Duration of the fade effect in seconds.\n"
   "@param bFadeGridIn Direction of fade. True - fade to grid. False - fade to scene.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRProvider>::instanceOrNull())
      OPENVR->fadeGrid(fSeconds, bFadeGridIn);
}

DefineEngineStaticMethod(OpenVR, getCurrentGridAlpha, F32, (), ,
   "Get current alpha value of the grid. This can be used to determine the current state "
   "of the grid fade effect.\n\n"
   "@returns the current alpha value of the grid fade effect.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRProvider>::instanceOrNull())
      return OPENVR->getCurrentGridAlpha();
   return false;
}

DefineEngineStaticMethod(OpenVR, fadeToColor, void, (F32 fSeconds, LinearColorF color, bool bBackground), (false),
   "Fades the view on the HMD to the specified color. This color is faded on top of the scene "
   "based on the alpha parameter. Removing the fade color instantly would be "
   "FadeToColor(0.0, \"0.0, 0.0, 0.0, 0.0\").\n\n"
   "@param fSeconds Duration of the fade effect in seconds.\n"
   "@param color The color to fade to.\n"
   "@param bBackground Undocumented parameter, default false.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRProvider>::instanceOrNull())
      OPENVR->fadeToColor(fSeconds, color, bBackground);
}

DefineEngineStaticMethod(OpenVR, getCurrentFadeColor, LinearColorF, (bool bBackground), (false),
   "Get current fade color value. This can be used to determine the current state of the color fade effect.\n\n"
   "@param bBackground Undocumented parameter, default false.\n"
   "@returns the current color value of the fade effect.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRProvider>::instanceOrNull())
      return OPENVR->getCurrentFadeColor(bBackground);
   return LinearColorF::ZERO;
}
