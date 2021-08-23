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

#ifndef _OPENVR_PROVIDER_H_
#define _OPENVR_PROVIDER_H_

#include "math/mQuat.h"
#include "math/mPoint4.h"
#include "math/util/frustum.h"
#include "core/util/tSingleton.h"

#include "gfx/gfxDevice.h"
#include "gfx/gfxVertexBuffer.h"
#include "gfx/gfxPrimitiveBuffer.h"
#include "gfx/gfxTarget.h"

#include "platform/input/OpenVR/openVRStructs.h"
#include "platform/input/IInputDevice.h"
#include "platform/input/event.h"
#include "platform/output/IDisplayDevice.h"
#include "materials/materialDefinition.h"
#include "materials/baseMatInstance.h"

class OpenVRHMDDevice;
class OpenVROverlay;
class BaseMatInstance;
class SceneRenderState;
struct MeshRenderInst;
class Namespace;
class NamedTexTarget;
class OpenVRRenderModel;
class OpenVRStageModelData;

typedef vr::ETrackingResult OpenVRTrackingResult;
typedef vr::ETrackingUniverseOrigin OpenVRTrackingUniverseOrigin;
typedef vr::EVRState OpenVRState;
typedef vr::TrackedDeviceClass OpenVRTrackedDeviceClass;
typedef vr::EVRControllerAxisType OpenVRControllerAxisType;
typedef vr::ETrackedControllerRole OpenVRTrackedControllerRole;

DefineEnumType(OpenVRTrackingResult);
DefineEnumType(OpenVRTrackingUniverseOrigin);
DefineEnumType(OpenVRState);
DefineEnumType(OpenVRTrackedDeviceClass);
DefineEnumType(OpenVRControllerAxisType);
DefineEnumType(OpenVRTrackedControllerRole);

namespace OpenVRUtil
{
   /// Convert a matrix in OVR space to torque space
   void convertTransformFromOVR(const MatrixF &inRotTMat, MatrixF& outRotation);

   /// Convert a matrix in torque space to OVR space
   void convertTransformToOVR(const MatrixF& inRotation, MatrixF& outRotation);

   /// Converts vr::HmdMatrix34_t to a MatrixF
   MatrixF convertSteamVRAffineMatrixToMatrixFPlain(const vr::HmdMatrix34_t &mat);

   /// Converts a MatrixF to a vr::HmdMatrix34_t
   void convertMatrixFPlainToSteamVRAffineMatrix(const MatrixF &inMat, vr::HmdMatrix34_t &outMat);

   U32 convertOpenVRButtonToTorqueButton(uint32_t vrButton);

   /// Converts a point to OVR coords
   inline Point3F convertPointToOVR(const Point3F &point)
   {
      return Point3F(-point.x, -point.z, point.y);
   }

   /// Converts a point from OVR coords
   inline Point3F convertPointFromOVR(const Point3F &point)
   {
      return Point3F(-point.x, point.z, -point.y);
   }

   // Converts a point from OVR coords, from an input float array
   inline Point3F convertPointFromOVR(const vr::HmdVector3_t& v)
   {
      return Point3F(-v.v[0], v.v[2], -v.v[1]);
   }
};

/** The mappable IVRInput action types */
enum EOpenVRActionType
{
   OpenVRActionType_Digital = 0,
   OpenVRActionType_Analog = 1,
   OpenVRActionType_Pose = 2,
   OpenVRActionType_Skeleton = 3,
};
typedef EOpenVRActionType OpenVRActionType;
DefineEnumType(OpenVRActionType);

//------------------------------------------------------------

class OpenVRProvider : public IDisplayDevice, public IInputDevice
{
public:

   OpenVRProvider();
   ~OpenVRProvider();

   static void staticInit();

   bool enable();
   bool disable();

   bool getActive() { return mHMD != NULL; }
   inline vr::IVRRenderModels* getRenderModels() { return mRenderModels; }

   /// @name Input handling
   /// {
   virtual bool process();
   /// }

   /// @name Display handling
   /// {
   virtual bool providesFrameEyePose() const;
   virtual void getFrameEyePose(IDevicePose *pose, S32 eyeId) const;

   virtual bool providesEyeOffsets() const;
   /// Returns eye offset not taking into account any position tracking info
   virtual void getEyeOffsets(Point3F *dest) const;

   virtual bool providesFovPorts() const;
   virtual void getFovPorts(FovPort *out) const;

   virtual void getStereoViewports(RectI *out) const;
   virtual void getStereoTargets(GFXTextureTarget **out) const;

   virtual void setDrawCanvas(GuiCanvas *canvas);
   virtual void setDrawMode(GFXDevice::GFXDeviceRenderStyles style);

   virtual void setCurrentConnection(GameConnection *connection);
   virtual GameConnection* getCurrentConnection();

   virtual GFXTexHandle getPreviewTexture();

   virtual void onStartFrame();
   virtual void onEndFrame();

   virtual void onEyeRendered(U32 index);

   virtual void setRoomTracking(bool room);

   bool _handleDeviceEvent(GFXDevice::GFXDeviceEventType evt);

   /// }

   /// @name OpenVR handling
   /// {
   void processVREvent(const vr::VREvent_t & event);
   void updateHMDPose();
   IDevicePose getTrackedDevicePose(U32 idx);

   void orientUniverse(const MatrixF &mat);
   void rotateUniverse(const F32 yaw);

   //void mapDeviceToEvent(U32 deviceIdx, S32 eventIdx);
   //void resetEventMap();

   /// }

   /// @name Overlay registration
   /// {
   void registerOverlay(OpenVROverlay* overlay);
   void unregisterOverlay(OpenVROverlay* overlay);
   /// }

   /// @name Model loading
   /// {
   const S32 preloadRenderModel(StringTableEntry deviceName, StringTableEntry name);
   const S32 preloadRenderModelTexture(StringTableEntry deviceName, U32 index);
   bool getRenderModel(S32 idx, OpenVRRenderModel **ret, bool &failed);
   bool getRenderModelTexture(S32 idx, bool &failed);
   bool getRenderModelTextureName(S32 idx, String &outName);
   void resetRenderModels();
   /// }

   /// @name Compositor Skinning
   /// {
   /// Override the skybox used in the compositor (e.g. for during level loads when the app can't feed scene images fast enough).
   /// Returns true on success.
   bool setSkyboxOverride(CubemapData *cubemap);

   /// Resets the compositor skybox back to defaults.
   void clearSkyboxOverride();

   /// Override the stage model used in the compositor to replace the grid. The render model and texture
   /// will be loaded asynchronously from disk and uploaded to the gpu by the runtime.  Once ready for
   /// rendering, the VREvent StageOverrideReady will be sent. Use FadeGrid to reveal.
   /// Call clearStageOverride to free the associated resources when finished.
   bool setStageOverride_Async(const OpenVRStageModelData* modelData, const MatrixF& transform);

   /// Resets the stage to its default user specified setting.
   void clearStageOverride();

   /// Fade the Grid in or out over fSeconds.
   void fadeGrid(F32 fSeconds, bool bFadeGridIn);

   /// Get current alpha value of the grid. This can be used to determine the current state of the grid fade effect.
   float getCurrentGridAlpha();

   /// Fades the view on the HMD to the specified color. The fade will take fSeconds, and the color values are between
   /// 0.0 and 1.0. This color is faded on top of the scene based on the alpha parameter. Removing the fade color instantly
   /// would be FadeToColor( 0.0, 0.0, 0.0, 0.0, 0.0 ). Values are in un-premultiplied alpha space.
   void fadeToColor(F32 fSeconds, LinearColorF& color, bool bBackground = false);

   /// Get current fade color value. This can be used to determine the current state of the color fade effect.
   LinearColorF getCurrentFadeColor(bool bBackground = false);

   /// }

   /// @name Console API
   /// {
   OpenVROverlay *getGamepadFocusOverlay();

   bool isDashboardVisible();
   void showDashboard(const char *overlayToShow);

   vr::TrackedDeviceIndex_t getPrimaryDashboardDevice();

   void setKeyboardTransformAbsolute(const MatrixF &xfm);
   void setKeyboardPositionForOverlay(OpenVROverlay *overlay, const RectI &rect);

   StringTableEntry getControllerModel(U32 idx);

   U32 getOVRDeviceType() { return mDeviceType; }

   String getDeviceClass(U32 deviceIdx);
   String getDevicePropertyString(U32 deviceIdx, U32 propID);
   bool getDevicePropertyBool(U32 deviceIdx, U32 propID);
   S32 getDevicePropertyInt(U32 deviceIdx, U32 propID);
   String getDevicePropertyUInt(U32 deviceIdx, U32 propID);
   F32 getDevicePropertyFloat(U32 deviceIdx, U32 propID);
   String getControllerAxisType(U32 deviceIdx, U32 axisID);
   String getTrackedDeviceIndices(OpenVRTrackedDeviceClass deviceClass);
   /// }

   /// @name OpenVR state
   /// {
   vr::IVRSystem *mHMD;
   vr::IVRRenderModels *mRenderModels;
   String mDriver;
   String mDisplay;
   vr::TrackedDevicePose_t mTrackedDevicePose;
   IDevicePose mCurrentHMDPose;
   OpenVRRenderState mHMDRenderState;

   vr::ETrackingUniverseOrigin mTrackingSpace;
   F32 mStandingHMDHeight;

   Vector<OpenVROverlay*> mOverlays;

   Vector<LoadedRenderModel> mLoadedModels;
   Vector<LoadedRenderTexture> mLoadedTextures;
   Map<StringTableEntry, S32> mLoadedModelLookup;
   Map<U32, S32> mLoadedTextureLookup;

   /// }

   GuiCanvas* mDrawCanvas;
   GameConnection* mGameConnection;

   /// @name HMD Rotation offset
   /// {
   static F32 smUniverseYawOffset;
   static F32 smHMDmvYaw;
   static bool smRotateYawWithMoveActions;
   static MatrixF smUniverseRotMat;
   /// }

   static String smShapeCachePath;

public:
   // For ManagedSingleton.
   static const char* getSingletonName() { return "OpenVRProvider"; }
};

/// Returns the OpenVRProvider singleton.
#define OPENVR ManagedSingleton<OpenVRProvider>::instance()

#endif   // _OPENVR_PROVIDER_H_
