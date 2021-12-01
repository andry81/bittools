[+ AutoGen5 template txt=%s.txt +]
[+ AppModuleName +].exe.
  Bit stream synchronization and operation utility.

Usage: [+ AppModuleName +].exe [/?] [<Flags>] [//] <Mode> <BitsPerBaud> <InputFile> [<OutputFileDir>]
       [+ AppModuleName +].exe [/?] [<Flags>] [/gen_token <token>] [//] gen <BitsPerBaud> <InputFile> [<OutputFileDir>]
  Description:
    /?
    This help.

    //:
    Character sequence to stop parse <Flags> command line parameters.

    <Flags>:
      /stream-byte-size <size>
      /s <size>
        Stream size in bytes to search for synchro sequence (must be greater
        than 32 bits).

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
        bit-shift:          0-1
        combination-index:  0-23

    <Mode>: gen | sync
      gen     - generate mode.
      sync    - synchronization mode.

    <BitsPerBaud>
      Bits per baud in stream (must be <= 2).

    <InputFile>
      Input file path.

    <OutputFileDir>
      Output directory path for output files (current directory if not set).
