import os
from dataclasses import dataclass, field
from dotenv import load_dotenv
from typing import Any, Dict, List
import base64
import requests
from requests import Response, HTTPError, JSONDecodeError
import csv
from typing import Optional
import time
from urllib.parse import urlparse, parse_qs

from currency_utils import get_conversion_rate 

def get_ticker_from_isin(isin: str) -> Optional[str]:
    url = "https://query2.finance.yahoo.com/v1/finance/search"
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36'
    }
    params = {'q': isin, 'quotesCount': 1, 'newsCount': 0}

    try:
        response = requests.get(url, params=params, headers=headers, timeout=5)
        response.raise_for_status() 
        
        data = response.json()
        
        if 'quotes' in data and data['quotes']:
            symbol = data['quotes'][0].get('symbol')
            return symbol
        else:
            print(f"Warning: No quote found for ISIN {isin}")
            return None

    except requests.exceptions.RequestException as e:
        print(f"Error fetching data for {isin}: {e}")
        return None
    except (IndexError, KeyError, TypeError, ValueError) as e:
        print(f"Error parsing response for {isin}: {e}")
        return None

@dataclass
class Portfolio:
    assets: Dict[str, tuple[str, float]] = field(default_factory=dict)


class Trading212Client:
    def __init__(self):
        self.API_KEY = os.getenv("TRADING212_API_KEY")
        self.SECRET_KEY = os.getenv("TRADING212_SECRET_KEY")
        self.host = "https://live.trading212.com"
        self.api_version = "v0"

    def get(self, endpoint: str, params=None) -> Any:
        credentials = f"{self.API_KEY}:{self.SECRET_KEY}"
        encoded_credentials = base64.b64encode(credentials.encode('utf-8')).decode('utf-8')
        auth_header = f"Basic {encoded_credentials}"
        full_url = f"{self.host}/api/{self.api_version}/{endpoint}"

        try:
            response: Response = requests.get(
                url=full_url,
                headers={
                    "Authorization": auth_header,
                    "Content-Type": "application/json"
                },
                params=params
            )

            response.raise_for_status() 
            return response.json()

        except HTTPError as error:
            print(f"HTTP error occurred: {error}")
            print(f"Full error response text: {response.text}")
            raise error
        except JSONDecodeError:
            print(f"Failed to decode JSON from response: {response.text}")
            raise

    def get_all_orders(self) -> List[Dict]:
        orders = []
        cursor = None
        has_more = True
        
        print("Fetching order history...")
        
        while has_more:
            params = {"limit": 50} 
            if cursor:
                params["cursor"] = cursor
            
            response = self.get("equity/history/orders", params=params)
            
            current_batch = response.get('items', [])
            orders.extend(current_batch)
            
            next_page_path = response.get('nextPagePath')
            
            if next_page_path:
                parsed_url = urlparse(next_page_path)
                query_params = parse_qs(parsed_url.query)
                
                if 'cursor' in query_params:
                    cursor = query_params['cursor'][0]
                    print(f"Fetched {len(orders)} orders so far...")
                    time.sleep(0.5) 
                else:
                    has_more = False
            else:
                has_more = False
                
        print(f"Total orders fetched: {len(orders)}")
        return orders

    def create_ticker_to_isin_map(self) -> Dict[str, str]:
        mapping: Dict[str, str] = {}
        print("Fetching all instrument data to build T212Ticker-to-ISIN map...")
        
        instruments_response = self.get(endpoint="equity/metadata/instruments")
        
        for instrument in instruments_response:
            if 'ticker' in instrument and 'isin' in instrument:
                mapping[instrument['ticker']] = instrument['isin']
        
        print(f"Map built with {len(mapping)} instruments.")
        return mapping

    def retrieve_portfolio(self) -> Portfolio:  
        portfolio = Portfolio()
        
        cash_response = self.get(endpoint="equity/account/cash")
        time.sleep(1) 
        
        portfolio_response = self.get(endpoint="equity/portfolio")
        time.sleep(1) 

        print(cash_response)
        print(portfolio_response)

        total_value_gbp: float = cash_response['total']

        free_cash: float = cash_response['free']
        free_cash_proportion: float = free_cash / total_value_gbp
        portfolio.assets["free_cash"] = ("FREE_CASH", free_cash_proportion)

        blocked_cash: float = cash_response['blocked']
        blocked_cash_proportion: float = blocked_cash / total_value_gbp
        portfolio.assets["blocked_cash"] = ("BLOCKED_CASH", blocked_cash_proportion)

        ticker_to_isin_map = self.create_ticker_to_isin_map()
        
        usd_to_gbp = get_conversion_rate("USD", "GBP")
        eur_to_gbp = get_conversion_rate("EUR", "GBP")

        for asset in portfolio_response:
            t212_ticker: str = asset['ticker']
            
            isin = ticker_to_isin_map.get(t212_ticker)
            if not isin:
                print(f"Warning: Could not find ISIN for T212 ticker {t212_ticker}. Skipping.")
                continue

            yahoo_ticker: str = get_ticker_from_isin(isin)
            if not yahoo_ticker:
                print(f"Warning: Could not find Yahoo ticker for ISIN {isin}. Using T212 ticker as fallback.")
                yahoo_ticker = t212_ticker.split("_")[0] # temporary fix for most of the existing tickers.

            quantity: float = asset['quantity']
            current_price: float = asset['currentPrice']

            if "_US_EQ" in t212_ticker:
                current_price *= usd_to_gbp
            elif "_EUR_EQ" in t212_ticker:
                current_price *= eur_to_gbp
            
            value_gbp = quantity * current_price
            proportional_value = value_gbp / total_value_gbp
            
            portfolio.assets[isin] = (yahoo_ticker, proportional_value)

        return portfolio
    

def export_portfolio_to_csv(portfolio: Portfolio, filename: str) -> bool:        
    save_to = "data"
    os.makedirs(save_to, exist_ok=True) 

    filepath = os.path.join(save_to, filename + ".csv")
    with open(filepath, 'w', newline='') as csvfile:
        fieldnames = ['isin', 'ticker', 'proportion']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()

        for isin, val in portfolio.assets.items():
            if isin != "free_cash" and isin != "blocked_cash": # don't include cash in portfolio right now
                writer.writerow({'isin': isin, 'ticker': val[0], 'proportion': val[1]})

    return True

def export_orders_to_csv(orders: List[Dict], filename: str):
    if not orders:
        print("No orders to export.")
        return

    save_to = "data"
    os.makedirs(save_to, exist_ok=True)
    filepath = os.path.join(save_to, filename + ".csv")

    headers = orders[0].keys()

    with open(filepath, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=headers)
        writer.writeheader()
        writer.writerows(orders)
    
    print(f"Saved to {filepath}")


if __name__ == "__main__":
    load_dotenv()

    client = Trading212Client()

    portfolio: Portfolio = client.retrieve_portfolio()
    portfolio_exported: bool = export_portfolio_to_csv(portfolio, "current_portfolio")

    all_orders = client.get_all_orders()
    export_orders_to_csv(all_orders, "trade_history")