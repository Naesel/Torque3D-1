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

#ifndef _OPENVR_INPUT_H_
#define _OPENVR_INPUT_H_

//-----------------------------------------------------------------------------
typedef vr::EVRInputError OpenVRInputError;

DefineEnumType(OpenVRInputError);

//-----------------------------------------------------------------------------

class OpenVRInput
{
protected:
   enum
   {
      MaxActiveActionSets = 5,  // The maximum number of action set layers that can be active at one time.
   };

public:
   OpenVRInput();
   virtual ~OpenVRInput();

public:
   /// @name IVRInput handling
   /// {
private:
   bool mInputInitialized;
   Vector<VRActionSet> mActionSets;
   Vector<VRAnalogAction> mAnalogActions;
   Vector<VRDigitalAction> mDigitalActions;
   Vector<VRPoseAction> mPoseActions;
   Vector<VRSkeletalAction> mSkeletalActions;
   Vector<vr::VRActionHandle_t> mHapticOutputs;

   U32 mNumSetsActive;
   vr::VRActiveActionSet_t mActiveSets[MaxActiveActionSets];
   S32 mActiveSetIndexes[MaxActiveActionSets];

   void resetActiveSets();
   void processDigitalActions();
   void processAnalogActions();
   void processPoseActions();
   void processSkeletalActions();

public:
   void processInput(); // Called from the message loop to process all input events.
   OpenVRInputError setActionManifestPath(const char* manifestPath);

   S32 addActionSet(const char* setName);
   S32 addAnalogAction(U32 setIndex, const char* actionName, const char* callbackFunc);
   S32 addDigitalAction(U32 setIndex, const char* actionName, const char* callbackFunc);
   S32 addPoseAction(U32 setIndex, const char* actionName, const char* poseCallback, const char* velocityCallback, S32 moveIndex);
   S32 addSkeletalAction(U32 setIndex, const char* actionName, S32 moveIndex);
   S32 addHapticOutput(const char* outputName);

   S32 getPoseIndex(const char* actionName);
   bool getCurrentPose(S32 poseIndex, Point3F& position, QuatF& rotation);
   bool setPoseCallbacks(S32 poseIndex, const char* poseCallback, const char* velocityCallback);
   S32 getSkeletonIndex(const char* actionName);
   bool getSkeletonNodes(S32 skeletonIndex, vr::VRBoneTransform_t* boneData);
   bool setSkeletonMode(S32 skeletonIndex, bool withController);

   bool activateActionSet(S32 controllerIndex, U32 setIndex);
   bool pushActionSetLayer(S32 controllerIndex, U32 setIndex);
   bool popActionSetLayer(S32 controllerIndex, U32 setIndex);
   bool triggerHapticEvent(U32 actionIndex, float fStartSecondsFromNow, float fDurationSeconds, float fFrequency, float fAmplitude);

   void showActionOrigins(U32 setIndex, OpenVRActionType actionType, U32 actionIndex);
   void showActionSetBinds(U32 setIndex);
   /// }

public:
   // For ManagedSingleton.
   static const char* getSingletonName() { return "OpenVRInput"; }
};

/// Returns the OpenVRInput singleton.
#define OVRINPUT ManagedSingleton<OpenVRInput>::instance()

#endif
