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
#include "platform/input/openVR/openVRInput.h"

DECLARE_SCOPE(OVRInput);
IMPLEMENT_SCOPE(OVRInput, OpenVRInput, , "");
ConsoleDoc(
   "@class OpenVRInput\n"
   "@brief This class exposes the IVRInput interface to Torque Script.\n\n"
   "@tsexample\n"
   "@endtsexample\n"
   "@ingroup OVRInput\n"
   "@ingroup OpenVR\n"
);

// Enum impls
ImplementEnumType(OpenVRInputError,
   "All possible IVRInput error codes.\n\n"
   "@ingroup OVRInput\n"
   "@ingroup OpenVR")
{ vr::VRInputError_None, "None", },
{ vr::VRInputError_NameNotFound, "NameNotFound", },
{ vr::VRInputError_WrongType, "WrongType" },
{ vr::VRInputError_InvalidHandle, "InvalidHandle", },
{ vr::VRInputError_InvalidParam, "InvalidParam" },
{ vr::VRInputError_NoSteam, "NoSteam", },
{ vr::VRInputError_MaxCapacityReached, "MaxCapacityReached" },
{ vr::VRInputError_IPCError, "IPCError", },
{ vr::VRInputError_NoActiveActionSet, "NoActiveActionSet" },
{ vr::VRInputError_InvalidDevice, "InvalidDevice", },
{ vr::VRInputError_InvalidSkeleton, "InvalidSkeleton" },
{ vr::VRInputError_InvalidBoneCount, "InvalidBoneCount", },
{ vr::VRInputError_InvalidCompressedData, "InvalidCompressedData" },
{ vr::VRInputError_NoData, "NoData", },
{ vr::VRInputError_BufferTooSmall, "BufferTooSmall" },
{ vr::VRInputError_MismatchedActionManifest, "MismatchedActionManifest", },
{ vr::VRInputError_MissingSkeletonData, "MissingSkeletonData" },
{ vr::VRInputError_InvalidBoneIndex, "InvalidBoneIndex", },
{ vr::VRInputError_InvalidPriority, "InvalidPriority" },
{ vr::VRInputError_PermissionDenied, "PermissionDenied", },
{ vr::VRInputError_InvalidRenderModel, "InvalidRenderModel" },
EndImplementEnumType;


// IVRInput Methods
// Initialization
DefineEngineStaticMethod(OVRInput, setActionManifestPath, OpenVRInputError, (const char* manifestPath), ,
   "Sets the path to the action manifest JSON file that is used by this application. If this information "
   "was set on the Steam partner site, calls to this function are ignored. If the Steam partner site "
   "setting and the path provided by this call are different, VRInputError_MismatchedActionManifest is returned. "
   "This call must be made before the first call to UpdateActionState or IVRSystem::PollNextEvent.\n"
   "Call this function immediately after the first call to OpenVR::setEnabled(true); The first time "
   "the input system is successfully initialized, the onOVRInputReady() callback will be executed. "
   "All action sets and actions should be installed from the onOVRInputReady() callback. If this method "
   "is called before VR has been enabled, \"InvalidHandle\" will be returned.\n"
   "@param manifestPath - The file path to the input manifest json file that defines all bindable controller events for the game."
   "@return The IVRInput error code. If \"None\" or \"MismatchedActionManifest\" is returned, the input "
   "system is initialized and ready to begin polling.\n"
   "@ingroup OVRInput\n"
   "@ingroup OpenVR\n")
{
   if (!ManagedSingleton<OpenVRInput>::instanceOrNull())
      return vr::VRInputError_InvalidHandle;

   return OVRINPUT->setActionManifestPath(manifestPath);
}

DefineEngineStaticMethod(OVRInput, addActionSet, S32, (const char* setName), ,
   "Adds an action set and saves the handle for it.\n\n"
   "@param setName The action set identifier from the action manifest .json file.\n"
   "@return The integer identifier to be used in subsequent calls to reference the action set. "
   "The return value will be -1 if the action set could not be found by IVRInput.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->addActionSet(setName);
   return -1;
}

DefineEngineStaticMethod(OVRInput, addAnalogAction, S32, (U32 setIndex, const char* actionName, const char* callbackFunc), ,
   "Adds an analog action, maps it's callback function and saves the handle for it.\n\n"
   "@param setIndex The action set index returned from addActionSet() that this action is added to.\n"
   "@param actionName The action name from the action manifest .json file.\n"
   "@param callbackFunc The function to call anytime the axes data changes.\n"
   " parameters: %controller, %xAxis, %yAxis, %zAxis\n\n"
   "@return The integer identifier to be used in subsequent calls to reference the action. "
   "The return value will be -1 if the action could not be found by IVRInput. The integer "
   "identifier is needed to reference the action in calls to get updated glyphs and bindings from IVRInput.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->addAnalogAction(setIndex, actionName, callbackFunc);
   return -1;
}

DefineEngineStaticMethod(OVRInput, addDigitalAction, S32, (U32 setIndex, const char* actionName, const char* callbackFunc), ,
   "Adds a digital action, maps it's callback function and saves the handle for it.\n\n"
   "@param setIndex The action set index returned from addActionSet() that this action is added to.\n"
   "@param actionName The action name from the action manifest .json file.\n"
   "@param callbackFunc The function to call when the input state changes.\n"
   " parameters: %controller, %state\n\n"
   "@return The integer identifier to be used in subsequent calls to reference the action. "
   "The return value will be -1 if the action could not be found by IVRInput. The integer "
   "identifier is needed to reference the action in calls to get updated glyphs and bindings.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->addDigitalAction(setIndex, actionName, callbackFunc);
   return -1;
}

DefineEngineStaticMethod(OVRInput, addPoseAction, S32,
   (U32 setIndex, const char* actionName, const char* poseCallback, const char* velocityCallback, S32 moveIndex), ("", "", -1),
   "Adds a device pose action, maps it's callback function and saves the handle for it.\n\n"
   "@param setIndex The action set index returned from addActionSet() that this action is added to.\n"
   "@param actionName The action name from the action manifest .json file.\n"
   "@param poseCallback The function to call with updated position and velocity data.\n"
   " parameters: %controller, %xPos, %yPos, %zPos, %xRot, %yRot, %zRot, %wRot\n\n"
   "@param velocityCallback The function to call with updated linear and angular velocity data.\n"
   " parameters: %controller, %xLinVel, %yLinVel, %zLinVel, %xAngVel, %yAngVel, %zAngVel\n\n"
   "@param moveIndex If set, the position and rotation will be assigned into the extended move "
   "at this index.\n"
   "@return The integer identifier to be used in subsequent calls to reference the action. "
   "The return value will be -1 if the action could not be found by IVRInput. The integer "
   "identifier is needed to reference the action in calls to get updated glyphs and bindings.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->addPoseAction(setIndex, actionName, poseCallback, velocityCallback, moveIndex);
   return -1;
}

DefineEngineStaticMethod(OVRInput, setPoseCallbacks, bool,
   (S32 poseIndex, const char* poseCallback, const char* velocityCallback), ("", ""),
   "Resets the callbacks for a pose action. i.e. turning on and off the velocity "
   "callback depending on object held.\n\n"
   "@param poseIndex The index value returned from addPoseAction().\n"
   "@param poseCallback The function to call with updated position and velocity data.\n"
   " parameters: %controller, %xPos, %yPos, %zPos, %xRot, %yRot, %zRot, %wRot\n\n"
   "@param velocityCallback The function to call with updated linear and angular velocity data.\n"
   " parameters: %controller, %xLinVel, %yLinVel, %zLinVel, %xAngVel, %yAngVel, %zAngVel\n\n"
   "@return True if the pose was found and updated. False otherwise.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->setPoseCallbacks(poseIndex, poseCallback, velocityCallback);
   return false;
}

DefineEngineStaticMethod(OVRInput, addSkeletalAction, S32,
   (U32 setIndex, const char* actionName, S32 moveIndex), (-1),
   "Adds a hand skeleton action, maps it to a move manager index and saves the handle for it.\n\n"
   "@param setIndex The action set index returned from addActionSet() that this action is added to.\n"
   "@param actionName The action name from the action manifest .json file.\n"
   "@param moveIndex The skeleton will be assigned into the extended move manager as a "
   "binary blob at this index.\n"
   "@return The integer identifier to be used in subsequent calls to reference the action. "
   "The return value will be -1 if the action could not be found by IVRInput. The integer "
   "identifier is needed to reference the action in calls to get updated glyphs and bindings.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->addSkeletalAction(setIndex, actionName, moveIndex);
   return -1;
}

DefineEngineStaticMethod(OVRInput, setSkeletonMode, S32,
   (S32 skeletonIndex, bool withController), (true),
   "Toggles the skeleton mode between ranged with controller and without.\n\n"
   "@param skeletonIndex The index value returned from addSkeletalAction().\n"
   "@param withController True to use vr::VRSkeletalMotionRange_WithController.\n"
   "False to use vr::VRSkeletalMotionRange_WithoutController."
   "@return True if the skeleton action was found and updated. False otherwise.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->setSkeletonMode(skeletonIndex, withController);
   return false;
}

DefineEngineStaticMethod(OVRInput, addHapticOutput, S32, (const char* outputName), ,
   "Loads the event handle for a vr controller haptic output event.\n\n"
   "@param outputName The vibration event name from the action manifest .json file.\n"
   "@return The integer identifier to be used in subsequent calls to reference the action. "
   "The return value will be -1 if the action could not be found by IVRInput. The integer "
   "identifier is needed to reference the action in calls to triggerHapticEvent().\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->addHapticOutput(outputName);
   return -1;
}

DefineEngineStaticMethod(OVRInput, triggerHapticEvent, bool, (U32 actionIndex,
   float fStartSecondsFromNow, float fDurationSeconds, float fFrequency, float fAmplitude), ,
   "Loads the event handle for a vr controller haptic output event.\n\n"
   "@param outputName The vibration event name from the action manifest .json file.\n"
   "@return The integer identifier to be used in subsequent calls to reference the action. "
   "The return value will be -1 if the action could not be found by IVRInput. The integer "
   "identifier is needed to reference the action in calls to triggerHapticEvent().\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->triggerHapticEvent(actionIndex, fStartSecondsFromNow, fDurationSeconds, fFrequency, fAmplitude);
   return false;
}

DefineEngineStaticMethod(OVRInput, activateActionSet, bool, (S32 controllerIndex, U32 setIndex), ,
   "Activate the specified action set on one or both vr controllers.\n\n"
   "@param controllerIndex Zero-based index of the controller 0 - getNumControllers()-1. "
   "The value -1 may be passed to have the action set activated on all controllers.\n"
   "@param setIndex The index value that was returned from addActionSet().\n"
   "@return Returns true if the set was activated, false if it could not be found.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->activateActionSet(controllerIndex, setIndex);
   return false;
}

DefineEngineStaticMethod(OVRInput, pushActionSetLayer, bool, (S32 controllerIndex, U32 setIndex), ,
   "Activate the specified action set as the highest priority set on the stack.\n\n"
   "@param controllerIndex Zero-based index of the controller 0 - getNumControllers()-1. "
   "The value -1 may be passed to have the action set activated on all controllers.\n"
   "@param setIndex The index value that was returned from addActionSet().\n"
   "@return Returns true if the set was activated, false if it could not be found or "
   "if there are already the maximum number of layers active (default 5).\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->pushActionSetLayer(controllerIndex, setIndex);
   return false;
}

DefineEngineStaticMethod(OVRInput, popActionSetLayer, bool, (S32 controllerIndex, U32 setIndex), ,
   "Removes the specified action set from the stack and deactivates it's actions. "
   "You cannot pop the last action set layer, use activateActionSet() to replace it.\n\n"
   "@param controllerIndex Zero-based index of the controller 0 - getNumControllers()-1. "
   "The value -1 may be passed to have the action set activated on all controllers.\n"
   "@param setIndex The index value that was returned from addActionSet().\n"
   "@return Returns true if the set was deactivated, false if there was an error.\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->popActionSetLayer(controllerIndex, setIndex);
   return false;
}

DefineEngineStaticMethod(OVRInput, showActionOrigins, void, (U32 setIndex, OpenVRActionType actionType, U32 actionIndex), ,
   "Shows the current binding for the action in-headset. \"At the moment this "
   "function shows the entire binding UI, but that behavior will likely change down the road.\"\n\n"
   "@param setIndex The index value that was returned from addActionSet().\n"
   "@param actionType The type of action \"Analog\", \"Digital\", \"Pose\" or \"Skeletal\".\n"
   "@param setIndex The index value that was returned from add...Action().\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->showActionOrigins(setIndex, actionType, actionIndex);
}

DefineEngineStaticMethod(OVRInput, showActionSetBinds, void, (U32 setIndex), ,
   "Shows the current binding for all of the actions in the specified action set. "
   "\"At the moment this function shows the entire binding UI, but that behavior "
   "will likely change down the road.\"\n\n"
   "@param setIndex The index value that was returned from addActionSet().\n"
   "@ingroup OpenVR")
{
   if (ManagedSingleton<OpenVRInput>::instanceOrNull())
      return OVRINPUT->showActionSetBinds(setIndex);
}
