[+ AutoGen5 template txt=%s.txt +]
[+ AppModuleName +].exe.
  Bit stream synchronization and operation utility.

Usage: [+ AppModuleName +].exe [/?] [<Flags>] [//] <Mode> <BitsPerBaud> <InputFile> [<OutputFileDir>]
       [+ AppModuleName +].exe [/?] [<Flags>] [/gen_index <index>] [//] gen <BitsPerBaud> <InputFile> [<OutputFileDir>]
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
        Synchro sequence size in bits (must be less or equal to 32 bits).

      /syncseq-int32 <value>
        Synchro sequence value to search for (must be not 0).
        `<value>` is an integer positive 32-bit number.

      /syncseq-repeat <value>
        Synchro sequence minimal repeat quantity to treat it as found.
        `<value>` is an integer positive 32-bit number.

    <Mode>: gen | sync
      gen     - generate mode.
      sync    - synchronization mode.

    <BitsPerBaud>
      Bits per baud in stream (must be <= 2).

    <InputFile>
      Input file path.

    <OutputFileDir>
      Output directory path for output files (current directory if not set).
