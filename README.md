# Algorithmic Trading Platform

Visual programming platform for creating cryptocurrency trading strategies with real-time market data.

## Features

- **Visual Strategy Builder** - Node-based visual programming for creating trading algorithms
- **Real-time Market Data** - Live OHLCV data from Bybit exchange
- **Technical Indicators** - RSI, MACD, Bollinger Bands, SMA/EMA
- **Backtesting** - Test strategies on historical data

## Setup

### 1. Requirements
- Visual Studio 2019 or newer
- C++17 support
- Bybit account

### 2. Get Bybit API Keys
- Go to [Bybit API Management](https://www.bybit.com/app/user/api-management)
- Create new API key with "Read" permissions
- Copy your API Key and Secret

### 3. Configure API Keys
Edit `config.ini` in project root:
```ini
[system]
debug_mode = true

[exchanges]
bybit_demo = true

[keys]
bybit_key = YOUR_API_KEY_HERE
bybit_signature = YOUR_SECRET_HERE
```


### 4. Build & Run
- Open solution in Visual Studio
- Build (Ctrl+Shift+B)
- Run (F5)

## Creating Strategy

1. Open visual editor
2. Add OHLCV data node
3. Add indicator nodes (RSI, MACD, etc)
4. Add logic nodes (if/else, comparisons)
5. Connect to execution nodes
6. Backtest → Paper trade → Live

## Configuration

**Demo Mode** (`bybit_demo = true`) - Uses demo, no real money

**Live Mode** (`bybit_demo = false`) - Real trading, be careful!


## Disclaimer

Educational purposes only. Trading involves risk. Not responsible for financial losses.

## Support

Open issue for bugs or questions
