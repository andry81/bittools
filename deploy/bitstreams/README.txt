File name format:

<name>-[<offset-dec>-<period-dec>-<syncseq-hex>].bin

Examples:

* to sync:

  >
  bitsync.exe /autocorr-min 0.95 /s 4000 /k 0x1d0af /q 20 /r 3 sync test_w_offset-[907-2964-1d0af].bin .

  <
  length/value:                  20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  max periods in offset:         16
  period in min/max:             0 / -
  period out min/max:            40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr min:                 0.950000
  corr value min/max:            0.119832 / 1.000000
  corr mean min/used/max:        0.316829 / 1.000000 / 1.000000
  num corr means calc:           143708462
  num corr values iter:          1105447163
  corr means mem accum/max:      1 / 409600 Kb
  corr means mem used:           632 Kb
  stream bit length:             32000

* to sync with input noise:

  >
  bitsync.exe /autocorr-min 0.81 /s 4000 /inn 5 100 /tee-input test_input_w_noise.bin /k 0x1d0af /q 20 /r 3 sync test_w_offset-[907-2964-1d0af].bin .

  <
  length/value:                  20 / 0x0001d0af
  offset:                        907
  period (width):                2964
  max periods in offset:         16
  period in min/max:             0 / -
  period out min/max:            40 / 10666
  period min/used/max repeat:    3 / 10 / 16
  user corr min:                 0.810000
  corr value min/max:            0.130461 / 0.920720
  corr mean min/used/max:        0.223689 / 0.881969 / 0.891035
  num corr means calc:           143708462
  num corr values iter:          1105447163
  corr means mem accum/max:      53 / 409600 Kb
  corr means mem used:           632 Kb
  stream bit length:             32000
