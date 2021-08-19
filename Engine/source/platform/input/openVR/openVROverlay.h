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

#ifndef _OPENVR_OVERLAY_H_
#define _OPENVR_OVERLAY_H_

typedef vr::VROverlayTransformType OpenVROverlayTransformType;
typedef vr::VROverlayInputMethod OpenVROverlayInputMethod;
typedef vr::EGamepadTextInputMode OpenVRGamepadTextInputMode;
typedef vr::EGamepadTextInputLineMode OpenVRGamepadTextInputLineMode;
typedef vr::EKeyboardFlags OpenVRKeyboardFlags;
typedef vr::VRMessageOverlayResponse OpenVRMessageResponse;

DefineEnumType(OpenVROverlayTransformType);
DefineEnumType(OpenVROverlayInputMethod);
DefineEnumType(OpenVRGamepadTextInputMode);
DefineEnumType(OpenVRGamepadTextInputLineMode);
DefineEnumType(OpenVRKeyboardFlags);
DefineEnumType(OpenVRMessageResponse);

struct RayInfo;
class GuiOffscreenCanvas;
class OpenVROverlay : public SimObject
{
public:
   typedef SimObject Parent;

   enum OverlayType
   {
      OVERLAYTYPE_OVERLAY,
      OVERLAYTYPE_DASHBOARD,
   };

   // Mask of flags that we support setting from script.
   static U32 smSupportedFlags;

   DECLARE_CALLBACK(void, onKeyboardClosed, (U32 userValue));
   DECLARE_CALLBACK(void, onKeyboardInput, (const char* inputText, U32 userValue));
   DECLARE_CALLBACK(void, onKeyboardDone, (U32 userValue));

protected:
   // Handles for vr overlay objects.
   vr::VROverlayHandle_t mOverlayHandle;
   vr::VROverlayHandle_t mThumbOverlayHandle;

   // Overlay that is being used as a cursor on this one.
   OpenVROverlay* mCursorOverlay;

   // Overlay that this overlay is mounted to.
   OpenVROverlay* mMountToOverlay;

   // Overlays that accept input or have dynamic textures need an offscreen canvas to render and process input.
   GuiOffscreenCanvas* mGuiCanvas;

   // Static overlays need a texture to render.
   String mTextureFile;
   bool mTextureLoaded;

   // Dashboard overlays can specify a texture for the thumbnail image.
   String mThumbnailFile;
   bool mThumbnailLoaded;

   // UVMin and UVMax defining the area of the texture to render on the overlay.
   Point2F mBoundsUVMin;
   Point2F mBoundsUVMax;

   String mOverlayName;

   // Desired OpenVR state
   U32 mOverlayFlags;
   F32 mOverlayWidth;

   vr::VROverlayTransformType mOverlayTransformType;
   MatrixF mTransform;
   vr::TrackedDeviceIndex_t mTransformDeviceIndex;
   String mTransformDeviceComponent;


   vr::VROverlayInputMethod mInputMethod;
   Point2F mMouseScale;

   LinearColorF mOverlayColor;
   F32 mTexelAspect;
   S32 mSortOrder;
   F32 mCurvature;

   bool mOverlayTypeDirty; ///< Overlay type is dirty
   bool mOverlayDirty; ///< Overlay properties are dirty
   OverlayType mOverlayType;

   //

public:
   OpenVROverlay();
   virtual ~OpenVROverlay();

   static void initPersistFields();

   DECLARE_CONOBJECT(OpenVROverlay);

   bool onAdd();
   void onRemove();

   /// Called when the mGuiCanvas is deleted
   virtual void onDeleteNotify(SimObject *object);

   void resetOverlay();
   void updateOverlay();

   void showOverlay();
   void hideOverlay();

   void setCursorOverlay(OpenVROverlay* cursor);
   void setMountToOverlay(OpenVROverlay* mountObj);

   bool isOverlayVisible();
   bool isOverlayHoverTarget();

   bool triggerHapticVibration(F32 durationSeconds, F32 frequency, F32 amplitude);
   bool setCursorPositionOverride(Point2F cursorPos);
   bool clearCursorPositionOverride();

   bool isActiveDashboardOverlay();
   S32 getPrimaryDashboardDevice();

   MatrixF getTransformForOverlayCoordinates(const Point2F &pos);
   bool castRay(const Point3F &origin, const Point3F &direction, RayInfo *info);
   Point2I getOverlayTextureSize();

   // Virtual keyboard functions
   bool showKeyboard(OpenVRGamepadTextInputMode inputMode, OpenVRGamepadTextInputLineMode lineMode, U32 flags,
      const char* pchDescription, U32 unCharMax, const char* pchExistingText, U32 uUserValue);
   bool showKeyboardForOverlay(OpenVRGamepadTextInputMode inputMode, OpenVRGamepadTextInputLineMode lineMode, U32 flags,
      const char* pchDescription, U32 unCharMax, const char* pchExistingText, U32 uUserValue);
   bool hideKeyboard();
   void getKeyboardText(String& retStr);
   void setKeyboardTransformAbsolute(const MatrixF& keyboardMat);
   void setKeyboardPositionForOverlay(const RectF& avoidRect);

   // System messagebox functions
   vr::VRMessageOverlayResponse showMessageOverlay(const char* pchText, const char* pchCaption, const char* pchButton0Text,
      const char* pchButton1Text = nullptr, const char* pchButton2Text = nullptr, const char* pchButton3Text = nullptr);
   void closeMessageOverlay();

   void moveGamepadFocusToNeighbour();

   void handleOpenVREvents();
   void onCanvasFrame();

   vr::VROverlayHandle_t getOverlayHandle() { return mOverlayHandle; }

protected:
   void setOverlayName(const char* overlayName);
   void setOverlayCanvas(GuiOffscreenCanvas* canvas);
   void setOverlayTextureFile(const char* fileName);
   void setThumbnailTextureFile(const char* fileName);

   bool setOverlayTexture(vr::VROverlayHandle_t overlayHandle, String& fileName);
   void setOverlayFlags();
   void setOverlayTransform();

private:
   // Protected set methods
   static bool setProtectedOverlayTypeDirty(void* object, const char* index, const char* data);
   static bool setProtectedOverlayDirty(void* object, const char* index, const char* data);
   static bool setProtectedOverlayName(void* object, const char* index, const char* data);
   static bool setProtectedOverlayCanvas(void* object, const char* index, const char* data);
   static bool setProtectedTextureFile(void* object, const char* index, const char* data);
   static bool setProtectedThumbnailFile(void* object, const char* index, const char* data);
   static bool setProtectedCursorOverlay(void* object, const char* index, const char* data);
   static bool setProtectedMountToOverlay(void* object, const char* index, const char* data);
};

typedef OpenVROverlay::OverlayType OpenVROverlayType;
DefineEnumType(OpenVROverlayType);

#endif
