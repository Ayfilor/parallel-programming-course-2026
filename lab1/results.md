# Main Bench

Single Threaded:
- (1) 415.335 – count=12470809783

Global Lock (simple mutex):
- (1)  104.854 – count=3148191592
- (2)  47.3898 – count=1414390254
- (4)  36.5972 – count=1095012652
- (8)  27.697  – count=832056299
- (16) 20.1725 – count=599695534

Stub Lock (lock without body):
- (1)  117.372
- (2)  60.733
- (4)  49.6068
- (8)  35.8053
- (16) 31.3645

Striped Lock (Shards):
- (1)  94.5216 – count=2855602679
- (2)  27.6585 – count=3678978681
- (4)  24.6077 – count=4418759780
- (8)  21.4375 – count=5062308170
- (16) 15.1389 – count=5511599812

Thread Local State:
- (1)  242.301 – count=7293124834
- (2)  462.07  – count=21156289946
- (4)  789.379 – count=45201367397
- (8)  937.586 – count=73384659217
- (16) 969.911 – count=102482976890

Double Buffer Collector:
- (1)  217.614 - count=6532563827 
- (2)  416.612 - count=19089077084 
- (4)  665.391 - count=39329171474 
- (8)  832.971 - count=64391342149 
- (16) 792.533 - count=88335189833 


# Stress Test

Striped Lock:
- Fraction: 95.4
- Greater: 1614
- Lower: 7926
- Final Count: 448221
- Workers Requests: 448221

Thread Local State:
- Fraction: 98.08
- Greater: 0
- Lower: 9808
- Final Count: 6023093
- Workers Requests: 6023093

Double Buffer State (without 3 stage):
- Fraction: 99.17
- Greater: 0
- Lower: 9917
- Final Count: 8299021
- Workers Requests: 8299025

Double Buffer State:
- Fraction: 0
- Greater: 0
- Lower: 0
