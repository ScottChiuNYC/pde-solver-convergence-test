setlocal

:: time Zone: should be -0400 in the summer but whatever
set "TZ_OFFSET=-0500"
set "COMMIT_TIME=22:00:00"

:: get today's date (usually YYYY-MM-DD)
for /f "tokens=2-4 delims=/ " %%a in ('echo %date%') do (
    set "year=%%c"
    set "month=%%a"
    set "day=%%b"
)
:: backup: if system time is in the format YYYY/MM/DD
if "%year%"=="" (
    for /f "tokens=1-3 delims=/ " %%a in ('echo %date%') do (
        set "year=%%a"
        set "month=%%b"
        set "day=%%c"
    )
)

:: ISO 8601 format
set "FINAL_DATE=%year%-%month%-%day%T%COMMIT_TIME%%TZ_OFFSET%"

:: commit and push
set GIT_AUTHOR_DATE=%FINAL_DATE%
set GIT_COMMITTER_DATE=%FINAL_DATE%

@REM git config --global user.name "beginnerSC"
@REM git config --global user.email "25188222+beginnerSC@users.noreply.github.com"
git add *
git commit -m "auto save"
git push

endlocal