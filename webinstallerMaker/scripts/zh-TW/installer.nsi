# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Required Plugins:
# AppAssocReg    http://nsis.sourceforge.net/Application_Association_Registration_plug-in
# ApplicationID  http://nsis.sourceforge.net/ApplicationID_plug-in
# CityHash       http://mxr.mozilla.org/mozilla-central/source/other-licenses/nsis/Contrib/CityHash
# ShellLink      http://nsis.sourceforge.net/ShellLink_plug-in
# UAC            http://nsis.sourceforge.net/UAC_plug-in
# ServicesHelper Mozilla specific plugin that is located in /other-licenses/nsis

; Set verbosity to 3 (e.g. no script) to lessen the noise in the build logs
!verbose 3

; 7-Zip provides better compression than the lzma from NSIS so we add the files
; uncompressed and use 7-Zip to create a SFX archive of it
SetDatablockOptimize on
SetCompress off
CRCCheck on

; MO changes
RequestExecutionLevel admin

!addplugindir ../../../common/m-c/
!addplugindir ../../../common/other-licenses/nsis/Plugins/
!addplugindir ./

Var TmpVal
Var InstallType
Var AddStartMenuSC
Var AddQuickLaunchSC
Var AddDesktopSC
Var InstallMaintenanceService
Var PageName

; MO changes, variables
Var MOIntroImage
Var MOIntroLabel1
Var MOIntroLabel2
Var MOIntroLabel3
Var MOIntroLabel4

Var MOCancelDownload

Var MOProgressBar
Var MOPromptLabel

; MO changes, for NSIS callbacks
SetPluginUnload alwaysoff

; By defining NO_STARTMENU_DIR an installer that doesn't provide an option for
; an application's Start Menu PROGRAMS directory and doesn't define the
; StartMenuDir variable can use the common InstallOnInitCommon macro.
!define NO_STARTMENU_DIR

; On Vista and above attempt to elevate Standard Users in addition to users that
; are a member of the Administrators group.
!define NONADMIN_ELEVATE

!define AbortSurveyURL "http://www.kampyle.com/feedback_form/ff-feedback-form.php?site_code=8166124&form_id=12116&url="

; Other included files may depend upon these includes!
; The following includes are provided by NSIS.
; MO changes, for MUI2
!include FileFunc.nsh
!include InstallOptions.nsh
!include Library.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include nsDialogs.nsh
!include WinMessages.nsh
!include WinVer.nsh
!include WordFunc.nsh

!insertmacro GetOptions
!insertmacro GetParameters
!insertmacro GetSize
!insertmacro StrFilter
!insertmacro WordFind
!insertmacro WordReplace

; The following includes are custom.
!include branding.nsi
!include defines.nsi
!include common.nsh
!include locales.nsi

VIAddVersionKey "FileDescription" "${BrandShortName} Installer"
VIAddVersionKey "OriginalFilename" "setup.exe"

; Must be inserted before other macros that use logging
!insertmacro _LoggingCommon

; MO changes, comment out macros not used here
!insertmacro AddDisabledDDEHandlerValues
!insertmacro ChangeMUIHeaderImage
!insertmacro CheckForFilesInUse
;!insertmacro CleanUpdatesDir
;!insertmacro CopyFilesFromDir
;!insertmacro CreateRegKey
!insertmacro GetLongPath
!insertmacro GetPathFromString
!insertmacro GetParent
!insertmacro InitHashAppModelId
!insertmacro IsHandlerForInstallDir
!insertmacro IsPinnedToTaskBar
!insertmacro IsUserAdmin
;!insertmacro LogDesktopShortcut
;!insertmacro LogQuickLaunchShortcut
!insertmacro LogStartMenuShortcut
!insertmacro ManualCloseAppPrompt
!insertmacro PinnedToStartMenuLnkCount
!insertmacro RegCleanAppHandler
;!insertmacro RegCleanMain
;!insertmacro RegCleanUninstall
!ifdef MOZ_METRO
!insertmacro RemoveDEHRegistrationIfMatching
!endif
;!insertmacro SetAppLSPCategories
!insertmacro SetBrandNameVars
!insertmacro UpdateShortcutAppModelIDs
!insertmacro UnloadUAC
!insertmacro WriteRegStr2
;!insertmacro WriteRegDWORD2
!insertmacro CheckIfRegistryKeyExists

!include shared.nsh

; Helper macros for ui callbacks. Insert these after shared.nsh
!insertmacro CheckCustomCommon
!insertmacro InstallEndCleanupCommon
!insertmacro InstallOnInitCommon
;!insertmacro InstallStartCleanupCommon
!insertmacro LeaveDirectoryCommon
!insertmacro LeaveOptionsCommon
!insertmacro OnEndCommon
!insertmacro PreDirectoryCommon

Name "${BrandFullName}"
OutFile "setup.exe"
!ifdef HAVE_64BIT_OS
  InstallDir "$PROGRAMFILES64\${BrandFullName}\"
!else
  InstallDir "$PROGRAMFILES32\${BrandFullName}\"
!endif
ShowInstDetails nevershow

################################################################################
# Modern User Interface - MUI

!define MOZ_MUI_CUSTOM_ABORT
!define MUI_CUSTOMFUNCTION_ABORT "CustomAbort"
!define MUI_ICON setup.ico
!define MUI_UNICON setup.ico
!define MUI_WELCOMEPAGE_TITLE_3LINES
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_WELCOMEFINISHPAGE_BITMAP wizWatermark.bmp

; Use a right to left header image when the language is right to left
!ifdef ${AB_CD}_rtl
!define MUI_HEADERIMAGE_BITMAP_RTL wizHeaderRTL.bmp
!else
!define MUI_HEADERIMAGE_BITMAP wizHeader.bmp
!endif

/**
 * Installation Pages
 */
; Welcome Page
!define MUI_PAGE_CUSTOMFUNCTION_PRE preWelcome
!insertmacro MUI_PAGE_WELCOME

; Custom Options Page
Page custom preOptions leaveOptions

; Select Install Directory Page
!define MUI_PAGE_CUSTOMFUNCTION_PRE preDirectory
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE leaveDirectory
!define MUI_DIRECTORYPAGE_VERIFYONLEAVE
!insertmacro MUI_PAGE_DIRECTORY

; Custom Components Page
!ifdef MOZ_MAINTENANCE_SERVICE
Page custom preComponents leaveComponents
!endif

; Custom Shortcuts Page
Page custom preShortcuts leaveShortcuts

; MO changes, Custom Addons Page
Page custom preMOAddons leaveMOAddons

; Custom Summary Page
Page custom preSummary leaveSummary

; MO changes, Custom Downloading Page
Page custom preMODownloading leaveMODownloading

; Install Files Page
; MO changes
;!insertmacro MUI_PAGE_INSTFILES

; Finish Page
; MO changes
!define MUI_FINISHPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchApp
!define MUI_FINISHPAGE_RUN_TEXT $(LAUNCH_TEXT)
!define MUI_PAGE_CUSTOMFUNCTION_PRE preFinish
!define MUI_PAGE_CUSTOMFUNCTION_SHOW showMOFinish
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE leaveMOFinish
!insertmacro MUI_PAGE_FINISH

; Use the default dialog for IDD_VERIFY for a simple Banner
ChangeUI IDD_VERIFY "${NSISDIR}\Contrib\UIs\default.exe"

################################################################################
# Install Sections

Section "-InstallStartCleanup"
  ; MO changes, nothing to do here
SectionEnd

Section "-Application" APP_IDX
  ; MO changes, nothing to do here
SectionEnd

Section "-InstallEndCleanup"
  ; MO changes, nothing to do here
SectionEnd

################################################################################
# Install Abort Survey Functions

Function CustomAbort
  ; MO changes, stop download
  StrCpy $MOCancelDownload 1
  Mydownload::fnCancelDownload
  Call MOTempCleanUp
  ; MO changes, nothing to do here
FunctionEnd

Function AbortSurveyWelcome
  ExecShell "open" "${AbortSurveyURL}step1"
FunctionEnd

Function AbortSurveyOptions
  ExecShell "open" "${AbortSurveyURL}step2"
FunctionEnd

Function AbortSurveyDirectory
  ExecShell "open" "${AbortSurveyURL}step3"
FunctionEnd

Function AbortSurveyShortcuts
  ExecShell "open" "${AbortSurveyURL}step4"
FunctionEnd

Function AbortSurveySummary
  ExecShell "open" "${AbortSurveyURL}step5"
FunctionEnd

################################################################################
# Helper Functions

; MO changes

Function CheckExistingInstall
  ; If there is a pending file copy from a previous upgrade don't allow
  ; installing until after the system has rebooted.
  IfFileExists "$INSTDIR\${FileMainEXE}.moz-upgrade" +1 +4
  MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(WARN_RESTART_REQUIRED_UPGRADE)" IDNO +2
  Reboot
  Quit

  ; If there is a pending file deletion from a previous uninstall don't allow
  ; installing until after the system has rebooted.
  IfFileExists "$INSTDIR\${FileMainEXE}.moz-delete" +1 +4
  MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(WARN_RESTART_REQUIRED_UNINSTALL)" IDNO +2
  Reboot
  Quit

  ${If} ${FileExists} "$INSTDIR\${FileMainEXE}"
    ; Disable the next, cancel, and back buttons
    GetDlgItem $0 $HWNDPARENT 1 ; Next button
    EnableWindow $0 0
    GetDlgItem $0 $HWNDPARENT 2 ; Cancel button
    EnableWindow $0 0
    GetDlgItem $0 $HWNDPARENT 3 ; Back button
    EnableWindow $0 0

    Banner::show /NOUNLOAD "$(BANNER_CHECK_EXISTING)"

    ${If} "$TmpVal" == "FoundMessageWindow"
      Sleep 5000
    ${EndIf}

    ${PushFilesToCheck}

    ; Store the return value in $TmpVal so it is less likely to be accidentally
    ; overwritten elsewhere.
    ${CheckForFilesInUse} $TmpVal

    Banner::destroy

    ; Enable the next, cancel, and back buttons
    GetDlgItem $0 $HWNDPARENT 1 ; Next button
    EnableWindow $0 1
    GetDlgItem $0 $HWNDPARENT 2 ; Cancel button
    EnableWindow $0 1
    GetDlgItem $0 $HWNDPARENT 3 ; Back button
    EnableWindow $0 1

    ${If} "$TmpVal" == "true"
      StrCpy $TmpVal "FoundMessageWindow"
      ${ManualCloseAppPrompt} "${WindowClass}" "$(WARN_MANUALLY_CLOSE_APP_INSTALL)"
      StrCpy $TmpVal "true"
    ${EndIf}
  ${EndIf}
FunctionEnd

Function LaunchApp
  ${ManualCloseAppPrompt} "${WindowClass}" "$(WARN_MANUALLY_CLOSE_APP_LAUNCH)"

  ClearErrors
  ${GetParameters} $0
  ${GetOptions} "$0" "/UAC:" $1
  ${If} ${Errors}
    Exec "$\"$INSTDIR\${FileMainEXE}$\""
  ${Else}
    GetFunctionAddress $0 LaunchAppFromElevatedProcess
    UAC::ExecCodeSegment $0
  ${EndIf}
FunctionEnd

Function LaunchAppFromElevatedProcess
  ; Find the installation directory when launching using GetFunctionAddress
  ; from an elevated installer since $INSTDIR will not be set in this installer
  ${StrFilter} "${FileMainEXE}" "+" "" "" $R9
  ReadRegStr $0 HKLM "Software\Clients\StartMenuInternet\$R9\DefaultIcon" ""
  ${GetPathFromString} "$0" $0
  ${GetParent} "$0" $1
  ; Set our current working directory to the application's install directory
  ; otherwise the 7-Zip temp directory will be in use and won't be deleted.
  SetOutPath "$1"
  Exec "$\"$0$\""
FunctionEnd

; MO changes, clean up files
Function MOTempCleanUp
  Delete $TEMP\Firefox-latest.exe
  Delete $TEMP\Firefox-latest.exe_0
  Delete $TEMP\Firefox-latest.exe_1
  Delete $TEMP\Firefox-latest.exe_2
  Delete $TEMP\Firefox-latest.exe_3
  Delete $TEMP\Firefox-latest.exe_4
  Delete $TEMP\addonlist.ini
  Delete $TEMP\extensions.xml
  Delete $TEMP\res.7z
  RMDir /r '$TEMP\resTemp'
  RMDir /r '$TEMP\MozillaFirefox'
FunctionEnd

################################################################################
# Macros defination, MO changes

!macro MO_NSD_CHECKSTATE NAME
  SendMessage ${NAME} ${BM_GETCHECK} 0 0 $R0
  Push $R0
!macroend
!define MO_NSD_CHECKSTATE "!insertmacro MO_NSD_CHECKSTATE"

################################################################################
# Timer Callback Functions, MO changes

Function MOConfigDownloadTimer.Callback
  ${NSD_KillTimer} MOConfigDownloadTimer.Callback
  Mydownload::fnShowConfigurableUI
  ${If} $R0 == "error"
    inetc::get /SILENT "http://adu.myfirefox.com.tw/neterror/neterror.gif?channel=${MOChannel}&type=error&action=show" "$EXEDIR\3.htm" /end
    MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(MO_ERROR_INSTALL_EXIT)" /SD IDNO IDYES +1 IDNO +4

    inetc::get /SILENT "http://adu.myfirefox.com.tw/neterror/neterror.gif?channel=${MOChannel}&type=error&action=fullok" "$EXEDIR\3.htm" /end
    ExecShell "open" "http://download.myfirefox.com.tw/releases/full/latest/zh-TW/Firefox-full-latest.exe"

    GetDlgItem $0 $HWNDPARENT 1 ; Next button
    EnableWindow $0 1
    SendMessage $0 ${BM_CLICK} 0 0
  ${Endif}
FunctionEnd

Function MOExtDownloadTimer.Callback
  ${NSD_KillTimer} MOExtDownloadTimer.Callback
  Mydownload::fnMyDownloadProgress

  ${If} $R0 != "error"
    ${If} $MOCancelDownload == 0
      GetDlgItem $0 $HWNDPARENT 2 ; Cancel button
      EnableWindow $0 0
      ${NSD_SetText} $MOPromptLabel "$(MO_STATUS_INSTALLING)"
      Mydownload::fnExecuteCmd '"$EXEDIR\7za.exe" x $TEMP\Firefox-latest.exe -o$TEMP\MozillaFirefox\'
      Mydownload::fnMoveExtensionFiles "$TEMP\MozillaFirefox\core\distribution\myextensions\"
      WriteINIStr "$TEMP\MozillaFirefox\core\distribution\distribution.ini" "Preferences" "app.taiwanedition.channel" ${MOChannel}
      WriteINIStr "$TEMP\MozillaFirefox\core\distribution\myextensions\installdate.ini" "FILETIME" dwLowDateTime $R4
      WriteINIStr "$TEMP\MozillaFirefox\core\distribution\myextensions\installdate.ini" "FILETIME" dwHighDateTime $R5
      CopyFiles "$TEMP\CCTV_FF_Plug-in.xpi" "$TEMP\MozillaFirefox\core\distribution\myextensions\"
      Mydownload::fnExecuteCmd '"$EXEDIR\7za.exe" x $TEMP\CCTV_FF_Plug-in.xpi -o$TEMP\MozillaFirefox\core\distribution\extensions\cctvplayer-plugin@www.cctv.com\'

      ClearErrors
      ${DeleteFile} "$INSTDIR\${FileMainEXE}"
      ${If} ${Errors}
        ${ManualCloseAppPrompt} "${WindowClass}" "$(WARN_MANUALLY_CLOSE_APP_INSTALL)"
      ${EndIf}
      Mydownload::fnExecuteCmd '"$TEMP\MozillaFirefox\setup.exe" /INI=$PLUGINSDIR\fxSetup.ini'

      ;${NSD_SetText} $MOPromptLabel "$(MO_STATUS_SUCCEED)"
      ;ShowWindow $MOProgressBar ${SW_HIDE}

      ;Set as default browser
	  ${Unless} ${Silent}
	    ${INSTALLOPTIONS_READ} $0 "summary.ini" "Field 4" "State"
        ${If} "$0" == "1"
          ${GetParameters} $0
          ${GetOptions} "$0" "/UAC:" $0
          ${If} ${Errors}
            Call SetAsDefaultAppUserHKCU
          ${Else}
            GetFunctionAddress $0 SetAsDefaultAppUserHKCU
            UAC::ExecCodeSegment $0
          ${EndIf}
        ${EndIf}
      ${EndUnless}

      ; Refresh desktop icons
      System::Call "shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)"
    ${Endif}
  ${Endif}

  ${NSD_SetText} $MOPromptLabel "$(MO_STATUS_POST_CLEANUP)"
  ${If} $R0 == "error"
    inetc::get /SILENT "http://adu.myfirefox.com.tw/neterror/neterror.gif?channel=${MOChannel}&type=error&action=show" "$EXEDIR\3.htm" /end
    MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(MO_ERROR_INSTALL_EXIT)" /SD IDNO IDYES +1 IDNO +4

    inetc::get /SILENT "http://adu.myfirefox.com.tw/neterror/neterror.gif?channel=${MOChannel}&type=error&action=fullok" "$EXEDIR\3.htm" /end
    ExecShell "open" "http://download.myfirefox.com.tw/releases/full/latest/zh-TW/Firefox-full-latest.exe"

    GetDlgItem $0 $HWNDPARENT 2 ; Cancel Button
    EnableWindow $0 1
    SendMessage $0 ${BM_CLICK} 0 0
  ${Endif}

  GetDlgItem $0 $HWNDPARENT 1 ; Next button
  EnableWindow $0 1
  SendMessage $0 ${BM_CLICK} 0 0

  ${InstallEndCleanupCommon}
FunctionEnd

################################################################################
# Language

!insertmacro MOZ_MUI_LANGUAGE 'baseLocale'
!verbose push
!verbose 3
!include "overrideLocale.nsh"
!include "customLocale.nsh"
!verbose pop

; Set this after the locale files to override it if it is in the locale
; using " " for BrandingText will hide the "Nullsoft Install System..." branding
BrandingText " "

################################################################################
# Page pre, show, and leave functions

Function preWelcome
  StrCpy $PageName "Welcome"
  ${If} ${FileExists} "$EXEDIR\core\distribution\modern-wizard.bmp"
    Delete "$PLUGINSDIR\modern-wizard.bmp"
    CopyFiles /SILENT "$EXEDIR\core\distribution\modern-wizard.bmp" "$PLUGINSDIR\modern-wizard.bmp"
  ${EndIf}
FunctionEnd

Function preOptions
  StrCpy $PageName "Options"
  ${If} ${FileExists} "$EXEDIR\core\distribution\modern-header.bmp"
  ${AndIf} $hHeaderBitmap == ""
    Delete "$PLUGINSDIR\modern-header.bmp"
    CopyFiles /SILENT "$EXEDIR\core\distribution\modern-header.bmp" "$PLUGINSDIR\modern-header.bmp"
    ${ChangeMUIHeaderImage} "$PLUGINSDIR\modern-header.bmp"
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT "$(OPTIONS_PAGE_TITLE)" "$(OPTIONS_PAGE_SUBTITLE)"
  !insertmacro INSTALLOPTIONS_DISPLAY "options.ini"
FunctionEnd

Function leaveOptions
  ${INSTALLOPTIONS_READ} $0 "options.ini" "Settings" "State"
  ${If} $0 != 0
    Abort
  ${EndIf}
  ${INSTALLOPTIONS_READ} $R0 "options.ini" "Field 2" "State"
  StrCmp $R0 "1" +1 +2
  StrCpy $InstallType ${INSTALLTYPE_BASIC}
  ${INSTALLOPTIONS_READ} $R0 "options.ini" "Field 3" "State"
  StrCmp $R0 "1" +1 +2
  StrCpy $InstallType ${INSTALLTYPE_CUSTOM}

  ${LeaveOptionsCommon}

  ${If} $InstallType == ${INSTALLTYPE_BASIC}
    Call CheckExistingInstall
  ${EndIf}
FunctionEnd

Function preDirectory
  StrCpy $PageName "Directory"
  ${PreDirectoryCommon}
FunctionEnd

Function leaveDirectory
  ${If} $InstallType == ${INSTALLTYPE_BASIC}
    Call CheckExistingInstall
  ${EndIf}
  ${LeaveDirectoryCommon} "$(WARN_DISK_SPACE)" "$(WARN_WRITE_ACCESS)"
FunctionEnd

Function preShortcuts
  StrCpy $PageName "Shortcuts"
  ${CheckCustomCommon}
  !insertmacro MUI_HEADER_TEXT "$(SHORTCUTS_PAGE_TITLE)" "$(SHORTCUTS_PAGE_SUBTITLE)"
  !insertmacro INSTALLOPTIONS_DISPLAY "shortcuts.ini"
FunctionEnd

Function leaveShortcuts
  ${INSTALLOPTIONS_READ} $0 "shortcuts.ini" "Settings" "State"
  ${If} $0 != 0
    Abort
  ${EndIf}
  ${INSTALLOPTIONS_READ} $AddDesktopSC "shortcuts.ini" "Field 2" "State"

  ; If we have a Metro browser and are Win8, then we don't have a Field 3
!ifdef MOZ_METRO
  ${Unless} ${AtLeastWin8}
!endif
    ${INSTALLOPTIONS_READ} $AddStartMenuSC "shortcuts.ini" "Field 3" "State"
!ifdef MOZ_METRO
  ${EndIf}
!endif

  ; Don't install the quick launch shortcut on Windows 7
  ${Unless} ${AtLeastWin7}
    ${INSTALLOPTIONS_READ} $AddQuickLaunchSC "shortcuts.ini" "Field 4" "State"
  ${EndUnless}

  ${If} $InstallType == ${INSTALLTYPE_CUSTOM}
    Call CheckExistingInstall
  ${EndIf}
FunctionEnd

; MO changes
Function preMOAddons
  ${If} $InstallType == ${INSTALLTYPE_BASIC}
    Abort
  ${EndIf}

  !insertmacro MUI_HEADER_TEXT "$(MO_CUSTOM_ADDON_PAGE_TITLE)" "$(MO_CUSTOM_ADDON_PAGE_SUBTITLE)"

  nsDialogs::Create 1018
  Pop $0

  ${NSD_CreateBitmap} 60% 12u 180u 220u ""
  Pop $MOIntroImage
  ShowWindow $MOIntroImage ${SW_HIDE}

  ${NSD_CreateLabel} 60% 150 220u 50% ""
  Pop $MOIntroLabel1
  ShowWindow $MOIntroLabel1 ${SW_HIDE}
  ${NSD_CreateLabel} 60% 168 220u 50% ""
  Pop $MOIntroLabel2
  ShowWindow $MOIntroLabel2 ${SW_HIDE}
  ${NSD_CreateLabel} 60% 186 220u 50% ""
  Pop $MOIntroLabel3
  ShowWindow $MOIntroLabel3 ${SW_HIDE}
  ${NSD_CreateLabel} 60% 204 220u 50% ""
  Pop $MOIntroLabel4
  ShowWindow $MOIntroLabel4 ${SW_HIDE}

  GetDlgItem $0 $HWNDPARENT 1 ; Next button
  EnableWindow $0 0
  ;GetDlgItem $0 $HWNDPARENT 3 ; Back button
  ;EnableWindow $0 0

  ${NSD_CreateProgressBar} 0 30 100% 12u "Configuration Download Progress"
  Pop $1
  ShowWindow $1 ${SW_HIDE}
  ${NSD_CreateLabel} 0 10 100% 12u "$(MO_STATUS_CONFIG_DOWNLOADING)"
  Pop $2
  ShowWindow $2 ${SW_HIDE}

  ${NSD_CreateTimer} MOConfigDownloadTimer.Callback 10
  nsDialogs::Show
FunctionEnd

Function leaveMOAddons
  ; MO changes, nothing to do here
FunctionEnd

!ifdef MOZ_MAINTENANCE_SERVICE
Function preComponents
  ; If the service already exists, don't show this page
  ServicesHelper::IsInstalled "MozillaMaintenance"
  Pop $R9
  ${If} $R9 == 1
    ; The service already exists so don't show this page.
    Abort
  ${EndIf}

  ; On Windows 2000 we do not install the maintenance service.
  ${Unless} ${AtLeastWinXP}
    Abort
  ${EndUnless}

  ; Don't show the custom components page if the
  ; user is not an admin
  Call IsUserAdmin
  Pop $R9
  ${If} $R9 != "true"
    Abort
  ${EndIf}

  ; Only show the maintenance service page if we have write access to HKLM
  ClearErrors
  WriteRegStr HKLM "Software\Mozilla" \
              "${BrandShortName}InstallerTest" "Write Test"
  ${If} ${Errors}
    ClearErrors
    Abort
  ${Else}
    DeleteRegValue HKLM "Software\Mozilla" "${BrandShortName}InstallerTest"
  ${EndIf}

  StrCpy $PageName "Components"
  ${CheckCustomCommon}
  !insertmacro MUI_HEADER_TEXT "$(COMPONENTS_PAGE_TITLE)" "$(COMPONENTS_PAGE_SUBTITLE)"
  !insertmacro INSTALLOPTIONS_DISPLAY "components.ini"
FunctionEnd

Function leaveComponents
  ${INSTALLOPTIONS_READ} $0 "components.ini" "Settings" "State"
  ${If} $0 != 0
    Abort
  ${EndIf}
  ${INSTALLOPTIONS_READ} $InstallMaintenanceService "components.ini" "Field 2" "State"
  ${If} $InstallType == ${INSTALLTYPE_CUSTOM}
    Call CheckExistingInstall
  ${EndIf}
FunctionEnd
!endif

Function preSummary
  ; MO changes, write install option to the customized ini file
  ${If} $R0 == "error"
    Abort
  ${Endif}
  WriteINIStr "$PLUGINSDIR\fxSetup.ini" "Install" InstallDirectoryPath "$INSTDIR"
  ${If} $AddQuickLaunchSC == 0
    WriteINIStr "$PLUGINSDIR\fxSetup.ini" "Install" QuickLaunchShortcut "false"
  ${Else}
    WriteINIStr "$PLUGINSDIR\fxSetup.ini" "Install" QuickLaunchShortcut "true"
  ${EndIf}
  ${If} $AddDesktopSC == 0
    WriteINIStr "$PLUGINSDIR\fxSetup.ini" "Install" DesktopShortcut "false"
  ${Else}
    WriteINIStr "$PLUGINSDIR\fxSetup.ini" "Install" DesktopShortcut "true"
  ${EndIf}
  ${If} $AddStartMenuSC == 0
    WriteINIStr "$PLUGINSDIR\fxSetup.ini" "Install" StartMenuShortcuts "false"
  ${Else}
    WriteINIStr "$PLUGINSDIR\fxSetup.ini" "Install" StartMenuShortcuts "true"
  ${EndIf}
  ${If} $InstallMaintenanceService == 0
    WriteINIStr "$PLUGINSDIR\fxSetup.ini" "Install" MaintenanceService "false"
  ${EndIf}

  StrCpy $PageName "Summary"

  ; MO changes, for installdate.ini ?
  GetFileTime "$PLUGINSDIR\options.ini" $R4 $R5

  ; Setup the summary.ini file for the Custom Summary Page
  WriteINIStr "$PLUGINSDIR\summary.ini" "Settings" NumFields "3"

  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 1" Type   "label"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 1" Text   "$(SUMMARY_INSTALLED_TO)"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 1" Left   "0"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 1" Right  "-1"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 1" Top    "5"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 1" Bottom "15"

  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 2" Type   "text"
  ; The contents of this control must be set as follows in the pre function
  ; ${INSTALLOPTIONS_READ} $1 "summary.ini" "Field 2" "HWND"
  ; SendMessage $1 ${WM_SETTEXT} 0 "STR:$INSTDIR"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 2" state  ""
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 2" Left   "0"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 2" Right  "-1"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 2" Top    "17"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 2" Bottom "30"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 2" flags  "READONLY"

  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 3" Type   "label"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 3" Left   "0"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 3" Right  "-1"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 3" Top    "130"
  WriteINIStr "$PLUGINSDIR\summary.ini" "Field 3" Bottom "150"

  ${If} ${FileExists} "$INSTDIR\${FileMainEXE}"
    WriteINIStr "$PLUGINSDIR\summary.ini" "Field 3" Text "$(SUMMARY_UPGRADE_CLICK)"
    WriteINIStr "$PLUGINSDIR\summary.ini" "Settings" NextButtonText "$(UPGRADE_BUTTON)"
  ${Else}
    WriteINIStr "$PLUGINSDIR\summary.ini" "Field 3" Text "$(SUMMARY_INSTALL_CLICK)"
    DeleteINIStr "$PLUGINSDIR\summary.ini" "Settings" NextButtonText
  ${EndIf}


  ; Remove the "Field 4" ini section in case the user hits back and changes the
  ; installation directory which could change whether the make default checkbox
  ; should be displayed.
  DeleteINISec "$PLUGINSDIR\summary.ini" "Field 4"

  ; Check if it is possible to write to HKLM
  ClearErrors
  WriteRegStr HKLM "Software\Mozilla" "${BrandShortName}InstallerTest" "Write Test"
  ${Unless} ${Errors}
    DeleteRegValue HKLM "Software\Mozilla" "${BrandShortName}InstallerTest"
    ; Check if Firefox is the http handler for this user.
    SetShellVarContext current ; Set SHCTX to the current user
    ${IsHandlerForInstallDir} "http" $R9
    ${If} $TmpVal == "HKLM"
      SetShellVarContext all ; Set SHCTX to all users
    ${EndIf}
    ; If Firefox isn't the http handler for this user show the option to set
    ; Firefox as the default browser.
    ${If} "$R9" != "true"
    ${AndIf} ${AtMostWin2008R2}
      WriteINIStr "$PLUGINSDIR\summary.ini" "Settings" NumFields "4"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field 4" Type   "checkbox"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field 4" Text   "$(SUMMARY_TAKE_DEFAULTS)"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field 4" Left   "0"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field 4" Right  "-1"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field 4" State  "1"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field 4" Top    "32"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field 4" Bottom "53"
    ${EndIf}
  ${EndUnless}

  ${If} "$TmpVal" == "true"
    ; If there is already a Type entry in the "Field 4" section with a value of
    ; checkbox then the set as the default browser checkbox is displayed and
    ; this text must be moved below it.
    ReadINIStr $0 "$PLUGINSDIR\summary.ini" "Field 4" "Type"
    ${If} "$0" == "checkbox"
      StrCpy $0 "5"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field $0" Top    "53"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field $0" Bottom "68"
    ${Else}
      StrCpy $0 "4"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field $0" Top    "35"
      WriteINIStr "$PLUGINSDIR\summary.ini" "Field $0" Bottom "50"
    ${EndIf}
    WriteINIStr "$PLUGINSDIR\summary.ini" "Settings" NumFields "$0"

    WriteINIStr "$PLUGINSDIR\summary.ini" "Field $0" Type   "label"
    WriteINIStr "$PLUGINSDIR\summary.ini" "Field $0" Text   "$(SUMMARY_REBOOT_REQUIRED_INSTALL)"
    WriteINIStr "$PLUGINSDIR\summary.ini" "Field $0" Left   "0"
    WriteINIStr "$PLUGINSDIR\summary.ini" "Field $0" Right  "-1"
  ${EndIf}

  !insertmacro MUI_HEADER_TEXT "$(SUMMARY_PAGE_TITLE)" "$(SUMMARY_PAGE_SUBTITLE)"

  ; The Summary custom page has a textbox that will automatically receive
  ; focus. This sets the focus to the Install button instead.
  !insertmacro INSTALLOPTIONS_INITDIALOG "summary.ini"
  GetDlgItem $0 $HWNDPARENT 1
  System::Call "user32::SetFocus(i r0, i 0x0007, i,i)i"
  ${INSTALLOPTIONS_READ} $1 "summary.ini" "Field 2" "HWND"
  SendMessage $1 ${WM_SETTEXT} 0 "STR:$INSTDIR"
  !insertmacro INSTALLOPTIONS_SHOW
FunctionEnd

Function leaveSummary
  ; MO changes, nothing to do here
FunctionEnd

Function preMODownloading
  ${If} $R0 == "error"
    Abort
  ${Endif}
  !insertmacro MUI_HEADER_TEXT "$(MO_SETUP_PAGE_TITLE)" "$(MO_SETUP_PAGE_SUBTITLE)"
  nsDialogs::Create 1018
  Pop $0
  ${NSD_CreateLabel} 0 10 100% 12u ""
  Pop $MOPromptLabel

  ${NSD_CreateProgressBar} 0 30 100% 12u "Extensions Download Progress"
  Pop $MOProgressBar

  GetDlgItem $0 $HWNDPARENT 3 ; Back button
  ShowWindow $0 ${SW_HIDE}
  GetDlgItem $0 $HWNDPARENT 1 ; Next button
  EnableWindow $0 0

  StrCpy $MOCancelDownload 0
  ${NSD_CreateTimer} MOExtDownloadTimer.Callback 10
  nsDialogs::Show
FunctionEnd

Function leaveMODownloading
  ; MO changes, nothing to do here
FunctionEnd

; When we add an optional action to the finish page the cancel button is
; enabled. This disables it and leaves the finish button as the only choice.
Function preFinish
  ; MO changes,
  ${If} $R0 == "error"
    Abort
  ${EndIf}
  GetDlgItem $0 $HWNDPARENT 1046 ; What is 1046 ?
  ShowWindow $0 ${SW_HIDE}
  GetDlgItem $0 $HWNDPARENT 3 ; Back button
  ShowWindow $0 ${SW_HIDE}
  StrCpy $PageName ""
  ${EndInstallLog} "${BrandFullName}"
  ; MO changes, ioSpecial.ini not really used by MUI2
  GetDlgItem $0 $HWNDPARENT 2 ; Cancel button
  EnableWindow $0 0
FunctionEnd

Function showMOFinish
FunctionEnd

Function leaveMOFinish
  ReadINIStr $1 "$TEMP\addonlist.ini" "Addons" "List"
  ${If} $InstallType == ${INSTALLTYPE_BASIC}
    inetc::get /SILENT "http://adu.myfirefox.com.tw/install/install.gif?s=def&l=zh-TW&addons=$1&version=${MOInstallerVersion}" "$EXEDIR\3.htm" /end
  ${Else}
    inetc::get /SILENT "http://adu.myfirefox.com.tw/install/install.gif?s=diy&l=zh-TW&addons=$1&version=${MOInstallerVersion}" "$EXEDIR\3.htm" /end
  ${EndIf}
  call MOTempCleanUp
FunctionEnd

################################################################################
# Initialization Functions

Function .onInit
  ; MO changes, detect conflicting setup
  System::Call 'kernel32::CreateMutexW(i 0, i 0, t "myMutex") ?e'
  Pop $R0
  StrCmp $R0 0 +3
  MessageBox MB_OK '$(MO_ERROR_INSTALL_CONFLICT)'
  Abort
  Call MOTempCleanUp

  ; Remove the current exe directory from the search order.
  ; This only effects LoadLibrary calls and not implicitly loaded DLLs.
  System::Call 'kernel32::SetDllDirectoryW(w "")'

  StrCpy $PageName ""
  StrCpy $LANGUAGE 0
  ${SetBrandNameVars} "$EXEDIR\core\distribution\setup.ini"

  ${InstallOnInitCommon} "$(WARN_MIN_SUPPORTED_OS_MSG)"

  !insertmacro InitInstallOptionsFile "options.ini"
  !insertmacro InitInstallOptionsFile "shortcuts.ini"
  !insertmacro InitInstallOptionsFile "components.ini"
  !insertmacro InitInstallOptionsFile "summary.ini"

  WriteINIStr "$PLUGINSDIR\options.ini" "Settings" NumFields "5"

  WriteINIStr "$PLUGINSDIR\options.ini" "Field 1" Type   "label"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 1" Text   "$(OPTIONS_SUMMARY)"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 1" Left   "0"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 1" Right  "-1"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 1" Top    "0"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 1" Bottom "10"

  WriteINIStr "$PLUGINSDIR\options.ini" "Field 2" Type   "RadioButton"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 2" Text   "$(OPTION_STANDARD_RADIO)"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 2" Left   "0"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 2" Right  "-1"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 2" Top    "25"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 2" Bottom "35"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 2" State  "1"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 2" Flags  "GROUP"

  WriteINIStr "$PLUGINSDIR\options.ini" "Field 3" Type   "RadioButton"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 3" Text   "$(OPTION_CUSTOM_RADIO)"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 3" Left   "0"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 3" Right  "-1"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 3" Top    "55"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 3" Bottom "65"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 3" State  "0"

  WriteINIStr "$PLUGINSDIR\options.ini" "Field 4" Type   "label"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 4" Text   "$(OPTION_STANDARD_DESC)"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 4" Left   "15"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 4" Right  "-1"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 4" Top    "37"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 4" Bottom "57"

  WriteINIStr "$PLUGINSDIR\options.ini" "Field 5" Type   "label"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 5" Text   "$(OPTION_CUSTOM_DESC)"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 5" Left   "15"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 5" Right  "-1"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 5" Top    "67"
  WriteINIStr "$PLUGINSDIR\options.ini" "Field 5" Bottom "87"

  ; Setup the shortcuts.ini file for the Custom Shortcuts Page
  ; Don't offer to install the quick launch shortcut on Windows 7
  ${If} ${AtLeastWin7}
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Settings" NumFields "3"
  ${Else}
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Settings" NumFields "4"
  ${EndIf}

  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 1" Type   "label"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 1" Text   "$(CREATE_ICONS_DESC)"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 1" Left   "0"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 1" Right  "-1"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 1" Top    "5"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 1" Bottom "15"

  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 2" Type   "checkbox"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 2" Text   "$(ICONS_DESKTOP)"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 2" Left   "0"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 2" Right  "-1"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 2" Top    "20"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 2" Bottom "30"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 2" State  "1"
  WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 2" Flags  "GROUP"

  ; Don't offer to install the start menu shortcut on Windows 8
  ; for Metro builds.
!ifdef MOZ_METRO
  ${Unless} ${AtLeastWin8}
!endif
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 3" Type   "checkbox"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 3" Text   "$(ICONS_STARTMENU)"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 3" Left   "0"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 3" Right  "-1"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 3" Top    "40"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 3" Bottom "50"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 3" State  "1"
!ifdef MOZ_METRO
  ${EndIf}
!endif

  ; Don't offer to install the quick launch shortcut on Windows 7
  ${Unless} ${AtLeastWin7}
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 4" Type   "checkbox"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 4" Text   "$(ICONS_QUICKLAUNCH)"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 4" Left   "0"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 4" Right  "-1"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 4" Top    "60"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 4" Bottom "70"
    WriteINIStr "$PLUGINSDIR\shortcuts.ini" "Field 4" State  "1"
  ${EndUnless}

  ; Setup the components.ini file for the Components Page
  WriteINIStr "$PLUGINSDIR\components.ini" "Settings" NumFields "2"

  WriteINIStr "$PLUGINSDIR\components.ini" "Field 1" Type   "label"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 1" Text   "$(OPTIONAL_COMPONENTS_DESC)"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 1" Left   "0"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 1" Right  "-1"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 1" Top    "5"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 1" Bottom "25"

  WriteINIStr "$PLUGINSDIR\components.ini" "Field 2" Type   "checkbox"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 2" Text   "$(MAINTENANCE_SERVICE_CHECKBOX_DESC)"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 2" Left   "0"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 2" Right  "-1"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 2" Top    "27"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 2" Bottom "37"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 2" State  "1"
  WriteINIStr "$PLUGINSDIR\components.ini" "Field 2" Flags  "GROUP"

  ; MO changes, why ?
  ; There must always be a core directory.
  ;${GetSize} "$EXEDIR\core\" "/S=0K" $R5 $R7 $R8
  ;SectionSetSize ${APP_IDX} $R5
  SectionSetSize ${APP_IDX} 40000

  ; Initialize $hHeaderBitmap to prevent redundant changing of the bitmap if
  ; the user clicks the back button
  StrCpy $hHeaderBitmap ""

  ; MO changes, third-party plugins detection ?
  Mydownload::fnInitial
  Mydownload::fnDownloadResFiles ${MOResUrl}
  Mydownload::fnDownload /THREADNUMBER=5 ${MOBaseInstallerUrl} Firefox-latest.exe
FunctionEnd

Function .onGUIEnd
  ${OnEndCommon}
FunctionEnd
