@echo off
rem Run from the repository root in an x64 Native Tools Command Prompt (v143).
if not exist .verification mkdir .verification
cl /nologo /c /EHsc /std:c++20 /utf-8 ProjectName\include\MyLib\Math\Vector2.ixx /ifcOutput .verification\ /Fo.verification\Vector2.obj
if errorlevel 1 exit /b 1
cl /nologo /c /EHsc /std:c++20 /utf-8 /I ProjectName\include\nlohmann ProjectName\code\Phase1Config.ixx /ifcOutput .verification\ /Fo.verification\Phase1Config.obj
if errorlevel 1 exit /b 1
cl /nologo /c /EHsc /std:c++20 /utf-8 /ifcSearchDir .verification ProjectName\code\Component\Move\MoveWithKey\MovementMath.ixx /ifcOutput .verification\ /Fo.verification\MovementMath.obj
if errorlevel 1 exit /b 1
cl /nologo /c /EHsc /std:c++20 /utf-8 /ifcSearchDir .verification ProjectName\code\GameSystem\Camera\CameraGeometry.ixx /ifcOutput .verification\ /Fo.verification\CameraGeometry.obj
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /utf-8 /I ProjectName\include\nlohmann /ifcSearchDir .verification Tests\MovementTests.cpp .verification\Vector2.obj .verification\Phase1Config.obj .verification\MovementMath.obj .verification\CameraGeometry.obj /Fo.verification\MovementTests.obj /Fe.verification\MovementTests.exe
if errorlevel 1 exit /b 1
.verification\MovementTests.exe
if errorlevel 1 exit /b 1
cl /nologo /c /EHsc /std:c++20 /utf-8 /ifcSearchDir .verification ProjectName\code\Object\MovingBase\BaseMotion.ixx /ifcOutput .verification\ /Fo.verification\BaseMotion.obj
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /utf-8 /I ProjectName\include\nlohmann /ifcSearchDir .verification Tests\BaseMotionTests.cpp .verification\Vector2.obj .verification\Phase1Config.obj .verification\BaseMotion.obj /Fo.verification\BaseMotionTests.obj /Fe.verification\BaseMotionTests.exe
if errorlevel 1 exit /b 1
.verification\BaseMotionTests.exe
exit /b %errorlevel%
