# Bugcoin

**Bugcoin** is a lightweight cryptocurrency designed for low-end devices, old PCs, and smartphones. It allows you to mine coins with minimal resources, track your balance, and see their value in USD. Each Bugcoin is currently valued at **$10 USD**, and there is a maximum supply of **100 million coins**.

---

## Features

- Ultra-lightweight, works on low-end PCs and phones.
- Mine Bugcoin without heavy CPU usage.
- Track your balance locally using a `.bugcoin` file.
- Supports offline calculation of total Bugcoins and USD value.
- Optional integration with LocalStorage or Termux for storing your files.
- No wallets required for basic usage.

---

## Requirements

- **C Compiler** (GCC) to compile the Bugcoin node.
- **Python** and **Flask** if using a local API or web interface (optional).
- Termux (for Android) or any Linux-based system.

---

## Compiling Bugcoin Node

```bash
gcc bugcoin.c -o bugcoin
```

---

# Usage

## Start Node

```bash
./bugcoin --true
```

## Stop Node

```bash
./bugcoin --false
```

## Mining

```bash
./bugcoin --mine True --d 5m --threads 5
```

--d sets duration (5m = 5 minutes, 20s = 20 seconds).

--threads sets number of CPU threads to use.



---

# Viewing Your Bugcoins

## Your mined blocks are saved in a hidden file called .bugcoin in your home directory:

```bash
/data/data/com.termux/files/home/bugcoin/.bugcoin
```

## You can view it with:

```bash
cat ~/.bugcoin
```

Or use a web interface that reads .bugcoin and calculates total Bugcoins and USD value automatically.


---

# Using Bugcoin on Web

Open the HTML web page (index.html) on your browser.

Upload your .bugcoin file.

The web page will show:

Total Bugcoins mined.

Equivalent value in USD.


LocalStorage can be used to remember your file for future visits.



---

# Notes

**Bugcoin is designed to be educational and lightweight.**

**Each block gives a fixed reward, and total supply is capped at 100 million Bugcoins.**

**Can be integrated with APIs or web platforms to simulate purchases or exchanges.**



---

Enjoy mining Bugcoin! 🚀
