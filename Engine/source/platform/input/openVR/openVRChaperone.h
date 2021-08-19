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

#ifndef _OPENVR_CHAPERONE_H_
#define _OPENVR_CHAPERONE_H_

typedef vr::ChaperoneCalibrationState OpenVRChaperoneCalibrationState;
DefineEnumType(OpenVRChaperoneCalibrationState);

class OpenVRChaperone
{
public:
   /// @name IVRChaperone handling
   /// {

   OpenVRChaperoneCalibrationState getCalibrationState();
   Point2F getPlayAreaSize();
   Box3F getPlayAreaRect();
   void reloadInfo();
   void setSceneColor(LinearColorF color);
   bool areBoundsVisible();
   void forceBoundsVisible(bool bForce);
   void resetZeroPose(vr::ETrackingUniverseOrigin eTrackingUniverseOrigin);

   /// }

public:
   // For ManagedSingleton.
   static const char* getSingletonName() { return "OpenVRChaperone"; }
};

/// Returns the OpenVRChaperone singleton.
#define OVRCHAPERONE ManagedSingleton<OpenVRChaperone>::instance()

#endif
