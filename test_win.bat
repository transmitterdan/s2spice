          rem Ensure deps and wxWidgets DLL path like in build step
          setlocal enabledelayedexpansion
          echo Setting path
          set "path=%path%;%GITHUB_WORKSPACE%\wxWidgets-3.3.1\lib\vc14x_x64_dll"
          echo Finished path...echoing path
          echo %path%
          echo Locate built binary
          set "BIN=%GITHUB_WORKSPACE%\build\Release\s2spice.exe"
          echo BIN = %BIN%
          if not exist "%BIN%" (
            echo Could not find built binary: %BIN%
            dir /b "%GITHUB_WORKSPACE%\build"
            exit /b 1
          )
          echo If no sample inputs, skip gracefully
          dir /b "%GITHUB_WORKSPACE%\Test\*.s?p" >nul 2>&1
          if errorlevel 1 (
            echo No Touchstone test files found in Test\ - skipping tests.
            exit /b 0
          )
          set FAILED=0
          cd "%GITHUB_WORKSPACE%\Test"
          echo Run over all Touchstone files and verify outputs
          for %%F in ("*.s?p") do (
            echo Running "%BIN%" -l -s -q -f "%%F"
            "%BIN%" -l -s -q -f "%%F"
            if not errorlevel 1 (
              if not exist "%%~nF.inc" (
                echo Missing output: %%~nF.inc
                set FAILED=1
              )
              if not exist "%%~nF.asy" (
                echo Missing output: %%~nF.asy
                set FAILED=1
              )
            )
          )
          del *.inc
          del *.asy
          for %%F in ("*.ts") do (
            echo Running "%BIN%" -l -s -q "%%F"
            "%BIN%" -l -s -q -f "%%F"
            if not errorlevel 1 (
              if not exist "%%~nF.inc" (
                echo Missing output: %%~nF.inc
                set FAILED=1
              )
              if not exist "%%~nF.asy" (
                echo Missing output: %%~nF.asy
                set FAILED=1
              )
            )
          )
          if "%FAILED%"=="1" (
            echo One or more CLI tests failed.
            exit /b 1
          )
          echo CLI smoke tests passed.
