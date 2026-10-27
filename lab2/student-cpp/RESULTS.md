
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


## Stage 6

```
# 6a
$ o/lab2 collector-nostep3-1x1 dfs
explored 112 schedules, 12 fail  (0.01 s)
first failing: ABBBAABAAB -> LOST 1 record(s): counted 0 of 1
$ o/lab2 collector-nostep3-1x1 replay ABBBAABAAB log
  A  active -> 0
  B  active -> 0
  B  active = 1
  B  await done
  A  inside[W1] = 0
  A  cnt[W1][0] -> 0
  B  cnt[W1][0] -> 0
  A  cnt[W1][0] = 1
  A  inside[W1] = -1
  B  cnt[W1][0] = 0
schedule ABBBAABAAB -> LOST 1 record(s): counted 0 of 1
```

```
$ o/lab2 collector-nostep3-1x2 dfs
explored 2023 schedules, 105 fail  (0.12 s)
first failing: AAAAAABBBAABAAB -> LOST 1 record(s): counted 1 of 2

$ o/lab2 collector-nostep3-1x3 dfs
explored 12859 schedules, 348 fail  (0.68 s)
first failing: AAAAAAAAAAABBBAABAAB -> LOST 1 record(s): counted 2 of 3

$ o/lab2 collector-nostep3-1x4 dfs
explored 50120 schedules, 816 fail  (2.70 s)
first failing: AAAAAAAAAAAAAAAABBBAABAAB -> LOST 1 record(s): counted 3 of 4
```


```
$ o/lab2 collector-plain-1x1 dfs
explored 29 schedules, 0 fail  (0.00 s)
$ o/lab2 collector-acqrel-1x1 dfs
explored 972 schedules, 0 fail  (0.07 s)
$ o/lab2 collector-ok-1x1 dfs
explored 972 schedules, 0 fail  (0.07 s)
```

Почему одного писателя с одним вызовом record() оказалось достаточно, чтобы найти баг в алгоритме?
- Потому что мы пропускаем шаг 3 очевидно – читать не выхода писателя из буфера, писатель не проверяет активность буффера

Почему collector-plain-1x1 даёт 0 ошибок при наличии бага в коде?
- Мы не трекаем inside, поэтому не позволяем читателям собирать метрики со старой корзины

В чём разница между нулём ошибок у collector-acqrel-1x1 и нулём ошибок у collector-ok-1x1?
- На самом деле acqrel model не исполняется, так как get и set ведут себя как seqence  


## Stage 7

- Relaxed  — yes
- Plain    — yes
- Aqcrel   — yes
- Seq      — no
- volatile — no
- naive    — yes, 3/3

```
$ o/ministress acqrel 200000 100
mode=acqrel samples=20000000
  (0,0)       19536494   97.682%  forbidden under seq_cst
  (0,1)         381376    1.907%
  (1,0)          82122    0.411%
  (1,1)              8    0.000%

$ o/ministress acqrel 200 100000
mode=acqrel samples=20000000
  (0,0)        1456868    7.284%  forbidden under seq_cst
  (0,1)        9478705   47.394%
  (1,0)        9064420   45.322%
  (1,1)              7    0.000%
```

First case: мало ячеек, они залетают в кэш, поэтому store-buffer особождается чаще

Second case: а теперь ячеек много, поэтому выполнение потоков размазано, поэтому кэш инвалидируется тоже размазано.
