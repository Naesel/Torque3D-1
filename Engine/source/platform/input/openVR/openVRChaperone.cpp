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

#include "platform/input/openVR/openVRProvider.h"
#include "platform/input/openVR/openVRChaperone.h"

// IVRChaperone Methods

OpenVRChaperoneCalibrationState OpenVRChaperone::getCalibrationState()
{
   if (vr::VRChaperone())
      return vr::VRChaperone()->GetCalibrationState();

   return vr::ChaperoneCalibrationState_Error;
}

Point2F OpenVRChaperone::getPlayAreaSize()
{
   if (vr::VRChaperone())
   {
      float x, y;
      vr::VRChaperone()->GetPlayAreaSize(&x, &y);
      return Point2F(x, y);
   }

   return Point2F::Zero;
}

Box3F OpenVRChaperone::getPlayAreaRect()
{
   if (vr::VRChaperone())
   {
      vr::HmdQuad_t quad;
      if (vr::VRChaperone()->GetPlayAreaRect(&quad))
      {
         Box3F areaBox(OpenVRUtil::convertPointFromOVR(quad.vCorners[0]), OpenVRUtil::convertPointFromOVR(quad.vCorners[1]));
         areaBox.intersect(OpenVRUtil::convertPointFromOVR(quad.vCorners[2]));
         areaBox.intersect(OpenVRUtil::convertPointFromOVR(quad.vCorners[3]));
         return areaBox;
      }
   }

   return Box3F::Zero;
}

void OpenVRChaperone::reloadInfo()
{
   if (vr::VRChaperone())
      vr::VRChaperone()->ReloadInfo();
}

void OpenVRChaperone::setSceneColor(LinearColorF color)
{
   if (vr::VRChaperone())
   {
      vr::HmdColor_t vrColor;
      vrColor.r = color.red;
      vrColor.g = color.green;
      vrColor.b = color.blue;
      vrColor.a = color.alpha;
      vr::VRChaperone()->SetSceneColor(vrColor);
   }
}

bool OpenVRChaperone::areBoundsVisible()
{
   if (vr::VRChaperone())
      return vr::VRChaperone()->AreBoundsVisible();
   return false;
}

void OpenVRChaperone::forceBoundsVisible(bool bForce)
{
   if (vr::VRChaperone())
      vr::VRChaperone()->ForceBoundsVisible(bForce);
}

void OpenVRChaperone::resetZeroPose(vr::ETrackingUniverseOrigin eTrackingUniverseOrigin)
{
   if (vr::VRChaperone())
      vr::VRChaperone()->ResetZeroPose(eTrackingUniverseOrigin);
}
