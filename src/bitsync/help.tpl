[+ AutoGen5 template txt=%s.txt +]
[+ AppModuleName +].exe.
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

      /syncseq-bit-size <value>
      /q <value>
        Synchro sequence size in bits (must be less or equal to 32 bits).

      /syncseq-int32 <value>
      /k <value>
        Synchro sequence value to search for (must be not 0).
        `<value>` is an integer positive 32-bit number.

      /syncseq-repeat <value>
      /r <value>
        Synchro sequence minimal repeat quantity to treat it as found.
        `<value>` is an integer positive 32-bit number.

      /gen-token <token>
      /g <token>
        Generate output file only for the combination represented as a token:
        `<bit-shift>-<combination-index>`, where:
        <bit-shift>:          0-1
        <combination-index>:  0-23

        Has meaning only for these modes: gen.

      /gen-input-noise <bit-block-size> <probability-per-block>
      /inn <bit-block-size> <probability-per-block>
        Generate noise in the input file:
        <bit-block-size>:               1-32767
          Bit block size where a single bit would be inverted with defined
          probability.
        <probability-per-block>: 1-100
          Probability for a single bit inversion in each bits block.

        Has meaning only for these modes: pipe.

        Examples:
          1. `/inn 1 100`
            Invert all bits.
          2. `/inn 8 100`
            Invert random bit in each byte.

    If `/stream-byte-size` option is not used, then the whole input file is
    read but less than 2^32 bytes.

    <Mode>: gen | sync | pipe
      gen     - generate mode, multiple output files.
      sync    - synchronization mode, single output file.
      pipe    - pipe mode, connect input file with sigle output file.

    <BitsPerBaud>
      Bits per baud in stream (must be <= 2).

    <InputFile>
      Input file path.

    <OutputFileDir>
      Output directory path for output files (current directory if not set).

    <OutputFile>
      Output file path for single output file.
