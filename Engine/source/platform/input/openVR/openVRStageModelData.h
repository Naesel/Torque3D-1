//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
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

#ifndef _OPENVR_STAGEMODELDATA_H_
#define _OPENVR_STAGEMODELDATA_H_

#ifndef _SIMOBJECT_H_
#include "console/simObject.h"
#endif

#ifndef _COLOR_H_
#include "core/color.h"
#endif

/// Script interface for assigning a stage model and presentation parameters for the 
/// SteamVR compositor.
class OpenVRStageModelData : public SimObject
{
   friend class OpenVRProvider;
   typedef SimObject Parent;   

public:   

   OpenVRStageModelData();
   ~OpenVRStageModelData();

   bool onAdd();
   static void initPersistFields();

   DECLARE_CONOBJECT(OpenVRStageModelData);


protected:
   /// Path and filename of the model to use as the compositor stage. The file must be in .obj
   /// format and exist as a loose file in the file system. The file will be loaded directly
   /// by SteamVR so it cannot be included in a zipped archive.
   /// See the render models that ship with SteamVR for examples of scale, orientation and how to
   /// specify the material. E.g. C:\Program Files (x86)\Steam\steamapps\common\SteamVR\resources\rendermodels\generic_hmd
   /// PNG is the recommended format for the texture.
   StringTableEntry  mObjModelFile;

   /// Primary color is applied as a tint to (i.e. multiplied with) the model's texture.
   LinearColorF mPrimaryColor;

   /// Secondary color is faded over the primary color bassed on the values set for Vignette and FresnelStrength.
   LinearColorF mSecondaryColor;

   /// Vignette radius is in meters and is used to fade to the specified secondary solid color over
   /// that 3D distance from the origin of the playspace. This is most commonly used with black to give
   /// the illusion of being in a pool of light centered on the playspace area.
   F32 mVignetteInnerRadius;
   F32 mVignetteOuterRadius;

   /// Fades to the secondary color based on view incidence. This variable controls the linearity
   /// of the effect.  It is mutually exclusive with vignette. The mesh is treated as faceted and
   /// lerps between the primary and secondary color based on triangle orientation to the viewer.
   F32 mFresnelStrength;

   /// Controls backface culling.
	bool mbBackfaceCulling;

   /// Converts the render model's texture to luma and applies to rgb equally.  This is useful to
   /// combat compression artifacts that can occur on desaturated source material.
	bool mbGreyscale;

   /// Renders mesh as a wireframe.
	bool mbWireframe;
};

#endif // _OPENVR_STAGEMODELDATA_H_
