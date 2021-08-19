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
#include "platform/input/openVR/openVROverlay.h"

#ifndef LINUX
#include "gfx/D3D11/gfxD3D11Device.h"
#include "gfx/D3D11/gfxD3D11TextureObject.h"
#include "gfx/D3D11/gfxD3D11EnumTranslate.h"
#endif

#ifdef TORQUE_OPENGL
#include "gfx/gl/gfxGLDevice.h"
#include "gfx/gl/gfxGLTextureObject.h"
#include "gfx/gl/gfxGLEnumTranslate.h"
#endif

#include "math/mTransform.h"
#include "gui/core/guiOffscreenCanvas.h"

ImplementEnumType(OpenVROverlayType,
   "Desired overlay type for OpenVROverlay.\n\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{ OpenVROverlay::OVERLAYTYPE_OVERLAY, "Overlay", "Overlay is rendered in the 3D world." },
{ OpenVROverlay::OVERLAYTYPE_DASHBOARD, "Dashboard", "Overlay is added as a tab on the VR Dashboard. Automatically shown and hidden with the dashboard." },
EndImplementEnumType;

ImplementEnumType(OpenVROverlayTransformType,
   "Transform options for an overlay.\n\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{ vr::VROverlayTransform_Absolute, "Absolute", "Sets the transform relative to the tracking origin." },
{ vr::VROverlayTransform_TrackedDeviceRelative, "TrackedDeviceRelative", "Sets the transform to relative to the transform of the specified tracked device." },
{ vr::VROverlayTransform_SystemOverlay, "SystemOverlay", "" },
{ vr::VROverlayTransform_TrackedComponent, "TrackedComponent", "Sets the transform to draw the overlay on a rendermodel component mesh instead of a quad. "
   "This will only draw when the system is drawing the device. Overlays with this transform type cannot receive mouse events." },
{ vr::VROverlayTransform_Cursor, "Cursor", "Overlay is used as the cursor on another overlay." },
{ vr::VROverlayTransform_DashboardTab, "DashboardTab", "" },
{ vr::VROverlayTransform_DashboardThumb, "DashboardThumb", "" },
{ vr::VROverlayTransform_Mountable, "Mountable", "This overlay is mounted as the child of another overlay. This overlays visibility will also track the parents visibility" },
{ vr::VROverlayTransform_Projection, "Projection", "Sets the overlay as a projection overlay?" },
EndImplementEnumType;

ImplementEnumType(OpenVROverlayInputMethod,
   "Types of input supported by VR Overlays.\n\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR")
{ vr::VROverlayInputMethod_None, "None", "No input events will be generated automatically for this overlay" },
{ vr::VROverlayInputMethod_Mouse, "Mouse", "Tracked controllers will get mouse events automatically" },
//{ vr::VROverlayInputMethod_DualAnalog, "DualAnalog" }, // No longer supported
EndImplementEnumType;

ImplementEnumType(OpenVRGamepadTextInputMode,
   "Input modes for the Big Picture gamepad text entry.\n\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR")
{ vr::k_EGamepadTextInputModeNormal, "Normal", },
{ vr::k_EGamepadTextInputModePassword, "Password", },
{ vr::k_EGamepadTextInputModeSubmit, "Submit" },
EndImplementEnumType;

ImplementEnumType(OpenVRGamepadTextInputLineMode,
   "Controls number of allowed lines for the Big Picture gamepad text entry.\n\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR")
{ vr::k_EGamepadTextInputLineModeSingleLine, "SingleLine" },
{ vr::k_EGamepadTextInputLineModeMultipleLines, "MultipleLines" },
EndImplementEnumType;

ImplementEnumType(OpenVRKeyboardFlags,
   "Controls number of allowed lines for the Big Picture gamepad text entry.\n\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR")
{ vr::KeyboardFlag_Minimal, "Minimal", "Makes the keyboard send key events immediately instead of accumulating a buffer." },
{ vr::KeyboardFlag_Modal, "Modal", "Makes the keyboard take all focus and dismiss when clicking off the panel." },
EndImplementEnumType;

ImplementEnumType(OpenVRMessageResponse,
   "MessageBox response codes.\n\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR")
{ vr::VRMessageOverlayResponse_ButtonPress_0, "Button0", "Button 0 was pressed." },
{ vr::VRMessageOverlayResponse_ButtonPress_1, "Button1", "Button 1 was pressed." },
{ vr::VRMessageOverlayResponse_ButtonPress_2, "Button2", "Button 2 was pressed." },
{ vr::VRMessageOverlayResponse_ButtonPress_3, "Button3", "Button 3 was pressed." },
{ vr::VRMessageOverlayResponse_CouldntFindSystemOverlay, "CouldntFindSystemOverlay", "Message overlay could not be found." },
{ vr::VRMessageOverlayResponse_CouldntFindOrCreateClientOverlay, "CouldntFindClientOverlay", "Message overlay could not be found or created." },
{ vr::VRMessageOverlayResponse_ApplicationQuit, "ApplicationQuit", "The application quit before the system message overlay was closed." },
EndImplementEnumType;

IMPLEMENT_CALLBACK(OpenVROverlay, onKeyboardClosed, void, (U32 userValue), (userValue),
   "@brief Called when the virtual keyboard is closed without entering text or clicking done.\n\n"
   "@param userValue The userValue that was passed to showKeyboard()\n"
   "@see OpenVROverlay::showKeyboard()\n"
   "@see OpenVROverlay::showKeyboardForOverlay()\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR");

IMPLEMENT_CALLBACK(OpenVROverlay, onKeyboardInput, void, (const char* inputText, U32 userValue), (inputText, userValue),
   "@brief Called for each character entered in a \"Minimal\" virtual keyboard.\n\n"
   "@param inputText Up to 8 bytes of new text that was entered.\n"
   "@param userValue The userValue that was passed to showKeyboard()\n"
   "@see OpenVROverlay::showKeyboard()\n"
   "@see OpenVROverlay::showKeyboardForOverlay()\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR");

IMPLEMENT_CALLBACK(OpenVROverlay, onKeyboardDone, void, (U32 userValue), (userValue),
   "@brief Called when done is clicked on the virtual keyboard.\n\n"
   "@param userValue The userValue that was passed to showKeyboard()\n"
   "@see OpenVROverlay::showKeyboard()\n"
   "@see OpenVROverlay::showKeyboardForOverlay()\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR");


IMPLEMENT_CONOBJECT(OpenVROverlay);

U32 OpenVROverlay::smSupportedFlags = vr::VROverlayFlags_NoDashboardTab | vr::VROverlayFlags_SendVRDiscreteScrollEvents |
   vr::VROverlayFlags_SendVRTouchpadEvents | vr::VROverlayFlags_ShowTouchPadScrollWheel | vr::VROverlayFlags_SideBySide_Parallel |
   vr::VROverlayFlags_SideBySide_Crossed | vr::VROverlayFlags_Panorama | vr::VROverlayFlags_StereoPanorama |
   vr::VROverlayFlags_SortWithNonSceneOverlays | vr::VROverlayFlags_VisibleInDashboard | vr::VROverlayFlags_MakeOverlaysInteractiveIfVisible |
   vr::VROverlayFlags_SendVRSmoothScrollEvents | vr::VROverlayFlags_ProtectedContent | vr::VROverlayFlags_HideLaserIntersection |
   vr::VROverlayFlags_WantsModalBehavior | vr::VROverlayFlags_IsPremultiplied;

OpenVROverlay::OpenVROverlay()
:  mOverlayHandle(0),
   mThumbOverlayHandle(0),
   mCursorOverlay(NULL),
   mMountToOverlay(NULL),
   mGuiCanvas(NULL),
   mTextureLoaded(false),
   mThumbnailLoaded(false),
   mBoundsUVMin(Point2F::Zero),
   mBoundsUVMax(Point2F::One),
   mOverlayFlags(0),
   mOverlayWidth(1.5f),
   mOverlayTransformType(vr::VROverlayTransform_Absolute),
   mTransform(MatrixF(true)),
   mTransformDeviceIndex(vr::k_unTrackedDeviceIndex_Hmd),
   mInputMethod(vr::VROverlayInputMethod_None),
   mMouseScale(Point2F::One),
   mOverlayColor(LinearColorF(1, 1, 1, 1)),
   mTexelAspect(1),
   mSortOrder(0),
   mCurvature(0),
   mOverlayTypeDirty(false),
   mOverlayDirty(false),
   mOverlayType(OVERLAYTYPE_OVERLAY)
{
}

OpenVROverlay::~OpenVROverlay()
{

}

bool OpenVROverlay::setProtectedOverlayTypeDirty(void *obj, const char *array, const char *data)
{
   OpenVROverlay *object = static_cast<OpenVROverlay*>(obj);
   object->mOverlayTypeDirty = true;
   return true;
}

bool OpenVROverlay::setProtectedOverlayDirty(void *obj, const char *array, const char *data)
{
   OpenVROverlay *object = static_cast<OpenVROverlay*>(obj);
   object->mOverlayDirty = true;
   return true;
}

bool OpenVROverlay::setProtectedOverlayName(void* obj, const char* index, const char* data)
{
   OpenVROverlay* object = static_cast<OpenVROverlay*>(obj);
   object->setOverlayName(data);
   return false;
}

bool OpenVROverlay::setProtectedOverlayCanvas(void* obj, const char* index, const char* data)
{
   OpenVROverlay* object = static_cast<OpenVROverlay*>(obj);
   GuiOffscreenCanvas* canvas = dynamic_cast<GuiOffscreenCanvas*>(Sim::findObject(data));
   object->setOverlayCanvas(canvas);
   return false;
}

bool OpenVROverlay::setProtectedTextureFile(void* obj, const char* index, const char* data)
{
   OpenVROverlay* object = static_cast<OpenVROverlay*>(obj);
   object->setOverlayTextureFile(data);
   return false;
}

bool OpenVROverlay::setProtectedThumbnailFile(void* obj, const char* index, const char* data)
{
   OpenVROverlay* object = static_cast<OpenVROverlay*>(obj);
   object->setThumbnailTextureFile(data);
   return false;
}

bool OpenVROverlay::setProtectedCursorOverlay(void* obj, const char* index, const char* data)
{
   OpenVROverlay* object = static_cast<OpenVROverlay*>(obj);
   OpenVROverlay* cursor = dynamic_cast<OpenVROverlay*>(Sim::findObject(data));
   object->setCursorOverlay(cursor);
   return false;
}

bool OpenVROverlay::setProtectedMountToOverlay(void* obj, const char* index, const char* data)
{
   OpenVROverlay* object = static_cast<OpenVROverlay*>(obj);
   OpenVROverlay* mountTo = dynamic_cast<OpenVROverlay*>(Sim::findObject(data));
   object->setMountToOverlay(mountTo);
   return false;
}

void OpenVROverlay::initPersistFields()
{
   addProtectedField("overlayName", TypeString, Offset(mOverlayName, OpenVROverlay), &setProtectedOverlayName, &defaultProtectedGetFn,
      "Name of this overlay.");
   addProtectedField("overlayType", TypeOpenVROverlayType, Offset(mOverlayType, OpenVROverlay), &setProtectedOverlayTypeDirty, &defaultProtectedGetFn,
      "Type of overlay (\"Overlay\" or \"Dashboard\".");
   addProtectedField("overlayFlags", TypeS32, Offset(mOverlayFlags, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Flags for overlay.");

   addGroup("Texture");
   addProtectedField("offscreenCanvas", TYPEID< GuiOffscreenCanvas >(), Offset(mGuiCanvas, OpenVROverlay), &setProtectedOverlayCanvas, &defaultProtectedGetFn,
      "Overlays that accept input or have dynamic textures need an offscreen canvas to render and process input. Set the Id of the canvas here or 0 to load texture from file.");
   addProtectedField("textureFile", TypeString, Offset(mTextureFile, OpenVROverlay), &setProtectedTextureFile, &defaultProtectedGetFn,
      "The texture to display on static overlays.");
   addProtectedField("thumbnailFile", TypeString, Offset(mThumbnailFile, OpenVROverlay), &setProtectedThumbnailFile, &defaultProtectedGetFn,
      "The texture to display on the thumbnail for dashboard overlays.");
   addProtectedField("textureUVMin", TypePoint2F, Offset(mBoundsUVMin, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Gets the part of the texture to use for the overlay. UV Min is the upper left corner and UV Max is the lower right corner (V components flipped on OpenGL for consistent appearance).");
   addProtectedField("textureUVMax", TypePoint2F, Offset(mBoundsUVMax, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Gets the part of the texture to use for the overlay. UV Min is the upper left corner and UV Max is the lower right corner (V components flipped on OpenGL for consistent appearance).");
   endGroup("Texture");

   addProtectedField("overlayWidth", TypeF32, Offset(mOverlayWidth, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Width of overlay in meters.");
   addProtectedField("overlayColor", TypeColorF, Offset(mOverlayColor, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Backing color of overlay.");
   addProtectedField("texelAspect", TypeF32, Offset(mTexelAspect, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Sets the aspect ratio of the texels in the overlay. 1.0 means the texels are square. 2.0 means the texels "
      "are twice as wide as they are tall. Defaults to 1.0.");
   addProtectedField("sortOrder", TypeS32, Offset(mSortOrder, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Overlays are rendered lowest sort order to highest sort order. Overlays with the same sort order are "
      "rendered back to front based on distance from the HMD. Sort order defaults to 0.");
   addProtectedField("curvature", TypeF32, Offset(mCurvature, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Use to draw overlay as a curved surface. Curvature is a percentage from (0..1] where 1 is a fully "
      "closed cylinder. For a specific radius, curvature can be computed as : overlay.width / (2 PI r).");

   addProtectedField("transformType", TypeOpenVROverlayTransformType, Offset(mOverlayTransformType, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Transform type for overlay. One of: Absolute, TrackedDeviceRelative, SystemOverlay, TrackedComponent, Cursor, DashboardTab, DashboardThumb, Mountable or Projection.");
   addProtectedField("transformPosition", TypeMatrixPosition, Offset(mTransform, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Position of overlay. The Hot-Spot on a cursor overlay is set using the x and y components of position using UV scale (0,0 is upper left. 1,1 is lower right).");
   addProtectedField("transformRotation", TypeMatrixRotation, Offset(mTransform, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Rotation of overlay.");
   addProtectedField("transformDeviceIndex", TypeS32, Offset(mTransformDeviceIndex, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "The device to attach the overlay to when transformType is TrackedDeviceRelative. The HMD is always index 0. The overlay transform will be relative to this device.");
   addProtectedField("transformDeviceComponent", TypeString, Offset(mTransformDeviceComponent, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Sets a rendermodel component name. Only used when the transformType is TrackedComponent. The overlay "
      "will be drawn on this rendermodel component mesh instead of a quad. The overlay transform fields will "
      "be ignored when rendered on a mesh.");

   addProtectedField("inputMethod", TypeOpenVROverlayInputMethod, Offset(mInputMethod, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Type of input method.");
   addProtectedField("mouseScale", TypePoint2F, Offset(mMouseScale, OpenVROverlay), &setProtectedOverlayDirty, &defaultProtectedGetFn,
      "Scale of mouse input.");

   addProtectedField("cursorOverlay", TYPEID< OpenVROverlay >(), Offset(mCursorOverlay, OpenVROverlay), &setProtectedCursorOverlay, &defaultProtectedGetFn,
      "The OpenVROverlay object to use as a cursor on this overlay.");
   addProtectedField("mountToOverlay", TYPEID< OpenVROverlay >(), Offset(mMountToOverlay, OpenVROverlay), &setProtectedMountToOverlay, &defaultProtectedGetFn,
      "The OpenVROverlay object to mount this overlay to. When mounted, the set transform will be relative to the parent overlay transform.");

   Parent::initPersistFields();
}

bool OpenVROverlay::onAdd()
{
   if (Parent::onAdd())
   {
      mOverlayTypeDirty = true;
      mOverlayDirty = true;

      if (ManagedSingleton<OpenVRProvider>::instanceOrNull())
      {
         OPENVR->registerOverlay(this);
      }

      return true;
   }

   return false;
}

void OpenVROverlay::onRemove()
{
   if (mGuiCanvas)
   {
      clearNotify(mGuiCanvas);
      mGuiCanvas->getRenderSignal().remove(this, &OpenVROverlay::onCanvasFrame);
      mGuiCanvas = NULL;
   }

   if (vr::VROverlay())
   {
      if (mOverlayHandle)
      {
         vr::VROverlay()->DestroyOverlay(mOverlayHandle);
         mOverlayHandle = NULL;
      }

      if (mThumbOverlayHandle)
      {
         vr::VROverlay()->DestroyOverlay(mThumbOverlayHandle);
         mThumbOverlayHandle = NULL;
      }
   }

   if (ManagedSingleton<OpenVRProvider>::instanceOrNull())
   {
      OPENVR->unregisterOverlay(this);
   }
   Parent::onRemove();
}

void OpenVROverlay::onDeleteNotify(SimObject* object)
{  // The canvas we're connected to has been deleted
   if (object == mGuiCanvas)
      mGuiCanvas = NULL;
}

void OpenVROverlay::resetOverlay()
{
   vr::IVROverlay *overlay = vr::VROverlay();
   if (!overlay)
      return;

   if (mOverlayHandle)
   {
      overlay->DestroyOverlay(mOverlayHandle);
      mOverlayHandle = NULL;
   }

   if (mThumbOverlayHandle)
   {
      overlay->DestroyOverlay(mThumbOverlayHandle);
      mThumbOverlayHandle = NULL;
   }

   if (mOverlayType == OpenVROverlay::OVERLAYTYPE_DASHBOARD)
   {
      overlay->CreateDashboardOverlay(mInternalName, mOverlayName, &mOverlayHandle, &mThumbOverlayHandle);
   }
   else
   {
      overlay->CreateOverlay(mInternalName, mOverlayName, &mOverlayHandle);
   }

   mOverlayDirty = true;
   mOverlayTypeDirty = false;
   mTextureLoaded = false;
   mThumbnailLoaded = false;

   // Pre-render start frame so we have a texture available
   //if (!mTarget)
   //{
   //   renderFrame(false, false);
   //}
}

void OpenVROverlay::updateOverlay()
{
   if (mOverlayTypeDirty)
      resetOverlay();

   if (!mOverlayDirty)
      return;

   // Update params
   vr::IVROverlay *overlay = vr::VROverlay();
   if (!overlay || !mOverlayHandle)
      return;

   vr::HmdVector2_t ovrMouseScale;
   ovrMouseScale.v[0] = mMouseScale.x;
   ovrMouseScale.v[1] = mMouseScale.y;
   overlay->SetOverlayMouseScale(mOverlayHandle, &ovrMouseScale);

   overlay->SetOverlayColor(mOverlayHandle, mOverlayColor.red, mOverlayColor.green, mOverlayColor.blue);
   overlay->SetOverlayAlpha(mOverlayHandle, mOverlayColor.alpha);
   overlay->SetOverlayInputMethod(mOverlayHandle, mInputMethod);
   overlay->SetOverlayWidthInMeters(mOverlayHandle, mOverlayWidth);
   overlay->SetOverlayTexelAspect(mOverlayHandle, mTexelAspect);
   overlay->SetOverlaySortOrder(mOverlayHandle, mSortOrder);
   overlay->SetOverlayCurvature(mOverlayHandle, mCurvature);

   if (!mGuiCanvas && !mTextureLoaded && mTextureFile.isNotEmpty())
      mTextureLoaded = setOverlayTexture(mOverlayHandle, mTextureFile);

   if (mThumbOverlayHandle && !mThumbnailLoaded && mThumbnailFile.isNotEmpty())
      mThumbnailLoaded = setOverlayTexture(mThumbOverlayHandle, mThumbnailFile);

   vr::VRTextureBounds_t bounds;
   bounds.uMin = bounds.vMax = 0.0f; bounds.uMax = bounds.vMin = 1.0f;
   bounds.uMin = mBoundsUVMin.x;
   bounds.uMax = mBoundsUVMax.x;
   bounds.vMin = mBoundsUVMin.y;
   bounds.vMax = mBoundsUVMax.y;

#ifdef TORQUE_OPENGL
   if (GFX->getAdapterType() == OpenGL)
   {  // Vertical flip on OpenGL
      bounds.vMax = mBoundsUVMin.y;
      bounds.vMin = mBoundsUVMax.y;
   }
#endif

   overlay->SetOverlayTextureBounds(mOverlayHandle, &bounds);

   setOverlayFlags();
   setOverlayTransform();

   mOverlayDirty = false;
}

void OpenVROverlay::setOverlayName(const char* overlayName)
{
   mOverlayName = overlayName;
   if (mOverlayHandle && vr::VROverlay())
   {
      vr::EVROverlayError err = vr::VROverlay()->SetOverlayName(mOverlayHandle, overlayName);
      if (err != vr::VROverlayError_None)
      {
         Con::errorf("VR Overlay error (%s) in OpenVROverlay::setOverlayName!", vr::VROverlay()->GetOverlayErrorNameFromEnum(err));
      }
   }
}

void OpenVROverlay::setOverlayCanvas(GuiOffscreenCanvas* canvas)
{
   if (canvas == mGuiCanvas)
      return;

   // Clear the delete notification for the existing canvas
   if (mGuiCanvas)
   {
      clearNotify(mGuiCanvas);
      mGuiCanvas->getRenderSignal().remove(this, &OpenVROverlay::onCanvasFrame);
   }

   mGuiCanvas = canvas;

   if (mGuiCanvas)
   {
      // Set delete and render notifications on new canvas
      deleteNotify(mGuiCanvas);
      mGuiCanvas->getRenderSignal().notify(this, &OpenVROverlay::onCanvasFrame);

      Point2I extent = mGuiCanvas->getExtent();

      // Copy texture to overlay if it's available.
      if (mOverlayHandle && vr::VROverlay() && (mGuiCanvas->getRenderCount() > 0))
      {
         onCanvasFrame();
      }
   }
}

void OpenVROverlay::setOverlayTextureFile(const char* fileName)
{
   if (mTextureFile.equal(fileName, String::Mode::Case))
      return;

   mTextureFile = fileName;
   if (mOverlayHandle && vr::VROverlay() && mTextureFile.isNotEmpty())
   {
      mTextureLoaded = setOverlayTexture(mOverlayHandle, mTextureFile);
   }
}

void OpenVROverlay::setThumbnailTextureFile(const char* fileName)
{
   if (mThumbnailFile.equal(fileName, String::Mode::Case))
      return;

   mThumbnailFile = fileName;
   if (mThumbOverlayHandle && vr::VROverlay() && mThumbnailFile.isNotEmpty())
   {
      mThumbnailLoaded = setOverlayTexture(mThumbOverlayHandle, mThumbnailFile);
   }
}

void OpenVROverlay::setCursorOverlay(OpenVROverlay* cursor)
{
   if (mCursorOverlay == cursor)
      return;

   mCursorOverlay = cursor;
   if (mOverlayHandle && vr::VROverlay())
   {
      if (mCursorOverlay)
         vr::VROverlay()->SetOverlayCursor(mOverlayHandle, mCursorOverlay->getOverlayHandle());
      else
         vr::VROverlay()->SetOverlayCursor(mOverlayHandle, vr::k_ulOverlayHandleInvalid);
   }
}

void OpenVROverlay::setMountToOverlay(OpenVROverlay* mountObj)
{
   if (mMountToOverlay == mountObj)
      return;

   mMountToOverlay = mountObj;
   if (mOverlayHandle && vr::VROverlay())
      setOverlayTransform();
}

bool OpenVROverlay::setOverlayTexture(vr::VROverlayHandle_t overlayHandle, String& fileName)
{
   Torque::Path filePath(fileName);

   if (!Torque::FS::IsFile(filePath))
      return false;

   GFXTexHandle texHandle = GFXTexHandle(fileName, &GFXTexturePersistentSRGBProfile, avar("%s() - texHandle (line %d)", __FUNCTION__, __LINE__));

   vr::Texture_t tex;
   tex = { NULL, vr::TextureType_Invalid, vr::ColorSpace_Auto };
#if defined(TORQUE_OS_WIN64) || defined(TORQUE_D3D11)
   if (GFX->getAdapterType() == Direct3D11)
   {
      tex = { (void*)static_cast<GFXD3D11TextureObject*>((GFXTextureObject*)texHandle)->getResource(), vr::TextureType_DirectX, vr::ColorSpace_Auto };
   }
#endif
#ifdef TORQUE_OPENGL
   if (GFX->getAdapterType() == OpenGL)
   {
      tex = { (void*)(uintptr_t)static_cast<GFXGLTextureObject*>((GFXTextureObject*)texHandle)->getHandle(), vr::TextureType_OpenGL, vr::ColorSpace_Auto };
   }
#endif
   if (tex.eType == vr::TextureType_Invalid)
   {
      return false;
   }

   vr::EVROverlayError err = vr::VROverlay()->SetOverlayTexture(overlayHandle, &tex);
   if (err != vr::VROverlayError_None)
   {
      Con::errorf("VR: Error (%s) setting overlay texture from %s.", vr::VROverlay()->GetOverlayErrorNameFromEnum(err), fileName.c_str());
      return false;
   }

   return true;
}

void OpenVROverlay::setOverlayFlags()
{
   for (U32 i = vr::VROverlayFlags_NoDashboardTab; i <= vr::VROverlayFlags_IsPremultiplied; i = (i << 1))
   {
      if (i & smSupportedFlags)
      {
         //vr::EVROverlayError err = vr::VROverlay()->SetOverlayFlag(mOverlayHandle, (vr::VROverlayFlags)i, mOverlayFlags & i); <- Changed for easier debugging
         vr::EVROverlayError err;
         if (mOverlayFlags & i)
            err = vr::VROverlay()->SetOverlayFlag(mOverlayHandle, (vr::VROverlayFlags)i, true);
         else
            err = vr::VROverlay()->SetOverlayFlag(mOverlayHandle, (vr::VROverlayFlags)i, false);
         if (err != vr::VROverlayError_None)
            Con::errorf("VR: Error (%s) setting flag %u.", vr::VROverlay()->GetOverlayErrorNameFromEnum(err), i);
      }
   }
}

void OpenVROverlay::setOverlayTransform()
{
   MatrixF vrMat(1);
   vr::HmdMatrix34_t ovrMat;
   vr::HmdVector2_t cursorHotSpot;
   OpenVRUtil::convertTransformToOVR(mTransform, vrMat);
   OpenVRUtil::convertMatrixFPlainToSteamVRAffineMatrix(vrMat, ovrMat);

   switch (mOverlayTransformType)
   {
   case vr::VROverlayTransform_Absolute:
      // Sets the transform relative to the absolute tracking origin.
      vr::VROverlay()->SetOverlayTransformAbsolute(mOverlayHandle, OPENVR->mTrackingSpace, &ovrMat);
      break;
   case vr::VROverlayTransform_TrackedDeviceRelative:
      // Sets the transform to relative to the transform of the specified tracked device.
      vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(mOverlayHandle, mTransformDeviceIndex, &ovrMat);
      break;
   case vr::VROverlayTransform_TrackedComponent:
      // Sets the transform to draw the overlay on a rendermodel component mesh instead of a quad.
      vr::VROverlay()->SetOverlayTransformTrackedDeviceComponent(mOverlayHandle, mTransformDeviceIndex, mTransformDeviceComponent.c_str());
      break;
   case vr::VROverlayTransform_Cursor:
      // Sets the hotspot for the specified overlay when that overlay is used as a cursor.
      // These are in texture space with 0,0 in the upper left corner of the texture
      // and 1, 1 in the lower right corner of the texture.
      // Uses x and y from transform position.
      cursorHotSpot.v[0] = mTransform[3];
      cursorHotSpot.v[1] = mTransform[7];
      vr::VROverlay()->SetOverlayTransformCursor(mOverlayHandle, &cursorHotSpot);
      break;
   case vr::VROverlayTransform_Mountable:
      // Overlay is mounted to another overlay and this is the relative transform.
      if (mMountToOverlay)
         vr::VROverlay()->SetOverlayTransformOverlayRelative(mOverlayHandle, mMountToOverlay->getOverlayHandle(), &ovrMat);
      break;
   //case vr::VROverlayTransform_Projection:
   //   break;

   // NOTE: VROverlayTransform_SystemOverlay not handled here - doesn't seem possible to create these
   // VROverlayTransform_DashboardTab and VROverlayTransform_DashboardThumb get no transfom applied.
   // VROverlayTransform_Projection is not implemented yet.
   default:
      break;
   }
}

void OpenVROverlay::showOverlay()
{
   updateOverlay();
   if (mOverlayHandle == NULL)
      return;

   if (mOverlayType == OVERLAYTYPE_DASHBOARD)
   {
      vr::VROverlay()->ShowDashboard(mInternalName);
   }
   else
   {
      vr::EVROverlayError err = vr::VROverlay()->ShowOverlay(mOverlayHandle);
      if (err != vr::VROverlayError_None)
      {
         Con::errorf("VR Overlay error (%s) in OpenVROverlay::showOverlay!", vr::VROverlay()->GetOverlayErrorNameFromEnum(err));
      }
   }
}

void OpenVROverlay::hideOverlay()
{
   if (mOverlayHandle == NULL)
      return;

   if (mOverlayType != OVERLAYTYPE_DASHBOARD)
   {
      if (vr::VROverlay())
         vr::VROverlay()->HideOverlay(mOverlayHandle);
   }
}


bool OpenVROverlay::isOverlayVisible()
{
   if (mOverlayHandle == NULL)
      return false;

   return vr::VROverlay()->IsOverlayVisible(mOverlayHandle);
}

bool OpenVROverlay::isOverlayHoverTarget()
{
   if (mOverlayHandle == NULL)
      return false;

   return vr::VROverlay()->IsHoverTargetOverlay(mOverlayHandle);
}

bool OpenVROverlay::triggerHapticVibration(F32 durationSeconds, F32 frequency, F32 amplitude)
{
   if (mOverlayHandle == NULL)
      return false;

   vr::EVROverlayError err = vr::VROverlay()->TriggerLaserMouseHapticVibration(mOverlayHandle, durationSeconds, frequency, amplitude);
   if (err != vr::VROverlayError_None)
   {
      Con::errorf("VR Overlay error (%s) in OpenVROverlay::triggerHapticVibration!", vr::VROverlay()->GetOverlayErrorNameFromEnum(err));
      return false;
   }
   return true;
}

bool OpenVROverlay::setCursorPositionOverride(Point2F cursorPos)
{
   if (mOverlayHandle == NULL)
      return false;

   vr::HmdVector2_t vrPos;
   vrPos.v[0] = cursorPos.x;
   vrPos.v[1] = cursorPos.y;
   vr::EVROverlayError err = vr::VROverlay()->SetOverlayCursorPositionOverride(mOverlayHandle, &vrPos);
   if (err != vr::VROverlayError_None)
   {
      Con::errorf("VR Overlay error (%s) in OpenVROverlay::setCursorPositionOverride!", vr::VROverlay()->GetOverlayErrorNameFromEnum(err));
      return false;
   }
   return true;
}

bool OpenVROverlay::clearCursorPositionOverride()
{
   if (mOverlayHandle == NULL)
      return false;

   vr::EVROverlayError err = vr::VROverlay()->ClearOverlayCursorPositionOverride(mOverlayHandle);
   if (err != vr::VROverlayError_None)
   {
      Con::errorf("VR Overlay error (%s) in OpenVROverlay::clearCursorPositionOverride!", vr::VROverlay()->GetOverlayErrorNameFromEnum(err));
      return false;
   }
   return true;
}

bool OpenVROverlay::isActiveDashboardOverlay()
{
   if (mOverlayHandle == NULL)
      return false;

   return vr::VROverlay()->IsActiveDashboardOverlay(mOverlayHandle);
}

S32 OpenVROverlay::getPrimaryDashboardDevice()
{
   if (vr::VROverlay())
      return vr::VROverlay()->GetPrimaryDashboardDevice();

   return -1;
}

MatrixF OpenVROverlay::getTransformForOverlayCoordinates(const Point2F &pos)
{
   if (mOverlayHandle == NULL)
      return MatrixF::Identity;

   vr::HmdVector2_t vec;
   vec.v[0] = pos.x;
   vec.v[1] = pos.y;
   vr::HmdMatrix34_t outMat;
   MatrixF outTorqueMat;
   vr::EVROverlayError err = vr::VROverlay()->GetTransformForOverlayCoordinates(mOverlayHandle, OPENVR->mTrackingSpace, vec, &outMat);
   if (err != vr::VROverlayError_None)
   {
      Con::errorf("VR Overlay error (%s) in OpenVROverlay::getTransformForOverlayCoordinates!", vr::VROverlay()->GetOverlayErrorNameFromEnum(err));
      return MatrixF::Identity;
   }

   MatrixF vrMat(1);
   vrMat = OpenVRUtil::convertSteamVRAffineMatrixToMatrixFPlain(outMat);
   OpenVRUtil::convertTransformFromOVR(vrMat, outTorqueMat);
   return outTorqueMat;
}

bool OpenVROverlay::castRay(const Point3F &origin, const Point3F &direction, RayInfo *info)
{
   if (mOverlayHandle == NULL)
      return false;

   vr::VROverlayIntersectionParams_t params;
   vr::VROverlayIntersectionResults_t result;

   Point3F ovrOrigin = OpenVRUtil::convertPointToOVR(origin);
   Point3F ovrDirection = OpenVRUtil::convertPointToOVR(direction);

   params.eOrigin = OPENVR->mTrackingSpace;
   params.vSource.v[0] = ovrOrigin.x;
   params.vSource.v[1] = ovrOrigin.y;
   params.vSource.v[2] = ovrOrigin.z;
   params.vDirection.v[0] = ovrDirection.x;
   params.vDirection.v[1] = ovrDirection.y;
   params.vDirection.v[2] = ovrDirection.z;

   bool rayHit = vr::VROverlay()->ComputeOverlayIntersection(mOverlayHandle, &params, &result);

   if (rayHit && info)
   {
      info->t = result.fDistance;
      info->point = OpenVRUtil::convertPointFromOVR(result.vPoint);
      info->normal = OpenVRUtil::convertPointFromOVR(result.vNormal);
      info->texCoord = Point2F(result.vUVs.v[0], result.vUVs.v[1]);
      info->object = NULL;
      info->userData = this;
   }

   return rayHit;
}

Point2I OpenVROverlay::getOverlayTextureSize()
{
   if (mOverlayHandle)
   {
      U32 width, height;
      vr::EVROverlayError err = vr::VROverlay()->GetOverlayTextureSize(mOverlayHandle, &width, &height);
      if (err == vr::VROverlayError_None)
         return Point2I(width, height);

      Con::errorf("VR Overlay error (%s) in OpenVROverlay::getOverlayTextureSize!", vr::VROverlay()->GetOverlayErrorNameFromEnum(err));
   }

   return Point2I::Zero;
}

bool OpenVROverlay::showKeyboard(OpenVRGamepadTextInputMode inputMode, OpenVRGamepadTextInputLineMode lineMode,
   U32 flags, const char* pchDescription, U32 unCharMax, const char* pchExistingText, U32 uUserValue)
{
   if (vr::VROverlay())
   {
      vr::EVROverlayError err = vr::VROverlay()->ShowKeyboard(inputMode, lineMode, flags, pchDescription, unCharMax, pchExistingText, (uint64_t)uUserValue);
      if (err == vr::VROverlayError_None)
         return true;

      Con::errorf("VR Overlay error (%s) in OpenVROverlay::showKeyboard!", vr::VROverlay()->GetOverlayErrorNameFromEnum(err));
   }
   return false;
}

bool OpenVROverlay::showKeyboardForOverlay(OpenVRGamepadTextInputMode inputMode, OpenVRGamepadTextInputLineMode lineMode,
   U32 flags, const char* pchDescription, U32 unCharMax, const char* pchExistingText, U32 uUserValue)
{
   if (mOverlayHandle)
   {
      vr::EVROverlayError err = vr::VROverlay()->ShowKeyboardForOverlay(mOverlayHandle, inputMode, lineMode, flags, pchDescription, unCharMax, pchExistingText, (uint64_t)uUserValue);
      if (err == vr::VROverlayError_None)
         return true;

      Con::errorf("VR Overlay error (%s) in OpenVROverlay::showKeyboardForOverlay!", vr::VROverlay()->GetOverlayErrorNameFromEnum(err));
   }
   return false;
}

bool OpenVROverlay::hideKeyboard()
{
   if (vr::VROverlay())
   {
      vr::VROverlay()->HideKeyboard();
      return true;
   }
   return false;
}

void OpenVROverlay::getKeyboardText(String& retStr)
{
   if (vr::VROverlay())
   {
      uint32_t unRequiredBufferLen = vr::VROverlay()->GetKeyboardText(NULL, 0);
      if (unRequiredBufferLen == 0)
         return;

      unRequiredBufferLen++;
      char *pchBuffer = new char[unRequiredBufferLen];
      unRequiredBufferLen = vr::VROverlay()->GetKeyboardText(pchBuffer, unRequiredBufferLen);
      retStr = pchBuffer;
      delete[] pchBuffer;
   }
}

void OpenVROverlay::setKeyboardTransformAbsolute(const MatrixF& keyboardMat)
{
   if (!vr::VROverlay())
      return;

   MatrixF vrMat(1);
   vr::HmdMatrix34_t ovrMat;
   OpenVRUtil::convertTransformToOVR(keyboardMat, vrMat);
   OpenVRUtil::convertMatrixFPlainToSteamVRAffineMatrix(vrMat, ovrMat);

   vr::VROverlay()->SetKeyboardTransformAbsolute(OPENVR->mTrackingSpace, &ovrMat);
}

void OpenVROverlay::setKeyboardPositionForOverlay(const RectF& avoidRect)
{
   if (mOverlayHandle == NULL)
      return;

   vr::HmdRect2_t ovrRect;
   ovrRect.vTopLeft.v[0] = avoidRect.point.x;
   ovrRect.vTopLeft.v[1] = avoidRect.point.y;
   ovrRect.vBottomRight.v[0] = avoidRect.point.x + avoidRect.extent.x;
   ovrRect.vBottomRight.v[1] = avoidRect.point.y + avoidRect.extent.y;
   vr::VROverlay()->SetKeyboardPositionForOverlay(mOverlayHandle, ovrRect);
}

vr::VRMessageOverlayResponse OpenVROverlay::showMessageOverlay(const char* pchText, const char* pchCaption,
   const char* pchButton0Text, const char* pchButton1Text, const char* pchButton2Text, const char* pchButton3Text)
{
   if (!vr::VROverlay())
      return vr::VRMessageOverlayResponse_CouldntFindSystemOverlay;

   return vr::VROverlay()->ShowMessageOverlay(pchText, pchCaption, pchButton0Text, pchButton1Text, pchButton2Text, pchButton3Text);
}

void OpenVROverlay::closeMessageOverlay()
{
   if (vr::VROverlay())
      vr::VROverlay()->CloseMessageOverlay();
}

void OpenVROverlay::moveGamepadFocusToNeighbour()
{

}

void OpenVROverlay::handleOpenVREvents()
{
   vr::VREvent_t vrEvent;
   while (vr::VROverlay()->PollNextOverlayEvent(mOverlayHandle, &vrEvent, sizeof(vrEvent)))
   {
      InputEventInfo eventInfo;
      eventInfo.deviceType = MouseDeviceType;
      eventInfo.deviceInst = 0;
      eventInfo.objType = SI_AXIS;
      eventInfo.modifier = (InputModifiers)0;
      eventInfo.ascii = 0;

      //Con::printf("Overlay event %i", vrEvent.eventType);

      switch (vrEvent.eventType)
      {
      // Mouse Events
      case vr::VREvent_MouseMove:
         if (mGuiCanvas && mGuiCanvas->isActiveCanvas())
         {
            //Con::printf("mousemove %f,%f", vrEvent.data.mouse.x, vrEvent.data.mouse.y);
            Point2I extent = mGuiCanvas->getExtent();
            eventInfo.objType = SI_AXIS;
            eventInfo.objInst = SI_XAXIS;
            eventInfo.action = SI_MAKE;
            if (extent.x < extent.y)
            {  // If the texture is taller than wide, it will be centered horizontally in the overlay quad.
               Point2F fExtent(extent.x, extent.y);
               F32 mouseAdj = (vrEvent.data.mouse.x - ((fExtent.y - fExtent.x) / (2 * fExtent.y))) * (fExtent.y / fExtent.x);
               eventInfo.fValue = extent.x * mouseAdj;
            }
            else
               eventInfo.fValue = extent.x * vrEvent.data.mouse.x;
            mGuiCanvas->processInputEvent(eventInfo);

            eventInfo.objType = SI_AXIS;
            eventInfo.objInst = SI_YAXIS;
            eventInfo.action = SI_MAKE;
            if (extent.y < extent.x)
            {  // If the texture is wider than tall, it will be centered vertically in the overlay quad.
               Point2F fExtent(extent.x, extent.y);
               F32 mouseAdj = (vrEvent.data.mouse.y - ((fExtent.x - fExtent.y) / (2 * fExtent.x))) * (fExtent.x / fExtent.y);
               eventInfo.fValue = extent.y * (1.0 - mouseAdj);
            }
            else
               eventInfo.fValue = extent.y * (1.0 - vrEvent.data.mouse.y);
            mGuiCanvas->processInputEvent(eventInfo);
         }
         break;

      case vr::VREvent_MouseButtonDown:
         if (mGuiCanvas && mGuiCanvas->isActiveCanvas())
         {
            eventInfo.objType = SI_BUTTON;
            eventInfo.objInst = (InputObjectInstances)OpenVRUtil::convertOpenVRButtonToTorqueButton(vrEvent.data.mouse.button);
            eventInfo.action = SI_MAKE;
            eventInfo.fValue = 1.0f;
            mGuiCanvas->processInputEvent(eventInfo);
         }
         break;

      case vr::VREvent_MouseButtonUp:
         if (mGuiCanvas && mGuiCanvas->isActiveCanvas())
         {
            eventInfo.objType = SI_BUTTON;
            eventInfo.objInst = (InputObjectInstances)OpenVRUtil::convertOpenVRButtonToTorqueButton(vrEvent.data.mouse.button);
            eventInfo.action = SI_BREAK;
            eventInfo.fValue = 0.0f;
            mGuiCanvas->processInputEvent(eventInfo);
         }
         break;

      case vr::VREvent_OverlayShown:
      {
         //markDirty();
      }
      break;

      case vr::VREvent_Quit:
         AssertFatal(false, "vr::VREvent_Quit event received.");
         break;

      // Keyboard events
      case vr::VREvent_KeyboardClosed:
         onKeyboardClosed_callback(vrEvent.data.keyboard.uUserValue);
         break;
      case vr::VREvent_KeyboardCharInput:
         onKeyboardInput_callback(vrEvent.data.keyboard.cNewInput, vrEvent.data.keyboard.uUserValue);
         break;
      case vr::VREvent_KeyboardDone:
         AssertWarn(vrEvent.data.keyboard.cNewInput[0] == '\0', "Text data in VREvent_KeyboardDone message!");
         onKeyboardDone_callback(vrEvent.data.keyboard.uUserValue);
         break;

      default:
         Con::warnf("Unhandled VROverlay() event #%u, %s", vrEvent.eventType, vr::VRSystem()->GetEventTypeNameFromEnum((vr::EVREventType) vrEvent.eventType));
         break;
      }
   }

   if (mThumbOverlayHandle != vr::k_ulOverlayHandleInvalid)
   {
      while (vr::VROverlay()->PollNextOverlayEvent(mThumbOverlayHandle, &vrEvent, sizeof(vrEvent)))
      {
         switch (vrEvent.eventType)
         {
         case vr::VREvent_OverlayShown:
         {
            //markDirty();
         }
         break;
         }
      }
   }
}

void OpenVROverlay::onCanvasFrame()
{  // The offscreen canvas has updated it's render target.
   vr::IVROverlay *overlay = vr::VROverlay();
   if (!overlay || !mOverlayHandle || !mGuiCanvas)
      return;

   updateOverlay();

   NamedTexTargetRef texTarget = mGuiCanvas->getTarget();

   vr::Texture_t tex;
   tex = { NULL, vr::TextureType_Invalid, vr::ColorSpace_Auto };
#if defined(TORQUE_OS_WIN64) || defined(TORQUE_OS_WIN32) || defined(TORQUE_D3D11)
   if (GFX->getAdapterType() == Direct3D11)
   {
      tex = { (void*)static_cast<GFXD3D11TextureObject*>(texTarget->getTexture())->getResource(), vr::TextureType_DirectX, vr::ColorSpace_Auto };
   }
#endif
#ifdef TORQUE_OPENGL
   if (GFX->getAdapterType() == OpenGL)
   {
      tex = { (void*)(uintptr_t)static_cast<GFXGLTextureObject*>(texTarget->getTexture())->getHandle(), vr::TextureType_OpenGL, vr::ColorSpace_Auto };
   }
#endif
   if (tex.eType == vr::TextureType_Invalid)
   {
      return;
   }

   vr::EVROverlayError err = overlay->SetOverlayTexture(mOverlayHandle, &tex);
   if (err != vr::VROverlayError_None)
   {
      Con::errorf("VR: Error (%s) setting overlay texture.", vr::VROverlay()->GetOverlayErrorNameFromEnum(err));
   }

   return;
}

DefineEngineMethod(OpenVROverlay, showOverlay, void, (), ,
   "Shows the VR overlay. For dashboard overlays, the dashboard will also be activated.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   object->showOverlay();
}

DefineEngineMethod(OpenVROverlay, hideOverlay, void, (), ,
   "Hides the VR overlay. For dashboard overlays, only the Dashboard Manager is allowed to call this. "
   "The dashboard cannot be hidden by an application, so this call will have no effect for dashboard overlays.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   object->hideOverlay();
}

DefineEngineMethod(OpenVROverlay, isOverlayVisible, bool, (), ,
   "Returns true if the overlay is visible.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->isOverlayVisible();
}

DefineEngineMethod(OpenVROverlay, isActiveDashboardOverlay, bool, (), ,
   "Returns true if the dashboard is visible and this overlay is the active system Overlay.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->isActiveDashboardOverlay();
}

DefineEngineMethod(OpenVROverlay, isHoverTarget, bool, (), ,
   "Returns true if the specified overlay is the hover target. An overlay is the hover target when it "
   "is the last overlay \"moused over\" by the virtual mouse pointer.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->isOverlayHoverTarget();
}

DefineEngineMethod(OpenVROverlay, setCursorOverlay, void, (OpenVROverlay* cursor), ,
   "Sets the OpenVROverlay object to use as a cursor on this overlay. This will be drawn instead of "
   "the generic blob when the laser mouse is pointed at the overlay.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   object->setCursorOverlay(cursor);
}

DefineEngineMethod(OpenVROverlay, triggerHapticVibration, bool, (F32 duration, F32 frequency, F32 amplitude), ,
   "Triggers a haptic event on the laser mouse controller for the specified overlay.\n"
   "@param duration - Duration of vibration in seconds.\n"
   "@param frequency - undocumented.\n"
   "@param amplitude - undocumented. (TODO: Find range? 0-1?)\n"
   "@return Returns true if the command completed successfully. False on error.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->triggerHapticVibration(duration, frequency, amplitude);
}

DefineEngineMethod(OpenVROverlay, setCursorPositionOverride, bool, (Point2F cursorPos), ,
   "Sets the override cursor position to use for this overlay in overlay mouse coordinates.\n"
   "@param cursorPos - This position will be used to draw the cursor instead of whatever the "
   "laser mouse cursor position is.\n"
   "@return Returns true if the command completed successfully. False on error.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->setCursorPositionOverride(cursorPos);
}

DefineEngineMethod(OpenVROverlay, clearCursorPositionOverride, bool, (), ,
   "Clears the override cursor position for this overlay.\n"
   "@return Returns true if the command completed successfully. False on error.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->clearCursorPositionOverride();
}

DefineEngineMethod(OpenVROverlay, getTransformForOverlayCoordinates, TransformF, (Point2F overlayPos), ,
   "Get the transform in 3d space associated with a specific 2d point in the overlay's coordinate "
   "space. +Y points out of the overlay.\n"
   "@param overlayPos - This is the position in 2d overlay space (0,0 is the lower left) that will "
   "be converted to a 3D transform.\n"
   "@return Returns a 3d transform corresponding to the point or an identity matrix on error.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->getTransformForOverlayCoordinates(overlayPos);
}

DefineEngineMethod(OpenVROverlay, getPrimaryDashboardDevice, S32, (), ,
   "Returns the index of the tracked device that has the laser pointer in the dashboard.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->getPrimaryDashboardDevice();
}

DefineEngineMethod(OpenVROverlay, mountToOverlay, void, (OpenVROverlay* mountObj), ,
   "Sets the OpenVROverlay object to mount this overlay to. When mounted, the set transform will be "
   "relative to the parent overlay transform.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   object->setMountToOverlay(mountObj);
}

DefineEngineMethod(OpenVROverlay, showKeyboard, bool, (OpenVRGamepadTextInputMode inputMode, OpenVRGamepadTextInputLineMode lineMode,
   OpenVRKeyboardFlags flags, const char* description, U32 maxChars, const char* existingText, U32 userValue), (0),
   "Show the virtual keyboard to accept input. In most cases, you should pass "
   "OpenVRKeyboardFlags::Modal to enable modal overlay behavior on the keyboard itself.\n"
   "@param inputMode Text input display mode. \"Normal\", \"Password\" or \"Submit\".\n"
   "@param lineMode \"SingleLine\" or \"MultipleLines\"\n"
   "@param flags \"Minimal\" or \"Modal\". Minimal makes the keyboard send key events "
   "immediately instead of accumulating a buffer. Modal makes the keyboard take all "
   "focus and dismiss when clicking off the panel.\n"
   "@param description Description text to be placed on the keyboard.\n"
   "@param maxChars Maximum number of characters that the input should accept.\n"
   "@param existingText The initial text to place in the keyboard input line(s).\n"
   "@param userValue An arbitrary U32 value that can be attached to the keyboard. "
   "Setting this to the object id of the text input gui that is requesting the keyboard "
   "can be useful for verifying the text target after a modal keyboard returns.\n"
   "@return Returns true if the command completed successfully. False on error.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->showKeyboard(inputMode, lineMode, flags, description, maxChars, existingText, userValue);
}

DefineEngineMethod(OpenVROverlay, showKeyboardForOverlay, bool, (OpenVRGamepadTextInputMode inputMode, OpenVRGamepadTextInputLineMode lineMode,
   OpenVRKeyboardFlags flags, const char* description, U32 maxChars, const char* existingText, U32 userValue), (0),
   "Show the virtual keyboard to accept input attached to this overlay. In most cases, you should pass "
   "OpenVRKeyboardFlags::Modal to enable modal overlay behavior on the keyboard itself.\n"
   "@param inputMode Text input display mode. \"Normal\", \"Password\" or \"Submit\".\n"
   "@param lineMode \"SingleLine\" or \"MultipleLines\"\n"
   "@param flags \"Minimal\" or \"Modal\". Minimal makes the keyboard send key events "
   "immediately instead of accumulating a buffer. Modal makes the keyboard take all "
   "focus and dismiss when clicking off the panel.\n"
   "@param description Description text to be placed on the keyboard.\n"
   "@param maxChars Maximum number of characters that the input should accept.\n"
   "@param existingText The initial text to place in the keyboard input line(s).\n"
   "@param userValue An arbitrary U32 value that can be attached to the keyboard. "
   "Setting this to the object id of the text input gui that is requesting the keyboard "
   "can be useful for verifying the text target after a modal keyboard returns.\n"
   "@return Returns true if the command completed successfully. False on error.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->showKeyboardForOverlay(inputMode, lineMode, flags, description, maxChars, existingText, userValue);
}

DefineEngineMethod(OpenVROverlay, hideKeyboard, bool, (), ,
   "Hide the virtual keyboard.\n"
   "@return Returns true if the command completed successfully. False on error.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->hideKeyboard();
}

DefineEngineMethod(OpenVROverlay, getKeyboardText, String, (), ,
   "Get the text that was entered into the virtual keyboard.\n"
   "@return The user text.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   String retString;
   object->getKeyboardText(retString);
   return retString;
}

DefineEngineMethod(OpenVROverlay, setKeyboardTransformAbsolute, void, (TransformF kbTransform), ,
   "Set the position of the keyboard in world space.\n"
   "@param kbTransform Transform for keyboard relative to the vr tracking space origin.\n"
   "@return Returns true if the command completed successfully. False on error.\n"
   "@see OpenVROverlay::showKeyboard()\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   object->setKeyboardTransformAbsolute(kbTransform.getMatrix());
}

DefineEngineMethod(OpenVROverlay, setKeyboardPositionForOverlay, void, (RectF avoidRect), ,
   "Set the position of the keyboard in overlay space by telling it to avoid a rectangle in the overlay.\n"
   "@param avoidRect A rectangle in overlay units. Rectangle coords have (0,0) in the bottom left and (1,1) in the upper right.\n"
   "@return Returns true if the command completed successfully. False on error.\n"
   "@see OpenVROverlay::showKeyboardForOverlay()\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   object->setKeyboardPositionForOverlay(avoidRect);
}

DefineEngineMethod(OpenVROverlay, showMessageOverlay, OpenVRMessageResponse, (const char* title, const char* message, const char* button0Text,
   const char* button1Text, const char* button2Text, const char*button3Text), ("", "", ""),
   "Show the message overlay. This will block and return you a result.\n"
   "@param title The title to display.\n"
   "@param message The message caption text.\n"
   "@param button0Text The text to display on button 0.\n"
   "@param button1Text The text to display on button 1 (optional).\n"
   "@param button2Text The text to display on button 2 (optional).\n"
   "@param button3Text The text to display on button 3 (optional).\n"
   "@return A message response code. @see OpenVRMessageResponse\n"
   "@see OpenVROverlay::showKeyboardForOverlay()\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   return object->showMessageOverlay(title, message, button0Text, button1Text, button2Text, button3Text);
}

DefineEngineMethod(OpenVROverlay, closeMessageOverlay, void, (), ,
   "If the calling process owns the system message overlay and it's open, this will close it.\n"
   "@ingroup OpenVROverlay\n"
   "@ingroup OpenVR\n")
{
   object->closeMessageOverlay();
}
