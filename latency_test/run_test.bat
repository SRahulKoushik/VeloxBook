@echo off
echo ========================================
echo WebSocket vs REST Latency Test
echo ========================================
echo.
echo This test better reflects real-world performance by:
echo - Testing continuous updates (not just single orders)
echo - Excluding WebSocket setup overhead
echo - Measuring efficiency over time
echo.
echo Make sure the VeloxBook backend is running on localhost:18080
echo.

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python 3.7+ and try again
    pause
    exit /b 1
)

REM Check if required packages are installed
echo Checking required packages...
python -c "import websockets, aiohttp, matplotlib" >nul 2>&1
if errorlevel 1 (
    echo Installing required packages from requirements.txt...
    pip install -r requirements.txt
    if errorlevel 1 (
        echo ERROR: Failed to install required packages
        pause
        exit /b 1
    )
)

echo.
echo Starting latency test...
echo This will take about 3-5 minutes for comprehensive testing...
echo.

REM Run the test
python websocket_latency_test.py --iterations 50

echo.
echo Test completed! Check the generated improved_latency_comparison.png for visualization.
pause 