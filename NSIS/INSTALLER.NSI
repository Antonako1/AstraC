; ============================================================
; AstraC Installer Script
; Copyright (c) 2026 Antonako1
; MIT License
; ============================================================

; --- Includes ---
!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "StrFunc.nsh"
!include "LogicLib.nsh"
!include "Sections.nsh"
!include "WinMessages.nsh"
; --- StrFunc Declarations ---
${StrStr}
${UnStrStr}
${StrRep}
${UnStrRep}

; --- Compiler Flags ---
SetCompressor /SOLID lzma
SetCompressorDictSize 32

; --- General ---
!define PRODUCT_NAME "AstraC"
!include ..\VERSION\VERSION.nsh
!define PRODUCT_PUBLISHER "Antonako1"
!define PRODUCT_FULL_NAME "AstraC Compiler, Assembler and Disassembler"
!define PRODUCT_WEB_SITE "https://github.com/Antonako1/AstraC"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_DIR_REGKEY "Software\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}"

; Architecture target (x86-unicode is compatible with all Windows versions)
Target x86-unicode

Name "${PRODUCT_FULL_NAME} v${PRODUCT_VERSION}"
OutFile "..\build\${PRODUCT_NAME}-Setup-${PRODUCT_VERSION}.exe"
ShowInstDetails show
ShowUninstDetails show
BrandingText "${PRODUCT_NAME} v${PRODUCT_VERSION}"

InstallDir "$PROGRAMFILES\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" "InstallDir"

RequestExecutionLevel highest

; --- Interface Settings ---
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\win.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\win.bmp"
!define MUI_ICON "AC.ico"
!define MUI_UNICON "AC.ico"

; --- Variables ---
Var InstallModeAllUsers
Var AddToPath
Var AssocAC
Var AssocAH
Var AssocAS
Var StartMenuFolder
Var hCtl_InstallMode_AllUsers
Var hCtl_InstallMode_CurrentUser
Var hCtl_AddToPath
Var hCtl_AssocAC
Var hCtl_AssocAH
Var hCtl_AssocAS
Var Dialog
Var ShellContextSet

; --- Pages ---
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
Page custom nsInstallModePage nsInstallModeLeave
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
Page custom nsOptionsPage nsOptionsLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; --- Language ---
!insertmacro MUI_LANGUAGE "English"

; ============================================================
; .onInit — Check for existing installation + admin detection
; ============================================================
Function .onInit
  StrCpy $InstallModeAllUsers "1"
  StrCpy $AddToPath "1"
  StrCpy $AssocAC "1"
  StrCpy $AssocAH "1"
  StrCpy $AssocAS "1"

  StrCpy $INSTDIR "$PROGRAMFILES64\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}"

  ; Detect if running as admin
  ClearErrors
  UserInfo::GetAccountType
  Pop $0
  ${If} $0 != "Admin"
    StrCpy $InstallModeAllUsers ""
  ${EndIf}

  ; Check HKLM for existing all-users install
  ReadRegStr $0 HKLM "${PRODUCT_UNINST_KEY}" "UninstallString"
  ${If} $0 != ""
    ${GetParent} "$0" $1
    ${If} ${FileExists} "$0"
      MessageBox MB_YESNO|MB_ICONQUESTION \
        "AstraC is already installed for all users at:$\n$1$\n$\nDo you want to uninstall it before proceeding?$\n(Selecting No will abort setup.)" \
        /SD IDYES IDYES doUninstallHKLM IDNO abortSetup
      doUninstallHKLM:
        DetailPrint "Removing previous installation..."
        ExecWait '"$0" /S _?=$1' $2
        ${If} $2 != 0
          DetailPrint "Silent uninstall returned $2, trying normal uninstall..."
          ExecWait '"$0" _?=$1' $2
        ${EndIf}
        Goto doneExistingCheck
      abortSetup:
        Abort
    ${EndIf}
    doneExistingCheck:
  ${EndIf}

  ; Check HKCU for existing per-user install
  ReadRegStr $0 HKCU "${PRODUCT_UNINST_KEY}" "UninstallString"
  ${If} $0 != ""
    ${GetParent} "$0" $1
    ${If} ${FileExists} "$0"
      MessageBox MB_YESNO|MB_ICONQUESTION \
        "AstraC is already installed for this user at:$\n$1$\n$\nDo you want to uninstall it before proceeding?$\n(Selecting No will abort setup.)" \
        /SD IDYES IDYES doUninstallHKCU IDNO abortSetup2
      doUninstallHKCU:
        DetailPrint "Removing previous user installation..."
        ExecWait '"$0" /S _?=$1' $2
        ${If} $2 != 0
          DetailPrint "Silent uninstall returned $2, trying normal uninstall..."
          ExecWait '"$0" _?=$1' $2
        ${EndIf}
        Goto doneExistingCheck2
      abortSetup2:
        Abort
    ${EndIf}
    doneExistingCheck2:
  ${EndIf}
FunctionEnd

; ============================================================
; un.onInit
; ============================================================
Function un.onInit
  MessageBox MB_YESNO|MB_ICONQUESTION \
    "Are you sure you want to completely remove ${PRODUCT_FULL_NAME}?$\n$\nThis will delete all program files, remove PATH entries, and remove file associations." \
    /SD IDYES IDYES +2
  Abort
FunctionEnd

; ============================================================
; Custom Page: Install Mode (all users vs current user)
; ============================================================
Function nsInstallModePage
  nsDialogs::Create 1044
  Pop $Dialog
  ${If} $Dialog == error
    Abort
  ${EndIf}

  ${NSD_CreateGroupBox} 0 0 100% 80u "Installation Scope"
  Pop $0

  ${NSD_CreateRadioButton} 15u 20u 90% 12u "Install for &all users (requires administrator privileges)"
  Pop $hCtl_InstallMode_AllUsers
  ${NSD_AddStyle} $hCtl_InstallMode_AllUsers ${WS_GROUP}

  ${NSD_CreateRadioButton} 15u 40u 90% 12u "Install for &current user only (no administrator privileges required)"
  Pop $hCtl_InstallMode_CurrentUser

  ${If} $InstallModeAllUsers == "1"
    ${NSD_Check} $hCtl_InstallMode_AllUsers
  ${Else}
    ${NSD_Check} $hCtl_InstallMode_CurrentUser
  ${EndIf}

  ${If} $InstallModeAllUsers != "1"
    EnableWindow $hCtl_InstallMode_AllUsers 0
    ${NSD_CreateLabel} 15u 62u 90% 24u \
      "Note: Administrator privileges are not available.$\nInstalling for the current user only."
    Pop $0
  ${EndIf}

  nsDialogs::Show
FunctionEnd

Function nsInstallModeLeave
  ${NSD_GetState} $hCtl_InstallMode_AllUsers $InstallModeAllUsers
  ${If} $InstallModeAllUsers == ${BST_CHECKED}
    StrCpy $InstallModeAllUsers "1"
    SetShellVarContext all
    StrCpy $ShellContextSet "1"
    StrCpy $INSTDIR "$PROGRAMFILES64\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}"
  ${Else}
    StrCpy $InstallModeAllUsers ""
    SetShellVarContext current
    StrCpy $ShellContextSet ""
    StrCpy $INSTDIR "$LOCALAPPDATA\Programs\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}"
  ${EndIf}
FunctionEnd

; ============================================================
; Custom Page: Install Options (PATH, file associations)
; ============================================================
Function nsOptionsPage
  nsDialogs::Create 1044
  Pop $Dialog
  ${If} $Dialog == error
    Abort
  ${EndIf}

  ${NSD_CreateGroupBox} 0 0 100% 45u "Environment"
  Pop $0

  ${NSD_CreateCheckbox} 15u 20u 95% 12u "&Add AstraC to the PATH environment variable"
  Pop $hCtl_AddToPath
  ${If} $AddToPath == "1"
    ${NSD_Check} $hCtl_AddToPath
  ${EndIf}

  ${NSD_CreateGroupBox} 0 50u 100% 75u "File Associations"
  Pop $0

  ${NSD_CreateCheckbox} 15u 70u 95% 12u "Associate &.AC files (AstraC source) with AstraC"
  Pop $hCtl_AssocAC
  ${If} $AssocAC == "1"
    ${NSD_Check} $hCtl_AssocAC
  ${EndIf}

  ${NSD_CreateCheckbox} 15u 88u 95% 12u "Associate &.AH files (AstraC header) with AstraC"
  Pop $hCtl_AssocAH
  ${If} $AssocAH == "1"
    ${NSD_Check} $hCtl_AssocAH
  ${EndIf}

  ${NSD_CreateCheckbox} 15u 106u 95% 12u "Associate &.AS files (Assembly source) with AstraC"
  Pop $hCtl_AssocAS
  ${If} $AssocAS == "1"
    ${NSD_Check} $hCtl_AssocAS
  ${EndIf}

  nsDialogs::Show
FunctionEnd

Function nsOptionsLeave
  ${NSD_GetState} $hCtl_AddToPath $AddToPath
  ${NSD_GetState} $hCtl_AssocAC  $AssocAC
  ${NSD_GetState} $hCtl_AssocAH  $AssocAH
  ${NSD_GetState} $hCtl_AssocAS  $AssocAS
FunctionEnd

; ============================================================
; Add to PATH
; ============================================================
Function AddToPathFunc
  ${If} $AddToPath != ${BST_CHECKED}
    Return
  ${EndIf}

  ${If} $InstallModeAllUsers == "1"
    ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  ${Else}
    ReadRegStr $0 HKCU "Environment" "Path"
    ${If} $0 == ""
      ; If user PATH var doesn't exist, try reading from system PATH as base
      ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    ${EndIf}
  ${EndIf}

  ; Check if install dir is already in PATH
  ${StrStr} $1 "$0" "$INSTDIR"
  ${If} $1 == ""
    ${If} $0 == ""
      StrCpy $0 "$INSTDIR"
    ${Else}
      StrCpy $0 "$0;$INSTDIR"
    ${EndIf}

    ${If} $InstallModeAllUsers == "1"
      WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$0"
    ${Else}
      WriteRegExpandStr HKCU "Environment" "Path" "$0"
    ${EndIf}

    SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
  ${EndIf}
FunctionEnd

; ============================================================
; Remove from PATH (uninstall)
; ============================================================
Function un.RemoveFromPath
  ReadRegStr $0 HKLM "${PRODUCT_DIR_REGKEY}" "InstallScope"
  ReadRegStr $1 HKLM "${PRODUCT_DIR_REGKEY}" "InstallDir"

  ${If} $1 == ""
    StrCpy $1 $INSTDIR
  ${EndIf}

  ${If} $0 == "AllUsers"
    ReadRegStr $2 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    ${If} $2 != ""
      ${UnStrStr} $3 "$2" "$1"
      ${If} $3 != ""
        ${UnStrRep} $4 "$2" "$1;" ""
        ${UnStrRep} $4 "$4" ";$1" ""
        WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$4"
      ${EndIf}
    ${EndIf}
  ${Else}
    ReadRegStr $2 HKCU "Environment" "Path"
    ${If} $2 != ""
      ${UnStrStr} $3 "$2" "$1"
      ${If} $3 != ""
        ${UnStrRep} $4 "$2" "$1;" ""
        ${UnStrRep} $4 "$4" ";$1" ""
        WriteRegExpandStr HKCU "Environment" "Path" "$4"
      ${EndIf}
    ${EndIf}
  ${EndIf}

  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
FunctionEnd

; ============================================================
; Register File Associations
; ============================================================
Function RegisterFileAssociations
  ${If} $InstallModeAllUsers == "1"
    ${If} $AssocAC == ${BST_CHECKED}
      WriteRegStr HKLM "Software\Classes\.ac"                       "" "AstraC.AC"
      WriteRegStr HKLM "Software\Classes\AstraC.AC"                 "" "AstraC AC Source File"
      WriteRegStr HKLM "Software\Classes\AstraC.AC\DefaultIcon"     "" "$INSTDIR\AC.ico"
      WriteRegStr HKLM "Software\Classes\AstraC.AC\shell\open\command" "" '"$INSTDIR\AstraC.exe" comp "%1"'
    ${EndIf}

    ${If} $AssocAH == ${BST_CHECKED}
      WriteRegStr HKLM "Software\Classes\.ah"                       "" "AstraC.AH"
      WriteRegStr HKLM "Software\Classes\AstraC.AH"                 "" "AstraC AH Header File"
      WriteRegStr HKLM "Software\Classes\AstraC.AH\DefaultIcon"     "" "$INSTDIR\AH.ico"
      WriteRegStr HKLM "Software\Classes\AstraC.AH\shell\open\command" "" '"$INSTDIR\AstraC.exe" comp "%1"'
    ${EndIf}

    ${If} $AssocAS == ${BST_CHECKED}
      WriteRegStr HKLM "Software\Classes\.as"                       "" "AstraC.AS"
      WriteRegStr HKLM "Software\Classes\AstraC.AS"                 "" "AstraC AS Assembly File"
      WriteRegStr HKLM "Software\Classes\AstraC.AS\DefaultIcon"     "" "$INSTDIR\AS.ico"
      WriteRegStr HKLM "Software\Classes\AstraC.AS\shell\open\command" "" '"$INSTDIR\AstraC.exe" asm "%1"'
    ${EndIf}
  ${Else}
    ${If} $AssocAC == ${BST_CHECKED}
      WriteRegStr HKCU "Software\Classes\.ac"                       "" "AstraC.AC"
      WriteRegStr HKCU "Software\Classes\AstraC.AC"                 "" "AstraC AC Source File"
      WriteRegStr HKCU "Software\Classes\AstraC.AC\DefaultIcon"     "" "$INSTDIR\AC.ico"
      WriteRegStr HKCU "Software\Classes\AstraC.AC\shell\open\command" "" '"$INSTDIR\AstraC.exe" comp "%1"'
    ${EndIf}

    ${If} $AssocAH == ${BST_CHECKED}
      WriteRegStr HKCU "Software\Classes\.ah"                       "" "AstraC.AH"
      WriteRegStr HKCU "Software\Classes\AstraC.AH"                 "" "AstraC AH Header File"
      WriteRegStr HKCU "Software\Classes\AstraC.AH\DefaultIcon"     "" "$INSTDIR\AH.ico"
      WriteRegStr HKCU "Software\Classes\AstraC.AH\shell\open\command" "" '"$INSTDIR\AstraC.exe" comp "%1"'
    ${EndIf}

    ${If} $AssocAS == ${BST_CHECKED}
      WriteRegStr HKCU "Software\Classes\.as"                       "" "AstraC.AS"
      WriteRegStr HKCU "Software\Classes\AstraC.AS"                 "" "AstraC AS Assembly File"
      WriteRegStr HKCU "Software\Classes\AstraC.AS\DefaultIcon"     "" "$INSTDIR\AS.ico"
      WriteRegStr HKCU "Software\Classes\AstraC.AS\shell\open\command" "" '"$INSTDIR\AstraC.exe" asm "%1"'
    ${EndIf}
  ${EndIf}

  ; Refresh shell icons so associations show immediately
  System::Call 'shell32.dll::SHChangeNotify(i 0x08000000, i 0x1000, i 0, i 0)'
FunctionEnd

; ============================================================
; Unregister File Associations
; ============================================================
Function un.RegisterFileAssociations
  ReadRegStr $0 HKLM "${PRODUCT_DIR_REGKEY}" "InstallScope"

  ${If} $0 == "AllUsers"
    DeleteRegKey HKLM "Software\Classes\.ac"
    DeleteRegKey HKLM "Software\Classes\AstraC.AC"
    DeleteRegKey HKLM "Software\Classes\.ah"
    DeleteRegKey HKLM "Software\Classes\AstraC.AH"
    DeleteRegKey HKLM "Software\Classes\.as"
    DeleteRegKey HKLM "Software\Classes\AstraC.AS"
  ${Else}
    DeleteRegKey HKCU "Software\Classes\.ac"
    DeleteRegKey HKCU "Software\Classes\AstraC.AC"
    DeleteRegKey HKCU "Software\Classes\.ah"
    DeleteRegKey HKCU "Software\Classes\AstraC.AH"
    DeleteRegKey HKCU "Software\Classes\.as"
    DeleteRegKey HKCU "Software\Classes\AstraC.AS"
  ${EndIf}

  System::Call 'shell32.dll::SHChangeNotify(i 0x08000000, i 0x1000, i 0, i 0)'
FunctionEnd

; ============================================================
; Install Sections
; ============================================================

Section "${PRODUCT_NAME} (required)" SecMain
  SectionIn RO
  SetOutPath "$INSTDIR"

  File "..\build\Release\AstraC.exe"
  File "AC.ico"
  File "AH.ico"
  File "AS.ico"
  File "..\LICENSE"

  ; Store install scope
  ${If} $InstallModeAllUsers == "1"
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "InstallScope" "AllUsers"
  ${Else}
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "InstallScope" "CurrentUser"
  ${EndIf}

  ; Write uninstall registry keys
  ${If} $InstallModeAllUsers == "1"
    WriteRegStr   HKLM "${PRODUCT_UNINST_KEY}" "DisplayName"     "${PRODUCT_FULL_NAME}"
    WriteRegStr   HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr   HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon"     '"$INSTDIR\AstraC.exe"'
    WriteRegStr   HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion"  "${PRODUCT_VERSION}"
    WriteRegStr   HKLM "${PRODUCT_UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
    WriteRegStr   HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout"    "${PRODUCT_WEB_SITE}"
    WriteRegStr   HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify"        "1"
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair"        "1"
    WriteRegStr   HKLM "${PRODUCT_DIR_REGKEY}"  "InstallDir"     "$INSTDIR"
  ${Else}
    WriteRegStr   HKCU "${PRODUCT_UNINST_KEY}" "DisplayName"     "${PRODUCT_FULL_NAME}"
    WriteRegStr   HKCU "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr   HKCU "${PRODUCT_UNINST_KEY}" "DisplayIcon"     '"$INSTDIR\AstraC.exe"'
    WriteRegStr   HKCU "${PRODUCT_UNINST_KEY}" "DisplayVersion"  "${PRODUCT_VERSION}"
    WriteRegStr   HKCU "${PRODUCT_UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
    WriteRegStr   HKCU "${PRODUCT_UNINST_KEY}" "URLInfoAbout"    "${PRODUCT_WEB_SITE}"
    WriteRegStr   HKCU "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegDWORD HKCU "${PRODUCT_UNINST_KEY}" "NoModify"        "1"
    WriteRegDWORD HKCU "${PRODUCT_UNINST_KEY}" "NoRepair"        "1"
    WriteRegStr   HKCU "${PRODUCT_DIR_REGKEY}"  "InstallDir"     "$INSTDIR"
  ${EndIf}

  ; Estimated size
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  ${If} $InstallModeAllUsers == "1"
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
  ${Else}
    WriteRegDWORD HKCU "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
  ${EndIf}

  ; Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; Start Menu shortcuts — clean slate
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  RMDir /r "$SMPROGRAMS\$StartMenuFolder"
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortCut  "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_FULL_NAME}.lnk" \
                    "$INSTDIR\AstraC.exe" "" "$INSTDIR\AstraC.exe" 0
    CreateShortCut  "$SMPROGRAMS\$StartMenuFolder\AstraC Command Prompt.lnk" \
                    "%COMSPEC%" '/k cd /d "$INSTDIR" && AstraC.exe help' \
                    "$INSTDIR\AstraC.exe" 0 "" "" \
                    "Open a command prompt with AstraC ready to use"
    CreateShortCut  "$SMPROGRAMS\$StartMenuFolder\Uninstall AstraC.lnk" \
                    "$INSTDIR\uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END

  ; Clean up any old file association entries from either scope
  DeleteRegKey HKLM "Software\Classes\.ac"
  DeleteRegKey HKLM "Software\Classes\AstraC.AC"
  DeleteRegKey HKLM "Software\Classes\.ah"
  DeleteRegKey HKLM "Software\Classes\AstraC.AH"
  DeleteRegKey HKLM "Software\Classes\.as"
  DeleteRegKey HKLM "Software\Classes\AstraC.AS"
  DeleteRegKey HKCU "Software\Classes\.ac"
  DeleteRegKey HKCU "Software\Classes\AstraC.AC"
  DeleteRegKey HKCU "Software\Classes\.ah"
  DeleteRegKey HKCU "Software\Classes\AstraC.AH"
  DeleteRegKey HKCU "Software\Classes\.as"
  DeleteRegKey HKCU "Software\Classes\AstraC.AS"

  ; Add to PATH, register file associations, refresh environment
  Call AddToPathFunc
  Call RegisterFileAssociations
SectionEnd

Section "Example Files" SecExamples
  SetOutPath "$INSTDIR\examples\AS"
  File "..\AS\BOOTLOADER.AS"
  File "..\AS\INCLUDE.as"
  File "..\AS\MAIN.AS"

  SetOutPath "$INSTDIR\examples\AC"
  File "..\AC\INCLUDE.AH"
  File "..\AC\MAIN.AC"
  File "..\AC\MAIN.AS"
  File "..\AC\TEST.AC"
  File "..\AC\TEST.AH"
  File "..\AC\TEST.AS"
  File "..\AC\TEST2.AC"
SectionEnd

Section "Documentation" SecDocs
  SetOutPath "$INSTDIR\docs"
  File "..\DOCS\AC.md"
  File "..\DOCS\AS.md"
  File "..\DOCS\AC_LANG.md"
  File "..\DOCS\DSM.md"
  File "..\DOCS\PREPROCESSOR.md"
  File "..\DOCS\AC_FILEHEADER.md"
  File "..\DOCS\AC.png"
  File "..\DOCS\AH.png"
  File "..\DOCS\AS.png"

SectionEnd

Section /o "Source Code" SecSource
  SetOutPath "$INSTDIR\src"
  File "..\ASTRAC.c"
  File "..\ASTRAC.h"
  File "..\ASTRAC.rc.in"
  File "..\LICENSE"
  File "..\CMakeLists.txt"

  SetOutPath "$INSTDIR\src\ASSEMBLER"
  File "..\ASSEMBLER\*.c"
  File "..\ASSEMBLER\*.h"

  SetOutPath "$INSTDIR\src\COMPILER"
  File "..\COMPILER\*.c"
  File "..\COMPILER\*.h"

  SetOutPath "$INSTDIR\src\DISSASEMBLER"
  File "..\DISSASEMBLER\*.c"
  File "..\DISSASEMBLER\*.h"

  SetOutPath "$INSTDIR\src\SHARED"
  File "..\SHARED\*.c"
#  File "..\SHARED\*.h"

  SetOutPath "$INSTDIR\src\STDLIB"
  File "..\STDLIB\*.c"
  File "..\STDLIB\*.h"

  SetOutPath "$INSTDIR\src\VERSION"
  File "..\VERSION\VERSION.h"
  File "..\VERSION\VERSION.txt"

  SetOutPath "$INSTDIR\src\NSIS"
  File "..\NSIS\*.ico"
  File "..\NSIS\INSTALLER.NSI"

  SetOutPath "$INSTDIR\src\SCRIPTS\WIN"
  File "..\SCRIPTS\WIN\CREATE_NSIS.BAT"

  SetOutPath "$INSTDIR\src\DOCS"
  File "..\DOCS\*.md"
  File "..\DOCS\*.png"
SectionEnd

; --- Section Descriptions ---
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMain}     "The AstraC compiler, assembler, and disassembler executable. Also installs file icons and the MIT license."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecExamples} "Example .AS assembly sources and .AC compiler sources to help you get started with AstraC."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocs}     "Full documentation including assembler, compiler, disassembler, preprocessor, and file format references."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSource}   "Full project source code (C files, headers, build scripts). Off by default."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ============================================================
; Uninstall Sections
; ============================================================
Section "Uninstall"
  Call un.RegisterFileAssociations
  Call un.RemoveFromPath

  ; Remove Start Menu shortcuts
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_FULL_NAME}.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\AstraC Command Prompt.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall AstraC.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Documentation.lnk"
  RMDir  "$SMPROGRAMS\$StartMenuFolder"

  ; Remove installed files
  Delete "$INSTDIR\AstraC.exe"
  Delete "$INSTDIR\AC.ico"
  Delete "$INSTDIR\AH.ico"
  Delete "$INSTDIR\AS.ico"
  Delete "$INSTDIR\LICENSE"
  Delete "$INSTDIR\uninstall.exe"

  Delete "$INSTDIR\examples\AS\BOOTLOADER.AS"
  Delete "$INSTDIR\examples\AS\INCLUDE.as"
  Delete "$INSTDIR\examples\AS\MAIN.AS"
  Delete "$INSTDIR\examples\AC\INCLUDE.AH"
  Delete "$INSTDIR\examples\AC\MAIN.AC"
  Delete "$INSTDIR\examples\AC\MAIN.AS"
  Delete "$INSTDIR\examples\AC\TEST.AC"
  Delete "$INSTDIR\examples\AC\TEST.AH"
  Delete "$INSTDIR\examples\AC\TEST.AS"
  Delete "$INSTDIR\examples\AC\TEST2.AC"
  RMDir  "$INSTDIR\examples\AS"
  RMDir  "$INSTDIR\examples\AC"
  RMDir  "$INSTDIR\examples"

  Delete "$INSTDIR\docs\AC.md"
  Delete "$INSTDIR\docs\AS.md"
  Delete "$INSTDIR\docs\AC_LANG.md"
  Delete "$INSTDIR\docs\DSM.md"
  Delete "$INSTDIR\docs\PREPROCESSOR.md"
  Delete "$INSTDIR\docs\AC_FILEHEADER.md"
  Delete "$INSTDIR\docs\AC.png"
  RMDir  "$INSTDIR\docs"

  RMDir /r "$INSTDIR\src"

  RMDir  "$INSTDIR"

  ; Remove registry keys
  ReadRegStr $0 HKLM "${PRODUCT_DIR_REGKEY}" "InstallScope"
  ${If} $0 == "AllUsers"
    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  ${Else}
    DeleteRegKey HKCU "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKCU "${PRODUCT_DIR_REGKEY}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  ${EndIf}

  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
  System::Call 'shell32.dll::SHChangeNotify(i 0x08000000, i 0x1000, i 0, i 0)'
SectionEnd
