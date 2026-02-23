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
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Give the C program a moment to initialize and print any startup errors
        time.sleep(0.5)
        if process.poll() is not None:
            print(f"Error: handler.exe terminated immediately with exit code {process.returncode}")
            stderr_output = process.stderr.read()
            if stderr_output:
                print(f"[HANDLER_STARTUP_ERR]: {stderr_output.strip()}")
            return

        finnhub_client = finnhub.Client(api_key=api_key)

        while True:
            # Check if the C process is still alive before trying to write
            if process.poll() is not None:
                print(f"Error: handler.exe terminated unexpectedly with exit code {process.returncode}")
                stderr_output = process.stderr.read()
                if stderr_output:
                    print(f"[HANDLER_RUNTIME_ERR]: {stderr_output.strip()}")
                break # Exit the loop as handler.exe is gone

            # 1. Get ticker price from Finnhub
            quote = finnhub_client.quote(ticker)
            price = quote.get('c')

            if price is None or price == 0:
                print(f"Could not retrieve price for {ticker}. Check symbol or API limits.")
            else:
                message = f"{ticker}: {price}"
                print(f"Current Price: {price}. Sending to handler.exe...")
                
                try:
                    process.stdin.write(message + '\n')
                    process.stdin.flush()
                except OSError as e:
                    print(f"Error writing to handler.exe stdin: {e}")
                    if process.poll() is not None:
                        print(f"handler.exe status: Terminated with exit code {process.returncode}")
                    break # Exit the loop if we can't write
                
            # Read any output from stderr (where Handler.c prints its messages)
            # This is non-blocking and will only read what's available.
            while True:
                line = process.stderr.readline()
                if line:
                    print(f"[HANDLER_ERR]: {line.strip()}")
                else:
                    break # No more lines to read for now

            time.sleep(1) # Wait for 1 second before the next update

    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        if process and process.poll() is None:
            print("Terminating handler.exe process.")
            try:
                process.stdin.close()
            except OSError as e:
                print(f"Error closing stdin of handler.exe: {e}")
            process.terminate()
            process.wait(timeout=5) # Wait for a bit for clean termination
            
            # Print any remaining stderr output from the C program
            stderr_output = process.stderr.read()
            if stderr_output:
                print(f"[HANDLER_FINAL_ERR]: {stderr_output.strip()}")
            stdout_output = process.stdout.read()
            if stdout_output:
                print(f"[HANDLER_FINAL_OUT]: {stdout_output.strip()}")
        elif process and process.poll() is not None:
            print(f"handler.exe already terminated with exit code {process.returncode}.")
            stderr_output = process.stderr.read()
            if stderr_output:
                print(f"[HANDLER_FINAL_ERR]: {stderr_output.strip()}")
            stdout_output = process.stdout.read()
            if stdout_output:
                print(f"[HANDLER_FINAL_OUT]: {stdout_output.strip()}")

if __name__ == "__main__":
    run_bridge("AAPL")