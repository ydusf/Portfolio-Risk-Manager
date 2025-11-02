import os
from dataclasses import dataclass, field
from dotenv import load_dotenv
from typing import Any, Dict
import base64
import requests
from requests import Response, HTTPError, JSONDecodeError

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

    def retrieve_portfolio(self) -> Portfolio:
        portfolio = Portfolio()

        cash_response = self.get(endpoint="equity/account/cash")
        portfolio_response = self.get(endpoint="equity/portfolio")

        print(cash_response)
        print(portfolio_response)

        total_value: float = cash_response['total']

        free_cash: float = cash_response['free']
        free_cash_proportion: float = free_cash / total_value
        portfolio.assets["free_cash"] = free_cash_proportion

        blocked_cash: float = cash_response['blocked']
        blocked_cash_proportion: float = blocked_cash / total_value
        portfolio.assets["blocked_cash"] = blocked_cash_proportion

        for asset in portfolio_response:
            ticker: str = asset['ticker']
            quantity: float = asset['quantity']
            current_price: float = asset['currentPrice']
            value = quantity * current_price
            proportional_value = value / total_value
            portfolio.assets[ticker] = proportional_value

        return portfolio
    

def export_portfolio_to_csv(portfolio: Portfolio, filename: str) -> bool:    
    filepath: str = "data/" + filename + ".csv"
    with open(filepath, 'w'):
        # save portfolio to file
        pass


    return True


if __name__ == "__main__":
    load_dotenv()

    client = Trading212Client()

    portfolio: Portfolio = client.retrieve_portfolio()
    print(portfolio)

    total_value = 0
    for asset, perc in portfolio.assets.items():
        total_value += perc
    
    print(total_value)
    #portfolio_exported: bool = export_portfolio_to_csv(portfolio)