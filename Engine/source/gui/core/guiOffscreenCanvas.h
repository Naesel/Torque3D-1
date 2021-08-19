#ifndef _GUIOFFSCREENCANVAS_H_
#define _GUIOFFSCREENCANVAS_H_

#include "math/mMath.h"
#include "gui/core/guiCanvas.h"
#include "core/util/tVector.h"

#ifndef _MATTEXTURETARGET_H_
#include "materials/matTextureTarget.h"
#endif

#ifndef _COLLISION_H_
#include "collision/collision.h"
#endif

class GuiTextureDebug;

class GuiOffscreenCanvas : public GuiCanvas
{
public:
   typedef GuiCanvas Parent;
   typedef Signal< void() > OffscreenRenderSignal;

   enum KeyTranslationMode
   {
      TranslationMode_Platform,
      TranslationMode_Callback,
      TranslationMode_Ignore,
   };

   DECLARE_CALLBACK(bool, onEnableKeyboardTranslation, ());
   DECLARE_CALLBACK(bool, onDisableKeyboardTranslation, ());
   DECLARE_CALLBACK(bool, onSetNativeAcceleratorsEnabled, (bool enable));
   
   GuiOffscreenCanvas();
   ~GuiOffscreenCanvas();
   
   bool onAdd();
   void onRemove();
   
   void renderFrame(bool preRenderOnly, bool bufferSwap);
   virtual void onFrameRendered();
   
   Point2I getWindowSize();

   Point2I getCursorPos();
   void setCursorPos(const Point2I &pt);
   void setCursorPosFromUV(const Point2F &pt);
   void showCursor(bool state);
   bool isCursorShown();
   
   void _onTextureEvent( GFXTexCallbackCode code );

   void dumpTarget(const char *filename);

   void _setupTargets();
   void _teardownTargets();

   NamedTexTargetRef getTarget() { return &mNamedTarget; }
   bool isActiveCanvas() { return sActiveOffscreenCanvas == this; }
   void setCanvasActive(bool active);
   bool canInteract() { return mCanInteract; }
   F32 getMaxInteractDistance() { return mMaxInteractDistance; }
   GuiCursor* getMouseCursor() { return mDefaultCursor; }

   void markDirty() { mTargetDirty = true; }

   static void initPersistFields();
   
   virtual void enableKeyboardTranslation();
   virtual void disableKeyboardTranslation();
   virtual void setNativeAcceleratorsEnabled(bool enabled);

   DECLARE_CONOBJECT(GuiOffscreenCanvas);

protected:
   GFXTextureTargetRef mTarget;
   NamedTexTarget mNamedTarget;
   GFXTexHandle mTargetTexture;

   GFXFormat mTargetFormat;
   Point2I mTargetSize;
   String mTargetName;

   bool mTargetDirty;
   bool mDynamicTarget;
   
   bool mUseDepth;
   GFXTexHandle mTargetDepth;
   bool mCanInteract;
   F32 mMaxInteractDistance;
   U32 mRenderCount;

   OffscreenRenderSignal mRenderSignal;

   KeyTranslationMode mKeyTranslationMode;
   KeyTranslationMode mNativeAcceleratorMode;

public:
   static GuiOffscreenCanvas* sActiveOffscreenCanvas;
   static Vector<GuiOffscreenCanvas*> sList;
   static GuiOffscreenCanvas *getCanvasFromRayInfo(RayInfo &info);

   OffscreenRenderSignal& getRenderSignal() { return mRenderSignal; }
   U32 getRenderCount() { return mRenderCount; }
};

typedef GuiOffscreenCanvas::KeyTranslationMode OSCanvasTranslationMode;
DefineEnumType(OSCanvasTranslationMode);

#endif
