# cwavtool

A tool for converting **WAV/OGG** files to [**(B)CWAV**](https://www.3dbrew.org/wiki/BCWAV) files.

## Usage

This tool can convert to any encoding supported by the **(B)CWAV** file format (pcm16 by default). Optionally, a loop point can be specified:
```
> cwavtool.exe <args>
Available arguments:
  -i/--input: WAV/OGG input file.
  -o/--output: CWAV output file.
  -e/--encoding: Optional. Encoding of the created CWAV (pcm8/pcm16/imaadpcm/dspadpcm).
  -ls/--loopstartframe: Optional. Sample to return to when looping.
  -le/--loopendframe: Optional. Sample to loop at or "end".
```

## Credits & License
- This project is a modified work of [Steviece10's bannertool](https://github.com/Steveice10/bannertool) and is licensed under the [MIT License](LICENSE).
- This project uses [David Bryant's adpcm-xq](https://github.com/dbry/adpcm-xq) for IMA-ADPCM encoding ([License](source/3ds/imaadpcm/LICENSE)).
- This project uses [Jack Andersen's gc-dspadpcm-encode](https://github.com/jackoalan/gc-dspadpcm-encode) for DSP-ADPCM encoding ([License](source/3ds/dspadpcm/LICENSE)).