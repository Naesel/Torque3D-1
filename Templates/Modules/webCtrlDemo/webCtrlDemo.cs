//-----------------------------------------------------------------------------
// Module creation functions.
//-----------------------------------------------------------------------------

function webCtrlDemo::create( %this )
{
   %this.startupCEF();
}

function webCtrlDemo::destroy( %this )
{
   
}

function webCtrlDemo::toggleDemo( %this )
{
   if (WebDemoDlg.isAwake())
      $GameCanvas.popDialog(WebDemoDlg);
   else
      $GameCanvas.pushDialog(WebDemoDlg);
}

function webCtrlDemo::toggleBrowser( %this )
{
   if (WebBrowserGui.isAwake())
      $GameCanvas.popDialog(WebBrowserGui);
   else
   {
      WebBrowserGui-->DemoCEFBrowser.loadURL("torque3d.org");
      $GameCanvas.pushDialog(WebBrowserGui);
   }
}

function webCtrlDemo::toggleURLTester( %this )
{
   if (WebURLGui.isAwake())
      $GameCanvas.popDialog(WebURLGui);
   else
      $GameCanvas.pushDialog(WebURLGui);
}

function webCtrlDemo::startupCEF( %this )
{
   // The root directory that all CefSettings.cache_path ($Cef::cachePath) and
   // CefRequestContextSettings.cache_path values must have in common. If this
   // value is empty and CefSettings.cache_path is non-empty then it will
   // default to the CefSettings.cache_path value. If this value is non-empty
   // then it must be an absolute path. Failure to set this value correctly may
   // result in the sandbox blocking read/write access to the cache_path
   // directory. Cef will need write permission to this directory.
   $Cef::rootCachePath = getUserPath() @ "/cef/webcache";
   //$Cef::rootCachePath = "";

   // The location where data for the global browser cache will be stored on
   // disk. If this value is non-empty then it must be an absolute path that is
   // either equal to or a child directory of $Cef::rootCachePath. If
   // this value is empty then browsers will be created in "incognito mode" where
   // in-memory caches are used for storage and no data is persisted to disk.
   // HTML5 databases such as localStorage will only persist across sessions if a
   // cache path is specified. Can be overridden for individual CefRequestContext
   // instances via the CefRequestContextSettings.cache_path value.
   // Cef will need write permission to this directory.
   $Cef::cachePath = getUserPath() @ "/cef/webcache";
   //$Cef::cachePath = getMainDotCsDir() @ "/cef/webcache";

   // The location where user data such as spell checking dictionary files will
   // be stored on disk. If this value is empty then the default
   // platform-specific user data directory will be used ("~/.cef_user_data"
   // directory on Linux, "~/Library/Application Support/CEF/User Data" directory
   // on Mac OS X, "Local Settings\Application Data\CEF\User Data" directory
   // under the user profile directory on Windows). If this value is non-empty
   // then it must be an absolute path. Cef will need write permission to this
   // directory.
   $Cef::userDataPath = getUserPath() @ "/cef/data";
   //$Cef::userDataPath = getMainDotCsDir() @ "/cef/data";

   // The locale string that will be passed to Blink. If empty the default locale
   // of "en - US" will be used. This value is ignored on Linux where locale is
   // determined using environment variable parsing with the precedence order:
   // LANGUAGE, LC_ALL, LC_MESSAGES and LANG. Locale files are located in 
   // game/cef/locales
   $Cef::localeString = "";

   // The directory and file name to use for the debug log. If empty, the default
   // name of "debug.log" will be used and the file will be written to the
   // application directory.  Cef will need write permission to this file.
   $Cef::logPath = getUserPath() @ "/cef/cef.log";
   //$Cef::logPath = "cef/cef.log";

   // The log severity. Only messages of this severity level or higher will be
   // logged. Options are: Default, Verbose, Info, Warning, Error and None
   $Cef::logSeverity = "Warning";

   // The fully qualified path for the resources directory. If this value is empty
   // the cef.pak and/or devtools_resources.pak files must be located in the module
   // directory on Windows/Linux or the app bundle Resources directory on Mac OS X.
   $Cef::resourcePath = getMainDotCsDir() @ "/cef";

   // The fully qualified path for the locales directory. If this value is empty the
   // locales directory must be located in the module directory. This value is ignored
   // on Mac OS X where pack files are always loaded from the app bundle Resources directory.
   $Cef::localesPath = getMainDotCsDir() @ "/cef/locales";

   // Value that will be returned as the User-Agent HTTP header. If empty the
   // default User-Agent string will be used.
   // https://www.whatismybrowser.com/detect/what-is-my-user-agent
   // default(~): "Mozilla/5.0 (Windows NT 6.2; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/87.0.4280.66 Safari/537.36"
   $Cef::userAgent = "Torque3D/4.0";

   // With CEF in windowless rendering mode (needed for T3D), use software
   // rendering and compositing (disable GPU) for increased FPS and decreased
   // CPU usage. 
   // See https://bitbucket.org/chromiumembedded/cef/issues/1257 for details.
   // This will also disable WebGL. If you need WebGL, set false and inclued
   // libEGL.dll and libGLESv2.dll in your game install directory.
   $CefSettings::disableGPU = true;

   // Now that the values are set, start the cef process.
   WebEngine::initializeCEF();

   %this.loadControllerMap();
}

function webCtrlDemo::loadControllerMap( %this )
{  // Map navigation keys to gamepad events for controller navigation
   WebEngine::mapDeviceEvent("gamepad", "btn_r", "tab");
   WebEngine::mapDeviceEvent("gamepad", "btn_l", "shift tab");
   WebEngine::mapDeviceEvent("gamepad", "btn_b", "enter");
   WebEngine::mapDeviceEvent("gamepad", "btn_a", "space");
   WebEngine::mapDeviceEvent("gamepad", "upov", "up");
   WebEngine::mapDeviceEvent("gamepad", "dpov", "down");
   WebEngine::mapDeviceEvent("gamepad", "lpov", "left");
   WebEngine::mapDeviceEvent("gamepad", "rpov", "right");
   WebEngine::mapDeviceEvent("gamepad", "btn_back", "GoBack");
   WebEngine::mapDeviceEvent("gamepad", "btn_start", "GoForward");
}

function webCtrlDemo::initClient( %this )
{
   %this.queueExec("./scripts/initWebDemo.cs");
}