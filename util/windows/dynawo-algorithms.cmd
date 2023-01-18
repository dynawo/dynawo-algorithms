@echo off

:: Copyright (c) 2015-2022, RTE (http://www.rte-france.com)
:: See AUTHORS.txt
:: All rights reserved.
:: This Source Code Form is subject to the terms of the Mozilla Public
:: License, v. 2.0. If a copy of the MPL was not distributed with this
:: file, you can obtain one at http://mozilla.org/MPL/2.0/.
:: SPDX-License-Identifier: MPL-2.0
::
:: This file is part of Dynawo, an hybrid C++/Modelica open source suite
:: of simulation tools for power systems.


:: internal facility for build option
if /i "%~1"=="_EXIT_" (
  if not "%~2"=="" del "%~2" 2>NUL
  exit /B %~3
)

setlocal

:: check options
set _verbose=
if not defined DYNAWO_BUILD_TYPE set DYNAWO_BUILD_TYPE=Release
set _show=
:NEXT_OPT
if /i "%~1"=="VERBOSE" (
  set _verbose=true
) else if /i "%~1"=="DEBUG" (
  set DYNAWO_BUILD_TYPE=Debug
) else if /i "%~1"=="SHOW" (
  set _show=true
) else (
  goto:LAST_OPT
)
shift /1
goto:NEXT_OPT

:LAST_OPT

:: look for dynawoAlgorithms build environment
if not defined DYNAWO_ALGORITHMS_HOME (
  if exist "%~dp0..\..\util\windows\%~nx0" (
    :: command is run from dynawoAlgorithms build environment or in its own deployment directory
    set DYNAWO_ALGORITHMS_HOME=%~dp0..\..
  ) else if exist "%~dp0..\dynawo-algorithms\util\windows\%~nx0" (
    :: command is run from dynawoAlgorithms default installation directory
    set DYNAWO_ALGORITHMS_HOME=%~dp0..\dynawo-algorithms
  ) else if exist "%~dp0..\..\dynawo-algorithms\util\windows\%~nx0" (
    :: command is run from dynawoAlgorithms in common deployment directory
    set DYNAWO_ALGORITHMS_HOME=%~dp0..\..\dynawo-algorithms
  )
)
for %%G in ("%DYNAWO_ALGORITHMS_HOME%") do set DYNAWO_ALGORITHMS_HOME=%%~fG
if defined _verbose echo info: using DYNAWO_ALGORITHMS_HOME=%DYNAWO_ALGORITHMS_HOME% 1>&2

:: look for dynawoAlgorithms installation directory
set DYNAWO_ALGORITHMS_INSTALL_DIR=
if exist "%~dp0bin\dynawoAlgorithms.exe" (
  :: command is run from dynawoAlgorithms distribution, deployment or installation directory
  set DYNAWO_ALGORITHMS_INSTALL_DIR=%~dp0.
  if exist "%~dp0share\dynawoalgorithms-config-release.cmake" set DYNAWO_BUILD_TYPE=Release
  if exist "%~dp0share\dynawoalgorithms-config-debug.cmake" set DYNAWO_BUILD_TYPE=Debug
)
if defined _verbose echo info: using DYNAWO_BUILD_TYPE=%DYNAWO_BUILD_TYPE% 1>&2

:: set build directory
if defined DYNAWO_ALGORITHMS_HOME (
  if %DYNAWO_BUILD_TYPE%==Release set DYNAWO_ALGORITHMS_BUILD_DIR=%DYNAWO_ALGORITHMS_HOME%\b
  if %DYNAWO_BUILD_TYPE%==Debug set DYNAWO_ALGORITHMS_BUILD_DIR=%DYNAWO_ALGORITHMS_HOME%\bd
)

if defined DYNAWO_ALGORITHMS_HOME if not defined DYNAWO_ALGORITHMS_INSTALL_DIR (
  setlocal enableDelayedExpansion

  :: command is run from dynawoAlgorithms build environment ; try to find installation directory
  for /f "tokens=2 delims==" %%G in ('find "CMAKE_INSTALL_PREFIX:PATH=" "%DYNAWO_ALGORITHMS_BUILD_DIR%\CMakeCache.txt" 2^>NUL') do set DYNAWO_ALGORITHMS_INSTALL_DIR=%%~fG
  if defined DYNAWO_ALGORITHMS_INSTALL_DIR if not exist "!DYNAWO_ALGORITHMS_INSTALL_DIR!\bin\dynawoAlgorithms.exe" set DYNAWO_ALGORITHMS_INSTALL_DIR=

  if /i "%~1"=="BUILD" (
    :: in case of build, set installation directory from optional argument or to default value
    if not "%~2"=="" set DYNAWO_ALGORITHMS_INSTALL_DIR=%~2
    if not defined DYNAWO_ALGORITHMS_INSTALL_DIR if %DYNAWO_BUILD_TYPE%==Release set DYNAWO_ALGORITHMS_INSTALL_DIR=%DYNAWO_ALGORITHMS_HOME%\..\da-i
    if not defined DYNAWO_ALGORITHMS_INSTALL_DIR if %DYNAWO_BUILD_TYPE%==Debug set DYNAWO_ALGORITHMS_INSTALL_DIR=%DYNAWO_ALGORITHMS_HOME%\..\da-id
  )
  setlocal disableDelayedExpansion
)
for %%G in ("%DYNAWO_ALGORITHMS_INSTALL_DIR%") do set DYNAWO_ALGORITHMS_INSTALL_DIR=%%~fG
if defined _verbose echo info: using DYNAWO_ALGORITHMS_INSTALL_DIR=%DYNAWO_ALGORITHMS_INSTALL_DIR% 1>&2


:: look for dynawo deploy directory
if not defined DYNAWO_HOME (
  if exist "%~dp0bin\dynawo.exe" (
    :: command is run from a dynawoAlgorithms distribution directory
    set DYNAWO_HOME=%~dp0.
  ) else (
    :: dynawo deployment directory is found in path
    for /f "tokens=*" %%G in ('where dynawo.exe 2^>NUL') do set DYNAWO_HOME=%%~dpG..
  )
)
if defined DYNAWO_HOME goto:DYNAWO_FOUND

set _dynawo_release=
set _dynawo_debug=

:: command is run from dynawoAlgorithms default installation directory with dynawo in common deployment directory
call:FIND_DYNAWO ..
:: command is run from dynawoAlgorithms default installation directory with dynawo in its own deployment directory
call:FIND_DYNAWO ..\dynawo
:: command is run from dynawoAlgorithms in common deployment directory with dynawo in common deployment directory
call:FIND_DYNAWO ..\..
:: command is run from dynawoAlgorithms in common deployment directory with dynawo in its own deployment directory
call:FIND_DYNAWO ..\..\dynawo
:: command is run from dynawoAlgorithms build environment (or in its own deployment directory) with dynawo in common deployment directory
call:FIND_DYNAWO ..\..\..
:: command is run from dynawoAlgorithms build environment (or in its own deployment directory) with dynawo in its own deployment directory
call:FIND_DYNAWO ..\..\..\dynawo

:: keep the right dynawo according build type
set DYNAWO_HOME=%_dynawo_release%
if %DYNAWO_BUILD_TYPE%==Debug set DYNAWO_HOME=%_dynawo_debug%

:DYNAWO_FOUND
for %%G in ("%DYNAWO_HOME%") do set DYNAWO_HOME=%%~fG
if defined _verbose echo info: using DYNAWO_HOME=%DYNAWO_HOME% 1>&2


:: check preconditions
if not defined DYNAWO_HOME (
  echo error: unable to find dynawo deployment directory ^(please set DYNAWO_HOME^) ! 1>&2
  exit /B 1
)

if %DYNAWO_BUILD_TYPE%==Release if not exist "%DYNAWO_HOME%\share\dynawo-config-release.cmake" (
  echo error: incompatible dynawo deployment directory ^(should be Release^) ! 1>&2
  exit /B 1
)
if %DYNAWO_BUILD_TYPE%==Debug if not exist "%DYNAWO_HOME%\share\dynawo-config-debug.cmake" (
  echo error: incompatible dynawo deployment directory ^(should be Debug^) ! 1>&2
  exit /B 1
)

if not defined DYNAWO_ALGORITHMS_INSTALL_DIR if not "%~1"=="" if /i not "%~1"=="HELP" if /i not "%~1"=="CLEAN" if /i not "%~1"=="TESTS" (
  echo error: you have to build dynawoAlgorithms before being able to use it ! 1>&2
  exit /B 1
)


:: check python interpreter
if /i not "%~1"=="BUILD" if /i not "%~1"=="TESTS" if /i not "%~1"=="NRT" goto:PYTHON_UNUSED
if not defined DYNAWO_PYTHON_COMMAND set DYNAWO_PYTHON_COMMAND=python
"%DYNAWO_PYTHON_COMMAND%" --version >NUL 2>&1
if %ERRORLEVEL% neq 0 (
  echo error: the python interpreter ^"%DYNAWO_PYTHON_COMMAND%^" does not work ^(please set DYNAWO_PYTHON_COMMAND^) ! 1>&2
  exit /B 1
)
:PYTHON_UNUSED


:: check developper mode
set _devmode=
:: if there is a CMakeLists.txt, developper mode is on
if defined DYNAWO_ALGORITHMS_HOME if exist "%DYNAWO_ALGORITHMS_HOME%\CMakeLists.txt" (
  set _devmode=true
)

:: check command
set _exitcode=0
if "%~1"=="" goto:HELP
if /i %~1==CS goto:CS
if /i %~1==SA goto:SA
if /i %~1==MC goto:MC
if /i %~1==NRT goto:NRT
if /i %~1==VERSION goto:VERSION
if /i %~1==BUILD goto:BUILD
if /i %~1==CLEAN goto:CLEAN
if /i %~1==TESTS goto:TESTS
if /i %~1==DEPLOY goto:DEPLOY
if /i %~1==DISTRIB goto:DISTRIB
if /i %~1==DISTRIB-HEADERS goto:DISTRIB
if /i %~1==DISTRIB-OMC goto:DISTRIB
if /i %~1==HELP goto:HELP

:: show help in case of invalid command
echo error: %~1 is not a valid command ! 1>&2
:ERROR
echo. 1>&2
set _exitcode=1


:HELP
if defined _devmode (
  echo usage: %~n0 [VERBOSE] [DEBUG] [HELP ^| ^<command^>]
) else (
  echo usage: %~n0 [VERBOSE] [HELP ^| ^<command^>]
)
echo.
echo HELP command displays this message.
echo Add VERBOSE option to echo environment variables used.
if defined _devmode echo All commands are run in Release mode by default. Add DEBUG option to turn in Debug mode.
if exist "%DYNAWO_ALGORITHMS_INSTALL_DIR%\bin\dynawoAlgorithms.exe" (
  echo.
  echo These are commands used by end-user:
  echo   CS^|SA^|MC [^<simulation_options^>] Launch dynawoAlgorithms simulation ^(see simulation_options below^)
  echo   [SHOW] nrt                      Launch dynawoAlgorithms Non Regressions Tests ^(option SHOW uses DYNAWO_BROWSER to show results^)
  echo   version                         Print dynawoAlgorithms version
  echo.
  echo where ^<simulation_options^> are:
  call:SET_ENV
  "%DYNAWO_ALGORITHMS_INSTALL_DIR%\bin\dynawoAlgorithms" --help | findstr /C:--help /C:--version /R /C:"^$" /V
)
if defined _devmode (
  echo.
  echo These are commands used by developer only:
  echo   build [^<install_dir^>]           Build and install dynawoAlgorithms
  echo   [SHOW] tests [^<unit_test^>]      Build and run unit tests or a specific unit test ^(option SHOW lists available unit tests^)
  echo   clean                           Clean build directory
  echo   deploy [^<deploy_dir^>]           Deploy the current version of dynawoAlgorithms binaries/libraries/includes
  echo                                   to be used as a release by an another project
  echo   distrib [^<distrib_dir^>]         Create distribution of dynawoAlgorithms
  echo   distrib-headers [^<distrib_dir^>] Create distribution of dynawoAlgorithms with headers
  echo   distrib-omc [^<distrib_dir^>]     Create distribution of dynawoAlgorithms with OpenModelica
)
exit /B %_exitcode%%



:: CS simulation
:CS
call:SET_ENV
"%DYNAWO_ALGORITHMS_INSTALL_DIR%\bin\dynawoAlgorithms" --simulationType CS %*
exit /B %ERRORLEVEL%

:: SA simulation
:SA
set _args=SA
goto:IS_MPI

:: MC simulation
:MC
set _args=MC
::goto:IS_MPI

:: check MPI runtime
:IS_MPI
mpiexec >NUL 2>&1
if %ERRORLEVEL% neq 0 ( 
  echo error: Microsoft MPI Runtime should be installed ! 1>&2
  exit /B 1
)
set _nbprocs=1

:: scan arguments for nb procs to use with MPI
:NEXT_ARG
shift /1
if [%1]==[] goto:LAST_ARG
if /i [%~1]==[--nbThreads] goto:GET_NBPROCS
if /i [%~1]==[-np] goto:GET_NBPROCS
call set _args=%%_args%% %1
goto:NEXT_ARG

:GET_NBPROCS
set _nbprocs=%~2
shift /1
goto:NEXT_ARG

:LAST_ARG
call:SET_ENV
mpiexec -n %_nbprocs% "%DYNAWO_ALGORITHMS_INSTALL_DIR%\bin\dynawoAlgorithms" --simulationType %_args% 
exit /B %ERRORLEVEL%


:: run NRT
:NRT
if not defined DYNAWO_BROWSER (
  setlocal enableDelayedExpansion

  :: look in the HKEY_CLASSES_ROOT\htmlfile\shell\open\command registry for the default browser
  set _value=
  for /f "skip=2 tokens=*" %%G in ('reg QUERY HKEY_CLASSES_ROOT\htmlfile\shell\open\command /ve') do set _value=%%G

  :: parse the input field looking for the second token
  for /f tokens^=^2^ eol^=^"^ delims^=^" %%G in ("!_value!") do set DYNAWO_BROWSER=%%G

  :: this may not be needed, check if reg returned a real file, if not unset browser
  if not exist "!DYNAWO_BROWSER!" set DYNAWO_BROWSER=

  :: setup a default browser in case we fail
  if not defined DYNAWO_BROWSER set DYNAWO_BROWSER=C:\Program Files\Internet Explorer\iexplore.exe

  setlocal disableDelayedExpansion
)
if defined _verbose echo info: using DYNAWO_BROWSER=%DYNAWO_BROWSER% 1>&2

if not defined DYNAWO_ALGORITHMS_NRT_DIR (
  if defined DYNAWO_ALGORITHMS_HOME if exist "%DYNAWO_ALGORITHMS_HOME%\nrt\nrtAlgo.py" (
    set DYNAWO_ALGORITHMS_NRT_DIR=%DYNAWO_ALGORITHMS_HOME%\nrt
  ) else (
    echo error: unable to find Non Regression Tests ^(please set DYNAWO_ALGORITHMS_NRT_DIR^) ! 1>&2
    exit /B 1
  )
)
for %%G in ("%DYNAWO_ALGORITHMS_NRT_DIR%") do set DYNAWO_ALGORITHMS_NRT_DIR=%%~fG
if defined _verbose echo info: using DYNAWO_ALGORITHMS_NRT_DIR=%DYNAWO_ALGORITHMS_NRT_DIR% 1>&2

if not defined DYNAWO_NB_PROCESSORS_USED set DYNAWO_NB_PROCESSORS_USED=2
if defined _verbose echo info: using DYNAWO_NB_PROCESSORS_USED=%DYNAWO_NB_PROCESSORS_USED% 1>&2

call:SET_ENV
set DYNAWO_NRT_DIFF_DIR=%DYNAWO_SCRIPTS_DIR%\nrt\nrt_diff
set DYNAWO_NRT_DIR=%DYNAWO_ALGORITHMS_NRT_DIR%
set DYNAWO_BRANCH_NAME=%DYNAWO_BUILD_TYPE%
set DYNAWO_CURVES_TO_HTML_DIR=%DYNAWO_HOME%\sbin\curvesToHtml
set DYNAWO_ENV_DYNAWO=%~f0
shift
del "%DYNAWO_ALGORITHMS_NRT_DIR%\output\%DYNAWO_BRANCH_NAME%\report.html" 2>NUL
"%DYNAWO_PYTHON_COMMAND%" -u "%DYNAWO_ALGORITHMS_NRT_DIR%\nrtAlgo.py" %*
set _failed_cases=%ERRORLEVEL%

if not exist "%DYNAWO_ALGORITHMS_NRT_DIR%\output\%DYNAWO_BRANCH_NAME%\report.html" (
  if %_failed_cases% equ 0 exit /B 0
  if %_failed_cases% neq 0 echo error: no report was generated by the non regression test script !
) else if defined _show if defined DYNAWO_BROWSER (
  "%DYNAWO_BROWSER%" "%DYNAWO_ALGORITHMS_NRT_DIR%\output\%DYNAWO_BRANCH_NAME%\report.html"
)

if %_failed_cases% neq 0 echo error: %_failed_cases% non regression tests failed !
exit /B %_failed_cases%


:: print version
:VERSION
call:SET_ENV
"%DYNAWO_ALGORITHMS_INSTALL_DIR%\bin\dynawoAlgorithms" -v
exit /B %ERRORLEVEL%


:: build dynawoAlgorithms
:BUILD
if not defined _devmode (
  echo error: 'build' command is only available in developper mode ! 1>&2
  exit /B 1
)

:: delegate to a temporary cmd file in case of self update !
cmake -E make_directory %DYNAWO_ALGORITHMS_INSTALL_DIR%
set _build_tmp=%DYNAWO_ALGORITHMS_INSTALL_DIR%\~build.tmp.cmd
(
  echo cmake -S "%DYNAWO_ALGORITHMS_HOME%" -B "%DYNAWO_ALGORITHMS_BUILD_DIR%" -DCMAKE_INSTALL_PREFIX="%DYNAWO_ALGORITHMS_INSTALL_DIR%" -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=%DYNAWO_BUILD_TYPE% -DDYNAWO_ALGORITHMS_HOME=. -DDYNAWO_HOME="%DYNAWO_HOME%" -DDYNAWO_ALGORITHMS_THIRD_PARTY_DIR="%DYNAWO_ALGORITHMS_HOME%" -DDYNAWO_PYTHON_COMMAND="%DYNAWO_PYTHON_COMMAND%" -G "NMake Makefiles" -Wno-dev
  echo cmake --build "%DYNAWO_ALGORITHMS_BUILD_DIR%" --target install
  echo %0 _EXIT_ "%_build_tmp%" %ERRORLEVEL%
) > "%_build_tmp%"
"%_build_tmp%"

:: must never be reached
goto:eof


:: clean build directory
:CLEAN
if not defined _devmode (
  echo error: 'clean' command is only available in developper mode ! 1>&2
  exit /B 1
)
rd /s /q "%DYNAWO_ALGORITHMS_BUILD_DIR%" 2>NUL
exit /B %ERRORLEVEL%


:: build and run unit tests
:TESTS
if not defined _devmode (
  echo error: 'tests' command is only available in developper mode ! 1>&2
  exit /B 1
)

:: indicates where a GTest installation may already exist
if not defined DYNAWO_GTEST_HOME set DYNAWO_GTEST_HOME=%DYNAWO_HOME%
if not defined DYNAWO_GMOCK_HOME set DYNAWO_GMOCK_HOME=%DYNAWO_HOME%

:: force XSD validation in case of tests
set DYNAWO_USE_XSD_VALIDATION=true

:: build with GTest and run all tests or a specific test
set _test=dynawo_algorithms_%~2_unittest-tests
if "%~2"=="" set _test=tests
cmake -S "%DYNAWO_ALGORITHMS_HOME%" -B "%DYNAWO_ALGORITHMS_BUILD_DIR%" -DCMAKE_INSTALL_PREFIX="%DYNAWO_ALGORITHMS_INSTALL_DIR%" -DBUILD_TESTS=ON -DGTEST_ROOT="%DYNAWO_GTEST_HOME%" -DGMOCK_HOME="%DYNAWO_GMOCK_HOME%" -DCMAKE_BUILD_TYPE=%DYNAWO_BUILD_TYPE% -DDYNAWO_ALGORITHMS_HOME=. -DDYNAWO_HOME="%DYNAWO_HOME%" -DDYNAWO_ALGORITHMS_THIRD_PARTY_DIR="%DYNAWO_ALGORITHMS_HOME%" -DDYNAWO_PYTHON_COMMAND="%DYNAWO_PYTHON_COMMAND%" -G "NMake Makefiles" -Wno-dev
if defined _show goto:LIST_TESTS
:: check if valid unit test
cmake --build "%DYNAWO_ALGORITHMS_BUILD_DIR%" --target help | find "%_test%" >NUL || goto:INVALID_TEST
cmake --build "%DYNAWO_ALGORITHMS_BUILD_DIR%" --target "%_test%"
exit /B %ERRORLEVEL%

:INVALID_TEST
echo error: %~2 is not a valid unit test ! 1>&2

:: list unit tests
:LIST_TESTS
echo.
echo Available unit tests are :
setlocal enableDelayedExpansion
for /f "tokens=2" %%G in ('cmake --build "%DYNAWO_ALGORITHMS_BUILD_DIR%" --target help ^| find "_unittest-tests"') do set _value=%%G & set _value=!_value:dynawo_algorithms_=    ! & echo !_value:_unittest-tests=!
setlocal disableDelayedExpansion
exit /B %ERRORLEVEL%


:: deploy in a directory
:DEPLOY
if not defined _devmode (
  echo error: 'deploy' command is only available in developper mode ! 1>&2
  exit /B 1
)
set _deploy_dir=deploy
if %DYNAWO_BUILD_TYPE%==Debug set _deploy_dir=deployd
set DYNAWO_ALGORITHMS_DEPLOY_DIR=%~2
if "%~2"=="" set DYNAWO_ALGORITHMS_DEPLOY_DIR=%DYNAWO_ALGORITHMS_HOME%\..\%_deploy_dir%\dynawo-algorithms
for %%G in ("%DYNAWO_ALGORITHMS_DEPLOY_DIR%") do set DYNAWO_ALGORITHMS_DEPLOY_DIR=%%~fG
if defined _verbose echo info: using DYNAWO_ALGORITHMS_DEPLOY_DIR=%DYNAWO_ALGORITHMS_DEPLOY_DIR% 1>&2

:: populate deployment directory
rd /s /q "%DYNAWO_ALGORITHMS_DEPLOY_DIR%" 2>NUL
cmake -E make_directory "%DYNAWO_ALGORITHMS_DEPLOY_DIR%"
xcopy "%DYNAWO_ALGORITHMS_INSTALL_DIR%\bin" "%DYNAWO_ALGORITHMS_DEPLOY_DIR%\bin" /e /i
xcopy "%DYNAWO_ALGORITHMS_INSTALL_DIR%\lib" "%DYNAWO_ALGORITHMS_DEPLOY_DIR%\lib" /e /i
if not %1==deploy-without-headers (
  xcopy "%DYNAWO_ALGORITHMS_INSTALL_DIR%\include" "%DYNAWO_ALGORITHMS_DEPLOY_DIR%\include" /e /i
)
xcopy "%DYNAWO_ALGORITHMS_INSTALL_DIR%\share" "%DYNAWO_ALGORITHMS_DEPLOY_DIR%\share" /e /i
xcopy "%DYNAWO_ALGORITHMS_INSTALL_DIR%\dynawo-algorithms.cmd" "%DYNAWO_ALGORITHMS_DEPLOY_DIR%"
xcopy "%DYNAWO_ALGORITHMS_HOME%\nrt\data\IEEE14\IEEE14_BlackBoxModels" "%DYNAWO_ALGORITHMS_DEPLOY_DIR%\examples\CS" /i
xcopy "%DYNAWO_ALGORITHMS_HOME%\nrt\data\IEEE14\SA\files" "%DYNAWO_ALGORITHMS_DEPLOY_DIR%\examples\SA" /i
xcopy "%DYNAWO_ALGORITHMS_HOME%\nrt\data\IEEE14\MC\files" "%DYNAWO_ALGORITHMS_DEPLOY_DIR%\examples\MC" /i
exit /B %ERRORLEVEL%


:: create a distribution file
:DISTRIB
if not defined _devmode (
  echo error: 'distrib' commands are only available in developper mode ! 1>&2
  exit /B 1
)
set DYNAWO_ALGORITHMS_DISTRIB_DIR=%~2
if "%~2"=="" set DYNAWO_ALGORITHMS_DISTRIB_DIR=%DYNAWO_ALGORITHMS_HOME%\..\distrib
for %%G in ("%DYNAWO_ALGORITHMS_DISTRIB_DIR%") do set DYNAWO_ALGORITHMS_DISTRIB_DIR=%%~fG
if defined _verbose echo info: using DYNAWO_ALGORITHMS_DISTRIB_DIR=%DYNAWO_ALGORITHMS_DISTRIB_DIR% 1>&2

:: deploy dynawo-algorithms in temporary directory
set _distrib_tmp=%DYNAWO_ALGORITHMS_DISTRIB_DIR%\tmp
if /i %~1==DISTRIB (
  call:DEPLOY deploy-without-headers "%_distrib_tmp%\dynawo-algorithms"
) else (
  call:DEPLOY deploy "%_distrib_tmp%\dynawo-algorithms"
)

:: complete distribution with dynawo files
pushd "%_distrib_tmp%"
xcopy "%DYNAWO_HOME%\bin" dynawo-algorithms\bin /e /i
if /i not %~1==DISTRIB (
  xcopy "%DYNAWO_HOME%\include" dynawo-algorithms\include /e /i
)
xcopy "%DYNAWO_HOME%\lib" dynawo-algorithms\lib /e /i
xcopy "%DYNAWO_HOME%\ddb" dynawo-algorithms\ddb /e /i
echo n|xcopy "%DYNAWO_HOME%\share" dynawo-algorithms\share /e /i
xcopy "%DYNAWO_HOME%\sbin" dynawo-algorithms\sbin /e /i
xcopy "%DYNAWO_HOME%\dynawo.cmd" dynawo-algorithms
if /i %~1==DISTRIB-OMC (
  xcopy "%DYNAWO_HOME%\OpenModelica" dynawo-algorithms\OpenModelica /e /i
)

:: combine dictionaries mapping
find /v "//" < "%DYNAWO_HOME%\share\dictionaries_mapping.dic" | findstr /v "^$">>dynawo-algorithms\share\dictionaries_mapping.dic

:: retrieve version and create zipfile
for /f "tokens=1" %%G in ('"%DYNAWO_ALGORITHMS_INSTALL_DIR%\bin\dynawoAlgorithms" -v') do set _version=%%G
set _distrib_zip=
if /i %~1==DISTRIB (
  set _distrib_zip=%DYNAWO_ALGORITHMS_DISTRIB_DIR%\DynawoAlgorithms_%DYNAWO_BUILD_TYPE%_V%_version%.zip
) else if /i %~1==DISTRIB-HEADERS (
  set _distrib_zip=%DYNAWO_ALGORITHMS_DISTRIB_DIR%\DynawoAlgorithms_headers_%DYNAWO_BUILD_TYPE%_V%_version%.zip
) else if /i %~1==DISTRIB-OMC (
  set _distrib_zip=%DYNAWO_ALGORITHMS_DISTRIB_DIR%\DynawoAlgorithms_omc_%DYNAWO_BUILD_TYPE%_V%_version%.zip
)
if defined _distrib_zip (
  del "%_distrib_zip%" 2>NUL
  tar -a -c -f "%_distrib_zip%" dynawo-algorithms 2>NUL
  if %ERRORLEVEL% neq 0 (
    7z a "%_distrib_zip%" -r dynawo-algorithms 2>NUL
    if %ERRORLEVEL% neq 0 (
      echo error: neither tar.exe nor 7z.exe are available to create distribution archive ! 1>&2
    )
  )
)

popd
rd /s /q "%_distrib_tmp%"
exit /B %ERRORLEVEL%


:: look for dynawo installation
:FIND_DYNAWO
if not defined _dynawo_release if exist "%~dp0%1\deploy\dynawo\share\dynawo-config-release.cmake" set _dynawo_release=%~dp0%1\deploy\dynawo
if not defined _dynawo_debug   if exist "%~dp0%1\deployd\dynawo\share\dynawo-config-debug.cmake"  set _dynawo_debug=%~dp0%1\deployd\dynawo
if not defined _dynawo_debug   if exist "%~dp0%1\deploy\dynawo\share\dynawo-config-debug.cmake"   set _dynawo_debug=%~dp0%1\deploy\dynawo
exit /B 0


:: set dynawo environment variables for runtime
:SET_ENV
if not defined DYNAWO_ALGORITHMS_LOCALE set DYNAWO_ALGORITHMS_LOCALE=en_GB
if defined DYNAWO_ALGORITHMS_INSTALL_DIR set DYNAWO_ALGORITHMS_XSD_DIR=%DYNAWO_ALGORITHMS_INSTALL_DIR%\share\xsd

set DYNAWO_INSTALL_DIR=%DYNAWO_HOME%
set DYNAWO_DDB_DIR=%DYNAWO_HOME%\ddb
set DYNAWO_DICTIONARIES=dictionaries_mapping
set DYNAWO_LOCALE=%DYNAWO_ALGORITHMS_LOCALE%
set DYNAWO_SCRIPTS_DIR=%DYNAWO_HOME%\sbin
if not defined DYNAWO_USE_XSD_VALIDATION set DYNAWO_USE_XSD_VALIDATION=false
set DYNAWO_RESOURCES_DIR=%DYNAWO_HOME%\share
set DYNAWO_XSD_DIR=%DYNAWO_RESOURCES_DIR%\xsd\
set DYNAWO_RESOURCES_DIR=%DYNAWO_RESOURCES_DIR%;%DYNAWO_XSD_DIR%
if defined DYNAWO_ALGORITHMS_INSTALL_DIR if not "%DYNAWO_HOME%"=="%DYNAWO_ALGORITHMS_INSTALL_DIR%" set DYNAWO_RESOURCES_DIR=%DYNAWO_ALGORITHMS_INSTALL_DIR%\share;%DYNAWO_ALGORITHMS_XSD_DIR%;%DYNAWO_RESOURCES_DIR%

set DYNAWO_LIBIIDM_EXTENSIONS=%DYNAWO_HOME%\bin

:: set path for runtime
set PATH=%DYNAWO_DDB_DIR%;%DYNAWO_HOME%\bin;%PATH%
if defined DYNAWO_ALGORITHMS_INSTALL_DIR if not "%DYNAWO_HOME%"=="%DYNAWO_ALGORITHMS_INSTALL_DIR%" set PATH=%DYNAWO_ALGORITHMS_INSTALL_DIR%\bin;%PATH%

exit /B 0
