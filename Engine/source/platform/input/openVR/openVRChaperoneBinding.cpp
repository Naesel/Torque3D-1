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
#include "platform/input/openVR/openVRChaperone.h"

DECLARE_SCOPE(OVRChaperone);
IMPLEMENT_SCOPE(OVRChaperone, OpenVRChaperone, , "");
ConsoleDoc(
   "@class OpenVRChaperone\n"
   "@brief This class exposes the IVRChaperone interface to Torque Script.\n\n"
   "HIGH LEVEL TRACKING SPACE ASSUMPTIONS:\n"
   "   0, 0, 0 is the preferred standing area center.\n"
   "   0Z is the floor height.\n"
   "   +Y is the preferred forward facing direction.\n"
   "To call the methods of IVRChaperone from script, lowercase the first character of "
   "the method name and prefix with 'OVRChaperone::'.\n"
   "@tsexample\n"
   "OVRChaperone::resetZeroPose(\"Seated\");\n"
   "echo(\"Chaperone bounds are \" @ (OVRChaperone::areBoundsVisible() ? \"Visible\" : \"Not Visible\"));\n"
   "@endtsexample\n"
   "@ingroup OVRChaperone\n"
   "@ingroup OpenVR\n"
);

// Enum impls
ImplementEnumType(OpenVRChaperoneCalibrationState,
   "The possible VR Chaperone calibration states.\n\n"
   "@ingroup OVRChaperone\n"
   "@ingroup OpenVR\n")
{ vr::ChaperoneCalibrationState_OK, "OK", "Chaperone is fully calibrated and working correctly" },
{ vr::ChaperoneCalibrationState_Warning, "Warning", "" },
{ vr::ChaperoneCalibrationState_Warning_BaseStationMayHaveMoved, "BaseStationMayHaveMoved", "A base station thinks that it might have moved" },
{ vr::ChaperoneCalibrationState_Warning_BaseStationRemoved, "BaseStationRemoved", "There are less base stations than when calibrated" },
{ vr::ChaperoneCalibrationState_Warning_SeatedBoundsInvalid, "SeatedBoundsInvalid", "Seated bounds haven't been calibrated for the current tracking center" },
{ vr::ChaperoneCalibrationState_Error, "Error", "" },
{ vr::ChaperoneCalibrationState_Error_BaseStationUninitialized, "BaseStationUninitialized", "Tracking center hasn't be calibrated for at least one of the base stations" },
{ vr::ChaperoneCalibrationState_Error_BaseStationConflict, "BaseStationConflict", "Tracking center is calibrated, but base stations disagree on the tracking space" },
{ vr::ChaperoneCalibrationState_Error_PlayAreaInvalid, "PlayAreaInvalid", "Play Area hasn't been calibrated for the current tracking center" },
{ vr::ChaperoneCalibrationState_Error_CollisionBoundsInvalid, "CollisionBoundsInvalid", "Collision Bounds haven't been calibrated for the current tracking center" },
EndImplementEnumType;


// IVRChaperone Methods
/** HIGH LEVEL TRACKING SPACE ASSUMPTIONS:
* 0,0,0 is the preferred standing area center.
* 0Y is the floor height. Converted to 0Z for T3D.
* -Z is the preferred forward facing direction. Converted to +Y for T3D.*/
DefineEngineStaticMethod(OVRChaperone, getCalibrationState, OpenVRChaperoneCalibrationState, (), ,
   "Get the current state of Chaperone calibration. This state can change at any time during "
   "a session due to physical base station changes.\n\n"
   "@return One of: OK, Warning, BaseStationMayHaveMoved, BaseStationRemoved, SeatedBoundsInvalid, "
   "Error, BaseStationUninitialized, BaseStationConflict, PlayAreaInvalid, CollisionBoundsInvalid.\n"
   "@ingroup OVRChaperone\n"
   "@ingroup OpenVR\n")
{
   if (!ManagedSingleton<OpenVRChaperone>::instanceOrNull())
      return vr::ChaperoneCalibrationState_Error;

   return OVRCHAPERONE->getCalibrationState();
}

DefineEngineStaticMethod(OVRChaperone, getPlayAreaSize, Point2F, (), ,
   "Returns the width and depth of the Play Area (formerly named Soft Bounds) in X and Y. "
   "Tracking space center(0, 0, 0) is the center of the Play Area.\n\n"
   "@return Point2F with width in x component and depth (length) in y component.\n"
   "@ingroup OVRChaperone\n"
   "@ingroup OpenVR\n")
{
   if (!ManagedSingleton<OpenVRChaperone>::instanceOrNull())
      return Point2F::Zero;

   return OVRCHAPERONE->getPlayAreaSize();
}

DefineEngineStaticMethod(OVRChaperone, getPlayAreaRect, Box3F, (), ,
   "Returns a box with 0 height representing the play area floor space. Standing center (0,0,0) "
   "is the center of the Play Area. It's a rectangle. 2 sides are parallel to the X axis and 2 "
   "sides are parallel to the Y axis.\n\n"
   "@ingroup OVRChaperone\n"
   "@ingroup OpenVR\n")
{
   if (!ManagedSingleton<OpenVRChaperone>::instanceOrNull())
      return Box3F::Zero;

   return OVRCHAPERONE->getPlayAreaRect();
}

DefineEngineStaticMethod(OVRChaperone, reloadInfo, void, (), ,
   "Reload Chaperone data from the .vrchap file on disk.\n\n"
   "@ingroup OVRChaperone\n"
   "@ingroup OpenVR\n")
{
   if (ManagedSingleton<OpenVRChaperone>::instanceOrNull())
      OVRCHAPERONE->getPlayAreaRect();
}

DefineEngineStaticMethod(OVRChaperone, setSceneColor, void, (LinearColorF color), ,
   "Optionally give the chaperone system a hint about the color and brightness in the scene.\n\n"
   "@ingroup OVRChaperone\n"
   "@ingroup OpenVR\n")
{
   if (ManagedSingleton<OpenVRChaperone>::instanceOrNull())
      OVRCHAPERONE->setSceneColor(color);
}

DefineEngineStaticMethod(OVRChaperone, areBoundsVisible, bool, (), ,
   "Returns true if the chaperone bounds are showing right now.\n\n"
   "@ingroup OVRChaperone\n"
   "@ingroup OpenVR\n")
{
   if (ManagedSingleton<OpenVRChaperone>::instanceOrNull())
      return OVRCHAPERONE->areBoundsVisible();

   return false;
}

DefineEngineStaticMethod(OVRChaperone, forceBoundsVisible, void, (bool forceShow), ,
   "Force the bounds to show, mostly for utilities.\n\n"
   "@ingroup OVRChaperone\n"
   "@ingroup OpenVR\n")
{
   if (ManagedSingleton<OpenVRChaperone>::instanceOrNull())
      OVRCHAPERONE->forceBoundsVisible(forceShow);
}

DefineEngineStaticMethod(OVRChaperone, resetZeroPose, void, (OpenVRTrackingUniverseOrigin universeOrigin), (vr::TrackingUniverseSeated ),
   "Sets the zero pose for the given tracker coordinate system to the current "
   "position and yaw of the HMD. After ResetZeroPose all GetDeviceToAbsoluteTrackingPose "
   "calls as the origin will be relative to this new zero pose. The new zero coordinate "
   "system will not change the fact that the Z axis is up in the real world, so the next "
   "pose returned from GetDeviceToAbsoluteTrackingPose after a call to ResetZeroPose may "
   "not be exactly an identity matrix.\n\n"
   "NOTE: This function overrides the user's previously saved zero pose "
   "and should only be called as the result of a user action. Users are also "
   "able to set their zero pose via the OpenVR Dashboard.\n\n"
   "@param universeOrigin The universe to zero. \"Seated\" or \"Standing\". Default \"Seated\".\n"
   "@ingroup OVRChaperone\n"
   "@ingroup OpenVR\n")
{
   if (ManagedSingleton<OpenVRChaperone>::instanceOrNull())
      OVRCHAPERONE->resetZeroPose(universeOrigin);
}

