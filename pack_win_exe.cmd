@echo off
setlocal enabledelayedexpansion

REM === 设置路径 ===
set "BUILD_DIR=build"
set "RELEASE_DIR=%BUILD_DIR%\Release"
set "DIST_DIR=%BUILD_DIR%\dist"

REM === 清理旧的 dist 输出目录 ===
if exist "%DIST_DIR%" (
    echo Cleaning old dist...
    rmdir /s /q "%DIST_DIR%"
)
mkdir "%DIST_DIR%"

REM === 拷贝所有 .exe/.dll/.qml/.qrc 文件 ===
echo Copying required files (*.exe, *.dll, *.qml, *.qrc)...
for %%F in (exe dll qml qrc) do (
    for /r "%RELEASE_DIR%" %%X in (*.^%%F) do (
        set "FNAME=%%~nxX"
        copy "%%X" "%DIST_DIR%\!FNAME!" >nul
    )
)

REM === 拷贝必要目录 ===
for %%D in (qml web platforms resources locales MyApp) do (
    if exist "%RELEASE_DIR%\%%D" (
        echo Copying folder: %%D
        xcopy /s /e /y /i "%RELEASE_DIR%\%%D" "%DIST_DIR%\%%D" >nul
    )
)

REM === 删除中间产物（保守清理）===
echo Deleting intermediate files...
for %%X in (obj pdb exp lib idb tlog ilk manifest log cache) do (
    for /r "%RELEASE_DIR%" %%F in (*.%%X) do (
        del /f /q "%%F"
    )
)

REM === 删除构建系统垃圾 ===
del /f /q "%RELEASE_DIR%\CMakeCache.txt" >nul 2>nul
del /f /q "%RELEASE_DIR%\Makefile" >nul 2>nul
if exist "%RELEASE_DIR%\CMakeFiles" (
    rmdir /s /q "%RELEASE_DIR%\CMakeFiles"
)
for /r "%RELEASE_DIR%" %%F in (*.cmake) do (
    del /f /q "%%F"
)

echo.
echo Dist build complete.
echo Output folder: %DIST_DIR%
endlocal
