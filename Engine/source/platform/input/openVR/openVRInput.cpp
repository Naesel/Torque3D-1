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

#include "T3D/gameBase/extended/extendedMove.h"

IMPLEMENT_GLOBAL_CALLBACK(onOVRInputReady, void, (), (),
   "Callback posted when the IVRInput api has been initialized. Game scripts should "
   "respond to this callback by loading all action and actionset handles.\n"
   "@ingroup OVRInput\n"
   "@ingroup OpenVR");


OpenVRInput::OpenVRInput() :
   mInputInitialized(false),
   mNumSetsActive(0U)
{
}

OpenVRInput::~OpenVRInput()
{
}

void OpenVRInput::processInput()
{
   // Process IVRInput action events
   if (mInputInitialized && mNumSetsActive)
   {
      vr::VRInput()->UpdateActionState(mActiveSets, sizeof(vr::VRActiveActionSet_t), mNumSetsActive);
      processDigitalActions();
      processAnalogActions();
      processPoseActions();
      processSkeletalActions();
   }
}

// IVRInput Methods
OpenVRInputError OpenVRInput::setActionManifestPath(const char* manifestPath)
{
   vr::EVRInputError vrError = vr::VRInputError_None;
   if (!mInputInitialized)
   {
      String fullPath = String::ToString("%s/%s", Platform::getMainDotCsDir(), manifestPath);

      // Not finding the file is not a fatal error since the runtime can override the path setting
      if (!Platform::isFile(fullPath.c_str()))
         Con::warnf("OpenVR action manifest file not found (%s)!", fullPath.c_str());

      vrError = vr::VRInput()->SetActionManifestPath(fullPath.c_str());
      if (vrError != vr::VRInputError_None && vrError != vr::VRInputError_MismatchedActionManifest)
         Con::errorf("OpenVRInput::initInput() failed to initialize IVRInput. Error code: %s", castConsoleTypeToString((OpenVRInputError) vrError));
      else
      {
         mInputInitialized = true;
         // Tell scripts to load the handles
         onOVRInputReady_callback();
      }
   }

   return vrError;
}

void OpenVRInput::processDigitalActions()
{
   static const char* argv[3];
   int numDigital = mDigitalActions.size();

   vr::InputDigitalActionData_t digitalData;
   for (int i = 0; i < numDigital; ++i)
   {
      if (mDigitalActions[i].active)
      {
         vr::EVRInputError vrError = vr::VRInput()->GetDigitalActionData(mDigitalActions[i].actionHandle,
            &digitalData, sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidInputValueHandle);
         if ((vrError == vr::VRInputError_None) && digitalData.bActive && digitalData.bChanged)
         {
            argv[0] = mDigitalActions[i].callback;
            argv[1] = Con::getIntArg((S32)digitalData.activeOrigin);
            argv[2] = Con::getBoolArg(digitalData.bState);
            Con::execute(3, argv);
         }
      }
   }
}

void OpenVRInput::processAnalogActions()
{
   static const char* argv[5];
   int numAnalog = mAnalogActions.size();

   vr::InputAnalogActionData_t analogData;
   for (int i = 0; i < numAnalog; ++i)
   {
      if (mAnalogActions[i].active)
      {
         vr::EVRInputError vrError = vr::VRInput()->GetAnalogActionData(mAnalogActions[i].actionHandle,
            &analogData, sizeof(vr::InputAnalogActionData_t), vr::k_ulInvalidInputValueHandle);
         if ((vrError == vr::VRInputError_None) && analogData.bActive &&
            ((analogData.deltaX != 0.0f) || (analogData.deltaY != 0.0f) || (analogData.deltaZ != 0.0f)))
         {
            argv[0] = mAnalogActions[i].callback;
            argv[1] = Con::getIntArg((S32)analogData.activeOrigin);
            argv[2] = Con::getFloatArg(analogData.x);
            argv[3] = Con::getFloatArg(analogData.y);
            argv[4] = Con::getFloatArg(analogData.z);
            Con::execute(5, argv);
         }
      }
   }
}

void OpenVRInput::processPoseActions()
{
   static const char* argv[9];
   int numPoses = mPoseActions.size();

   vr::InputPoseActionData_t poseData;
   Point3F posVal;
   QuatF rotVal;
   for (int i = 0; i < numPoses; ++i)
   {
      if (mPoseActions[i].active)
      {
         vr::EVRInputError vrError = vr::VRInput()->GetPoseActionDataRelativeToNow(mPoseActions[i].actionHandle,
            OPENVR->mTrackingSpace, 0.0f, &poseData, sizeof(vr::InputPoseActionData_t), vr::k_ulInvalidInputValueHandle);
         if ((vrError == vr::VRInputError_None) && poseData.bActive &&
            poseData.pose.bPoseIsValid && poseData.pose.bDeviceIsConnected)
         {
            MatrixF mat = OpenVRUtil::convertSteamVRAffineMatrixToMatrixFPlain(poseData.pose.mDeviceToAbsoluteTracking);
            if (!mIsZero(OPENVR->smUniverseYawOffset))
               mat.mulL(OPENVR->smUniverseRotMat);

            MatrixF torqueMat;
            OpenVRUtil::convertTransformFromOVR(mat, torqueMat);

            posVal = torqueMat.getPosition();
            if (OPENVR->mTrackingSpace == vr::TrackingUniverseStanding)
               posVal.z -= OPENVR->mStandingHMDHeight;
            mPoseActions[i].lastPosition = posVal;

            rotVal = QuatF(torqueMat);
            mPoseActions[i].lastRotation = rotVal;
            mPoseActions[i].validPose = true;

            S32 idx = mPoseActions[i].eMoveIndex;
            if (idx >= 0 && idx < ExtendedMove::MaxPositionsRotations)
            {
               ExtendedMoveManager::mDeviceIsActive[idx] = true;
               ExtendedMoveManager::mPosX[idx] = posVal.x;
               ExtendedMoveManager::mPosY[idx] = posVal.y;
               ExtendedMoveManager::mPosZ[idx] = posVal.z;
               ExtendedMoveManager::mRotAX[idx] = rotVal.x;
               ExtendedMoveManager::mRotAY[idx] = rotVal.y;
               ExtendedMoveManager::mRotAZ[idx] = rotVal.z;
               ExtendedMoveManager::mRotAW[idx] = rotVal.w;
            }

            if (mPoseActions[i].poseCallback && mPoseActions[i].poseCallback[0])
            {
               argv[0] = mPoseActions[i].poseCallback;
               argv[1] = Con::getIntArg((S32)poseData.activeOrigin);
               argv[2] = Con::getFloatArg(posVal.x);
               argv[3] = Con::getFloatArg(posVal.y);
               argv[4] = Con::getFloatArg(posVal.z);
               argv[5] = Con::getFloatArg(rotVal.x);
               argv[6] = Con::getFloatArg(rotVal.y);
               argv[7] = Con::getFloatArg(rotVal.z);
               argv[8] = Con::getFloatArg(rotVal.w);
               Con::execute(9, argv);
            }

            if (mPoseActions[i].velocityCallback && mPoseActions[i].velocityCallback[0])
            {
               argv[0] = mPoseActions[i].velocityCallback;
               argv[1] = Con::getIntArg((S32)poseData.activeOrigin);
               argv[2] = Con::getFloatArg(poseData.pose.vVelocity.v[0]);
               argv[3] = Con::getFloatArg(-poseData.pose.vVelocity.v[2]);
               argv[4] = Con::getFloatArg(poseData.pose.vVelocity.v[1]);
               argv[5] = Con::getFloatArg(poseData.pose.vAngularVelocity.v[0]);
               argv[6] = Con::getFloatArg(-poseData.pose.vAngularVelocity.v[2]);
               argv[7] = Con::getFloatArg(poseData.pose.vAngularVelocity.v[1]);
               Con::execute(8, argv);
            }
         }
      }
   }
}

void OpenVRInput::processSkeletalActions()
{
   int numActions = mSkeletalActions.size();

   vr::InputSkeletalActionData_t skeletonData;
   for (int i = 0; i < numActions; ++i)
   {
      if (mSkeletalActions[i].active)
      {
         vr::EVRInputError vrError = vr::VRInput()->GetSkeletalActionData(mSkeletalActions[i].actionHandle,
            &skeletonData, sizeof(vr::InputSkeletalActionData_t));
         if ((vrError == vr::VRInputError_None) && skeletonData.bActive)
         {
            U32 requiredSize;
            vr::EVRSkeletalMotionRange motionRange = mSkeletalActions[i].rangeWithController ?
               vr::VRSkeletalMotionRange_WithController : vr::VRSkeletalMotionRange_WithoutController;
            if (vr::VRInputError_None == vr::VRInput()->GetSkeletalBoneDataCompressed(mSkeletalActions[i].actionHandle,
               motionRange, (void*)&ExtendedMoveManager::mBinaryBlob[mSkeletalActions[i].eMoveIndex],
               ExtendedMove::MaxBinBlobSize, &requiredSize))
            {
               ExtendedMoveManager::mBinBlobSize[mSkeletalActions[i].eMoveIndex] = requiredSize;
            }
            else
            {
               AssertWarn(requiredSize < ExtendedMove::MaxBinBlobSize, "GetSkeletalBoneDataCompressed buffer size too small! Increase ExtendedMove::MaxBinBlobSize.");
            }
         }
      }
   }
}

S32 OpenVRInput::addActionSet(const char* setName)
{
   if (setName)
   {
      vr::VRActionSetHandle_t setHandle;
      vr::EVRInputError vrError = vr::VRInput()->GetActionSetHandle(setName, &setHandle);
      if (vrError == vr::VRInputError_None)
      {
         S32 retIndex = mActionSets.size();
         mActionSets.push_back(VRActionSet(setHandle, setName));
         //Con::printf("VRInput Action Set %s, Handle: %I64u, Index: %d", setName, setHandle, retIndex);
         return retIndex;
      }
      Con::warnf("OpenVRInput::addActionSet failed for action set: %s.", setName);
   }
   return -1;
}

S32 OpenVRInput::addAnalogAction(U32 setIndex, const char* actionName, const char* callbackFunc)
{
   if (actionName && callbackFunc && setIndex < mActionSets.size())
   {
      vr::VRActionHandle_t actionHandle;
      vr::EVRInputError vrError = vr::VRInput()->GetActionHandle(actionName, &actionHandle);
      if (vrError == vr::VRInputError_None)
      {
         S32 retIndex = mAnalogActions.size();
         mAnalogActions.push_back(VRAnalogAction(setIndex, actionHandle, actionName, callbackFunc));
         //Con::printf("VRInput Analog Action %s, Handle: %I64u, Index: %d", actionName, actionHandle, retIndex);
         return retIndex;
      }
      Con::warnf("OpenVRInput::addAnalogAction failed for action: %s.", actionName);
   }
   return -1;
}

S32 OpenVRInput::addDigitalAction(U32 setIndex, const char* actionName, const char* callbackFunc)
{
   if (actionName && callbackFunc && setIndex < mActionSets.size())
   {
      vr::VRActionHandle_t actionHandle;
      vr::EVRInputError vrError = vr::VRInput()->GetActionHandle(actionName, &actionHandle);
      if (vrError == vr::VRInputError_None)
      {
         S32 retIndex = mDigitalActions.size();
         mDigitalActions.push_back(VRDigitalAction(setIndex, actionHandle, actionName, callbackFunc));
         //Con::printf("VRInput Boolean Action %s, Handle: %I64u, Index: %d", actionName, actionHandle, retIndex);
         return retIndex;
      }
      Con::warnf("OpenVRInput::addDigitalAction failed for action: %s.", actionName);
   }
   return -1;
}

S32 OpenVRInput::addPoseAction(U32 setIndex, const char* actionName,
   const char* poseCallback, const char* velocityCallback, S32 moveIndex)
{
   if (actionName && setIndex < mActionSets.size())
   {
      vr::VRActionHandle_t actionHandle;
      vr::EVRInputError vrError = vr::VRInput()->GetActionHandle(actionName, &actionHandle);
      if (vrError == vr::VRInputError_None)
      {
         S32 retIndex = mPoseActions.size();
         mPoseActions.push_back(VRPoseAction(setIndex, actionHandle, actionName, poseCallback, velocityCallback, moveIndex));
         //Con::printf("VRInput Pose Action %s, Handle: %I64u, Index: %d", actionName, actionHandle, retIndex);
         return retIndex;
      }
      Con::warnf("OpenVRInput::addPoseAction failed for action: %s.", actionName);
   }
   return -1;
}

S32 OpenVRInput::addSkeletalAction(U32 setIndex, const char* actionName, S32 moveIndex)
{
   if (actionName && setIndex < mActionSets.size() && moveIndex >= 0 && moveIndex < ExtendedMove::MaxPositionsRotations)
   {
      vr::VRActionHandle_t actionHandle;
      vr::EVRInputError vrError = vr::VRInput()->GetActionHandle(actionName, &actionHandle);
      if (vrError == vr::VRInputError_None)
      {
         S32 retIndex = mSkeletalActions.size();
         mSkeletalActions.push_back(VRSkeletalAction(setIndex, actionHandle, actionName, moveIndex));
         //Con::printf("VRInput Skeletal Action %s, Handle: %I64u, Index: %d", actionName, actionHandle, retIndex);
         return retIndex;
      }
      Con::warnf("OpenVRInput::addSkeletalAction failed for action: %s.", actionName);
   }
   return -1;
}

S32 OpenVRInput::addHapticOutput(const char* outputName)
{
   if (outputName)
   {
      vr::VRActionHandle_t actionHandle;
      vr::EVRInputError vrError = vr::VRInput()->GetActionHandle(outputName, &actionHandle);
      if (vrError == vr::VRInputError_None)
      {
         S32 retIndex = mHapticOutputs.size();
         mHapticOutputs.push_back(actionHandle);
         //Con::printf("VRInput Haptic Output %s, Handle: %I64u, Index: %d", outputName, actionHandle, retIndex);
         return retIndex;
      }
      Con::warnf("OpenVRInput::addHapticOutput failed for action: %s.", outputName);
   }
   return -1;
}

S32 OpenVRInput::getPoseIndex(const char* actionName)
{
   int numPoses = mPoseActions.size();
   for (int i = 0; i < numPoses; ++i)
   {
      if (dStrncmp(actionName, mPoseActions[i].actionName, dStrlen(mPoseActions[i].actionName)) == 0)
         return i;
   }
   return -1;
}

bool OpenVRInput::getCurrentPose(S32 poseIndex, Point3F& position, QuatF& rotation)
{
   if (poseIndex >= 0 && poseIndex < mPoseActions.size())
   {
      position = mPoseActions[poseIndex].lastPosition;
      rotation = mPoseActions[poseIndex].lastRotation;
      return mPoseActions[poseIndex].validPose;
   }
   return false;
}

bool OpenVRInput::setPoseCallbacks(S32 poseIndex, const char* poseCallback, const char* velocityCallback)
{
   if (poseIndex >= 0 && poseIndex < mPoseActions.size())
   {
      mPoseActions[poseIndex].poseCallback = StringTable->insert(poseCallback, false);
      mPoseActions[poseIndex].velocityCallback = StringTable->insert(velocityCallback, false);
      return true;
   }
   return false;
}

S32 OpenVRInput::getSkeletonIndex(const char* actionName)
{
   int numActions = mSkeletalActions.size();
   for (int i = 0; i < numActions; ++i)
   {
      if (dStrncmp(actionName, mSkeletalActions[i].actionName, dStrlen(mSkeletalActions[i].actionName)) == 0)
         return i;
   }
   return -1;
}

bool OpenVRInput::getSkeletonNodes(S32 skeletonIndex, vr::VRBoneTransform_t* boneData)
{
   if (skeletonIndex >= 0 && skeletonIndex < mSkeletalActions.size())
   {
      vr::InputSkeletalActionData_t skeletonData;
      vr::EVRInputError vrError = vr::VRInput()->GetSkeletalActionData(mSkeletalActions[skeletonIndex].actionHandle,
         &skeletonData, sizeof(vr::InputSkeletalActionData_t));
      if ((vrError != vr::VRInputError_None) || !skeletonData.bActive)
      {
         return false;
      }

      vr::EVRSkeletalMotionRange motionRange = mSkeletalActions[skeletonIndex].rangeWithController ?
         vr::VRSkeletalMotionRange_WithController : vr::VRSkeletalMotionRange_WithoutController;

      if (vr::VRInputError_None == vr::VRInput()->GetSkeletalBoneData(mSkeletalActions[skeletonIndex].actionHandle,
         vr::VRSkeletalTransformSpace_Model, motionRange, boneData, /*eBone_Count*/ 31)) // TODO: (VR) Read bone count from API
         return true;
   }
   return false;
}

bool OpenVRInput::setSkeletonMode(S32 skeletonIndex, bool withController)
{
   if (skeletonIndex >= 0 && skeletonIndex < mSkeletalActions.size())
   {
      mSkeletalActions[skeletonIndex].rangeWithController = withController;
      return true;
   }
   return false;
}

bool OpenVRInput::activateActionSet(S32 controllerIndex, U32 setIndex)
{
   if (setIndex < mActionSets.size())
   {
      mNumSetsActive = 1U;
      mActiveSetIndexes[0] = setIndex;
      resetActiveSets();
      return true;
   }

   return false;
}

bool OpenVRInput::pushActionSetLayer(S32 controllerIndex, U32 setIndex)
{
   if (setIndex >= mActionSets.size())
      return false;

   // If it's already on the stack and not at the top pop it first
   for (S32 i = 0; i < mNumSetsActive; ++i)
   {
      if (mActiveSetIndexes[i] == setIndex)
      {
         if (i == (mNumSetsActive - 1))
            return true;  // It's already the top
         popActionSetLayer(controllerIndex, setIndex);
         break;
      }
   }

   if (mNumSetsActive < MaxActiveActionSets)
   {
      mActiveSetIndexes[mNumSetsActive] = setIndex;
      mNumSetsActive++;
      resetActiveSets();
      return true;
   }
   else
      Con::errorf("OpenVRInput::pushActionSetLayer - Too many action set layers are already active.");

   return false;
}

bool OpenVRInput::popActionSetLayer(S32 controllerIndex, U32 setIndex)
{
   if (setIndex >= mActionSets.size())
      return false;

   if (mNumSetsActive < 2)
   {
      Con::errorf("OpenVRInput::popActionSetLayer - You cannot pop the last action set layer.");
      return false;
   }

   bool setRemoved = false;
   for (U32 i = 0; i < mNumSetsActive; ++i)
   {
      if (mActiveSetIndexes[i] == setIndex)
      {
         setRemoved = true;
         mNumSetsActive--;
      }

      if (setRemoved && (i < mNumSetsActive))
         mActiveSetIndexes[i] = mActiveSetIndexes[i + 1];
   }
   resetActiveSets();
   return setRemoved;
}

void OpenVRInput::resetActiveSets()
{
   for (U32 activeIndex = 0; activeIndex < mNumSetsActive; ++activeIndex)
   {
      U32 setIndex = mActiveSetIndexes[activeIndex];
      mActiveSets[activeIndex].ulActionSet = mActionSets[setIndex].setHandle;
      mActiveSets[activeIndex].ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;
      mActiveSets[activeIndex].ulSecondaryActionSet = vr::k_ulInvalidInputValueHandle;
      mActiveSets[activeIndex].nPriority = activeIndex + 1;

      for (int i = 0; i < mAnalogActions.size(); ++i)
      {
         if ((activeIndex == 0) || (mAnalogActions[i].setIndex == setIndex))
            mAnalogActions[i].active = (mAnalogActions[i].setIndex == setIndex);
      }

      for (int i = 0; i < mDigitalActions.size(); ++i)
      {
         if ((activeIndex == 0) || (mDigitalActions[i].setIndex == setIndex))
            mDigitalActions[i].active = (mDigitalActions[i].setIndex == setIndex);
      }

      for (int i = 0; i < mPoseActions.size(); ++i)
      {
         if ((activeIndex == 0) || (mPoseActions[i].setIndex == setIndex))
            mPoseActions[i].active = (mPoseActions[i].setIndex == setIndex);
      }

      for (int i = 0; i < mSkeletalActions.size(); ++i)
      {
         if ((activeIndex == 0) || (mSkeletalActions[i].setIndex == setIndex))
            mSkeletalActions[i].active = (mSkeletalActions[i].setIndex == setIndex);
      }
   }
}

bool OpenVRInput::triggerHapticEvent(U32 actionIndex, float fStartSecondsFromNow, float fDurationSeconds, float fFrequency, float fAmplitude)
{
   if ((actionIndex < mHapticOutputs.size()) && (actionIndex >= 0))
   {
      vr::VRActionHandle_t actionHandle = mHapticOutputs[actionIndex];
      vr::EVRInputError vrError = vr::VRInput()->TriggerHapticVibrationAction(
         actionHandle, fStartSecondsFromNow, fDurationSeconds, fFrequency, fAmplitude, vr::k_ulInvalidInputValueHandle);
      if (vrError == vr::VRInputError_None)
         return true;
   }

   return false;
}

void OpenVRInput::showActionOrigins(U32 setIndex, OpenVRActionType actionType, U32 actionIndex)
{
   if (setIndex < mActionSets.size())
   {
      vr::VRActionHandle_t actionHandle = vr::k_ulInvalidInputValueHandle;
      switch (actionType)
      {
      case OpenVRActionType_Digital:
         if (actionIndex < mDigitalActions.size())
            actionHandle = mDigitalActions[actionIndex].actionHandle;
         break;
      case OpenVRActionType_Analog:
         if (actionIndex < mAnalogActions.size())
            actionHandle = mAnalogActions[actionIndex].actionHandle;
         break;
      case OpenVRActionType_Pose:
         if (actionIndex < mPoseActions.size())
            actionHandle = mPoseActions[actionIndex].actionHandle;
         break;
         //case OpenVRActionType_Skeleton:
         //   if (actionIndex < mSkeletalActions.size())
         //      actionHandle = mSkeletalActions[actionIndex].actionHandle;
         //   break;
      default:
         return;
      }

      if (actionHandle != vr::k_ulInvalidActionHandle)
      {
         if (vr::VRInputError_None != vr::VRInput()->ShowActionOrigins(mActionSets[setIndex].setHandle, actionHandle))
            Con::warnf("OpenVRInput::showActionOrigins - Error displaying action origins.");
      }
   }
}

void OpenVRInput::showActionSetBinds(U32 setIndex)
{
   if (setIndex >= mActionSets.size())
      return;

   vr::VRActiveActionSet_t activeSet;
   activeSet.ulActionSet = mActionSets[setIndex].setHandle;
   activeSet.ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;
   activeSet.ulSecondaryActionSet = vr::k_ulInvalidInputValueHandle;
   activeSet.nPriority = 1;

   if (vr::VRInputError_None != vr::VRInput()->ShowBindingsForActionSet(&activeSet, sizeof(vr::VRActiveActionSet_t), 1, vr::k_ulInvalidInputValueHandle))
      Con::warnf("OpenVRInput::showActionSetBinds - Error displaying action set.");
}
