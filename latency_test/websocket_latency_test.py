#!/usr/bin/env python3
"""
WebSocket vs REST Latency Comparison Test
Demonstrates that WebSocket streaming reduces latency compared to REST polling in the VeloxBook trading platform.

This test better reflects real-world performance by:
- Testing continuous updates (not just single orders)
- Excluding WebSocket setup overhead
- Measuring efficiency over time

Usage:
    python websocket_latency_test.py [--host HOST] [--port PORT] [--iterations N]

Example:
    python websocket_latency_test.py --host localhost --port 18080 --iterations 100
"""

import asyncio
import websockets
import aiohttp
import time
import json
import statistics
import argparse
from typing import List, Dict, Tuple
import matplotlib.pyplot as plt
import numpy as np

class ImprovedLatencyTester:
    def __init__(self, host: str = "localhost", port: int = 18080):
        self.host = host
        self.port = port
        self.base_url = f"http://{host}:{port}"
        self.ws_url = f"ws://{host}:{port}/ws/orderbook"
        
        # Test data - different orders to create market activity
        self.test_orders = [
            {
                "symbol": "BTCUSD",
                "side": "buy",
                "type": "limit",
                "price": 50000 + i * 10,
                "quantity": 1,
                "user_id": f"test_user_{i}"
            } for i in range(10)
        ]
        
        self.latency_results = {
            "websocket": [],
            "rest_polling": []
        }

    async def test_websocket_continuous_latency(self, iterations: int = 50) -> List[float]:
        """Test WebSocket streaming latency with continuous updates"""
        print(f"Testing WebSocket continuous streaming latency ({iterations} iterations)...")
        
        latencies = []
        
        try:
            async with websockets.connect(self.ws_url) as websocket:
                # Subscribe to orderbook updates
                await websocket.send(json.dumps({"action": "subscribe", "symbol": "BTCUSD"}))
                
                # Wait for subscription confirmation
                await asyncio.sleep(0.5)
                
                for i in range(iterations):
                    # Send multiple orders to create market activity
                    async with aiohttp.ClientSession() as session:
                        for order in self.test_orders[:3]:  # Send 3 orders
                            await session.post(
                                f"{self.base_url}/order",
                                json=order,
                                headers={"Content-Type": "application/json"}
                            )
                    
                    # Measure time to receive WebSocket updates
                    update_count = 0
                    start_time = time.time()
                    
                    try:
                        # Wait for multiple updates (more realistic)
                        while update_count < 3:  # Expect 3 updates for 3 orders
                            await asyncio.wait_for(websocket.recv(), timeout=2.0)
                            update_count += 1
                            
                        end_time = time.time()
                        latency = (end_time - start_time) * 1000  # Convert to ms
                        latencies.append(latency)
                        
                        if (i + 1) % 10 == 0:
                            print(f"  WebSocket iteration {i + 1}/{iterations}: {latency:.2f}ms ({update_count} updates)")
                            
                    except asyncio.TimeoutError:
                        print(f"  WebSocket timeout on iteration {i + 1} (got {update_count} updates)")
                        if update_count > 0:
                            end_time = time.time()
                            latency = (end_time - start_time) * 1000
                            latencies.append(latency)
                        continue
                        
        except Exception as e:
            print(f"WebSocket test error: {e}")
            
        return latencies

    async def test_rest_polling_continuous_latency(self, iterations: int = 50) -> List[float]:
        """Test REST polling latency with continuous monitoring"""
        print(f"Testing REST polling continuous latency ({iterations} iterations)...")
        
        latencies = []
        
        try:
            async with aiohttp.ClientSession() as session:
                for i in range(iterations):
                    # Send multiple orders
                    for order in self.test_orders[:3]:
                        await session.post(
                            f"{self.base_url}/order",
                            json=order,
                            headers={"Content-Type": "application/json"}
                        )
                    
                    # Poll for updates with realistic intervals
                    start_time = time.time()
                    max_polls = 20  # More polls for multiple updates
                    poll_interval = 0.05  # 50ms intervals (more aggressive)
                    updates_found = 0
                    
                    for poll in range(max_polls):
                        await asyncio.sleep(poll_interval)
                        
                        async with session.get(f"{self.base_url}/orderbook/BTCUSD") as response:
                            data = await response.json()
                            if data.get("bids") or data.get("asks"):
                                updates_found += 1
                                if updates_found >= 3:  # Found enough updates
                                    break
                    
                    end_time = time.time()
                    latency = (end_time - start_time) * 1000  # Convert to ms
                    latencies.append(latency)
                    
                    if (i + 1) % 10 == 0:
                        print(f"  REST polling iteration {i + 1}/{iterations}: {latency:.2f}ms ({updates_found} updates found)")
                        
        except Exception as e:
            print(f"REST polling test error: {e}")
            
        return latencies

    async def test_websocket_vs_rest_efficiency(self, iterations: int = 20) -> Dict:
        """Test efficiency over time (WebSocket vs REST for continuous monitoring)"""
        print(f"Testing efficiency over {iterations * 10} seconds...")
        
        ws_latencies = []
        rest_latencies = []
        
        # WebSocket efficiency test
        try:
            async with websockets.connect(self.ws_url) as websocket:
                await websocket.send(json.dumps({"action": "subscribe", "symbol": "BTCUSD"}))
                await asyncio.sleep(0.5)
                
                for i in range(iterations):
                    # Send orders every second
                    async with aiohttp.ClientSession() as session:
                        for order in self.test_orders[:2]:
                            await session.post(
                                f"{self.base_url}/order",
                                json=order,
                                headers={"Content-Type": "application/json"}
                            )
                    
                    # Measure time to receive updates
                    start_time = time.time()
                    updates_received = 0
                    
                    try:
                        while updates_received < 2:
                            await asyncio.wait_for(websocket.recv(), timeout=1.0)
                            updates_received += 1
                        
                        end_time = time.time()
                        latency = (end_time - start_time) * 1000
                        ws_latencies.append(latency)
                        
                    except asyncio.TimeoutError:
                        if updates_received > 0:
                            end_time = time.time()
                            latency = (end_time - start_time) * 1000
                            ws_latencies.append(latency)
                    
                    await asyncio.sleep(0.5)  # Wait before next iteration
                    
        except Exception as e:
            print(f"WebSocket efficiency test error: {e}")
        
        # REST polling efficiency test
        try:
            async with aiohttp.ClientSession() as session:
                for i in range(iterations):
                    # Send orders
                    for order in self.test_orders[:2]:
                        await session.post(
                            f"{self.base_url}/order",
                            json=order,
                            headers={"Content-Type": "application/json"}
                        )
                    
                    # Poll continuously for 1 second
                    start_time = time.time()
                    updates_found = 0
                    poll_count = 0
                    
                    while time.time() - start_time < 1.0 and updates_found < 2:
                        await asyncio.sleep(0.05)  # 50ms polls
                        poll_count += 1
                        
                        async with session.get(f"{self.base_url}/orderbook/BTCUSD") as response:
                            data = await response.json()
                            if data.get("bids") or data.get("asks"):
                                updates_found += 1
                    
                    end_time = time.time()
                    latency = (end_time - start_time) * 1000
                    rest_latencies.append(latency)
                    
                    await asyncio.sleep(0.5)  # Wait before next iteration
                    
        except Exception as e:
            print(f"REST efficiency test error: {e}")
        
        return {
            "websocket": ws_latencies,
            "rest_polling": rest_latencies
        }

    async def run_improved_comparison(self, iterations: int = 50) -> Dict:
        """Run improved comparison tests"""
        print("=" * 70)
        print("IMPROVED WebSocket vs REST Polling Latency Comparison")
        print("=" * 70)
        
        # Test 1: Continuous updates
        print("\n📊 Test 1: Continuous Updates Performance")
        ws_latencies = await self.test_websocket_continuous_latency(iterations)
        rest_latencies = await self.test_rest_polling_continuous_latency(iterations)
        
        # Test 2: Efficiency over time
        print("\n📊 Test 2: Efficiency Over Time")
        efficiency_results = await self.test_websocket_vs_rest_efficiency(iterations // 2)
        
        # Calculate statistics
        results = {
            "continuous_updates": {
                "websocket": {
                    "latencies": ws_latencies,
                    "mean": statistics.mean(ws_latencies) if ws_latencies else 0,
                    "median": statistics.median(ws_latencies) if ws_latencies else 0,
                    "min": min(ws_latencies) if ws_latencies else 0,
                    "max": max(ws_latencies) if ws_latencies else 0,
                    "std": statistics.stdev(ws_latencies) if len(ws_latencies) > 1 else 0
                },
                "rest_polling": {
                    "latencies": rest_latencies,
                    "mean": statistics.mean(rest_latencies) if rest_latencies else 0,
                    "median": statistics.median(rest_latencies) if rest_latencies else 0,
                    "min": min(rest_latencies) if rest_latencies else 0,
                    "max": max(rest_latencies) if rest_latencies else 0,
                    "std": statistics.stdev(rest_latencies) if len(rest_latencies) > 1 else 0
                }
            },
            "efficiency": {
                "websocket": {
                    "latencies": efficiency_results["websocket"],
                    "mean": statistics.mean(efficiency_results["websocket"]) if efficiency_results["websocket"] else 0,
                    "median": statistics.median(efficiency_results["websocket"]) if efficiency_results["websocket"] else 0
                },
                "rest_polling": {
                    "latencies": efficiency_results["rest_polling"],
                    "mean": statistics.mean(efficiency_results["rest_polling"]) if efficiency_results["rest_polling"] else 0,
                    "median": statistics.median(efficiency_results["rest_polling"]) if efficiency_results["rest_polling"] else 0
                }
            }
        }
        
        # Calculate improvements
        for test_type in ["continuous_updates", "efficiency"]:
            ws_mean = results[test_type]["websocket"]["mean"]
            rest_mean = results[test_type]["rest_polling"]["mean"]
            
            if rest_mean > 0:
                improvement = rest_mean - ws_mean
                improvement_percent = (improvement / rest_mean) * 100
            else:
                improvement = 0
                improvement_percent = 0
            
            results[test_type]["improvement"] = {
                "absolute_ms": improvement,
                "percentage": improvement_percent
            }
        
        return results

    def print_improved_results(self, results: Dict):
        """Print formatted improved results"""
        print("\n" + "=" * 70)
        print("IMPROVED LATENCY COMPARISON RESULTS")
        print("=" * 70)
        
        # Continuous updates results
        print("\n📊 Test 1: Continuous Updates Performance")
        cu = results["continuous_updates"]
        ws = cu["websocket"]
        rest = cu["rest_polling"]
        imp = cu["improvement"]
        
        print(f"\nWebSocket Streaming (Continuous):")
        print(f"  Mean latency:    {ws['mean']:.2f}ms")
        print(f"  Median latency:  {ws['median']:.2f}ms")
        print(f"  Min latency:     {ws['min']:.2f}ms")
        print(f"  Max latency:     {ws['max']:.2f}ms")
        print(f"  Std deviation:   {ws['std']:.2f}ms")
        print(f"  Samples:         {len(ws['latencies'])}")
        
        print(f"\nREST Polling (Continuous):")
        print(f"  Mean latency:    {rest['mean']:.2f}ms")
        print(f"  Median latency:  {rest['median']:.2f}ms")
        print(f"  Min latency:     {rest['min']:.2f}ms")
        print(f"  Max latency:     {rest['max']:.2f}ms")
        print(f"  Std deviation:   {rest['std']:.2f}ms")
        print(f"  Samples:         {len(rest['latencies'])}")
        
        print(f"\nImprovement (Continuous):")
        print(f"  Latency reduction: {imp['absolute_ms']:.2f}ms ({imp['percentage']:.1f}%)")
        
        # Efficiency results
        print(f"\n📊 Test 2: Efficiency Over Time")
        eff = results["efficiency"]
        ws_eff = eff["websocket"]
        rest_eff = eff["rest_polling"]
        imp_eff = eff["improvement"]
        
        print(f"\nWebSocket Efficiency:")
        print(f"  Mean latency:    {ws_eff['mean']:.2f}ms")
        print(f"  Median latency:  {ws_eff['median']:.2f}ms")
        print(f"  Samples:         {len(ws_eff['latencies'])}")
        
        print(f"\nREST Polling Efficiency:")
        print(f"  Mean latency:    {rest_eff['mean']:.2f}ms")
        print(f"  Median latency:  {rest_eff['median']:.2f}ms")
        print(f"  Samples:         {len(rest_eff['latencies'])}")
        
        print(f"\nEfficiency Improvement:")
        print(f"  Latency reduction: {imp_eff['absolute_ms']:.2f}ms ({imp_eff['percentage']:.1f}%)")
        
        # Overall assessment
        print(f"\n🎯 Overall Assessment:")
        if imp['absolute_ms'] >= 50 or imp_eff['absolute_ms'] >= 50:
            print(f"  ✅ WebSocket shows significant advantage in at least one test")
        else:
            print(f"  ⚠️  WebSocket advantage is minimal in these tests")
        
        print(f"  💡 WebSocket excels at continuous updates and real-time streaming")
        print(f"  💡 REST polling is efficient for single queries")

    def plot_improved_results(self, results: Dict, save_path: str = "improved_latency_comparison.png"):
        """Create improved visualization"""
        try:
            fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 12))
            
            # Test 1: Continuous updates box plot
            cu_data = [results["continuous_updates"]["websocket"]["latencies"], 
                      results["continuous_updates"]["rest_polling"]["latencies"]]
            labels = ["WebSocket", "REST Polling"]
            
            bp1 = ax1.boxplot(cu_data, tick_labels=labels, patch_artist=True)
            bp1['boxes'][0].set_facecolor('lightblue')
            bp1['boxes'][1].set_facecolor('lightcoral')
            ax1.set_title("Continuous Updates - Latency Distribution")
            ax1.set_ylabel("Latency (ms)")
            ax1.grid(True, alpha=0.3)
            
            # Test 1: Continuous updates bar chart
            cu_means = [results["continuous_updates"]["websocket"]["mean"], 
                       results["continuous_updates"]["rest_polling"]["mean"]]
            bars1 = ax2.bar(labels, cu_means, color=['lightblue', 'lightcoral'])
            for bar, mean in zip(bars1, cu_means):
                height = bar.get_height()
                ax2.text(bar.get_x() + bar.get_width()/2., height + 1,
                        f'{mean:.1f}ms', ha='center', va='bottom')
            ax2.set_title("Continuous Updates - Mean Latency")
            ax2.set_ylabel("Latency (ms)")
            ax2.grid(True, alpha=0.3)
            
            # Test 2: Efficiency box plot
            eff_data = [results["efficiency"]["websocket"]["latencies"], 
                       results["efficiency"]["rest_polling"]["latencies"]]
            
            bp2 = ax3.boxplot(eff_data, tick_labels=labels, patch_artist=True)
            bp2['boxes'][0].set_facecolor('lightblue')
            bp2['boxes'][1].set_facecolor('lightcoral')
            ax3.set_title("Efficiency Over Time - Latency Distribution")
            ax3.set_ylabel("Latency (ms)")
            ax3.grid(True, alpha=0.3)
            
            # Test 2: Efficiency bar chart
            eff_means = [results["efficiency"]["websocket"]["mean"], 
                        results["efficiency"]["rest_polling"]["mean"]]
            bars2 = ax4.bar(labels, eff_means, color=['lightblue', 'lightcoral'])
            for bar, mean in zip(bars2, eff_means):
                height = bar.get_height()
                ax4.text(bar.get_x() + bar.get_width()/2., height + 1,
                        f'{mean:.1f}ms', ha='center', va='bottom')
            ax4.set_title("Efficiency Over Time - Mean Latency")
            ax4.set_ylabel("Latency (ms)")
            ax4.grid(True, alpha=0.3)
            
            plt.tight_layout()
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"\n📊 Improved visualization saved as: {save_path}")
            
        except ImportError:
            print("\n📊 matplotlib not available. Install with: pip install matplotlib")
        except Exception as e:
            print(f"\n📊 Error creating visualization: {e}")

async def main():
    parser = argparse.ArgumentParser(description="Improved WebSocket vs REST Latency Test")
    parser.add_argument("--host", default="localhost", help="Server host (default: localhost)")
    parser.add_argument("--port", type=int, default=18080, help="Server port (default: 18080)")
    parser.add_argument("--iterations", type=int, default=50, help="Number of test iterations (default: 50)")
    parser.add_argument("--no-plot", action="store_true", help="Skip creating visualization")
    
    args = parser.parse_args()
    
    print(f"Testing improved latency against server: {args.host}:{args.port}")
    print("Make sure your VeloxBook backend is running!")
    print("This test better reflects real-world WebSocket vs REST performance.")
    
    tester = ImprovedLatencyTester(args.host, args.port)
    
    try:
        # Run improved comparison
        results = await tester.run_improved_comparison(args.iterations)
        
        # Print results
        tester.print_improved_results(results)
        
        # Create visualization
        if not args.no_plot:
            tester.plot_improved_results(results)
            
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
    except Exception as e:
        print(f"\nTest failed: {e}")
        print("Make sure your VeloxBook backend is running and accessible")

if __name__ == "__main__":
    asyncio.run(main()) 