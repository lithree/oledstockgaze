import os
import finnhub
import subprocess

def run_bridge(ticker="AAPL"):
    # Retrieve the API key from environment variables
    api_key = os.environ.get('FINNHUB_API_KEY')
    
    if not api_key:
        print("Error: FINNHUB_API_KEY environment variable not set.")
        return

    print(f"Starting Finnhub bridge for ticker: {ticker}")

    try:
        # 1. Get ticker price from Finnhub
        finnhub_client = finnhub.Client(api_key=api_key)
        quote = finnhub_client.quote(ticker)
        price = quote.get('c')

        if price is None or price == 0:
            print(f"Could not retrieve price for {ticker}. Check symbol or API limits.")
            return

        print(f"Current Price: {price}")

        # 2. Call handler.exe and 3. Send price as an argument
        # Assuming handler.exe is in the same directory as this script
        exe_path = "./handler.exe" 
        
        print(f"Calling {exe_path} with price...")
        result = subprocess.run(
            [exe_path, str(price)],
            capture_output=True,
            text=True
        )

        # Print what the C program returns
        if result.stdout:
            print(f"EXE Output: {result.stdout}")
        if result.stderr:
            print(f"EXE Error: {result.stderr}")

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    run_bridge("AAPL")