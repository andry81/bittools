File name format:

<name>-[<offset-dec>-<period-dec>-<syncseq-hex>].bin

Legend:

  [MWSOCM]

  /impl-token max-weighted-sum-of-corr-mean
  /impl-token max-input-noise-resistence
  /impl-mwsocm

    Calculate through the maximum weighted sum of correlation mean
    values.

    Algorithm:
      Phase 1: Calculate correlation values.
      Phase 2: Calculate correlation mean values.
      Phase 3: Calculate maximum weighted sum of correlation mean
               values.

    Major time complexity: N * N * ln(N)
      , where N - stream bit length

Additional flags:

  /return-sorted-result
    Do full sort of a result array by all fields instead of search and
    return a single field array as by default.

    Has meaning if the algorithm accumulating correlation values is used.

    Has meaning when need to debug the algorithm for result certainty or
    false positive instability.

  /skip-calc-on-filtered-corr-value-use
  /skip-calc-on-fcvu
    Skip calculation if a filtered correlation value is used (filtered by
    `/corr-min` option).

    Has meaning if the algorithm accumulating correlation values is used.

---

Examples:

* to sync:

  [MWSOCM]

  >
  bitsync /impl-mwsocm /s 4000 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_offset-[907-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  synseq length/value:           20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.650000 / 0.810000
  corr value min/max:            0.119832 / 1.000000
  corr mean min/used/max:        0.650389 / 1.000000 / 1.000000
  max corr values per period:    16
  num corr values calculated:    6181
  num corr values iterated:      10418296
  num corr means calculated:     18
  corr means mem accum/max:      1 / 409600 Kb
  corr means mem used:           1 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.007
    corr mean values:             98 | 0.641
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.651

* to sync with input noise:

  [MWSOCM]

  + 1 noised bit per each 5 bits

  >
  bitsync /impl-mwsocm /s 4000 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_offset-[907-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  synseq length/value:           20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.650000 / 0.810000
  corr value min/max:            0.123330 / 0.952066
  corr mean min/used/max:        0.650801 / 0.873737 / 0.888453
  max corr values per period:    16
  num corr values calculated:    14361
  num corr values iterated:      32222032
  num corr means calculated:     963
  corr means mem accum/max:      19 / 409600 Kb
  corr means mem used:           19 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.010
    corr mean values:             98 | 0.803
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.816

* to sync with mixed offset:

  [MWSOCM]

  >
  bitsync /impl-mwsocm /si 15727 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_mixed_offset-[907+927-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  synseq length/value:           20 / 0x0001d0af
  offset:                        927
  period (width):                2964
  stream bit length:             15728
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 5242
  period min/used/max repeat:    3 / 4 / 16
  user corr value/mean min:      0.650000 / 0.810000
  corr value min/max:            0.130461 / 1.000000
  corr mean min/used/max:        0.651335 / 1.000000 / 1.000000
  max corr values per period:    16
  num corr values calculated:    7046
  num corr values iterated:      8246228
  num corr means calculated:     366
  corr means mem accum/max:      8 / 409600 Kb
  corr means mem used:           8 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   2 | 0.005
    corr mean values:             96 | 0.203
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.210

  [MWSOCM]

  >
  bitsync /impl-mwsocm /si 32000 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_mixed_offset-[907+927-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  synseq length/value:           20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.650000 / 0.810000
  corr value min/max:            0.130461 / 1.000000
  corr mean min/used/max:        0.650483 / 0.949774 / 0.954356
  max corr values per period:    16
  num corr values calculated:    14448
  num corr values iterated:      34112111
  num corr means calculated:     1549
  corr means mem accum/max:      31 / 409600 Kb
  corr means mem used:           31 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.011
    corr mean values:             98 | 0.794
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.807

* to sync with mixed offset and input noise:

  [MWSOCM]

  + 1 noised bit per each 5 bits

  >
  bitsync /impl-mwsocm /si 15727 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync "test_w_mixed_offset-[907+927-2964-1d0af].bin" .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  synseq length/value:           20 / 0x0001d0af
  offset:                        1899
  period (width):                2022
  stream bit length:             15728
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 5242
  period min/used/max repeat:    3 / 6 / 16
  user corr value/mean min:      0.650000 / 0.810000
  corr value min/max:            0.133317 / 0.943810
  corr mean min/used/max:        0.650811 / 0.819532 / 0.883074
  max corr values per period:    16
  num corr values calculated:    8343
  num corr values iterated:      11523390
  num corr means calculated:     812
  corr means mem accum/max:      16 / 409600 Kb
  corr means mem used:           16 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   2 | 0.005
    corr mean values:             97 | 0.215
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.221

  [MWSOCM]

  + 1 noised bit per each 5 bits

  >
  bitsync /impl-mwsocm /si 32000 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync "test_w_mixed_offset-[907+927-2964-1d0af].bin" .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  synseq length/value:           20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.650000 / 0.810000
  corr value min/max:            0.130724 / 0.955587
  corr mean min/used/max:        0.650963 / 0.842374 / 0.876608
  max corr values per period:    16
  num corr values calculated:    17152
  num corr values iterated:      45746584
  num corr means calculated:     4001
  corr means mem accum/max:      79 / 409600 Kb
  corr means mem used:           79 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.010
    corr mean values:             98 | 0.831
    corr weighted means sum:       0 | 0.001
    all:                         100 | 0.844
