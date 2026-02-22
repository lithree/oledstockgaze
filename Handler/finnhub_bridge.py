import os
import finnhub
import subprocess
import time

def run_bridge(ticker="AAPL"):
    api_key = os.environ.get('FINNHUB_API_KEY')
    
    if not api_key:
        print("Error: FINNHUB_API_KEY environment variable not set.")
        return

    print(f"Starting Finnhub bridge for ticker: {ticker}")

    exe_path = "./handler.exe"
    process = None

    try:
        # Start handler.exe as a subprocess
        print(f"Launching {exe_path} for continuous communication...")
        process = subprocess.Popen(
            [exe_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, # Capture stdout for potential future use or debugging
            stderr=subprocess.PIPE, # Capture stderr for messages from Handler.c
            text=True  # Treat stdin/stdout/stderr as text
        )
        
        finnhub_client = finnhub.Client(api_key=api_key)

        while True:
            # 1. Get ticker price from Finnhub
            quote = finnhub_client.quote(ticker)
            price = quote.get('c')

            if price is None or price == 0:
                print(f"Could not retrieve price for {ticker}. Check symbol or API limits.")
            else:
                message = str(price)
                print(f"Current Price: {price}. Sending to handler.exe...")
                
                # Send the message to the C program's stdin
                process.stdin.write(message + '\n')
                process.stdin.flush()
                
            # Read any output from stderr (where Handler.c prints its messages)
            # This is non-blocking and will only read what's available.
            while True:
                line = process.stderr.readline()
                if line:
                    print(f"[HANDLER_ERR]: {line.strip()}")
                else:
                    break # No more lines to read for now

            time.sleep(5) # Wait for 5 second before the next update

    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        if process:
            print("Terminating handler.exe process.")
            process.stdin.close()
            process.terminate()
            process.wait()
            # Print any remaining stderr output from the C program
            stderr_output = process.stderr.read()
            if stderr_output:
                print(f"[HANDLER_FINAL_ERR]: {stderr_output.strip()}")
            stdout_output = process.stdout.read()
            if stdout_output:
                print(f"[HANDLER_FINAL_OUT]: {stdout_output.strip()}")

if __name__ == "__main__":
    run_bridge("AAPL")