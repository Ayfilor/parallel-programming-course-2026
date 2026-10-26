
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


## Stage 3

The `o/lab2 withdraw-v1 randoms 1000` output:
```
random: 1000 seeds, 498 fail (49.8%)
first failing seed 0: AABBBBAA -> OVERDRAFT balance=-100 results=true,true
replay AABBBBAA: same failure 10/10
```

The `o/lab2 withdraw-v1 replay AABBBBAAA log` output:
```
  A  lock mu
  A  unlock mu
  A  window
  B  lock mu
  B  unlock mu
  B  window
  B  lock mu
  B  unlock mu
  A  lock mu
  A  unlock mu
schedule AABBBBAA -> OVERDRAFT balance=-100 results=true,true
```

Answer: ну с логом есть отладочная информация (~_-)


## Stage 4

The `o/lab2 withdraw-v1 dfs` output:
```
explored 10 schedules, 6 fail  (0.00 s)
first failing: AABBAABB -> OVERDRAFT balance=-100 results=true,true
```

The `o/lab2 withdraw-v2 dfs` output:
```
explored 2 schedules, 0 fail  (0.00 s)
```

The `o/lab2 withdraw-v0 dfs` output:
```
explored 20 schedules, 18 fail  (0.00 s)
first failing: AABABB -> OVERDRAFT balance=-100 results=true,true
```

The `o/lab2 transfer dfs` output:
```
explored 6 schedules, 2 fail  (0.00 s)
first failing: AB -> DEADLOCK held={a:A, b:B}
```

Answer: ну в v2 один лок, а в v1 два – меньше простор действий


## Stage 5

The `o/lab2 withdraw-v1 bounded 0` output:
```
explored 2 schedules, 0 fail  (0.00 s)
```

The `o/lab2 withdraw-v1 bounded 1` output:
```
explored 6 schedules, 2 fail  (0.00 s)
first failing: AABBBBAA -> OVERDRAFT balance=-100 results=true,true
```

The `o/lab2 withdraw-v1 bounded 2` output:
```
explored 10 schedules, 6 fail  (0.00 s)
first failing: AABBAABB -> OVERDRAFT balance=-100 results=true,true
```

The `o/lab2 transfer bounded 0` output:
```
explored 2 schedules, 0 fail  (0.00 s)
```

The `o/lab2 transfer bounded 1` output:
```
explored 6 schedules, 2 fail  (0.00 s)
first failing: AB -> DEADLOCK held={a:A, b:B}
```

Task 2:

The `o/lab2 withdraw-v1 replay AABBBBAA log` output:
```
  A  lock mu
  A  unlock mu
  A  window
  B  lock mu
  B  unlock mu
  B  window
  B  lock mu
  B  unlock mu
  A  lock mu
  A  unlock mu
schedule AABBBBAA -> OVERDRAFT balance=-100 results=true,true
```

The `o/lab2 transfer replay AB log` output:
```
  A  lock a
  A  window
  B  lock b
  B  window
schedule AB -> DEADLOCK held={a:A, b:B}
```
