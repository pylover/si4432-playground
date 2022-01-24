# si4432-playground

Arduino UNO as a playground for SI4432 RF transmitter chip.


## Wiring

```
  Arduino                                     XL4432-SMT
    Uno                                       Transmitter
+----------+                                 +-----------+
|     3.3V |----------------------------+----| VDD   ANT |>-----))))))
|          |                            |    |           |
|          |            Sparkfun        |    |           |
|          |            BOB-12009       |    |           |
|          |          +-----------+     |    |           |
|       5V |----------| HV     LV |-----+    |           |
| SCK   13 |>---------| 5V   3.3V |>-------->| SCLK      |
| MOSI  11 |>---------| 5V   3.3V |>-------->| SDI       |
| SS    10 |>---------| 5V   3.3V |>-------->| NSEL      |
| DOUT   7 |>---------| 5V   3.3V |>-------->| SDN       |
|          |          |           |          |           |
|          |          |       GND |---+      |           |
|          |          +-----------+   |      |           |
|          |                          |      |           |
|      GND |--------------------------+------| GND       |
|          |                          |      |           |
|        8 |>----STNDBY-LED----220Ω---+      |           |
|          |                          |      |           |
| DOUT   9 |>-----BUSY-LED-----220Ω---+      |           |
|          |                                 |           |
| INT0   2 |<-------------------------------<| NIRQ      |
| MISO  12 |<-------------------------------<| SDO       |
+----------+                                 +-----------+            
```

## Commands

```bash
reg-get ADDR [MSB:LSB]
reg-set ADDR VALUE
power [ON/OFF]
```
