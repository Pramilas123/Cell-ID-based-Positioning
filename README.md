The A7670E modem, equipped with an LTE antenna and a GNSS antenna, is utilized to determine device location based on Cell ID information within the Dialog network coverage in Sri Lanka. To achieve this, the system integrates with the Unwired Labs API, a service provider that leverages the OpenCelliD database and advanced cell tower triangulation techniques to estimate geographic location.

Used API : pk.c31b06cebed5d6e677a0a1ddddc9e688


Unwired Labs offers 100 requests per day per API key, which means each device must be assigned a separate API key in order to make daily requests independently. The workflow begins by transmitting cellular tower parameters (such as MCC, MNC, LAC/TAC, and Cell ID) in JSON format to the Unwired Labs API.

Ex: {
"token": "Your_API_Token",
"radio": "lte",
"mcc": 413,
"mnc": 2,
"cells": [{
"lac": 55408,
"cid": 5398821,
"psc": 0
}],
"address": 1
}

In return, the service responds with a JSON-formatted output that provides an approximate geographic location, including latitude, longitude, accuracy (in meters), and timestamp. This allows devices to determine their rough position within about 55 seconds.
