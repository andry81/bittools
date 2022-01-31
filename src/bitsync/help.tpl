[+ AutoGen5 template txt=%s.txt +]
[+ AppModuleName +].exe, version [+ AppMajorVer +].[+ AppMinorVer +].[+ AppRevision +], build [+ AppBuildNum +].
  Bit stream synchronization and operation utility.

Usage: [+ AppModuleName +].exe [/?] [<Flags>] [//] <Mode> [<BitsPerBaud>] <InputFile> [<OutputFileDir> | <OutputFile>]
       [+ AppModuleName +].exe [/?] [<Flags>] [//] sync <BitsPerBaud> <InputFile> [<OutputFileDir>]
       [+ AppModuleName +].exe [/?] [<Flags>] [/gen-token ...] [//] gen <BitsPerBaud> <InputFile> [<OutputFileDir>]
       [+ AppModuleName +].exe [/?] [<Flags>] [/gen-input-noise ...] [//] pipe <InputFile> <OutputFile>
  Description:
    /?
    This help.

    //:
    Character sequence to stop parse <Flags> command line parameters.

    <Flags>:
      /stream-byte-size <size>
      /s <size>
        Stream size in bytes to search for synchro sequence (must be greater
        than 4 bytes (32 bits) and less than 2^32 bytes).

        CAUTION:
          Must be enough to fit the real stream period, otherwise the
          certainty of autocorrelation mean values would be not enough and the
          calculated offset and period will be incorrect independently to the
          input noise.

      /stream-min-period <value>
      /spmin <value>
        Suggests a bit stream minimum period to start search with to calculate
        autocorrelation mean values. Can significally decrease memory
        consumption described for the `/autocorr-mean-buf-max-size-mb`
        parameter.

        Must be greater than `/syncseq-bit-size` parameter value, but less
        than `/stream-byte-size` parameter value multiply 8.

        CAUTION:
          Must be not lesser than the real stream period, otherwise the
          certainty of autocorrelation mean values would be not enough and the
          calculated offset and period will be incorrect independently to the
          input noise.

      /stream-max-period <value>
      /spmax <value>
        Suggests a bit stream maximum period to start search with to calculate
        autocorrelation mean values. Can significally decrease memory
        consumption described for the `/autocorr-mean-buf-max-size-mb`
        parameter.

        Must be greater than `/syncseq-bit-size` parameter value and greater
        or equal than `/stream-min-period` parameter.

        CAUTION:
          Must be not greater than the real stream period, otherwise the
          certainty of autocorrelation mean values would be not enough and the
          calculated offset and period will be incorrect independently to the
          input noise.

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

        NOTE:
          The stream size should be enough to make requested repeats
          because each succeeded repeat does make at least of in distance of a
          stable stream period. You must proportionally increase stream size
          because repeat value decreases stream search length proportionally,
          otherwise the certainty of autocorrelation mean values would be not
          enough and the calculated offset and period will be incorrect
          independently to the input noise.

        CAUTION:
          If the synchro sequence width is used instead of a value,
          then the repeat option value should be enough to not result in a
          false positive compare.

      /syncseq-max-repeat <value>
      /rmax <value>
        Synchro sequence maximal repeat quantity to search for.
        The `<value>` is an integer positive 32-bit number.

        Default value is 16 but is not less than the mimimal repeat quantity.

        CAUTION:
          In case of a big value can slow down the autocorrelation mean values
          calculation.

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

      /tee-input <file>
        Duplicate input into <file>.
        Can be used to output the noised input.

      /autocorr-mean-min <value>
        Autocorrelation minimum mean value to treat it as certain.
        Can filter out uncertain offset and period result if found maximum
        autocorrelation mean value is less.

        If autocorrelation mean values algorithm is disabled then used as
        minimum for an autocorrelation value.

        Must be in range [0; 1].

      /autocorr-mean-buf-max-size-mb <value>
        Maximum buffer size in megabytes to store calculated autocorrelation
        mean values. Basically needs (T - TMIN) ^ 2 * 24 bytes of the buffer
        to calculate values with enough certainty, where:

          T     - real period of a bit stream greater than `/syncseq-bit-size`
                  parameter.
          TMIN  - value of `/stream-min-period` parameter or value of
                  `/syncseq-bit-size` parameter.

        CAUTION:
          If the buffer is not enough, then the certainty of autocorrelation
          mean values would be not enough and the calculated offset and period
          will be incorrect independently to the input noise.

        Default value is 400MB.

      /disable-calc-autocorr-mean
        Disable autocorrelation mean values calculation.
        By disabling that algorithm does increase uncertainty level of found
        offset within the input with noise, but can avoid consumption of much
        memory and time to compute it.
        Use it only for low levels of noise.

        CAUTION:
          Period search in case of disabled autocorrelation mean calculation
          is not implemented.

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

    * Can work in the `gen-sync` mode, where does execute both stages the
      generation and the synchronization, and all the intermediate operations
      does use the memory instead of files to produce an output.
