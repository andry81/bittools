File name format:

<name>-[<offset-dec>-<period-dec>-<syncseq-hex>].bin

Legend:

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

-------------------------------------------------------------------------------
1. To sync
-------------------------------------------------------------------------------

  >
  bitsync /impl-mwsocm /s 4000 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_offset-[907-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.510000 / 0.650000
  corr value min/max:            0.196064 / 1.000000
  corr mean min/used/max:        0.527856 / 1.000000 / 1.000000
  max corr values per period:    16
  num corr values calc/iter:     13708 / 31305639
  num corr means calc/iter:      25451 / 736960
  corr means mem accum/max:      497 / 409600 Kb
  corr means mem used:           497 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   0 | 0.008
    corr mean values:             98 | 0.773
    corr weighted means sum:       0 | 0.004
    all:                         100 | 0.784

  ---

  + /corr-mm-dispersed-value-prime1033

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /s 4000 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_offset-[907-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.510000 / 0.650000
  corr value min/max:            0.016671 / 1.000000
  corr mean min/used/max:        0.510626 / 1.000000 / 1.000000
  max corr values per period:    16
  num corr values calc/iter:     3162 / 4888774
  num corr means calc/iter:      8 / 1776
  corr means mem accum/max:      1 / 409600 Kb
  corr means mem used:           1 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.011
    corr mean values:             98 | 0.659
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.670

  ---

  + /use-linear-corr

  >
  bitsync /impl-mwsocm /use-linear-corr /s 4000 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_offset-[907-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.710000 / 0.810000
  corr value min/max:            0.442791 / 1.000000
  corr mean min/used/max:        0.710321 / 1.000000 / 1.000000
  max corr values per period:    16
  num corr values calc/iter:     15587 / 39310189
  num corr means calc/iter:      13078 / 1286500
  corr means mem accum/max:      256 / 409600 Kb
  corr means mem used:           256 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   0 | 0.008
    corr mean values:             98 | 0.852
    corr weighted means sum:       0 | 0.002
    all:                         100 | 0.862

  ---

  + /corr-mm-dispersed-value-prime1033
  + /use-linear-corr

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /use-linear-corr /s 4000 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_offset-[907-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.710000 / 0.810000
  corr value min/max:            0.129115 / 1.000000
  corr mean min/used/max:        0.712312 / 1.000000 / 1.000000
  max corr values per period:    16
  num corr values calc/iter:     3819 / 5966671
  num corr means calc/iter:      6 / 3688
  corr means mem accum/max:      1 / 409600 Kb
  corr means mem used:           1 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.011
    corr mean values:             98 | 0.640
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.652

-------------------------------------------------------------------------------
2. To sync with input noise
-------------------------------------------------------------------------------

  + /inn 5 100

  >
  bitsync /impl-mwsocm /s 4000 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_offset-[907-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.510000 / 0.650000
  corr value min/max:            0.049790 / 0.903243
  corr mean min/used/max:        0.527105 / 0.803750 / 0.811578
  max corr values per period:    16
  num corr values calc/iter:     13297 / 29959340
  num corr means calc/iter:      47402 / 648774
  corr means mem accum/max:      874 / 409600 Kb
  corr means mem used:           874 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   0 | 0.008
    corr mean values:             98 | 0.811
    corr weighted means sum:       0 | 0.007
    all:                         100 | 0.826

  ---

  + /inn 5 100
  + /corr-mm-dispersed-value-prime1033

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /s 4000 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_offset-[907-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.510000 / 0.650000
  corr value min/max:            0.016671 / 0.900244
  corr mean min/used/max:        0.511341 / 0.749496 / 0.788231
  max corr values per period:    16
  num corr values calc/iter:     8509 / 15611357
  num corr means calc/iter:      913 / 93291
  corr means mem accum/max:      18 / 409600 Kb
  corr means mem used:           18 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   2 | 0.017
    corr mean values:             97 | 0.710
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.727

  ---

  + /inn 5 100
  + /use-linear-corr

  >
  bitsync /impl-mwsocm /use-linear-corr /s 4000 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_offset-[907-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.710000 / 0.810000
  corr value min/max:            0.220674 / 0.950104
  corr mean min/used/max:        0.710465 / 0.897026 / 0.901123
  max corr values per period:    16
  num corr values calc/iter:     14694 / 35127649
  num corr means calc/iter:      29457 / 993631
  corr means mem accum/max:      573 / 409600 Kb
  corr means mem used:           573 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   0 | 0.008
    corr mean values:             98 | 0.838
    corr weighted means sum:       0 | 0.004
    all:                         100 | 0.851

-------------------------------------------------------------------------------
3. To sync with mixed offset
-------------------------------------------------------------------------------

  + /si 15727
  + /corr-min 0.65

  >
  bitsync /impl-mwsocm /corr-min 0.65 /si 15727 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_mixed_offset-[907+927-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        927
  period (width):                2964
  stream bit length:             15728
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 5242
  period min/used/max repeat:    3 / 4 / 16
  user corr value/mean min:      0.650000 / 0.650000
  corr value min/max:            0.148369 / 1.000000
  corr mean min/used/max:        0.651695 / 1.000000 / 1.000000
  max corr values per period:    16
  num corr values calc/iter:     1512 / 1150713
  num corr means calc/iter:      365 / 365
  corr means mem accum/max:      8 / 409600 Kb
  corr means mem used:           8 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   2 | 0.004
    corr mean values:             97 | 0.159
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.163

  ---

  + /si 32000

  >
  bitsync /impl-mwsocm /si 32000 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_mixed_offset-[907+927-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.510000 / 0.650000
  corr value min/max:            0.097121 / 1.000000
  corr mean min/used/max:        0.518506 / 0.908793 / 0.919734
  max corr values per period:    16
  num corr values calc/iter:     13346 / 29643937
  num corr means calc/iter:      48558 / 661699
  corr means mem accum/max:      890 / 409600 Kb
  corr means mem used:           890 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   0 | 0.005
    corr mean values:             98 | 0.846
    corr weighted means sum:       0 | 0.007
    all:                         100 | 0.858

  ---

  + /si 15727
  + /corr-mm-dispersed-value-prime1033

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /si 15727 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_mixed_offset-[907+927-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        927
  period (width):                2964
  stream bit length:             15728
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 5242
  period min/used/max repeat:    3 / 4 / 16
  user corr value/mean min:      0.510000 / 0.650000
  corr value min/max:            0.016671 / 1.000000
  corr mean min/used/max:        0.512103 / 1.000000 / 1.000000
  max corr values per period:    16
  num corr values calc/iter:     4037 / 3556344
  num corr means calc/iter:      296 / 20636
  corr means mem accum/max:      6 / 409600 Kb
  corr means mem used:           6 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   3 | 0.007
    corr mean values:             96 | 0.180
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.187

  ---

  + /si 32000
  + /corr-mm-dispersed-value-prime1033

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /si 32000 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_mixed_offset-[907+927-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.510000 / 0.650000
  corr value min/max:            0.019569 / 1.000000
  corr mean min/used/max:        0.511390 / 0.899047 / 0.908783
  max corr values per period:    16
  num corr values calc/iter:     8318 / 14673079
  num corr means calc/iter:      1271 / 87009
  corr means mem accum/max:      25 / 409600 Kb
  corr means mem used:           25 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.012
    corr mean values:             98 | 0.695
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.707

  ---

  + /si 15727
  + /corr-min 0.81
  + /use-linear-corr

  >
  bitsync /impl-mwsocm /corr-min 0.81 /use-linear-corr /si 15727 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_mixed_offset-[907+927-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        927
  period (width):                2964
  stream bit length:             15728
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 5242
  period min/used/max repeat:    3 / 4 / 16
  user corr value/mean min:      0.810000 / 0.810000
  corr value min/max:            0.385187 / 1.000000
  corr mean min/used/max:        0.829126 / 1.000000 / 1.000000
  max corr values per period:    16
  num corr values calc/iter:     883 / 672515
  num corr means calc/iter:      49 / 49
  corr means mem accum/max:      1 / 409600 Kb
  corr means mem used:           1 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.004
    corr mean values:             97 | 0.204
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.208

  ---

  + /si 32000
  + /use-linear-corr

  >
  bitsync /impl-mwsocm /use-linear-corr /si 32000 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_mixed_offset-[907+927-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.710000 / 0.810000
  corr value min/max:            0.311642 / 1.000000
  corr mean min/used/max:        0.710590 / 0.951845 / 0.957622
  max corr values per period:    16
  num corr values calc/iter:     14699 / 35068600
  num corr means calc/iter:      29927 / 1004229
  corr means mem accum/max:      583 / 409600 Kb
  corr means mem used:           583 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   0 | 0.008
    corr mean values:             98 | 0.852
    corr weighted means sum:       0 | 0.004
    all:                         100 | 0.865

  ---

  + /si 15727
  + /corr-mm-dispersed-value-prime1033
  + /use-linear-corr

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /use-linear-corr /si 15727 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_mixed_offset-[907+927-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        927
  period (width):                2964
  stream bit length:             15728
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 5242
  period min/used/max repeat:    3 / 4 / 16
  user corr value/mean min:      0.710000 / 0.810000
  corr value min/max:            0.129115 / 1.000000
  corr mean min/used/max:        0.711143 / 1.000000 / 1.000000
  max corr values per period:    16
  num corr values calc/iter:     4648 / 4295420
  num corr means calc/iter:      206 / 37466
  corr means mem accum/max:      5 / 409600 Kb
  corr means mem used:           5 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   3 | 0.006
    corr mean values:             96 | 0.177
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.183

  ---

  + /si 32000
  + /corr-mm-dispersed-value-prime1033
  + /use-linear-corr

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /use-linear-corr /si 32000 /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync test_w_mixed_offset-[907+927-2964-1d0af].bin .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.710000 / 0.810000
  corr value min/max:            0.139889 / 1.000000
  corr mean min/used/max:        0.710481 / 0.946308 / 0.951406
  max corr values per period:    16
  num corr values calc/iter:     9635 / 17753700
  num corr means calc/iter:      934 / 159163
  corr means mem accum/max:      19 / 409600 Kb
  corr means mem used:           19 Kb
  input noise pttn bits/prob:    -
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.008
    corr mean values:             98 | 0.702
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.710

-------------------------------------------------------------------------------
4. To sync with mixed offset and input noise
-------------------------------------------------------------------------------

  + /inn 5 100
  + /si 15727
  + /corr-min 0.65

  >
  bitsync /impl-mwsocm /si 15727 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync "test_w_mixed_offset-[907+927-2964-1d0af].bin" .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        927
  period (width):                2964
  stream bit length:             15728
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 5242
  period min/used/max repeat:    3 / 4 / 16
  user corr value/mean min:      0.650000 / 0.650000
  corr value min/max:            0.101221 / 0.902332
  corr mean min/used/max:        0.651467 / 0.801276 / 0.801276
  max corr values per period:    16
  num corr values calc/iter:     1506 / 1132611
  num corr means calc/iter:      335 / 335
  corr means mem accum/max:      7 / 409600 Kb
  corr means mem used:           7 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   2 | 0.004
    corr mean values:             97 | 0.151
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.156

  ---

  + /inn 5 100
  + /si 32000

  >
  bitsync /impl-mwsocm /si 32000 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync "test_w_mixed_offset-[907+927-2964-1d0af].bin" .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.510000 / 0.650000
  corr value min/max:            0.052524 / 0.900966
  corr mean min/used/max:        0.526649 / 0.755574 / 0.789074
  max corr values per period:    16
  num corr values calc/iter:     13367 / 29859157
  num corr means calc/iter:      54812 / 661088
  corr means mem accum/max:      972 / 409600 Kb
  corr means mem used:           972 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   0 | 0.007
    corr mean values:             98 | 0.829
    corr weighted means sum:       0 | 0.007
    all:                         100 | 0.844

  ---

  + /inn 5 100
  + /si 15727
  + /corr-mm-dispersed-value-prime1033

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /si 15727 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync "test_w_mixed_offset-[907+927-2964-1d0af].bin" .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        927
  period (width):                2964
  stream bit length:             15728
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 5242
  period min/used/max repeat:    3 / 4 / 16
  user corr value/mean min:      0.510000 / 0.650000
  corr value min/max:            0.016671 / 0.900314
  corr mean min/used/max:        0.512333 / 0.742424 / 0.742424
  max corr values per period:    16
  num corr values calc/iter:     4651 / 4151623
  num corr means calc/iter:      662 / 37214
  corr means mem accum/max:      13 / 409600 Kb
  corr means mem used:           13 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   2 | 0.004
    corr mean values:             97 | 0.174
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.178

  ---

  + /inn 5 100
  + /si 32000
  + /corr-mm-dispersed-value-prime1033

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /si 32000 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync "test_w_mixed_offset-[907+927-2964-1d0af].bin" .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.510000 / 0.650000
  corr value min/max:            0.018313 / 0.908288
  corr mean min/used/max:        0.511217 / 0.697552 / 0.746621
  max corr values per period:    16
  num corr values calc/iter:     9698 / 17691635
  num corr means calc/iter:      3242 / 169260
  corr means mem accum/max:      64 / 409600 Kb
  corr means mem used:           64 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.013
    corr mean values:             98 | 0.699
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.712

  ---

  + /inn 5 100
  + /si 15727
  + /use-linear-corr
  + /corr-min 0.81

  >
  bitsync /impl-mwsocm /use-linear-corr /corr-min 0.81 /si 15727 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync "test_w_mixed_offset-[907+927-2964-1d0af].bin" .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        927
  period (width):                2964
  stream bit length:             15728
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 5242
  period min/used/max repeat:    3 / 4 / 16
  user corr value/mean min:      0.810000 / 0.810000
  corr value min/max:            0.216927 / 0.948952
  corr mean min/used/max:        0.822814 / 0.883379 / 0.883379
  max corr values per period:    16
  num corr values calc/iter:     928 / 638298
  num corr means calc/iter:      48 / 48
  corr means mem accum/max:      1 / 409600 Kb
  corr means mem used:           1 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   2 | 0.004
    corr mean values:             97 | 0.158
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.163

  ---

  + /inn 5 100
  + /si 32000
  + /use-linear-corr

  >
  bitsync /impl-mwsocm /use-linear-corr /si 32000 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync "test_w_mixed_offset-[907+927-2964-1d0af].bin" .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          1 / inverted-xor-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.710000 / 0.810000
  corr value min/max:            0.308114 / 0.974693
  corr mean min/used/max:        0.710737 / 0.851762 / 0.887032
  max corr values per period:    16
  num corr values calc/iter:     14543 / 34613767
  num corr means calc/iter:      35302 / 952386
  corr means mem accum/max:      682 / 409600 Kb
  corr means mem used:           682 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   0 | 0.007
    corr mean values:             98 | 0.867
    corr weighted means sum:       0 | 0.005
    all:                         100 | 0.880

  ---

  + /inn 5 100
  + /si 15727
  + /corr-mm-dispersed-value-prime1033
  + /use-linear-corr

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /use-linear-corr /corr-min 0.81 /si 15727 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync "test_w_mixed_offset-[907+927-2964-1d0af].bin" .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        927
  period (width):                2964
  stream bit length:             15728
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 5242
  period min/used/max repeat:    3 / 4 / 16
  user corr value/mean min:      0.810000 / 0.810000
  corr value min/max:            0.129115 / 0.953663
  corr mean min/used/max:        0.886075 / 0.886075 / 0.886075
  max corr values per period:    16
  num corr values calc/iter:     488 / 343677
  num corr means calc/iter:      1 / 1
  corr means mem accum/max:      1 / 409600 Kb
  corr means mem used:           1 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   4 | 0.007
    corr mean values:             95 | 0.154
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.161

  ---

  + /inn 5 100
  + /si 32000
  + /corr-mm-dispersed-value-prime1033
  + /use-linear-corr

  >
  bitsync /impl-mwsocm /corr-mm-dispersed-value-prime1033 /use-linear-corr /si 32000 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 /skip-calc-on-fcvu sync "test_w_mixed_offset-[907+927-2964-1d0af].bin" .

  <
  impl token:                    1 / max-weighted-sum-of-corr-mean
  corr multiply method:          2 / dispersed-value-prime1033
  syncseq length/value:          20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  stream bit length:             32000
  max periods {in} offset:       0
  period {in} min/max:           0 / -
  period {io} min/max:           40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr value/mean min:      0.710000 / 0.810000
  corr value min/max:            0.137378 / 0.951797
  corr mean min/used/max:        0.710436 / 0.849785 / 0.873859
  max corr values per period:    16
  num corr values calc/iter:     11132 / 22293981
  num corr means calc/iter:      1792 / 306352
  corr means mem accum/max:      35 / 409600 Kb
  corr means mem used:           35 Kb
  input noise pttn bits/prob:    5 / 100 %
  modification flags:
                                 /use-linear-corr
                                 /skip-calc-on-fcvu
  calc phase times {% | sec}:
    corr values:                   1 | 0.014
    corr mean values:             98 | 0.741
    corr weighted means sum:       0 | 0.000
    all:                         100 | 0.756
