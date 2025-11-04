import os
from dataclasses import dataclass, field
from dotenv import load_dotenv
from typing import Any, Dict
import base64
import requests
from requests import Response, HTTPError, JSONDecodeError
import csv

from currency_utils import get_conversion_rate 

@dataclass
class Portfolio:
    assets: Dict[str, float] = field(default_factory=dict)


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

    def create_ticker_to_isin_map(self) -> Dict[str, str]:
        mapping: Dict[str, str] = {}

        instruments_response = self.get(endpoint="equity/metadata/instruments")
        for instrument in instruments_response:
            if 'ticker' in instrument and 'isin' in instrument:
                mapping[instrument['ticker']] = instrument['isin']

        return mapping


    def retrieve_portfolio(self) -> Portfolio:
        portfolio = Portfolio()
        
        cash_response = self.get(endpoint="equity/account/cash")
        portfolio_response = self.get(endpoint="equity/portfolio")

        print(cash_response)
        print(portfolio_response)

        total_value_gbp: float = cash_response['total']

        free_cash: float = cash_response['free']
        free_cash_proportion: float = free_cash / total_value_gbp
        portfolio.assets["free_cash"] = free_cash_proportion

        blocked_cash: float = cash_response['blocked']
        blocked_cash_proportion: float = blocked_cash / total_value_gbp
        portfolio.assets["blocked_cash"] = blocked_cash_proportion

        ticker_to_isin_map = self.create_ticker_to_isin_map()
        usd_to_gbp = get_conversion_rate("USD", "GBP")
        eur_to_gbp = get_conversion_rate("EUR", "GBP")
        for asset in portfolio_response:
            ticker: str = asset['ticker']
            isin = ticker_to_isin_map[ticker]
            quantity: float = asset['quantity']
            current_price: float = asset['currentPrice']
            if "_US_EQ" in ticker:
                current_price *= usd_to_gbp
                #ticker = ticker.replace("_US_EQ", "")
            elif "_EUR_EQ" in ticker:
                current_price *= eur_to_gbp
                #ticker = ticker.replace("_EUR_EQ", "")

            value_gbp = quantity * current_price
            proportional_value = value_gbp / total_value_gbp
            portfolio.assets[isin] = proportional_value

        return portfolio
    

def export_portfolio_to_csv(portfolio: Portfolio, filename: str) -> bool:        
    filepath = os.path.join("data", filename + ".csv")
    with open(filepath, 'w', newline='') as csvfile:
        fieldnames = ['ticker', 'proportion']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()

        for ticker, proportion in portfolio.assets.items():
            if ticker != "free_cash" and ticker != "blocked_cash": # don't include cash in portfolio right now
                writer.writerow({'ticker': ticker, 'proportion': proportion})

    return True


if __name__ == "__main__":
    load_dotenv()

    client = Trading212Client()

    portfolio: Portfolio = client.retrieve_portfolio()
    portfolio_exported: bool = export_portfolio_to_csv(portfolio, "current_portfolio")