# Advanced Volatility Estimator & Event Impact Analysis

##  Project Objective
To build a robust Python-based tool that computes, compares, and visualizes various volatility metrics (Historical, Rolling, EWMA) for a given asset. The secondary objective is to analyze how realized volatility changes in response to major macroeconomic events (e.g., CPI releases, FOMC meetings), demonstrating an understanding of volatility as a dynamic, regime-dependent risk metric rather than a static mathematical formula.

---

##   Prerequisites: Financial Theory
Before starting this project, you should understand the following financial concepts:

### 1. Simple Returns vs. Log Returns
* **Concept:** In finance, we rarely use simple percentage changes to calculate volatility. Instead, we use logarithmic returns.
* **Why?** Log returns are time-additive (the log return over 2 days is the sum of the log returns of each day) and are assumed to be normally distributed in standard quantitative models (like Black-Scholes).
* **Formula:** $r_t = \ln(P_t / P_{t-1})$ 

### 2. Historical (Realized) Volatility & Annualization
* **Concept:** Volatility is the standard deviation of returns. However, standard deviation is dependent on the sampling frequency (daily, hourly, etc.).
* **Annualization:** To compare volatilities across different assets and timeframes, it must be annualized.
* **Formula:** $\sigma_{annualized} = \sigma_{daily} \sqrt{252}$ *(Note: 252 is the standard number of trading days in a year for equities. Crypto would use 365).*

### 3. Rolling Window Volatility
* **Concept:** Volatility clusters. A single historical volatility number for a 5-year period hides periods of extreme calm and extreme panic.
* **Application:** You need to calculate volatility over rolling windows (e.g., 20-day, 60-day, 120-day) to see how risk evolves over time.

### 4. Exponentially Weighted Moving Average (EWMA)
* **Concept:** Standard rolling windows give equal weight to a shock from 19 days ago and a shock from yesterday. EWMA fixes this by assigning exponentially declining weights to older observations. It captures recent market shocks much faster.
* **The Decay Factor ($\lambda$):** Usually set around 0.94 (as popularized by RiskMetrics).
* **Formula:** $\sigma_t^2 = (1 - \lambda) r_{t-1}^2 + \lambda \sigma_{t-1}^2$

### 5. Macroeconomic Catalysts
* **Concept:** Markets re-price risk around scheduled information releases.
* **Key Events to Know:** * **CPI (Consumer Price Index):** Measures inflation.
    * **FOMC (Federal Open Market Committee):** Determines US interest rates.
    * **NFP (Non-Farm Payrolls):** Measures US employment.

---

## 💻 Prerequisites: Technical & Programming
Coming from a computer engineering background, the coding will be straightforward, but you should be familiar with these specific libraries and data engineering paradigms:

### 1. Time-Series Data Handling (`pandas`)
* **Datetime Indexing:** Your dataframes must be indexed by `pd.DatetimeIndex`.
* **Vectorization:** Never use `for` loops to calculate returns. Use vectorized pandas functions like `.shift()`, `.pct_change()`, and `np.log()`.
* **Rolling Operations:** Master the `.rolling(window=N)` method to compute moving standard deviations.
* **EWMA Computation:** Learn to use pandas' `.ewm(span=X).std()` or implement the recursive EWMA formula manually using NumPy.

### 2. Data Sourcing (APIs)
* You need reliable, free historical data.
* **Asset Pricing Data:** `yfinance` (Yahoo Finance API) is the easiest starting point for daily OHLCV (Open, High, Low, Close, Volume) data.
* **Macroeconomic Data:** `pandas_datareader` paired with FRED (Federal Reserve Economic Data) to pull historical CPI release dates or interest rate changes.

### 3. Data Visualization (`matplotlib` / `plotly`)
* **Requirement:** Quant research is useless if it cannot be communicated. You need to plot time series on dual axes (e.g., Asset Price on the left y-axis, Rolling Volatility on the right y-axis).
* **Event Annotation:** You must know how to draw vertical lines (`axvline` in matplotlib) to mark the exact dates of macroeconomic events on your time-series charts to visually inspect volatility spikes.

---

## 🚀 Steps to Build
1.  **Data Ingestion:** Fetch 5 years of daily `Adj Close` prices for a major ETF (e.g., SPY for S&P 500).
2.  **Return Calculation:** Compute the daily log returns.
3.  **Volatility Metrics:** Compute 20-day rolling volatility, 60-day rolling volatility, and EWMA volatility. Annualize all of them.
4.  **Visualization:** Plot the asset's price alongside the three volatility metrics.
5.  **Event Analysis (The Quant Edge):** Isolate a major event (e.g., the March 2020 COVID crash or a major 2022 CPI print). Calculate the realized volatility 30 days *before* the event and 30 days *after* the event. Compare the difference.

---

## ⚠️ Limitations & Production Considerations
This repository serves as a research prototype. If this logic were to be adapted for a production trading desk, the following institutional realities must be addressed:

1. **Data Resolution:** This model utilizes daily `Close` and `OHLC` data. In production, event-driven alpha (such as trading a CPI print) decays within milliseconds. Tick-level L2/L3 order book data would be required.
2. **Slippage & Market Impact:** The model assumes execution at exactly the recorded prices. In reality, liquidity evaporates and spreads widen aggressively immediately before and after macroeconomic events. A robust market impact model must be integrated.
3. **Execution Latency:** The research is written in Python. A live deployment of an event-driven strategy would require rewriting the execution layer in C++ or Rust to interface directly with the exchange via FIX or binary protocols.
4. **Instrument Selection:** Rather than trading the underlying delta-one asset (e.g., SPY shares), an institutional approach would likely monetize volatility expansions via options (e.g., Straddles/Strangles) or variance swaps.