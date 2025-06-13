This project helped me winning **Selectel Career Wave**!

WIP: Add award photo, enhance readme quality, add monitoring services code, and a pgpool2 setup

## Demo

https://github.com/user-attachments/assets/0a9c1fa0-1200-4319-828b-c59e3ecd1ecc

> [!NOTE]
> **Why?**
> 1. For FUN (not everything needs to be built for profit!)
> 2. To demonstrate my skills in multiple fields
> 3. Don't you want a relatively small production ready database cluster that you can **CARRY AROUND** ? Cmon now.
> 4. To drain my wallet. I work for a reason, right?

## What is it?

### Components

0. My laptop (optional, as a main node)
1. 2/3x raspberry pi 5 boards + 512gb pcie gen3 ssd
2. 1 Gbps switch
3. 20.000 mah power bank with Power Delivery (~50w for 3 pi boards and switch)
4. esp32 board and a 240x240 IPS screen (simple monitoring)

### Interconnection

1. esp32 <-- SPI --> IPS screen
2. main rpi5 <-- UART --> esp32
3. adjacent rpi5's <-- UDP socket --> main rpi5
4. laptop <-- network switch (postgres networking) --> rpi5 boards cluster

### Software Used

1. It's a PostgreSQL cluster with configured async and sync replication on different nodes. Any node can be a main one - can be my laptop, can be any other node.
2. pgpoolII - for load balancing and a failover script to search and promote other nodes when the main one fails
3. Ubuntu Server LTS 42 as an OS on rpi5 boards, and Fedora Workstation 41 as an OS on my laptop
4. PlatformIO with arduino/esp-idf frameworks to program esp32 board.
5. Python with some libs to write monitoring scripts for rpi5's

> also some docker action, bash scripting, a lot of UNIX administration like networking, privileges e.t.c. (not important)

