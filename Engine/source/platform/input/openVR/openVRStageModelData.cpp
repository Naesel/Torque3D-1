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

#include "platform/platform.h"
#include "platform/input/openVR/openVRStageModelData.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"

IMPLEMENT_CONOBJECT(OpenVRStageModelData);

OpenVRStageModelData::OpenVRStageModelData()
   : mObjModelFile(StringTable->EmptyString()),
   mPrimaryColor(LinearColorF::WHITE),
   mSecondaryColor(LinearColorF::WHITE),
   mVignetteInnerRadius(0),
   mVignetteOuterRadius(0),
   mFresnelStrength(0),
   mbBackfaceCulling(false),
   mbGreyscale(false),
   mbWireframe(false)
{
}

OpenVRStageModelData::~OpenVRStageModelData()
{
}

ConsoleDocClass( OpenVRStageModelData, 
   "Used to assign a stage model and render settings for the VR compositor.\n"
   //"@tsexample\n"
   //"singleton OpenVRStageModelData(T3D_VRLobby)\n"
   //"{\n"
   //"};\n"
   //"@endtsexample\n"
   "@see https://github.com/ValveSoftware/openvr/wiki/Compositor-Skinning for more details.\n"
   "@ingroup OpneVR\n" );

void OpenVRStageModelData::initPersistFields()
{
   addGroup("Model");
   addField("modelFileName", TypeShapeFilename, Offset(mObjModelFile, OpenVRStageModelData),
      "Path and filename of the model to use as the compositor stage. The file must be in .obj "
      "format and exist as a loose file in the file system. The file will be loaded directly "
      "by SteamVR so it cannot be included is a zipped archive.\n"
      "See the render models that ship with SteamVR for examples of scale, orientation and how to "
      "specify the material. E.g. C:\Program Files (x86)\Steam\steamapps\common\SteamVR\resources\rendermodels\generic_hmd "
      "PNG is the recommended format for the texture.\n");
   endGroup("Model");

   addGroup("Render Settings");
   addField("primaryColor", TypeColorF, Offset(mPrimaryColor, OpenVRStageModelData),
      "Primary color is applied as a tint to (i.e. multiplied with) the model's texture.\n"
      "Default: White\n");
   addField("secondaryColor", TypeColorF, Offset(mSecondaryColor, OpenVRStageModelData),
      "Secondary color is faded over the primary color bassed on the values set for Vignette and FresnelStrength.\n"
      "Default: White\n");
   addField("vignetteInnerRadius", TypeF32, Offset(mVignetteInnerRadius, OpenVRStageModelData),
      "Controls the inner radius of the Vignette color sphere. Vignette radius is in meters and is used to "
      "fade to the specified secondary solid color over that 3D distance from the origin of the playspace. "
      "This is most commonly used with black to give  the illusion "
      "of being in a pool of light centered on the playspace area.\n"
      "Default: 0\n");
   addField("vignetteOuterRadius", TypeF32, Offset(mVignetteOuterRadius, OpenVRStageModelData),
      "Controls the outer radius of the Vignette color sphere.\n"
      "Default: 0\n"
      "@see vignetteInnerRadius\n");
   addField("fresnelStrength", TypeF32, Offset(mFresnelStrength, OpenVRStageModelData),
      "Fades to the secondary color based on view incidence. This variable controls the linearity "
      "of the effect. It is mutually exclusive with vignette. The mesh is treated as faceted and "
      "lerps between the primary and secondary color based on triangle orientation to the viewer.\n"
      "Default: 0\n");
   addField("backfaceCulling", TypeBool, Offset(mbBackfaceCulling, OpenVRStageModelData),
      "Controls the rendering of triangles that face away from the camera.\n"
      "Default: false\n");
   addField("greyscale", TypeBool, Offset(mbGreyscale, OpenVRStageModelData),
      "Converts the render model's texture to luma and applies to rgb equally. This is useful to "
      "combat compression artifacts that can occur on desaturated source material.\n"
      "Default: false\n");
   addField("wireframe", TypeBool, Offset(mbWireframe, OpenVRStageModelData),
      "Renders the mesh as a wireframe.\n"
      "Default: false\n");
   endGroup("Render Settings");
}

bool OpenVRStageModelData::onAdd()
{
   if( !Parent::onAdd() )
      return false;

   // There's no point in verifying the model exists here, because it can be changed
   // at any time before the call to SetStageOverride_Async()
   return true;
}
