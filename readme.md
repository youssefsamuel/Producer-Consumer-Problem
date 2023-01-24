# Producer-Consumer 

Implementing the Bounded-Buffer Producer/Consumer problem. Each Producer would declare the live price of a commodity (e.g., GOLD, SILVER, CRUDEOIL, COTTON, …). One Consumer would show a dashboard for the prices of all commodities.

Producers and Consumer would run indefinitely sharing the prices through shared memory.

## Producer
Each producer is supposed to continuously declare the price of one commodity. Each commodity price follows a normal distribution with parameters (𝜇, 𝜎). 

Therefore, producers will generate a new random price, share it with the consumer, and then sleep for an interval before producing the next price.

All producers are processes running the same codebase. Producers are to be run concurrently, either in separate terminals, or in the background.

While running a producer, you will specify the following command line arguments:
1. Commodity name (e.g., GOLD – Assume no more than 10 characters.)
2. Commodity Price Mean; 𝜇 – a double value.
3. Commodity Price Standard Deviation; 𝜎 – a double value.
4. Length of the sleep interval in milliseconds; T – an integer. 

```bash
./producer NATURALGAS 0.5 0.25 200 
```
The command would run a producer that declares the current price of Natural Gas every 200ms according to a normal distribution with parameters (mean=0.5 and variance=0.25). 

## Consumer
The consumer is to print the current price of each commodity, along the average of the current and past 4 readings.

An Up/Down arrow to show whether the current Price (AvgPrice) got increased or decreased from the prior one.
For simplicity, the commodities are limited to GOLD, SILVER, CRUDEOIL, NATURALGAS, ALUMINIUM, COPPER, NICKEL, LEAD, ZINC, MENTHAOIL, and COTTON.

To run the consumer the size of the shared buffer is needed:
```bash
./consumer 40
```

Below is an example output.

+-------------------------------------+

| Currency | Price | AvgPrice |

+-------------------------------------+

| ALUMINIUM | 0.00 | 0.00 |

| COPPER | 0.00 | 0.00 |

| ... |

| GOLD | 1810.31↑ | 1815.25↑ |

| ... |

| SILVER | 22.36↓ | 22.80↓ |

| ZINC | 0.00 | 0.00 |

+-------------------------------------+
