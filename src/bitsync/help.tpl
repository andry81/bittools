[+ AutoGen5 template txt=%s.txt +]
[+ AppModuleName +].exe, version [+ AppMajorVer +].[+ AppMinorVer +].[+ AppRevision +], build [+ AppBuildNum +].
  Bit stream synchronization and operation utility.

Usage: [+ AppModuleName +].exe [/?] [<Flags>] [/impl-token <token>] [//] <Mode> [<BitsPerBaud>] <InputFile> [<OutputFileDir> | <OutputFile>]
       [+ AppModuleName +].exe [/?] [<Flags>] [//] sync <InputFile> [<OutputFileDir>]
       [+ AppModuleName +].exe [/?] [<Flags>] [/gen-token ...] [//] gen <BitsPerBaud> <InputFile> [<OutputFileDir>]
       [+ AppModuleName +].exe [/?] [<Flags>] [/gen-input-noise ...] [//] pipe <InputFile> <OutputFile>

  Description:
    /?
    This help.

    //:
    Character sequence to stop parse <Flags> command line parameters.

    <Flags>:
      /impl-token <token>
        Implementation method represented as a token:

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

          NOTE:
            The `/max-corr-values-per-period` option has memory consumption
            effect and does increase the algorithm major memory complexity.

          Pros:
            * Most resistant to the input noise, can ignore at maximum ~33%
              of noised bits per a synchro sequence length (for example, at
              maximum ~6 bits per each 20 bits of bit stream).

          Cons:
            * Too slow to be invoked on a bit stream in a non stop mode or
              on a bit stream with a big period (~ >10000 bits).
            * At maximum has a greater than quadratic memory consumption, so
              can consume too much memory and as a result by default does cut
              off the search increasing uncertainty of a result
              (See `/corr-mean-buf-max-size-mb` option description).

          Can not be used together with another `/impl-*` options.

          This is default implementation.

        /impl-token min-sum-of-corr-mean-deviat
        /impl-msocmd

          Calculate through the minimum sum of correlation mean devation
          values.

          Algorithm:
            Phase 1:   Calculate correlation values.
            Phase 2.1: Calculate correlation mean values.
            Phase 2.2: Calculate minimum sum of correlation mean deviation
                       values.

          Major time complexity: N * N * ln(N)
            , where N - stream bit length

          NOTE:
            The `/max-corr-values-per-period` option has memory consumption
            effect and does increase the algorithm major memory complexity.

          Cons:
            * Slower than implementation with calculation of maximum weighted
              sum of correlation mean values.
            * Not resistant to the input noise.

          Left as an alternative to analize a bit stream deviation from a mean
          values.

          Can not be used together with another `/impl-*` options.

        Has meaning only for these modes: sync | gen-sync.

      /corr-multiply-method <token>
      /corr-mm <token>
        Correlation multiplication method to use.

        /corr-multiply-method inverted-xor-prime1033
        /corr-mm-inverted-xor-prime1033

          The xor is treated as inverted (count zeros instead ones), because
          zero difference must gain a highest positive value. The maximum
          difference must gain a lowest positive value, but not a zero.
          As a result, is less the difference, then is higher the resulted
          value, so the multiplication of equal bit sequences will gain the
          highest value. Only after that we can convert the integer to the
          floating point number range (0; 1.0].

          This is default implementation.

          Can not be used together with another `/corr-multiply-method`
          option.

        /corr-multiply-method dispersed-value-prime1033
        /corr-mm-dispersed-value-prime1033

          The xor might be not enough because it gains not enough unique or
          low dispersion result. In that case we can increase multiplication
          dispersion to improve correlation certainty which is relied on more
          arranged or wider spectrum of a correlation value.

          Can not be used together with another `/corr-multiply-method`
          option.

        Has meaning only for these modes: sync | gen-sync.

      /no-zero-corr
        Avoid zero correlation values replacing them by `/corr-min` or by a
        minimal positive value.

        If `/corr-min` is defined to `0` or not defined, then a default
        builtin value is used instead:

          `0.51` - if `/use-linear-corr` is not defined.
          `0.71` - if `/use-linear-corr` is defined.

      /use-linear-corr

        Take a square root from each correlation value to convert it back to
        linear distribution. If not defined, then use quadratic correlation
        values as is.

      /use-max-corr-mean
      /skip-max-weighted-sum-of-corr-mean-calc
      /skip-mwsocm-calc
        Skip calculation of maximum weighted sum of correlation mean values and
        use maximal correlation mean value instead.

        Has meaning if the algorithm of maximum weighted sum of correlation
        mean values is used.

        CAUTION:
          This flag will decrease certainty of a result (increase false
          positive instability) in case of input noise or mixed
          synchro sequence offset/period.

      /sort-at-first-by-max-corr-mean
        Sort at first by maximum correlation mean and at second by minimum
        correlation mean deviation sum. By default only the sort by minimum
        correlation mean deviation sum is used.

        Has meaning if the algorithm of minimum sum of correlation mean
        deviation values is used.

        Has meaning when need to debug the algorithm for false positives.

        CAUTION:
          The first sort is limited by the size of container which takes into
          account by `/max-corr-values-per-period` option. So the order of the
          sort will lose of different minor elements.

      /return-sorted-result
        Do full sort of a result array by all fields instead of search and
        return a single field array as by default.

        Has meaning if the algorithm accumulating correlation values is used.

        Has meaning when need to debug the algorithm for result certainty or
        false positive instability.

      /stream-byte-size <size>
      /s <size>
        Stream size in bytes to process.

        In case of `sync` mode must be greater than 4 bytes (32 bits) and less
        than 2^32 bytes.

        CAUTION:
          To sync must be enough to fit the real stream period, otherwise the
          certainty of a result would be not enough and the calculated offset
          and period will be inaccurate or incorrect independently to the
          input noise.

      /stream-bit-size <size>
      /si <size>
        Stream size in bits to process. The same as `/stream-byte-size` but as
        number of bits.

        CAUTION:
          If a value is not a multiple of a byte, then the stream remainder
          may left unprocessed.

      /stream-min-period <value>
      /spmin <value>
        Suggests a bit stream minimum period to start search with to calculate
        correlation mean values or weighted correlation values.
        Can significally decrease memory and time consumption described for
        the `/corr-mean-buf-max-size-mb` parameter.

        Must be greater than `/syncseq-bit-size` parameter value, but less
        than `/stream-byte-size` parameter value multiply 8.

        CAUTION:
          Must be not lesser than the real stream period, otherwise the
          certainty of a result would be not enough and the calculated offset
          and period will be inaccurate or incorrect independently to the
          input noise.

        Has priority over `/stream-max-period` option.

      /stream-max-period <value>
      /spmax <value>
        Suggests a bit stream maximum period to start search with to calculate
        correlation mean values or weighted correlation values.
        Can significally decrease memory and time consumption described for
        the `/corr-mean-buf-max-size-mb` parameter.

        Must be greater than `/syncseq-bit-size` parameter value and greater
        or equal than `/stream-min-period` parameter.

        CAUTION:
          Must be not greater than the real stream period, otherwise the
          certainty of a result would be not enough and the calculated offset
          and period will be inaccurate or incorrect independently to the
          input noise.

      /max-periods-in-offset <value>
      /max-pio <value>
        Suggests maximal number of periods contained in an offset which
        correlation value is being accumulated. Does the calculation
        cutoff for each period when an offset becomes greater than requested
        number of periods. Can significally decrease memory and time
        consumption described for the `/corr-mean-buf-max-size-mb` parameter.

        Has meaning if the algorithm accumulating correlation values is used.

        NOTE:
          Has different meaning opposed to `/syncseq-max-repeat` option, where
          `/syncseq-max-repeat` describes maximum period repeats from an
          offset, when `/max-periods-in-offset` describes maximum periods in
          an offset (to an offset) to take offset into account.

        This is additional option to cut off the calculations without
        significantly or noticeably reduce certainty of a result. Otherwise
        you must increase this parameter to increase calculation certainty.

        Values:
          -1 = no limit
           0 = 1 period excluding first bit of 2d period
           1 = 1 period including first bit of 2d period
          >1 = N periods including first bit of N+1 period

        Default value is `0`.

        CAUTION:
          Must be enough to accumulate enough certainty for a result,
          otherwise the calculated offset and period will be inaccurate or
          incorrect independently to the input noise.

      /max-corr-values-per-period <value>
      /max-cvpp <value>
        Number of top maximum correlation values per period or correlation
        mean values per period or correlation mean deviation sum values per
        period, taken into account.

        All values still are greater or equal than `/corr-min` option for
        correlation values and than `/corr-mean-min` option for correlation
        mean values (both if defined).

        Has meaning if the algorithm accumulating correlation values is used.

        NOTE:
          Has effect on the algorithm major memory complexity, where a greater
          is a more memory consumption.
          Has effect on the algorithm minor time complexity, where a greater
          is a more time consumption.

        Default value is `16`.

      /syncseq-bit-size <value>
      /q <value>
        Synchro sequence (synchro mark) size in bits (must be less or equal
        to 32 bits).

      /syncseq-int32 <value>
      /k <value>
        Synchro sequence value to search for (must be not 0).
        The `<value>` is an integer positive 32-bit number.

      /syncseq-min-repeat <value>
      /r <value>
        Synchro sequence minimal repeat quantity to treat it as found.
        The `<value>` is an integer positive 32-bit number.

        Has meaning if the algorithm accumulating correlation values is used.

        CAUTION:
          The stream size should be enough to make requested repeats
          because each succeeded repeat does make at least of in distance of a
          stable stream period. You must proportionally increase stream size
          because repeat value decreases stream search length proportionally,
          otherwise the certainty of a result would be not enough and the
          calculated offset and period will be incorrect independently to the
          input noise.

        Has priority over `/syncseq-max-repeat` option.

      /syncseq-max-repeat <value>
      /rmax <value>
        Synchro sequence maximal repeat quantity to search for.
        The `<value>` is an integer positive 32-bit number.

        NOTE:
          Has different meaning opposed to `/max-periods-in-offset` option,
          where `/syncseq-max-repeat` describes maximum period repeats for an
          offset, when `/max-periods-in-offset` describes maximum periods in
          an offset to take offset into account.

        CAUTION:
          In case of a big value can slow down the correlation mean values
          calculation (if is used).

        Default value is `16` but is not less than the mimimal repeat
        quantity.

      /gen-token <token>
      /g <token>
        Generate output only for the combination represented as a token:
        `<bit-shift>-<combination-index>`, where:

        <bit-shift>               : 0-1
        <combination-index>       : 0-23

        Has meaning only for these modes: gen | gen-sync.

      /gen-input-noise <bit-block-size> <probability-per-block>
      /inn <bit-block-size> <probability-per-block>
        Generate noise in the input:

        <bit-block-size>          : 1-32767
          Bit block size where a single bit would be inverted with defined
          probability.

        <probability-per-block>   : 1-100
          Probability for a single bit inversion in each bits block.

        Has meaning only for these modes: gen | sync | pipe | gen-sync.

        Examples:

          1. `/inn 1 100`
            Invert all bits.
          2. `/inn 8 100`
            Invert random bit in each byte.

      /insert-output-syncseq <first_offset>[:<end_offset>] <period>[:<repeat>]
      /outss <first_offset>[:<end_offset>] <period>[:<repeat>]
        Insert synchro sequence in the output by <first_offset> and <period>.
        The options `/syncseq-int32` and `/syncseq-bit-size` must be defined.

        If <end_offset> is defined, then does stop the insert from the
        <end_offset>, otherwise continue until the end of stream.
        If <repeat> is defined, then does repeat the insert that set of times,
        otherwise repeat until the end of stream.

        Has meaning only for these modes: gen | pipe | gen-sync.

      /fill-output-syncseq <first_offset>[:<end_offset>] <period>[:<repeat>]
      /outssf <first_offset>[:<end_offset>] <period>[:<repeat>]
        The same as `/insert-output-syncseq` but does fill instead of the
        insert overwriting the input bits (does not increase the output size).

        Has no effect if `/insert-output-syncseq` option is used.

      /tee-input <file>
        Duplicate input into <file>.
        Can be used to output the noised input.

      /corr-min <value>
        Correlation minimum floating point value to treat it as certain.
        Does filter out uncertain offset and period result if a found
        correlation value is less.

        Has relation with `/use-linear-corr` flag.

        Must be in range [0.0; 1.0].

        Default value is:

            `0.65` - if correlation is linear.
            `0.42` - if correlation is quadratic.

      /corr-mean-min <value>
        Correlation mean minimum floating point value to treat it as certain.
        Does filter out uncertain offset and period result if a found
        correlation mean value is less.

        Has meaning if the algorithm accumulating correlation values is used.

        NOTE:
          The `/max-corr-values-per-period` option here takes into account the
          number of maximum correlation mean values for each period.

        Must be in range [0.0; 1.0].

        Default value is `0.81`.

      /skip-calc-on-filtered-corr-value-use
      /skip-calc-on-fcvu
        Skip calculation if a filtered correlation value is used (filtered by
        `/corr-min` option).

        Has meaning if the algorithm accumulating correlation values is used.

      /corr-mean-buf-max-size-mb <value>
        Maximum buffer size in megabytes to store calculated correlation mean
        values. Basically needs (T - TMIN) ^ 2 * 24 bytes of the buffer to
        calculate values with enough certainty, where:

          T     - real period of a bit stream greater than `/syncseq-bit-size`
                  parameter.
          TMIN  - value of `/stream-min-period` parameter or value of
                  `/syncseq-bit-size` parameter.

        Has meaning if the algorithm accumulating correlation values is used.

        CAUTION:
          If the buffer is not enough, then the certainty of correlation mean
          values would be not enough and the calculated offset and period may
          be inaccurate or incorrect independently to the input noise.

        Default value is 400MB.

    If `/stream-byte-size` option is not used, then the whole input is read
    but less than 2^32 bytes.

    <Mode>: gen | sync | pipe | gen-sync
      gen       - generation mode, multiple output.
      sync      - synchronization mode, single output.
      pipe      - pipe mode, connects a single input with a single output.
      gen-sync  - generation and synchronization mode, single output.

      NOTE:
        The `get-sync` mode is not yet implemented.

    <BitsPerBaud>
      Bits per baud in stream (must be <= 2).

      Has meaning only for these modes: gen | gen-sync.

    <InputFile>
      Input file path.

    <OutputFileDir>
      Output directory path for output files (current directory if not set).

    <OutputFile>
      Output file path for single output file.

  Pipeline variants:

    * [single input] -> [generation] -> [multiple output]    (`gen`)
    * [single input] -> [generation] -> [single output]      (`pipe` or `gen`
                                                              + `gen-token`)
    * [single input] -> [synchronization] -> [single output] (`sync`)
    * [single input] -> [generation] -> [multiple output] ->
      -> [synchronization] -> [multiple output]              (`gen-sync`)
    * [single input] -> [generation] -> [single output] ->
      -> [synchronization] -> [single output]                (`gen-sync` +
                                                              `gen-token`)

    , where:

      [single input/output] - does use as a file path.
      [multiple output]     - does use as a directory path or intermediate
        in-memory stage.

  Features:

    * Supports only 2-bits per baud streams as the input.

    * Can translate 2-bit per baud stream (B=2) into `(2^B)! x B = 48`
      variants and search in all the generated streams for the known
      synchro sequence (synchro mark) to find a stream period and closest
      available bit offset of the synchro sequence in the stream.
      The result is a multiple variants output.

    * Can output all the generated stream variants into files in a directory.

    * Can generate noise for the input to test synchronization stability.

    * Can work in the `pipe` mode to output a single stage output, for
      example, output noised input.

    * Can work in the `sync` mode to use an externally generated stream
      variant as the input.

    * (Not implemented) Can work in the `gen-sync` mode, where does execute
      both stages the generation and the synchronization, and all the
      intermediate operations does use the memory instead of files to produce
      an output.
