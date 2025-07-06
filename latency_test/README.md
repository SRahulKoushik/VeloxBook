# WebSocket vs REST Latency Test

This test demonstrates that WebSocket streaming reduces latency compared to REST polling in the VeloxBook trading platform.

## 📁 Files in this Directory

- `websocket_latency_test.py` - Main test script
- `requirements.txt` - Python dependencies
- `run_test.bat` - Windows batch runner
- `run_test.sh` - Unix/Linux shell runner
- `README.md` - This documentation

## 🚀 Quick Start

### Windows
```bash
# Double-click or run from command line
run_test.bat
```

### Linux/macOS
```bash
# Make executable (first time only)
chmod +x run_test.sh

# Run the test
./run_test.sh
```

### Manual Setup
```bash
# Install dependencies
pip install -r requirements.txt

# Run test
python websocket_latency_test.py --iterations 50
```

## 📊 What This Test Does

The latency test compares two approaches for real-time data delivery:

1. **WebSocket Streaming**: Real-time push notifications via WebSocket
2. **REST Polling**: Periodic HTTP requests to check for updates

This test better reflects real-world performance by:
- **Testing continuous updates** (not just single orders)
- **Excluding WebSocket setup overhead**
- **Measuring efficiency over time**

## 📊 Test Types

### **Test 1: Continuous Updates Performance**
- Sends multiple orders simultaneously
- Measures time to receive all updates
- More realistic for trading scenarios

### **Test 2: Efficiency Over Time**
- Tests sustained performance over time
- Measures consistency and reliability
- Better reflects production usage

## 🎯 Expected Results

- **WebSocket**: ~0-5ms latency (depends on update frequency)
- **REST Polling**: ~100-200ms latency (depends on polling interval)
- **Improvement**: WebSocket shows significant advantage in continuous scenarios

## ⚙️ Test Parameters

- `--host`: Server hostname (default: localhost)
- `--port`: Server port (default: 18080)
- `--iterations`: Number of test iterations (default: 50)
- `--no-plot`: Skip visualization generation

## 📈 Output

The test generates:

1. **Console Output**: Detailed statistics for both test types
2. **Visualization**: `improved_latency_comparison.png` showing:
   - Box plots for both test types
   - Bar charts comparing mean latencies
   - Comprehensive performance analysis

## 🔧 Prerequisites

- Python 3.7+ installed
- VeloxBook backend running on `localhost:18080`
- Required Python packages (auto-installed via requirements.txt):
  - `websockets>=10.0`
  - `aiohttp>=3.8.0`
  - `matplotlib>=3.5.0`
  - `numpy>=1.21.0`

## 📋 Sample Output

```
============================================================
IMPROVED LATENCY COMPARISON RESULTS
============================================================

📊 Test 1: Continuous Updates Performance

WebSocket Streaming (Continuous):
  Mean latency:    0.07ms
  Median latency:  0.00ms
  Min latency:     0.00ms
  Max latency:     1.55ms
  Std deviation:   0.27ms
  Samples:         50

REST Polling (Continuous):
  Mean latency:    184.95ms
  Median latency:  186.39ms
  Min latency:     168.65ms
  Max latency:     197.05ms
  Std deviation:   5.89ms
  Samples:         50

Improvement (Continuous):
  Latency reduction: 184.88ms (100.0%)

📊 Test 2: Efficiency Over Time

WebSocket Efficiency:
  Mean latency:    0.17ms
  Median latency:  0.00ms
  Samples:         25

REST Polling Efficiency:
  Mean latency:    119.29ms
  Median latency:  118.40ms
  Samples:         25

Efficiency Improvement:
  Latency reduction: 119.12ms (99.9%)

🎯 Overall Assessment:
  ✅ WebSocket shows significant advantage in at least one test
  💡 WebSocket excels at continuous updates and real-time streaming
  💡 REST polling is efficient for single queries
```

## 🔬 How It Works

1. **Continuous Updates Test**: 
   - Sends multiple orders to create market activity
   - Measures time to receive all WebSocket updates
   - Compares with REST polling for same updates

2. **Efficiency Test**:
   - Tests sustained performance over time
   - Measures consistency of both approaches
   - Simulates real-world trading scenarios

3. **Improved Analysis**:
   - Separates setup overhead from update latency
   - Provides more realistic performance metrics
   - Better reflects actual trading platform usage

## 🛠️ Technical Details

- Uses `asyncio` for concurrent testing
- Implements realistic market simulation
- Handles multiple updates per iteration
- Provides comprehensive statistical analysis
- Creates detailed visualizations

## 🚨 Troubleshooting

### "Connection refused" errors
- Ensure VeloxBook backend is running
- Check host/port settings
- Verify firewall settings

### "Module not found" errors
- Run: `pip install -r requirements.txt`
- Or manually: `pip install websockets aiohttp matplotlib numpy`

### Low improvement values
- Check network latency
- Ensure backend is not overloaded
- Try increasing iterations for more accurate results

## 📊 Performance Implications

The test demonstrates:

- **Real-time trading**: WebSocket advantage in continuous updates
- **System efficiency**: Reduced server load with WebSocket
- **User experience**: Faster response times for market data
- **Scalability**: WebSocket handles multiple updates efficiently

## 🔄 Customization

### Adjust Test Parameters
Edit `websocket_latency_test.py`:
```python
# Change number of orders per iteration
for order in self.test_orders[:3]:  # Change 3 to desired number

# Adjust polling interval
poll_interval = 0.05  # Change to 0.1 (100ms), 0.2 (200ms), etc.
```

### Modify Test Scenarios
- Change order types and quantities
- Adjust update frequency
- Modify test duration

## 📝 Notes

- Provides realistic performance metrics
- Results depend on system performance and network conditions
- WebSocket advantage is more apparent in continuous scenarios
- The test better reflects production trading environments 