@echo off
rem ============================================================
rem  一行编译 / 烧录（绕过 PowerShell 执行策略限制）
rem
rem  用法（在 cmd 里）：
rem    tools\build.cmd                    只编译
rem    tools\build.cmd -Upload COM5       编译 + 烧录
rem    tools\build.cmd -Upload COM5 -Monitor COM5
rem    tools\build.cmd -Clean             清缓存重编
rem    tools\build.cmd -Export arduino\esp32clock   导出给 Arduino IDE
rem ============================================================
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
exit /b %ERRORLEVEL%
