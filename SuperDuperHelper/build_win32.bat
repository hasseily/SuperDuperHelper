@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@set OUT_DIR=Debug
@set OUT_EXE=SuperDuperHelper
@set INCLUDES=/I..\imgui-1.89.4 /I..\imgui-1.89.4\backends /I%SDL2_DIR%\include
@set SOURCES=main.cpp ..\imgui-1.89.4\backends\imgui_impl_sdl2.cpp ..\imgui-1.89.4\backends\imgui_impl_opengl3.cpp ..\imgui-1.89.4\imgui*.cpp
@set LIBS=/LIBPATH:%SDL2_DIR%\lib\x86 SDL2.lib SDL2main.lib opengl32.lib shell32.lib
mkdir %OUT_DIR%
cl /nologo /Zi /MD %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console
