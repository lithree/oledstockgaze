import os
import json
import subprocess
import time
import websocket
import threading
import requests

# Global variables to share state between the WebSocket thread and the Main thread
latest_data = {
    "price": 0.0,
    "symbol": "AMD",
    "opening_price": 0.0
}
process = None

def on_message(ws, message):
    global latest_data
    try:
        data = json.loads(message)
        if data.get('type') == 'trade':
            # Continuously overwrite the global variable with the newest tick
            latest_data["symbol"] = data['data'][0]['s']
            latest_data["price"] = data['data'][0]['p']
        elif data.get('type') == 'ping':
            pass
    except Exception as e:
        print(f"Message parsing error: {e}")

def on_error(ws, error):
    print(f"WebSocket Error: {error}")

def on_close(ws, close_status_code, close_msg):
    print("WebSocket connection closed.")

def on_open(ws):
    print(f"WebSocket connection opened. Subscribing to {latest_data['symbol']}...")
    subscribe_message = {"type": "subscribe", "symbol": latest_data["symbol"]}
    ws.send(json.dumps(subscribe_message))

def websocket_thread_worker(api_key, ticker):
    """Background thread function to maintain the WebSocket connection."""
    ws_url = f"wss://ws.finnhub.io?token={api_key}"
    websocket.enableTrace(False)
    
    # Automatic Reconnection Loop for the background thread
    while True:
        ws = websocket.WebSocketApp(ws_url,
                                  on_message=on_message,
                                  on_error=on_error,
                                  on_close=on_close,
                                  on_open=on_open)
        
        ws.run_forever(ping_interval=30, ping_timeout=10)
        time.sleep(5) # Wait before reconnecting

def fetch_opening_price(api_key, ticker):
    """Fetch the opening price from Finnhub REST API."""
    try:
        url = f"https://finnhub.io/api/v1/quote?symbol={ticker}&token={api_key}"
        response = requests.get(url, timeout=5)
        data = response.json()
        opening_price = data.get('o', 0.0)  # 'o' is the opening price
        if opening_price > 0:
            print(f"Fetched opening price for {ticker}: ${opening_price}")
            return opening_price
        else:
            print(f"Warning: Could not fetch opening price for {ticker}, will use first price received")
            return 0.0
    except Exception as e:
        print(f"Error fetching opening price: {e}")
        return 0.0

def run_bridge(ticker="AMD", update_interval_seconds=1.0):
    global latest_data, process
    api_key = os.environ.get('FINNHUB_API_KEY')
    
    if not api_key:
        print("Error: FINNHUB_API_KEY environment variable not set.")
        return

    latest_data["symbol"] = ticker
    
    # Fetch the opening price from Finnhub REST API
    opening_price = fetch_opening_price(api_key, ticker)
    latest_data["opening_price"] = opening_price
    
    print(f"Starting Proportional Time Bridge for {ticker}. Update interval: {update_interval_seconds}s")
    
    exe_path = "./handler.exe"

    try:
        print(f"Launching {exe_path} for continuous communication...")
        process = subprocess.Popen(
            [exe_path],
            stdin=subprocess.PIPE,
            text=True
        )
        
        time.sleep(0.5)
        if process.poll() is not None:
            print(f"Error: handler.exe terminated immediately with exit code {process.returncode}")
            return

        # Send opening price as first data point if available
        if opening_price > 0:
            try:
                msg = f"{ticker}: {opening_price}"
                print(f"Sending opening price -> {msg}")
                process.stdin.write(msg + '\n')
                process.stdin.flush()
                time.sleep(0.5)
            except OSError as e:
                print(f"Error sending opening price: {e}")

        # Start the WebSocket client in a separate background daemon thread
        ws_thread = threading.Thread(target=websocket_thread_worker, args=(api_key, ticker), daemon=True)
        ws_thread.start()

        # Main Thread: Time-proportional sampling loop
        while process.poll() is None:
            # Only send data if we have received at least one valid price from the WebSocket
            if latest_data["price"] > 0.0:
                msg = f"{latest_data['symbol']}: {latest_data['price']}"
                print(f"Sampling -> {msg}")
                
                try:
                    process.stdin.write(msg + '\n')
                    process.stdin.flush()
                except OSError as e:
                    print(f"Error writing to handler.exe stdin: {e}")
                    break
            
            # The script pauses here, ensuring the MCU receives a data frame
            # at exactly the specified interval, making the OLED X-axis proportional.
            time.sleep(update_interval_seconds)

    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        if process and process.poll() is None:
            print("Terminating handler.exe process.")
            try:
                process.stdin.close()
            except OSError as e:
                print(f"Error closing stdin: {e}")
            process.terminate()
            process.wait(timeout=5)

if __name__ == "__main__":
    # The second parameter controls the chart speed. 
    # 1.0 means 1 pixel on your OLED equals exactly 1 second.
    run_bridge("AMD", 2.0)