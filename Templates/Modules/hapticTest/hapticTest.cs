//-----------------------------------------------------------------------------
// Module creation functions.
//-----------------------------------------------------------------------------

function hapticTest::create( %this )
{
}

function hapticTest::destroy( %this )
{
}

function hapticTest::initClient( %this )
{
   %this.queueExec("/scripts/hapticGuiProfiles.cs");
   %this.queueExec("/scripts/gui/effectBox.gui");
   %this.queueExec("/scripts/effectBox.cs");
   %this.queueExec("/scripts/gui/hapticTest.gui");
   %this.queueExec("/scripts/hapticTest.cs");
}

function hapticTest::toggleTestGui( %this )
{
   if (HapticTestDlg.isAwake())
      $GameCanvas.popDialog(HapticTestDlg);
   else
      $GameCanvas.pushDialog(HapticTestDlg);
}