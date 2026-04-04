import streamlit as st
import yfinance as yf
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# ---------------------- CONFIG ----------------------
st.set_page_config(
    page_title="Quant Volatility Screener",
    layout="wide"
)

st.title("📊 Dynamic Volatility & Event Screener")

# ---------------------- SIDEBAR ----------------------
with st.sidebar:
    st.header("⚙️ Parameters")

    ticker = st.text_input("Ticker Symbol", value="SPY").upper()
    lookback = st.slider("Lookback Window (Days)", 10, 120, 20)
    event_date = st.date_input(
        "Highlight Event Date",
        value=pd.to_datetime("2022-09-13")
    )

# ---------------------- DATA LOADING ----------------------
@st.cache_data(show_spinner="Fetching market data...")
def load_data(ticker: str, lookback: int) -> pd.DataFrame:
    df = yf.download(
        ticker,
        start="2020-01-01",
        end=pd.Timestamp.today().strftime('%Y-%m-%d'),
        auto_adjust=False,
        progress=False
    )

    df['Log_Return'] = np.log(df['Adj Close'] / df['Adj Close'].shift(1))

    df['Vol_Rolling'] = (
        df['Log_Return']
        .rolling(window=lookback)
        .std() * np.sqrt(252)
    )

    df['Vol_EWMA'] = (
        df['Log_Return']
        .ewm(span=lookback)
        .std() * np.sqrt(252)
    )

    return df.dropna()

data = load_data(ticker, lookback)

# ---------------------- PLOTTING ----------------------
st.subheader(f"📈 Historical Volatility Analysis: {ticker}")

fig, (ax1, ax2) = plt.subplots(
    2, 1,
    figsize=(14, 8),
    sharex=True
)

# Price Plot
ax1.plot(data.index, data['Adj Close'])
ax1.set_ylabel("Price")
ax1.set_title("Price Movement")

# Event marker
event_ts = pd.to_datetime(event_date)
ax1.axvline(event_ts, linestyle='--', alpha=0.6)

# Volatility Plot
ax2.plot(data.index, data['Vol_Rolling'], label=f'{lookback}D Rolling')
ax2.plot(data.index, data['Vol_EWMA'], label=f'{lookback}D EWMA')
ax2.set_ylabel("Annualized Volatility")
ax2.set_title("Volatility Measures")

# Event marker
ax2.axvline(event_ts, linestyle='--', alpha=0.6)

ax2.legend()

st.pyplot(fig)

# ---------------------- DATA TABLE ----------------------
st.subheader("📋 Recent Data")
st.dataframe(data.tail(10), use_container_width=True)