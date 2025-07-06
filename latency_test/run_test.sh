#!/bin/bash

echo "========================================"
echo "WebSocket vs REST Latency Test Runner"
echo "========================================"
echo
echo "This test better reflects real-world performance by:"
echo "- Testing continuous updates (not just single orders)"
echo "- Excluding WebSocket setup overhead"
echo "- Measuring efficiency over time"
echo
echo "Make sure your VeloxBook backend is running on localhost:18080"
echo

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo "ERROR: Python 3 is not installed or not in PATH"
    echo "Please install Python 3.7+ and try again"
    exit 1
fi

# Check if required packages are installed
echo "Checking required packages..."
python3 -c "import websockets, aiohttp, matplotlib" &> /dev/null
if [ $? -ne 0 ]; then
    echo "Installing required packages from requirements.txt..."
    pip3 install -r requirements.txt
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to install required packages"
        exit 1
    fi
fi

echo
echo "Starting latency test..."
echo "This will take about 3-5 minutes for comprehensive testing..."
echo

# Run the test
python3 websocket_latency_test.py --iterations 50

echo
echo "Test completed! Check the generated improved_latency_comparison.png for visualization." 