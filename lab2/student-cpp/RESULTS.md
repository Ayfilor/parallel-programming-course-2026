
## Stage 0

The `o/stress 10000 none v1` output:
```
stress V1, window=none: 10000 runs, 5832 overdrafts (58.32%)
stress V1, window=none: 10000 runs, 6764 overdrafts (67.64%)
stress V1, window=none: 10000 runs, 4390 overdrafts (43.90%)
```

The `o/stress 10000 yield v1` output:
```
stress V1, window=yield: 10000 runs, 9976 overdrafts (99.76%)
stress V1, window=yield: 10000 runs, 9984 overdrafts (99.84%)
stress V1, window=yield: 10000 runs, 9979 overdrafts (99.79%)
```

The `o/stress 10000 none v2` output:
```
stress V2, window=none: 10000 runs, 0 overdrafts (0.00%)
stress V2, window=none: 10000 runs, 0 overdrafts (0.00%)
stress V2, window=none: 10000 runs, 0 overdrafts (0.00%)
```

Answer: Планировщик ОС недетерминирован, поэтому v1 делает yield непредсказуемо, что влечёт разные результаты. 0 овердрафтов v1 говорило бы, что система зачастую корректна, однако для подтверждения полной корректности нужен не стресс тест, а что-то покрывающее все codepaths. 


## Stage 1
The `o/lab2 withdraw-v0 first log` output:
```
  A  balance -> 100
  A  window
  A  balance -> 100
  A  balance = 0
  B  balance -> 0
  B  window
schedule AAAB -> ok
```


## Stage 2
The `o/lab2 transfer first` output:
```
schedule AAAABBBB -> ok
```

The `o/lab2 transfer replay AB log` output:
```
  A  lock a
  A  window
  B  lock b
  B  window
schedule AB -> DEADLOCK held={a:A, b:B}
```
