; Screen Time Inno Setup 安装脚本
; 编译需要 Inno Setup 6.0 或更高版本
; 先运行 deploy-scripts\deploy_release.bat 生成 release-package\ScreenTime

#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif

#define MyAppName "Screen Time"
#define MyAppPublisher "Yiuk-max"
#define MyAppURL "https://github.com/Yiuk-max/Screen-Time"
#define MyAppExeName "ScreenTime.exe"
#define DeployDir "..\release-package\ScreenTime"

[Setup]
AppId={{8F7A2B3C-4D5E-6F7A-8B9C-0D1E2F3A4B5C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

DefaultDirName={autopf}\ScreenTime
DefaultGroupName={#MyAppName}
AllowNoIcons=yes

OutputDir=..\release-package
OutputBaseFilename=ScreenTime_Setup_{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

MinVersion=10.0
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

#if FileExists(AddBackslash(SourcePath) + "..\icons\app.ico")
SetupIconFile=..\icons\app.ico
#endif

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#DeployDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\*"

[Code]
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  KeepData: Boolean;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    KeepData := MsgBox('是否保留用户数据（使用记录数据库）？', mbConfirmation, MB_YESNO) = IDYES;
    if not KeepData then
    begin
      DelTree(ExpandConstant('{userappdata}\ScreenTime'), True, True, True);
    end;
  end;
end;
