import pandas as pd
import ast
import numpy as np

def analyse_trades(file_path):
    try:
        df = pd.read_csv(file_path)
    except FileNotFoundError:
        print("Error: 'trade_history.csv' not found.")
        return

    def parse_row(row):
        try:
            order = ast.literal_eval(row['order'])
            fill = ast.literal_eval(row['fill'])
            
            instrument = order.get('instrument', {})
            ticker = order.get('ticker', 'Unknown')
            name = instrument.get('name', ticker)
            
            date_str = fill.get('filledAt', order.get('createdAt'))
            
            wallet = fill.get('walletImpact', {})
            net_value = wallet.get('netValue', 0.0)
            pnl = wallet.get('realisedProfitLoss', 0.0)
            side = order.get('side')
            
            taxes = wallet.get('taxes', [])
            fees = sum(abs(t.get('quantity', 0)) for t in taxes)
            
            if side == 'BUY':
                volume = net_value - fees
            else:
                volume = net_value + fees

            return {
                'date': pd.to_datetime(date_str),
                'ticker': ticker,
                'name': name,
                'side': side,
                'volume': volume,
                'fees': fees,
                'pnl': pnl
            }
        except Exception:
            return None

    parsed_data = [x for x in df.apply(parse_row, axis=1) if x]
    
    if not parsed_data:
        print("No valid trades found to parse.")
        return

    trades = pd.DataFrame(parsed_data).sort_values('date')

    total_vol = trades['volume'].sum()
    total_fees = trades['fees'].sum()
    total_pnl = trades['pnl'].sum()
    
    sells = trades[trades['side'] == 'SELL'].copy()
    if not sells.empty:
        win_rate = (sells['pnl'] > 0).mean()
        gross_profit = sells[sells['pnl'] > 0]['pnl'].sum()
        gross_loss = abs(sells[sells['pnl'] < 0]['pnl'].sum())
        profit_factor = gross_profit / gross_loss if gross_loss != 0 else float('inf')
    else:
        win_rate = 0
        profit_factor = 0

    daily_pnl = trades.set_index('date')['pnl'].resample('D').sum().fillna(0)
    avg_daily_pnl = daily_pnl.mean()
    std_daily_pnl = daily_pnl.std()
    
    if std_daily_pnl != 0:
        sharpe_ratio = (avg_daily_pnl / std_daily_pnl) * np.sqrt(252)
    else:
        sharpe_ratio = 0.0

    trades['cum_pnl'] = trades['pnl'].cumsum()
    trades['high_water_mark'] = trades['cum_pnl'].cummax()
    trades['drawdown'] = trades['cum_pnl'] - trades['high_water_mark']
    max_drawdown = trades['drawdown'].min()
    
    print("-" * 30)
    print("PERFORMANCE REPORT")
    print("-" * 30)
    print(f"Total Traded Volume:  £{total_vol:,.2f}")
    print(f"Total Fees Paid:      £{total_fees:,.2f}")
    print(f"Net Realised PnL:     £{total_pnl:,.2f}")
    print("-" * 30)
    print(f"Profit Factor:        {profit_factor:.2f}")
    print(f"Win Rate:             {win_rate:.1%}")
    print(f"Sharpe Ratio (Ann.):  {sharpe_ratio:.2f}")
    print(f"Max Drawdown:         £{max_drawdown:,.2f}")
    print("-" * 30)

if __name__ == "__main__":
    analyse_trades('data/trade_history.csv')