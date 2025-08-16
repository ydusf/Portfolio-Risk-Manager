# Asian Option Pricing using Monte Carlo Simulation

This project implements a Monte Carlo simulation to price an **Asian Arithmetic Average Price Call Option**.

---

## What is an Asian Option?

An Asian option is a type of exotic option where the payoff depends on the **average price** of the underlying asset over a specified period, rather than the price at maturity. This helps reduce the effect of price manipulation and volatility spikes.

---

## Features

- Simulates asset price paths following Geometric Brownian Motion (GBM).
- Calculates the arithmetic average price for each simulated path.
- Computes the payoff of an Asian call option.
- Discounts payoffs to present value and averages them to estimate the option price.
- Supports adjustable parameters such as:
    - initial price
    - strike price
    - volatility
    - risk-free rate
    - maturity
    - steps
    - runs

---

## Usage


