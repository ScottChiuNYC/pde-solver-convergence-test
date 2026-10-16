@echo off
setlocal

set "PYEXE="
if defined VIRTUAL_ENV (
    set "PYEXE=%VIRTUAL_ENV%\Scripts\python.exe"
)

if not defined PYEXE (
    for /f "delims=" %%P in ('where python') do (
        set "PYEXE=%%P"
        goto :found_python
    )
)

:found_python
if defined PYEXE (
    cmake --preset windows-vcpkg -DPython_EXECUTABLE:FILEPATH="%PYEXE%" -DPython3_EXECUTABLE:FILEPATH="%PYEXE%"
) else (
    cmake --preset windows-vcpkg
)
if errorlevel 1 exit /b %errorlevel%

cmake --build --preset windows-debug
if errorlevel 1 exit /b %errorlevel%

ctest --preset windows-debug-tests
exit /b %errorlevel%
