# Project 3: Probability of Backtest Overfitting (PBO) & Deflated Sharpe Ratios

## 🎯 Project Objective
To build a statistical testing framework that evaluates the true robustness of a trading strategy. This project will simulate hundreds of random, zero-edge trading strategies to demonstrate the "Multiple Testing Problem," and then implement the **Deflated Sharpe Ratio (DSR)** to mathematically haircut inflated backtest results.

---

## 📚 Prerequisites: Financial & Statistical Theory

### 1. The Multiple Testing Problem
* **Concept:** If you flip a coin 10 times, getting 10 heads is a miracle (0.09% chance). But if you have 10,000 people flip a coin 10 times, it is almost guaranteed that someone will get 10 heads. 
* **Application to Finance:** If you optimize a moving average crossover strategy by testing 5,000 different parameter combinations, the "best" combination is likely just the one that accidentally perfectly fit the historical noise. 

### 2. The Statistical Nature of the Sharpe Ratio
* **Concept:** Retail traders treat the Sharpe Ratio ($SR = \frac{R_p - R_f}{\sigma_p}$) as an absolute truth. Quants treat it as a random variable with a probability distribution.
* **The Reality:** A Sharpe Ratio of 1.5 measured over 10 years is highly significant. A Sharpe Ratio of 1.5 measured over 2 months is statistically meaningless. You must calculate the **Standard Error of the Sharpe Ratio**.

### 3. The Deflated Sharpe Ratio (DSR)
* **Concept:** Developed by Marcos López de Prado (a legend in modern quant finance), DSR discounts your backtest's Sharpe ratio based on three things:
    1. The number of trials/strategies you tested (more tests = higher discount).
    2. The variance of the returns (non-normal returns = higher discount).
    3. The length of the backtest (shorter time = higher discount).

---

## 💻 Prerequisites: Technical & Programming

### 1. Monte Carlo Simulation (`numpy.random`)
* You need to be comfortable generating random walks. We will not use real stock data for the first half of this project. We will generate pure Gaussian noise (fake stock returns) to prove that we can "discover" a profitable strategy in data that has zero actual signal.

### 2. Statistical Distributions (`scipy.stats`)
* You will need to calculate the **Skewness** and **Kurtosis** (fat tails) of return distributions. Financial returns are never perfectly normal, and standard Sharpe ratios overestimate performance when kurtosis is high.
* You will use the Cumulative Distribution Function (CDF) of the standard normal distribution to calculate the final Probabilistic Sharpe Ratio.

---

## 🚀 Steps to Build

1. **The Setup (Generating Noise):** Use NumPy to simulate 5 years of daily returns for 1,000 completely random, zero-edge trading strategies. 
2. **The Illusion:** Calculate the annualized Sharpe Ratio for all 1,000 random strategies. Isolate the "best" performing strategy and plot its cumulative return equity curve. (It will look like a highly profitable strategy, even though it is entirely random).
3. **The Reality Check:** Calculate the Expected Maximum Sharpe Ratio (the benchmark you *should* expect to achieve purely by luck when running 1,000 trials).
4. **Probabilistic Sharpe Ratio (PSR):** Write a function to calculate the PSR, adjusting the Sharpe ratio for non-normal returns (skewness and kurtosis) and the track record length.
5. **Deflated Sharpe Ratio (DSR):** Write the final function that takes the PSR and penalizes it for the 1,000 independent trials you ran. Show that the "best" strategy's true probability of outperformance drops to near 0%.