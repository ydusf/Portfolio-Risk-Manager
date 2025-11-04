import requests
from typing import Any

def get_conversion_rate(from_currency, to_currency, timeout_seconds=5) -> Any:
    try:
        url = f"https://api.frankfurter.app/latest?from={from_currency}&to={to_currency}"
        response = requests.get(url, timeout=timeout_seconds)
        
        response.raise_for_status() 
        data = response.json()
        
        if 'rates' in data and to_currency in data['rates']:
            return data['rates'][to_currency]
        else:
            print(f"Error: Could not find rate for {to_currency} in API response.")
            return None

    except requests.exceptions.Timeout:
        print(f"Error: Request for {from_currency} to {to_currency} timed out after {timeout_seconds}s.")
        return None
    except requests.exceptions.RequestException as e:
        print(f"Error fetching currency rate: {e}")
        return None
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return None