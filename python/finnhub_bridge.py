import requests
import os
import subprocess
import sys

# 1. Get ticker price from Finnhub API
def get_ticker_price(symbol="AAPL"):
    """
    Fetches the current price for a given stock ticker symbol from Finnhub API.
    Requires FINNHUB_API_KEY environment variable to be set.
    """
    finnhub_api_key = os.getenv("d6as4hpr01qnr27in8g0d6as4hpr01qnr27in8gg")
    if not finnhub_api_key:
        print("Error: FINNHUB_API_KEY environment variable not set.", file=sys.stderr)
        return None

    base_url = "https://finnhub.io/api/v1"
    quote_url = f"{base_url}/quote?symbol={symbol}&token={finnhub_api_key}"

    try:
        response = requests.get(quote_url)
        response.raise_for_status()  # Raise an exception for HTTP errors (4xx or 5xx)
        data = response.json()

        # 'c' key holds the current price in Finnhub's quote response
        current_price = data.get('c')
        if current_price is not None:
            print(f"Successfully fetched price for {symbol}: {current_price}")
            return current_price
        else:
            print(f"Error: Could not find current price ('c' key) for {symbol} in Finnhub response. Response: {data}", file=sys.stderr)
            return None
    except requests.exceptions.RequestException as e:
        print(f"Error fetching data from Finnhub API: {e}", file=sys.stderr)
        return None
    except ValueError as e:
        print(f"Error parsing JSON response from Finnhub API: {e}", file=sys.stderr)
        return None

# 2. Call up handler.exe and 3. send ticker price to handler.exe
def send_price_to_handler(price):
    """
    Calls handler.exe and sends the ticker price as a command-line argument.
    Assumes handler.exe is in the current working directory or accessible via PATH.
    """
    if price is None:
        print("No price to send to handler.exe. Skipping handler execution.", file=sys.stderr)
        return

    # Adjust handler_path if handler.exe is located elsewhere.
    # For example: "../obj/handler.exe" or a full path.
    # Based on the file list, handler.exe would likely be generated in 'obj/'
    # So, let's assume it might be 'obj/handler.exe'
    handler_path = "obj/SPI_LCD.elf" # Using SPI_LCD.elf as handler.exe is not explicitly listed, but .elf is a common executable output.
                                    # If handler.exe is a separate program, adjust this path.
    # Note: On Windows, it would be 'obj/SPI_LCD.exe' if compiled for Windows.
    # On Linux/WSL, it would typically be 'obj/SPI_LCD.elf' if it's an ELF executable.

    # Ensure the price is a string for command-line argument
    price_str = str(price)

    try:
        print(f"Attempting to execute '{handler_path}' with argument: {price_str}")
        # Pass the price as a string argument to handler.exe
        # capture_output=True captures stdout and stderr
        # text=True decodes stdout/stderr as text
        # check=True raises CalledProcessError if the command returns a non-zero exit code
        result = subprocess.run([handler_path, price_str], capture_output=True, text=True, check=True)

        print(f"'{handler_path}' executed successfully.")
        if result.stdout:
            print(f"'{handler_path}' stdout:\n{result.stdout}")
        if result.stderr:
            print(f"'{handler_path}' stderr:\n{result.stderr}")

    except FileNotFoundError:
        print(f"Error: Executable '{handler_path}' not found.", file=sys.stderr)
        print("Please ensure it's compiled and placed in the correct path or accessible via system PATH.", file=sys.stderr)
    except subprocess.CalledProcessError as e:
        print(f"Error executing '{handler_path}'. Command returned non-zero exit code {e.returncode}.", file=sys.stderr)
        print(f"Command: {e.cmd}", file=sys.stderr)
        print(f"'{handler_path}' stdout:\n{e.stdout}", file=sys.stderr)
        print(f"'{handler_path}' stderr:\n{e.stderr}", file=sys.stderr)
    except Exception as e:
        print(f"An unexpected error occurred while running '{handler_path}': {e}", file=sys.stderr)

if __name__ == "__main__":
    # Example usage:
    ticker_symbol = "AAPL" # You can change this to any stock ticker symbol, e.g., "MSFT", "GOOG"
    print(f"Starting Finnhub bridge for ticker: {ticker_symbol}")
    
    # Step 1: Get ticker price
    price = get_ticker_price(ticker_symbol)

    # Steps 2 & 3: Call handler.exe and send price
    if price is not None:
        send_price_to_handler(price)
    else:
        print("Failed to retrieve ticker price. 'handler.exe' not invoked.", file=sys.stderr)
