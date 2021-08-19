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
#include "platform/input/openVR/openVRInput.h"
#include "platform/input/openVR/openVRChaperone.h"
#include "platform/input/openVR/openVRRenderModel.h"
#include "platform/input/openVR/openVROverlay.h"
#include "platform/platformInput.h"
#include "core/module.h"
#include "T3D/gameBase/gameConnection.h"
#include "gui/core/guiCanvas.h"
#include "postFx/postEffectCommon.h"
#include "scene/sceneRenderState.h"
#include "materials/baseMatInstance.h"
#include "materials/materialManager.h"
#include "math/mathUtils.h"
//#include "T3D/gameBase/extended/extendedMove.h"

#ifndef LINUX
#include "gfx/D3D11/gfxD3D11Device.h"
#include "gfx/D3D11/gfxD3D11TextureObject.h"
#include "gfx/D3D11/gfxD3D11EnumTranslate.h"
#endif

#include "materials/matTextureTarget.h"

#ifdef TORQUE_OPENGL
#include "gfx/gl/gfxGLDevice.h"
#include "gfx/gl/gfxGLTextureObject.h"
#include "gfx/gl/gfxGLEnumTranslate.h"
#endif

#include "gfx/gfxTextureManager.h"

//struct OpenVRLoadedTexture
//{
//   vr::TextureID_t texId;
//   NamedTexTarget texTarget;
//};

namespace OpenVRUtil
{
   void convertTransformFromOVR(const MatrixF &inRotTMat, MatrixF& outRotation)
   {
      Point4F col0; inRotTMat.getColumn(0, &col0);
      Point4F col1; inRotTMat.getColumn(1, &col1);
      Point4F col2; inRotTMat.getColumn(2, &col2);
      Point4F col3; inRotTMat.getColumn(3, &col3);

      // Set rotation.  We need to convert from sensor coordinates to
      // Torque coordinates.  The sensor matrix is stored row-major.
      // The conversion is:
      //
      // Sensor                       Torque
      // a b c         a  b  c        a -c  b
      // d e f   -->  -g -h -i  -->  -g  i -h
      // g h i         d  e  f        d -f  e
      outRotation.setRow(0, Point4F( col0.x, -col2.x,  col1.x,  col3.x));
      outRotation.setRow(1, Point4F(-col0.z,  col2.z, -col1.z, -col3.z));
      outRotation.setRow(2, Point4F( col0.y, -col2.y,  col1.y,  col3.y));
      outRotation.setRow(3, Point4F(0.0f, 0.0f, 0.0f, 1.0f));
   }

   void convertTransformToOVR(const MatrixF& inRotation, MatrixF& outRotation)
   {
      Point4F col0; inRotation.getColumn(0, &col0);
      Point4F col1; inRotation.getColumn(1, &col1);
      Point4F col2; inRotation.getColumn(2, &col2);
      Point4F col3; inRotation.getColumn(3, &col3);

      // This is basically a reverse of what is in convertTransformFromOVR
      outRotation.setColumn(0, Point4F(col0.x, col2.x, -col1.x, 0.0f));
      outRotation.setColumn(1, Point4F(col0.z, col2.z, -col1.z, 0.0f));
      outRotation.setColumn(2, Point4F(-col0.y, -col2.y, col1.y, 0.0f));
      outRotation.setColumn(3, Point4F(-col3.x, -col3.z, col3.y, 1.0f));
   }

   MatrixF convertSteamVRAffineMatrixToMatrixFPlain(const vr::HmdMatrix34_t &mat)
   {
      MatrixF outMat(1);

      outMat.setColumn(0, Point4F(mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0));
      outMat.setColumn(1, Point4F(mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0));
      outMat.setColumn(2, Point4F(mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0));
      outMat.setColumn(3, Point4F(mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f)); // pos

      return outMat;
   }

   void convertMatrixFPlainToSteamVRAffineMatrix(const MatrixF &inMat, vr::HmdMatrix34_t &outMat)
   {
      Point4F row0; inMat.getRow(0, &row0);
      Point4F row1; inMat.getRow(1, &row1);
      Point4F row2; inMat.getRow(2, &row2);

      outMat.m[0][0] = row0.x;
      outMat.m[0][1] = row0.y;
      outMat.m[0][2] = row0.z;
      outMat.m[0][3] = row0.w;

      outMat.m[1][0] = row1.x;
      outMat.m[1][1] = row1.y;
      outMat.m[1][2] = row1.z;
      outMat.m[1][3] = row1.w;

      outMat.m[2][0] = row2.x;
      outMat.m[2][1] = row2.y;
      outMat.m[2][2] = row2.z;
      outMat.m[2][3] = row2.w;
   }

   U32 convertOpenVRButtonToTorqueButton(uint32_t vrButton)
   {
      switch (vrButton)
      {
      case vr::VRMouseButton_Left:
         return KEY_BUTTON0;
      case vr::VRMouseButton_Right:
         return KEY_BUTTON1;
      case vr::VRMouseButton_Middle:
         return KEY_BUTTON2;
      default:
         return KEY_NULL;
      }
   }


   vr::VRTextureBounds_t TorqueRectToBounds(const RectI &rect, const Point2I &widthHeight)
   {
      vr::VRTextureBounds_t bounds;
      F32 xRatio = 1.0 / (F32)widthHeight.x;
      F32 yRatio = 1.0 / (F32)widthHeight.y;
      bounds.uMin = rect.point.x * xRatio;
      bounds.vMin = rect.point.y * yRatio;
      bounds.uMax = (rect.point.x + rect.extent.x) * xRatio;
      bounds.vMax = (rect.point.y + rect.extent.y) * yRatio;
      return bounds;
   }

   String GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
   {
      uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
      if (unRequiredBufferLen == 0)
         return "";

      char *pchBuffer = new char[unRequiredBufferLen];
      unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
      String sResult = pchBuffer;
      delete[] pchBuffer;
      return sResult;
   }

}

//------------------------------------------------------------

F32 OpenVRProvider::smUniverseYawOffset = 0.0f;
MatrixF OpenVRProvider::smUniverseRotMat = MatrixF(1);
F32 OpenVRProvider::smHMDmvYaw = 0;
bool OpenVRProvider::smRotateYawWithMoveActions = false;
String OpenVRProvider::smShapeCachePath("");

static String GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
{
   uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
   if (unRequiredBufferLen == 0)
      return "";

   char *pchBuffer = new char[unRequiredBufferLen];
   unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
   String sResult = pchBuffer;
   delete[] pchBuffer;
   return sResult;
}

//------------------------------------------------------------

MODULE_BEGIN(OpenVRProvider)

MODULE_INIT_AFTER(InputEventManager)
MODULE_SHUTDOWN_BEFORE(InputEventManager)

MODULE_INIT
{
   OpenVRProvider::staticInit();
   ManagedSingleton< OpenVRProvider >::createSingleton();
   ManagedSingleton< OpenVRChaperone >::createSingleton();
   ManagedSingleton< OpenVRInput >::createSingleton();
}

MODULE_SHUTDOWN
{
   ManagedSingleton< OpenVRInput >::deleteSingleton();
   ManagedSingleton< OpenVRChaperone >::deleteSingleton();
   ManagedSingleton< OpenVRProvider >::deleteSingleton();
}

MODULE_END;

IMPLEMENT_GLOBAL_CALLBACK(onHMDPose, void, (Point3F position, Point4F rotation, Point3F linVel, Point3F angVel),
   (position, rotation, linVel, angVel),
   "Callback posted with updated hmd tracking data.\n"
   "@ingroup OpenVR");

IMPLEMENT_GLOBAL_CALLBACK(onOVRDeviceActivated, void, (S32 deviceIndex), (deviceIndex),
   "Callback posted when a tracked device is detected and added to the system. This "
   "will be called during startup for each device initially detected and also any time "
   "a device is turned on after initialization.\n"
   "@param deviceIndex - The internal device index. Use this value to query additional "
   "information about the device with getControllerModel() and getDeviceProperty...()\n"
   "@ingroup OpenVR");

IMPLEMENT_GLOBAL_CALLBACK(onOVRDeviceRoleChanged, void, (), (),
   "Callback posted when a tracked device has changed roles. Usually in response to an "
   "ambidextrous controller being assigned to a different hand.\n"
   "@ingroup OpenVR");

bool OpenVRRenderState::setupRenderTargets(GFXDevice::GFXDeviceRenderStyles mode)
{
   if (!mHMD)
      return false;

   if (mRenderMode == mode)
      return true;

   mRenderMode = mode;

   if (mode == GFXDevice::RS_Standard)
   {
      reset(mHMD);
      return true;
   }

   U32 sizeX, sizeY;
   Point2I newRTSize;
   mHMD->GetRecommendedRenderTargetSize(&sizeX, &sizeY);

   if (mode == GFXDevice::RS_StereoSeparate)
   {
      mEyeViewport[0] = RectI(Point2I(0, 0), Point2I(sizeX, sizeY));
      mEyeViewport[1] = RectI(Point2I(0, 0), Point2I(sizeX, sizeY));

      newRTSize.x = sizeX;
      newRTSize.y = sizeY;
   }
   else
   {
      mEyeViewport[0] = RectI(Point2I(0, 0), Point2I(sizeX, sizeY));
      mEyeViewport[1] = RectI(Point2I(sizeX, 0), Point2I(sizeX, sizeY));

      newRTSize.x = sizeX * 2;
      newRTSize.y = sizeY;
   }

   GFXTexHandle stereoTexture;
   stereoTexture.set(newRTSize.x, newRTSize.y, GFXFormatR8G8B8A8_SRGB, &VRTextureProfile, "OpenVR Stereo RT Color");
   mStereoRenderTexture = stereoTexture;

   GFXTexHandle stereoDepthTexture;
   stereoDepthTexture.set(newRTSize.x, newRTSize.y, GFXFormatD24S8, &VRDepthProfile, "OpenVR Depth");
   mStereoDepthTexture = stereoDepthTexture;

   if (!mStereoRT )
   {
      mStereoRT = GFX->allocRenderToTextureTarget();
      mStereoRT->attachTexture(GFXTextureTarget::Color0, stereoTexture);
      mStereoRT->attachTexture(GFXTextureTarget::DepthStencil, stereoDepthTexture);
      GFXTextureManager::addEventDelegate( this, &OpenVRRenderState::_onTextureEvent );
   }

   mOutputEyeTextures.init(newRTSize.x, newRTSize.y, GFXFormatR8G8B8A8_SRGB, &VRTextureProfile, "OpenVR Stereo RT Color OUTPUT");

   return true;
}

void OpenVRRenderState::renderPreview()
{

}

void OpenVRRenderState::reset(vr::IVRSystem* hmd)
{
   mHMD = hmd;

   if (mStereoRT && mStereoRT.isValid())
      GFXTextureManager::removeEventDelegate( this, &OpenVRRenderState::_onTextureEvent );
   mStereoRT = NULL;

   mStereoRenderTexture = NULL;
   mStereoDepthTexture = NULL;

   mOutputEyeTextures.clear();

   if (!mHMD)
      return;

   updateHMDProjection();
}

void OpenVRRenderState::updateHMDProjection()
{
   vr::HmdMatrix34_t vrMat = mHMD->GetEyeToHeadTransform(vr::Eye_Left);
   MatrixF plainMat = OpenVRUtil::convertSteamVRAffineMatrixToMatrixFPlain(vrMat);
   OpenVRUtil::convertTransformFromOVR(plainMat, mEyePose[0]);

   vrMat = mHMD->GetEyeToHeadTransform(vr::Eye_Right);
   plainMat = OpenVRUtil::convertSteamVRAffineMatrixToMatrixFPlain(vrMat);
   OpenVRUtil::convertTransformFromOVR(plainMat, mEyePose[1]);

   mHMD->GetProjectionRaw(vr::Eye_Left, &mEyeFov[0].leftTan, &mEyeFov[0].rightTan, &mEyeFov[0].upTan, &mEyeFov[0].downTan);
   mHMD->GetProjectionRaw(vr::Eye_Right, &mEyeFov[1].leftTan, &mEyeFov[1].rightTan, &mEyeFov[1].upTan, &mEyeFov[1].downTan);

   mEyeFov[0].upTan = -mEyeFov[0].upTan;
   mEyeFov[0].leftTan = -mEyeFov[0].leftTan;
   mEyeFov[1].upTan = -mEyeFov[1].upTan;
   mEyeFov[1].leftTan = -mEyeFov[1].leftTan;

   // Up is Down?!?
   F32 tempF = mEyeFov[0].downTan; mEyeFov[0].downTan = mEyeFov[0].upTan; mEyeFov[0].upTan = tempF;
   tempF = mEyeFov[1].downTan; mEyeFov[1].downTan = mEyeFov[1].upTan; mEyeFov[1].upTan = tempF;
}

void OpenVRRenderState::_onTextureEvent( GFXTexCallbackCode code )
{
   if (code == GFXZombify)
   {
      reset(mHMD);
      mRenderMode = GFXDevice::RS_Standard;
   }
}

OpenVRProvider::OpenVRProvider() :
   mHMD(NULL),
   mRenderModels(NULL),
   mTrackingSpace(vr::TrackingUniverseStanding),
   mStandingHMDHeight(1.571f),
   mDrawCanvas(NULL),
   mGameConnection(NULL)
{
   mDeviceType = INPUTMGR->getNextDeviceType();
   GFXDevice::getDeviceEventSignal().notify(this, &OpenVRProvider::_handleDeviceEvent);
   INPUTMGR->registerDevice(this);
}

OpenVRProvider::~OpenVRProvider()
{
   resetRenderModels();
}

void OpenVRProvider::staticInit()
{
   // Overlay flags
   // Set this flag on a dashboard overlay to prevent a tab from showing up for that overlay
   Con::setIntVariable("$OpenVR::OverlayFlags_NoDashboardTab", vr::VROverlayFlags_NoDashboardTab);

   // When this is set the overlay will receive VREvent_ScrollDiscrete events like a mouse wheel.
   // Requires mouse input mode.
   Con::setIntVariable("$OpenVR::OverlayFlags_SendVRDiscreteScrollEvents", vr::VROverlayFlags_SendVRDiscreteScrollEvents);

   // Indicates that the overlay would like to receive
   Con::setIntVariable("$OpenVR::OverlayFlags_SendVRTouchpadEvents", vr::VROverlayFlags_SendVRTouchpadEvents);

   // If set this will render a vertical scroll wheel on the primary controller,
   //  only needed if not using VROverlayFlags_SendVRScrollEvents but you still want to represent a scroll wheel
   Con::setIntVariable("$OpenVR::OverlayFlags_ShowTouchPadScrollWheel", vr::VROverlayFlags_ShowTouchPadScrollWheel);

   // If this is set ownership and render access to the overlay are transferred
   // to the new scene process on a call to IVRApplications::LaunchInternalProcess
   Con::setIntVariable("$OpenVR::OverlayFlags_TransferOwnershipToInternalProcess",vr::VROverlayFlags_TransferOwnershipToInternalProcess);

   // If this is set ownership and render access to the overlay are transferred
   // to the new scene process on a call to IVRApplications::LaunchInternalProcess
   Con::setIntVariable("$OpenVR::OverlayFlags_TransferOwnershipToInternalProcess", vr::VROverlayFlags_TransferOwnershipToInternalProcess);

   // If set, renders 50% of the texture in each eye, side by side
   Con::setIntVariable("$OpenVR::OverlayFlags_SideBySide_Parallel", vr::VROverlayFlags_SideBySide_Parallel); // Texture is left/right
   Con::setIntVariable("$OpenVR::OverlayFlags_SideBySide_Crossed", vr::VROverlayFlags_SideBySide_Crossed); // Texture is crossed and right/left

   // Texture is a panorama
   Con::setIntVariable("$OpenVR::OverlayFlags_Panorama", vr::VROverlayFlags_Panorama);

   // Texture is a stereo panorama
   Con::setIntVariable("$OpenVR::OverlayFlags_StereoPanorama", vr::VROverlayFlags_StereoPanorama);

   // If this is set on an overlay owned by the scene application that overlay
   // will be sorted with the "Other" overlays on top of all other scene overlays
   Con::setIntVariable("$OpenVR::OverlayFlags_SortWithNonSceneOverlays", vr::VROverlayFlags_SortWithNonSceneOverlays);

   // // If set, the overlay will be shown in the dashboard, otherwise it will be hidden.
   Con::setIntVariable("$OpenVR::OverlayFlags_VisibleInDashboard", vr::VROverlayFlags_VisibleInDashboard);

   // If this is set and the overlay's input method is not none, the system-wide laser mouse
   // mode will be activated whenever this overlay is visible.
   Con::setIntVariable("$OpenVR::MakeOverlaysInteractiveIfVisible", vr::VROverlayFlags_MakeOverlaysInteractiveIfVisible);

   // If this is set the overlay will receive smooth VREvent_ScrollSmooth that emulate trackpad scrolling.
   // Requires mouse input mode.
   Con::setIntVariable("$OpenVR::OverlayFlags_SendVRSmoothScrollEvents",  vr::VROverlayFlags_SendVRSmoothScrollEvents);

   // If this is set, the overlay texture will be protected content, preventing unauthorized reads.
   Con::setIntVariable("$OpenVR::OverlayFlags_ProtectedContent", vr::VROverlayFlags_ProtectedContent);

   // If this is set, the laser mouse splat will not be drawn over this overlay. The overlay will
   // be responsible for drawing its own "cursor".
   Con::setIntVariable("$OpenVR::OverlayFlags_HideLaserIntersection", vr::VROverlayFlags_HideLaserIntersection);

   // If this is set, clicking away from the overlay will cause it to receive a VREvent_Modal_Cancel event.
   // This is ignored for dashboard overlays.
   Con::setIntVariable("$OpenVR::OverlayFlags_WantsModalBehavior", vr::VROverlayFlags_WantsModalBehavior);

   // If this is set, alpha composition assumes the texture is pre-multiplied
   Con::setIntVariable("$OpenVR::OverlayFlags_IsPremultiplied", vr::VROverlayFlags_IsPremultiplied);

   Con::addVariable("$OpenVR::TrackingUniverseYaw", TypeF32, &smUniverseYawOffset,
      "This yaw value (radians) is used to rotate the vr tracking universe into the 3D world. "
      "e.g. Spawning a player and their perception of forward should be something other than "
      "the +Y axis in the scene.");
   Con::addVariable("$OpenVR::HMDmvYaw", TypeF32, &smHMDmvYaw);

   Con::addVariable("$OpenVR::HMDRotateYawWithMoveActions", TypeBool, &smRotateYawWithMoveActions);
   Con::addVariable( "$OpenVR::cachePath", TypeRealString, &smShapeCachePath,
      "The file path to the directory where texture and shape data are to be cached.\n" );
}

bool OpenVRProvider::enable()
{
   disable();

   // Load openvr runtime
   vr::EVRInitError eError = vr::VRInitError_None;
   mHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

   //dMemset(mDeviceClassChar, '\0', sizeof(mDeviceClassChar));

   if (eError != vr::VRInitError_None)
   {
      mHMD = NULL;
      char buf[1024];
      dSprintf(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
      Con::printf(buf);
      return false;
   }

   mRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
   if (!mRenderModels)
   {
      mHMD = NULL;
      vr::VR_Shutdown();

      char buf[1024];
      dSprintf(buf, sizeof(buf), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
      Con::printf(buf);
      return false;
   }

   mDriver = GetTrackedDeviceString(mHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
   mDisplay = GetTrackedDeviceString(mHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

   mHMDRenderState.mHMDPose = MatrixF(1);
   mHMDRenderState.mEyePose[0] = MatrixF(1);
   mHMDRenderState.mEyePose[1] = MatrixF(1);

   mHMDRenderState.reset(mHMD);
   mEnabled = true;

   return true;
}

bool OpenVRProvider::disable()
{
   if (mHMD)
   {
      resetRenderModels();
      mHMD = NULL;
      mRenderModels = NULL;
      mHMDRenderState.reset(NULL);
      vr::VR_Shutdown();
   }

   mEnabled = false;

   return false;
}

bool OpenVRProvider::process()
{
   if (!mHMD)
      return true;

   if (!vr::VRCompositor())
      return true;

   if (smRotateYawWithMoveActions)
      smHMDmvYaw += MoveManager::mYawLeftSpeed - MoveManager::mYawRightSpeed;

   // Update the tracking universe rotation
   if (smHMDmvYaw != 0.0f)
   {
      smUniverseYawOffset += smHMDmvYaw;

      while (smUniverseYawOffset < -M_PI_F)
         smUniverseYawOffset += M_2PI_F;
      while (smUniverseYawOffset > M_PI_F)
         smUniverseYawOffset -= M_2PI_F;
      smUniverseRotMat.set(EulerF(0.0f, smUniverseYawOffset, 0.0f));
   }
   smHMDmvYaw = 0.0f;

   // Process SteamVR events
   vr::VREvent_t event;
   while (mHMD->PollNextEvent(&event, sizeof(event)))
   {
      processVREvent(event);
   }

   // process overlay events
   for (U32 i = 0; i < mOverlays.size(); i++)
   {
      mOverlays[i]->handleOpenVREvents();
   }

   // Update the hmd pose
   updateHMDPose();

   OVRINPUT->processInput();

   return true;
}

bool OpenVRProvider::providesFrameEyePose() const
{
   return mHMD != NULL;
}

void OpenVRTransformToRotPos(MatrixF mat, QuatF &outRot, Point3F &outPos)
{
   // Directly set the rotation and position from the eye transforms
   MatrixF torqueMat;
   OpenVRUtil::convertTransformFromOVR(mat, torqueMat);

   outRot = QuatF(torqueMat);
   outPos = torqueMat.getPosition();
}

void OpenVRTransformToRotPosMat(MatrixF mat, QuatF &outRot, Point3F &outPos, MatrixF &outMat)
{
   // Directly set the rotation and position from the eye transforms
   MatrixF torqueMat(1);
   OpenVRUtil::convertTransformFromOVR(mat, torqueMat);

   outRot = QuatF(torqueMat);
   outPos = torqueMat.getPosition();
   outMat = torqueMat;
}

void OpenVRProvider::getFrameEyePose(IDevicePose *pose, S32 eyeId) const
{
   AssertFatal(eyeId >= -1 && eyeId < 2, "Out of bounds eye");

   if (eyeId == -1)
   {
      // NOTE: this is codename for "head"
      pose->orientation.set(mHMDRenderState.mHMDPose);
      pose->position.set(mHMDRenderState.mHMDPose.getPosition());
      pose->velocity = Point3F(0);
      pose->angularVelocity = Point3F(0);
   }
   else
   {
      MatrixF mat = mHMDRenderState.mHMDPose * mHMDRenderState.mEyePose[eyeId];

      pose->orientation.set(mat);
      pose->position.set(mat.getPosition());
      pose->velocity = Point3F(0);
      pose->angularVelocity = Point3F(0);
   
      //Point3F hmdPos = mHMDRenderState.mHMDPose.getPosition();
      //QuatF testQuat(mHMDRenderState.mHMDPose);
      //Con::printf("Eye: %s - Rotations %s, HMD: %7.3f,  %7.3f,  %7.3f Eye:  %7.3f,  %7.3f,  %7.3f",
      //   eyeId == 0 ? "Left" : "Right", testQuat == pose->orientation ? "Equal" : "Not Equal",
      //   hmdPos.x, hmdPos.y, hmdPos.z, pose->position.x, pose->position.y, pose->position.z);

      //Point3F eyeFwd = mat.getForwardVector();
      //Point3F hmdFwd = mHMDRenderState.mHMDPose.getForwardVector();
      //Con::printf(" HMD Forward: %7.3f,  %7.3f,  %7.3f Eye Forward:  %7.3f,  %7.3f,  %7.3f",
      //   hmdFwd.x, hmdFwd.y, hmdFwd.z, eyeFwd.x, eyeFwd.y, eyeFwd.z);
   }
}

bool OpenVRProvider::providesEyeOffsets() const
{
   return mHMD != NULL;
}

/// Returns eye offset not taking into account any position tracking info
void OpenVRProvider::getEyeOffsets(Point3F *dest) const
{
   dest[0] = mHMDRenderState.mEyePose[0].getPosition();
   dest[1] = mHMDRenderState.mEyePose[1].getPosition();
}

bool OpenVRProvider::providesFovPorts() const
{
   return mHMD != NULL;
}

void OpenVRProvider::getFovPorts(FovPort *out) const
{
   dMemcpy(out, mHMDRenderState.mEyeFov, sizeof(mHMDRenderState.mEyeFov));
}

void OpenVRProvider::getStereoViewports(RectI *out) const
{
   out[0] = mHMDRenderState.mEyeViewport[0];
   out[1] = mHMDRenderState.mEyeViewport[1];
}

void OpenVRProvider::getStereoTargets(GFXTextureTarget **out) const
{
   out[0] = mHMDRenderState.mStereoRT;
   out[1] = mHMDRenderState.mStereoRT;
}

void OpenVRProvider::setDrawCanvas(GuiCanvas *canvas)
{
   if (!vr::VRCompositor())
   {
      Con::errorf("VR: Compositor initialization failed. See log file for details\n");
      return;
   }

   if (mDrawCanvas != canvas || mHMDRenderState.mHMD == NULL)
   {
      mHMDRenderState.setupRenderTargets(GFXDevice::RS_Standard);
   }
   mDrawCanvas = canvas;
}

void OpenVRProvider::setDrawMode(GFXDevice::GFXDeviceRenderStyles style)
{
   mHMDRenderState.setupRenderTargets(style);
}

void OpenVRProvider::setCurrentConnection(GameConnection *connection)
{
   mGameConnection = connection;
}

GameConnection* OpenVRProvider::getCurrentConnection()
{
   return mGameConnection;
}

GFXTexHandle OpenVRProvider::getPreviewTexture()
{
   return mHMDRenderState.mStereoRenderTexture; // TODO: render distortion preview
}

void OpenVRProvider::onStartFrame()
{
   if (!mHMD)
      return;

}

void OpenVRProvider::onEndFrame()
{
   if (!mHMD)
      return;
}

void OpenVRProvider::onEyeRendered(U32 index)
{
   if (!mHMD)
      return;

   vr::EVRCompositorError err = vr::VRCompositorError_None;
   vr::VRTextureBounds_t bounds;

   GFXTexHandle eyeTex = mHMDRenderState.mOutputEyeTextures.getTextureHandle();
   if (mHMDRenderState.mRenderMode == GFXDevice::RS_StereoSeparate)
   {
      mHMDRenderState.mStereoRT->resolveTo(eyeTex);
      mHMDRenderState.mOutputEyeTextures.advance();
   }
   else
   {
      // assuming side-by-side, so the right eye will be next
      if (index == 1)
      {
         mHMDRenderState.mStereoRT->resolveTo(eyeTex);
         mHMDRenderState.mOutputEyeTextures.advance();
      }
      else
      {
         return;
      }
   }

#if defined(TORQUE_OS_WIN64) || defined(TORQUE_OS_WIN32) || defined(TORQUE_D3D11)
   if (GFX->getAdapterType() == Direct3D11)
   {
      vr::Texture_t eyeTexture;
      if (mHMDRenderState.mRenderMode == GFXDevice::RS_StereoSeparate)
      {
         // whatever eye we are on
         eyeTexture = { (void*)static_cast<GFXD3D11TextureObject*>(eyeTex.getPointer())->get2DTex(), vr::TextureType_DirectX, vr::ColorSpace_Gamma };
         bounds = OpenVRUtil::TorqueRectToBounds(mHMDRenderState.mEyeViewport[index], mHMDRenderState.mStereoRenderTexture.getWidthHeight());
         err = vr::VRCompositor()->Submit((vr::EVREye)(vr::Eye_Left + index), &eyeTexture, &bounds);
      }
      else
      {
         // left & right at the same time
         eyeTexture = { (void*)static_cast<GFXD3D11TextureObject*>(eyeTex.getPointer())->get2DTex(), vr::TextureType_DirectX, vr::ColorSpace_Gamma };
         bounds = OpenVRUtil::TorqueRectToBounds(mHMDRenderState.mEyeViewport[0], mHMDRenderState.mStereoRenderTexture.getWidthHeight());
         err = vr::VRCompositor()->Submit((vr::EVREye)(vr::Eye_Left), &eyeTexture, &bounds);
         bounds = OpenVRUtil::TorqueRectToBounds(mHMDRenderState.mEyeViewport[1], mHMDRenderState.mStereoRenderTexture.getWidthHeight());
         err = vr::VRCompositor()->Submit((vr::EVREye)(vr::Eye_Right), &eyeTexture, &bounds);
      }
   }
#endif
#ifdef TORQUE_OPENGL
   if (GFX->getAdapterType() == OpenGL)
   {
      vr::Texture_t eyeTexture;
      F32 tempMin;
      if (mHMDRenderState.mRenderMode == GFXDevice::RS_StereoSeparate)
      {
         // whatever eye we are on
         eyeTexture = { (void*)(uintptr_t)static_cast<GFXGLTextureObject*>(eyeTex.getPointer())->getHandle(), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
         bounds = OpenVRUtil::TorqueRectToBounds(mHMDRenderState.mEyeViewport[index], mHMDRenderState.mStereoRenderTexture.getWidthHeight());
         tempMin = bounds.vMin; bounds.vMin = bounds.vMax; bounds.vMax = tempMin; // Flip vertically for ogl
         err = vr::VRCompositor()->Submit((vr::EVREye)(vr::Eye_Left + index), &eyeTexture, &bounds);
      }
      else
      {
         // left & right at the same time
         eyeTexture = { (void*)(uintptr_t)static_cast<GFXGLTextureObject*>(eyeTex.getPointer())->getHandle(), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
         bounds = OpenVRUtil::TorqueRectToBounds(mHMDRenderState.mEyeViewport[0], mHMDRenderState.mStereoRenderTexture.getWidthHeight());
         tempMin = bounds.vMin; bounds.vMin = bounds.vMax; bounds.vMax = tempMin; // Flip vertically for ogl
         err = vr::VRCompositor()->Submit((vr::EVREye)(vr::Eye_Left), &eyeTexture, &bounds);
         bounds = OpenVRUtil::TorqueRectToBounds(mHMDRenderState.mEyeViewport[1], mHMDRenderState.mStereoRenderTexture.getWidthHeight());
         tempMin = bounds.vMin; bounds.vMin = bounds.vMax; bounds.vMax = tempMin; // Flip vertically for ogl
         err = vr::VRCompositor()->Submit((vr::EVREye)(vr::Eye_Right), &eyeTexture, &bounds);
      }
   }
#endif

   AssertFatal(err == vr::VRCompositorError_None, "VR compositor error!");
}

void OpenVRProvider::setRoomTracking(bool room)
{
   vr::IVRCompositor* compositor = vr::VRCompositor();
   mTrackingSpace = room ? vr::TrackingUniverseStanding : vr::TrackingUniverseSeated;
   if (compositor)
      compositor->SetTrackingSpace(mTrackingSpace);
}

bool OpenVRProvider::_handleDeviceEvent(GFXDevice::GFXDeviceEventType evt)
{
   if (!ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      return true;
   }

   switch (evt)
   {
   case GFXDevice::deStartOfFrame:

      // Start of frame

      onStartFrame();

      break;

   case GFXDevice::dePostFrame:

      // End of frame

      onEndFrame();

      break;

   case GFXDevice::deDestroy:

      // Need to reinit rendering
      break;

   case GFXDevice::deLeftStereoFrameRendered:
      //

      onEyeRendered(0);
      break;

   case GFXDevice::deRightStereoFrameRendered:
      //

      onEyeRendered(1);
      break;

   default:
      break;
   }

   return true;
}

void OpenVRProvider::processVREvent(const vr::VREvent_t & evt)
{
//#ifdef TORQUE_DEBUG
//   Con::printf("OpenVR Event: %s", mHMD->GetEventTypeNameFromEnum((vr::EVREventType) evt.eventType));
//#endif
   switch (evt.eventType)
   {
   case vr::VREvent_InputFocusCaptured:
      //Con::executef()
      break;
   case vr::VREvent_TrackedDeviceActivated:
   {
      // Setup render model
      // Send script callback that a device is active
      onOVRDeviceActivated_callback(evt.trackedDeviceIndex);
      //Con::executef("onOVRDeviceActivated", Con::getIntArg(evt.trackedDeviceIndex));
   }
   break;
   case vr::VREvent_TrackedDeviceDeactivated:
   {
      // Deactivated
   }
   break;
   case vr::VREvent_TrackedDeviceUpdated:
   {
      // Updated
   }
   break;
   case vr::VREvent_IpdChanged:
   {
      mHMDRenderState.updateHMDProjection();
   }
   break;
   case vr::VREvent_TrackedDeviceRoleChanged:
   {
      // Send script callback that a device has changed roles
      onOVRDeviceRoleChanged_callback();
   }
   break;
   }
}

void OpenVRProvider::updateHMDPose()
{
   if (!mHMD)
      return;

   vr::IVRCompositor* compositor = vr::VRCompositor();

   if (!compositor)
      return;

   if (compositor->GetTrackingSpace() != mTrackingSpace)
   {
      compositor->SetTrackingSpace(mTrackingSpace);
   }

   compositor->WaitGetPoses(&mTrackedDevicePose, 1, NULL, 0);

   IDevicePose &inPose = mCurrentHMDPose;
   if (mTrackedDevicePose.bPoseIsValid)
   {
      MatrixF vrMat = OpenVRUtil::convertSteamVRAffineMatrixToMatrixFPlain(mTrackedDevicePose.mDeviceToAbsoluteTracking);
      vr::TrackedDevicePose_t &outPose = mTrackedDevicePose;

      // If the tracking universe has been rotated relative to the T3D world, rotate the Hmd pose
      if (!mIsZero(smUniverseYawOffset))
      {
         vrMat.mulL(smUniverseRotMat);
      }

      if (mTrackingSpace == vr::TrackingUniverseStanding)
      {  // Subtract calibrated standing height so we get consistent hmd positions across universes.
         vrMat[7] -= mStandingHMDHeight;
      }

      MatrixF torqueMat;
      OpenVRUtil::convertTransformFromOVR(vrMat, torqueMat);
      inPose.orientation.set(torqueMat);
      inPose.position.set(torqueMat.getPosition());
      mHMDRenderState.mHMDPose = torqueMat;

#ifdef DEBUG_DISPLAY_POSE
      OpenVRUtil::convertTransformFromOVR(mat, inPose.actualMatrix);
      inPose.originalMatrix = mat;
#endif

      inPose.state = outPose.eTrackingResult;
      inPose.valid = outPose.bPoseIsValid;
      inPose.connected = outPose.bDeviceIsConnected;

      inPose.velocity = OpenVRUtil::convertPointFromOVR(outPose.vVelocity);
      inPose.angularVelocity = OpenVRUtil::convertPointFromOVR(outPose.vAngularVelocity);

      Point4F hmdRot(inPose.orientation.x, inPose.orientation.y, inPose.orientation.z, inPose.orientation.w);
      onHMDPose_callback(inPose.position, hmdRot, inPose.velocity, inPose.angularVelocity);
   }
   else
   {
      inPose.valid = false;
   }
}

IDevicePose OpenVRProvider::getTrackedDevicePose(U32 idx)
{
   if (idx > vr::k_unTrackedDeviceIndex_Hmd)
   {
      IDevicePose ret;
      ret.connected = ret.valid = false;
      return ret;
   }

   return mCurrentHMDPose;
}

void OpenVRProvider::registerOverlay(OpenVROverlay* overlay)
{
   mOverlays.push_back(overlay);
}

void OpenVRProvider::unregisterOverlay(OpenVROverlay* overlay)
{
   S32 index = mOverlays.find_next(overlay);
   if (index != -1)
   {
      mOverlays.erase(index);
   }
}

const S32 OpenVRProvider::preloadRenderModelTexture(StringTableEntry deviceName, U32 index)
{
   S32 idx = -1;
   if (mLoadedTextureLookup.tryGetValue(index, idx))
      return idx;

   char buffer[1024];
   LoadedRenderTexture loadedTexture;

   loadedTexture.vrTextureId = index;
   loadedTexture.vrTexture = NULL;
   loadedTexture.textureError = vr::VRRenderModelError_Loading;

   dSprintf(buffer, sizeof(buffer), "%s%d", deviceName, index);
   loadedTexture.textureName = StringTable->insert(buffer, true);
   dSprintf(buffer, sizeof(buffer), "%s%s%d.png", smShapeCachePath.c_str(), deviceName, index);
   loadedTexture.texturePath = StringTable->insert(buffer, true);
   loadedTexture.textureCached = Torque::FS::IsFile( buffer );

   mLoadedTextures.push_back(loadedTexture);
   mLoadedTextureLookup[index] = mLoadedTextures.size() - 1;

   return mLoadedTextures.size() - 1;
}

const S32 OpenVRProvider::preloadRenderModel(StringTableEntry deviceName, StringTableEntry name)
{
   S32 idx = -1;
   if (mLoadedModelLookup.tryGetValue(name, idx))
      return idx;

   LoadedRenderModel loadedModel;
   loadedModel.deviceName = deviceName;
   loadedModel.name = name;
   loadedModel.model = NULL;
   loadedModel.vrModel = NULL;
   loadedModel.modelError = vr::VRRenderModelError_Loading;
   loadedModel.loadedTexture = false;
   loadedModel.textureId = -1;
   mLoadedModels.push_back(loadedModel);
   mLoadedModelLookup[name] = mLoadedModels.size() - 1;

   return mLoadedModels.size() - 1;
}


bool OpenVRProvider::getRenderModel(S32 idx, OpenVRRenderModel **ret, bool &failed)
{
   if (idx < 0 || idx > mLoadedModels.size())
   {
      failed = true;
      return true;
   }

   LoadedRenderModel &loadedModel = mLoadedModels[idx];
   failed = false;

   if (loadedModel.modelError > vr::VRRenderModelError_Loading)
   {
      failed = true;
      return true;
   }

   // Stage 1 : model
   if (!loadedModel.model)
   {
      loadedModel.modelError = vr::VRRenderModels()->LoadRenderModel_Async(loadedModel.name, &loadedModel.vrModel);
      if (loadedModel.modelError == vr::VRRenderModelError_None)
      {
         if (loadedModel.vrModel == NULL)
         {
            failed = true;
            return true;
         }
         // Load the model
         loadedModel.model = new OpenVRRenderModel();
      }
      else if (loadedModel.modelError == vr::VRRenderModelError_Loading)
      {
         return false;
      }
   }

   // Stage 2 : texture
   if (loadedModel.loadedTexture)
   {
      String materialName = MATMGR->getMapEntry(mLoadedTextures[loadedModel.textureId].textureName);
      if (materialName.isEmpty())
      {
         char buffer[256];
         materialName = mLoadedTextures[loadedModel.textureId].textureName;

         //Con::printf("RenderModel[%i] materialName == %s", idx, buffer);

         Material* mat = new Material();
         mat->mMapTo = mLoadedTextures[loadedModel.textureId].textureName;
         mat->mDiffuseMapFilename[0] = mLoadedTextures[loadedModel.textureId].texturePath;
         mat->mEmissive[0] = true;
         mat->mCastShadows = true;

         dSprintf(buffer, sizeof(buffer), "%s_Mat", mLoadedTextures[loadedModel.textureId].textureName);
         if (!mat->registerObject(buffer))
         {
            Con::errorf("Couldn't create placeholder openvr material %s!", buffer);
            failed = true;
            return true;
         }

         materialName = buffer;
      }

      loadedModel.model->reinitMaterial(materialName);
   }
   if (!loadedModel.loadedTexture && loadedModel.model)
   {
      if (loadedModel.textureId == -1)
      {
         loadedModel.textureId = preloadRenderModelTexture(loadedModel.deviceName, loadedModel.vrModel->diffuseTextureId);
      }

      if (loadedModel.textureId == -1)
      {
         failed = true;
         return true;
      }

      if (!getRenderModelTexture(loadedModel.textureId, failed))
      {
         return false;
      }

      if (failed)
      {
         return true;
      }

      loadedModel.loadedTexture = true;

      // Now we can load the model. Note we first need to get a Material for the mapped texture
      String materialName = MATMGR->getMapEntry(mLoadedTextures[loadedModel.textureId].textureName);
      if (materialName.isEmpty())
      {
         char buffer[256];
         materialName = mLoadedTextures[loadedModel.textureId].textureName;

         //Con::printf("RenderModel[%i] materialName == %s", idx, buffer);

         Material* mat = new Material();
         mat->mMapTo = mLoadedTextures[loadedModel.textureId].textureName;
         mat->mDiffuseMapFilename[0] = mLoadedTextures[loadedModel.textureId].texturePath;
         mat->mEmissive[0] = true;
         mat->mCastShadows = true;

         dSprintf(buffer, sizeof(buffer), "%s_Mat", mLoadedTextures[loadedModel.textureId].textureName);
         if (!mat->registerObject(buffer))
         {
            Con::errorf("Couldn't create placeholder openvr material %s!", buffer);
            failed = true;
            return true;
         }

         materialName = buffer;
      }

      loadedModel.model->init(*loadedModel.vrModel, materialName);
   }

   if ((loadedModel.modelError > vr::VRRenderModelError_Loading) ||
       (loadedModel.textureId >= 0 && mLoadedTextures[loadedModel.textureId].textureError > vr::VRRenderModelError_Loading))
   {
      failed = true;
   }

   if (!failed && ret)
   {
      *ret = loadedModel.model;
   }
   return true;
}

bool OpenVRProvider::getRenderModelTexture(S32 idx, bool &failed)
{
   if (idx < 0 || idx > mLoadedModels.size())
   {
      failed = true;
      return true;
   }

   failed = false;

   LoadedRenderTexture &loadedTexture = mLoadedTextures[idx];

   if (loadedTexture.textureError > vr::VRRenderModelError_Loading)
   {
      failed = true;
      return true;
   }

   if (!loadedTexture.textureCached)
      {
         loadedTexture.textureError = vr::VRRenderModels()->LoadTexture_Async(loadedTexture.vrTextureId, &loadedTexture.vrTexture);
         if (loadedTexture.textureError == vr::VRRenderModelError_None)
         {
            const U32 sz = loadedTexture.vrTexture->unWidth * loadedTexture.vrTexture->unHeight * 4;
            GBitmap *bmp = new GBitmap(loadedTexture.vrTexture->unWidth, loadedTexture.vrTexture->unHeight, false, GFXFormatR8G8B8A8);

            Swizzles::bgra.ToBuffer(bmp->getAddress(0,0,0), loadedTexture.vrTexture->rubTextureMapData, sz);

         // Now that we've cached the texture, we can release the original
         mRenderModels->FreeTexture(loadedTexture.vrTexture);

            FileStream fs;
            fs.open(loadedTexture.texturePath, Torque::FS::File::Write);
            bmp->writeBitmap("PNG", fs);
            fs.close();
            loadedTexture.textureCached = true;
         }
         else if (loadedTexture.textureError == vr::VRRenderModelError_Loading)
         {
            return false;
         }
      }

   if (loadedTexture.textureError > vr::VRRenderModelError_Loading)
   {
      failed = true;
   }

   return true;
}

bool OpenVRProvider::getRenderModelTextureName(S32 idx, String &outName)
{
   if (idx < 0 || idx >= mLoadedTextures.size())
      return false;

   outName = mLoadedTextures[idx].textureName;
      return true;
   }

void OpenVRProvider::resetRenderModels()
{
   if (!mRenderModels)
      return;

   for (U32 i = 0, sz = mLoadedModels.size(); i < sz; i++)
   {
      SAFE_DELETE(mLoadedModels[i].model);
      if (mLoadedModels[i].vrModel) mRenderModels->FreeRenderModel(mLoadedModels[i].vrModel);
   }
   mLoadedModels.clear();
   mLoadedTextures.clear();
   mLoadedModelLookup.clear();
   mLoadedTextureLookup.clear();
}

OpenVROverlay *OpenVRProvider::getGamepadFocusOverlay()
{
   return NULL;
}

bool OpenVRProvider::isDashboardVisible()
{
   return false;
}

void OpenVRProvider::showDashboard(const char *overlayToShow)
{

}

vr::TrackedDeviceIndex_t OpenVRProvider::getPrimaryDashboardDevice()
{
   return -1;
}

void OpenVRProvider::setKeyboardTransformAbsolute(const MatrixF &xfm)
{
   // mTrackingSpace
}

void OpenVRProvider::setKeyboardPositionForOverlay(OpenVROverlay *overlay, const RectI &rect)
{

}

StringTableEntry OpenVRProvider::getControllerModel(U32 idx)
{
   if (idx >= vr::k_unMaxTrackedDeviceCount || !mRenderModels)
      return NULL;

   String str = GetTrackedDeviceString(mHMD, idx, vr::Prop_RenderModelName_String, NULL);
   return StringTable->insert(str, true);
}

String OpenVRProvider::getDeviceClass(U32 deviceIdx)
{
   if (deviceIdx >= vr::k_unMaxTrackedDeviceCount)
      return String::EmptyString;

   OpenVRTrackedDeviceClass klass = mHMD->GetTrackedDeviceClass(deviceIdx);
   return castConsoleTypeToString(klass);
}

String OpenVRProvider::getControllerAxisType(U32 deviceIdx, U32 axisID)
{
   if (deviceIdx >= vr::k_unMaxTrackedDeviceCount || !mHMD)
      return String::EmptyString;

   OpenVRControllerAxisType axisType = (OpenVRControllerAxisType) mHMD->GetInt32TrackedDeviceProperty(deviceIdx,
         (vr::TrackedDeviceProperty) (vr::Prop_Axis0Type_Int32 + axisID), NULL);

   return castConsoleTypeToString(axisType);
}

String OpenVRProvider::getTrackedDeviceIndices(OpenVRTrackedDeviceClass deviceClass)
{
   if (!mHMD)
      return String::EmptyString;

   vr::TrackedDeviceIndex_t indexArray[vr::k_unMaxTrackedDeviceCount];
   String results;
   U32 numDevices = mHMD->GetSortedTrackedDeviceIndicesOfClass(deviceClass, indexArray, vr::k_unMaxTrackedDeviceCount, vr::k_unTrackedDeviceIndexInvalid);
   if (numDevices < vr::k_unMaxTrackedDeviceCount)
   {
      for (U32 i = 0; i < numDevices; ++i)
      {
         if (i > 0)
            results += " ";
         results += String::ToString(indexArray[i]);
      }
   }
   return results;
}

String OpenVRProvider::getDevicePropertyString(U32 deviceIdx, U32 propID)
{
   if (deviceIdx >= vr::k_unMaxTrackedDeviceCount || !mHMD)
      return String::EmptyString;

   return GetTrackedDeviceString(mHMD, deviceIdx, (vr::TrackedDeviceProperty) propID, NULL);
}

bool OpenVRProvider::getDevicePropertyBool(U32 deviceIdx, U32 propID)
{
   if (deviceIdx >= vr::k_unMaxTrackedDeviceCount || !mHMD)
      return false;

   return mHMD->GetBoolTrackedDeviceProperty(deviceIdx, (vr::TrackedDeviceProperty) propID, NULL);
}

S32 OpenVRProvider::getDevicePropertyInt(U32 deviceIdx, U32 propID)
{
   if (deviceIdx >= vr::k_unMaxTrackedDeviceCount || !mHMD)
      return 0;

   return mHMD->GetInt32TrackedDeviceProperty(deviceIdx, (vr::TrackedDeviceProperty) propID, NULL);
}

F32 OpenVRProvider::getDevicePropertyFloat(U32 deviceIdx, U32 propID)
{
   if (deviceIdx >= vr::k_unMaxTrackedDeviceCount || !mHMD)
      return 0.0f;

   return mHMD->GetFloatTrackedDeviceProperty(deviceIdx, (vr::TrackedDeviceProperty) propID, NULL);
}

String OpenVRProvider::getDevicePropertyUInt(U32 deviceIdx, U32 propID)
{
   if (deviceIdx >= vr::k_unMaxTrackedDeviceCount || !mHMD)
      return String::EmptyString;

   uint64_t ret = mHMD->GetUint64TrackedDeviceProperty(deviceIdx, (vr::TrackedDeviceProperty) propID, NULL);

   char buffer[64];
   dSprintf(buffer, sizeof(buffer), "%x", ret);
   return buffer;
}

void OpenVRProvider::orientUniverse(const MatrixF &mat)
{
   Point3F vecForward = mat.getForwardVector() * 10.0f;
   vecForward.z = 0; // flatten
   vecForward.normalizeSafe();

   F32 yawAng;
   F32 pitchAng;
   MathUtils::getAnglesFromVector(vecForward, yawAng, pitchAng);
   if (yawAng > M_PI_F)
      yawAng -= M_2PI_F;
   if (yawAng < -M_PI_F)
      yawAng += M_2PI_F;
   smUniverseYawOffset = yawAng;
   smUniverseRotMat.set(EulerF(0.0f, smUniverseYawOffset, 0.0f));
}

void OpenVRProvider::rotateUniverse(const F32 yaw)
{
   smUniverseYawOffset = yaw;
   smUniverseRotMat.set(EulerF(0.0f, smUniverseYawOffset, 0.0f));
}
