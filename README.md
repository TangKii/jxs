# JPEG XS Reference Software - 2nd Edition

## License and copyright

Please see the accompanied LICENSE.md for more details.

## Introduction

This source code package represents an integral part of ISO/IEC 21122-5 Reference Software. It provides the reference software  implementation of the ISO/IEC 21122 series (JPEG XS). The code has been successfully compiled and tested on Linux, Windows, and MacOS operating systems at the time of writing.

It is important to note that this reference software implementation represents just one way of implementing JPEG XS. Alternative implementations that comply to the ISO/IEC 21122 standards are possible and equally valid. This implementation can serve as a validation anchor to other implementations.

No guarantee of the coding quality and performance that will be achieved by an encoder is provided by its conformance to ISO/IEC 21122-1, as the conformance is only defined in terms of specific constraints imposed on the syntax of the generated codestream. In particular, while sample encoder software implementations may suffice to provide some illustrative examples of which quality can be achieved within the ISO/IEC 21122 series, they provide neither an assurance of minimum guaranteed image encoding quality nor maximum achievable image encoding quality.

Similarly, the computation resource characteristics in terms of program or data memory usage, execution speed, etc of sample software encoder or decoder implementations should not be construed as a representative of the typical, minimal or maximal computational resource characteristics to be exhibited by implementations of ISO/IEC 21122-1.

Finally, despite the fact that this implementation was tested to conform to the ISO/IEC 21122 standards, it is possible that bugs or mistakes exist in the code. The texts of the ISO/IEC 21122-1, 21122-2 and 21122-3 standards remain authorative in case of any discrepancy.

## How to build

- This repository uses CMake (3.12 or newer) to generate the build files.

- The code was written to work with GCC (9.3) and Visual Studio (16.9) compilers, but will likely compile with many other compilers and versions.

- For Windows and Visual Studio 2015 or newer, use the ```cmake-gui``` on Windows to generate a solution.

- For Linux, the easiest way it to use the default GNU Make builder:
  `mkdir _build`
  `cd _build`
  `cmake ..`
  `make`

## How to use

### Decoding

The decoder takes a .jxs file and generates from that a decoded image (many formats are supported, like PNM, YUV, and PGX). To
run it, use ```jxs_decoder [options] <codestream.jxs> <output>```.

The decoder provides the following options:

- ```-D```: To dump the actual JPEG XS configuration as a string. The string is a ```;```-separated line of XS settings. This string can be used by the encoder to encode using the exact same configuration. Applying the option twice (```-DD``` or ```-D -D```) will also show the gain and priority values. Outputs in the same syntax as used by the encoder ```-c``` option.
- ```-f <number>```: Tells the decoder to interpret the input and output as a sequence of files (for frame-based video sequencing). The number specifies the first frame index to process. To be used in combination with ```-n```.
- ```-n <number>```: Specifies the total number number of frames to process.
- ```-v```: Output verbose. Use multiple times to increase the verbosity.

Note: When using the ```-D``` option to show the XS configuration of the codestream, it is optional to specify an output file name. In the case that no output is given, the decoder will not decode the codestream (after showing the XS configuration).

Example:

```
jxs_decoder -v woman.jxs out.ppm
```

### Encoding

The encoder is capable of reading various input file formats, such as PNM (PPM/PGM), PGX, raw YUV, and raw RGB. Please check the ISO/IEC 21122-5 text for more information. To run it, use ```jxs_encoder [options] <input> <codestream.jxs>```.

The encoder provides the following options:

- ```-c <XS config>```: XS configuration arguments (semicolon separated to specify multiple XS options, but can also be specified multiple times). See below for more on the XS configuration syntax.
- ```-D```: To dump the actual JPEG XS configuration as a string. The string is a ```;```-separated line of XS settings. This string can be used by the encoder to encode using the exact same configuration. Applying the option twice (```-DD``` or ```-D -D```) will also show the gain and priority values. Outputs in the same syntax as used by the ```-c``` option.
- ```-f <number>```: Tells the decoder to interpret the input and output as a sequence of files (for frame-based video sequencing). The number specifies the first frame index to process. To be used in combination with ```-n```.
- ```-n <number>```: Specifies the total number number of frames to process.
- ```-v```: Output verbose. Use multiple times to increase the verbosity.
- ```-w <width>```: Specifies the width of the input file (required for raw input format like v210, yuv16, rgb16).
- ```-h <height>```: Specifies the height of the input file (required for raw input format like v210, yuv16, rgb16).
- ```-d <bitdepth>```: Specifies the bit depth of the input file (required for raw input format like v210, yuv16, rgb16).

Note: For raw input file formats, the number of components is implicitly derived from the file extension.

Examples:

```
jxs_encoder -c profile=Main444.12 -c rate=2 woman.ppm woman.jxs
jxs_encoder -c "profile=Main444.12;rate=2" woman.ppm woman.jxs  # same, but combining the -c options
```

## The XS configuration syntax

The encoder ```-c``` option and the encoder/decoder ```-D``` options all use the same syntax to describe JPEG XS codestream configuration parameters. These parameters are mostly one-to-one representations of codestream configuration options as described in ISO 21122-1 and 21122-2 (a few represent implicit properties, such as the target bitrate, target codestream size, or the rate-allocation line buffer budget).

The profiles, levels and sublevels that can be used are defined in ISO/IEC 21122-2. Setting either one of these will impose specific XS configuration settings and limitations. For example, selecting the Main420.12 profile will trigger the encoder to require subsampled 4:2:0 input. The encoder validates the provided XS configuration under the given restrictions and limitations and will produce an error in the case of conflicting settings.

The generic syntax of the XS configuration is always as follows:

- Each option is specified as a ```key=value``` pair. The key is an alpha-numeric string and is well defined (see below).
- Options are separated by semi-colons (```;```). Multiple ```-c``` arguments are internally concatenated to produce a single XS configuration line comprised of key/value pairs.
- Values can be either text or number (integral or floating point), but a value is never the empty string.
- The XS configuration line is case insensitive (everything is converted to lower case).
- Whitespaces are ignored.
- By not specifying an XS option, the encoder will resolve to either a default value (as defined by the profile) or an automated best-effort guess.

The following XS configuration options (keys) are defined (for more information, see the ISO/IEC 21122-1 and -2 texts):

- ```p```/```profile```: The profile to use (by name). See ISO/IEC 21122-2.
- ```l```/```lev```/```level```: The level to use (by name). See ISO/IEC 21122-2.
- ```s```/```sublev```/```sublevel```: The sublevel to use (by name). See ISO/IEC 21122-2.
- ```rate```/```bpp```: Specify a target bit-rate (in bpp). Cannot be combined with ```size```.
- ```size```: Specify a target codestream size (in bytes). Cannot be combined with ```rate```.
- ```budget_lines```/```budget_report_lines```: The budget report buffer size (in lines) for the encoder's rate allocation.
- ```cpih```/```mct```: Select the multiple component transform. Valid options are ```none```/```0```, ```rct```/```1```, and ```tetrix,<Cf>,<e1>,<e2>```/```3,<Cf>,<e1>,<e2>```. Note that for the Star-Tetrix, additional settings are required (comma separated).
- ```cfa```: In case of 1-component input with one of the Bayer profiles, a CFA pattern needs to be specified. Supported patterns are ```RGGB```, ```BGGR```, ```GRBG```, and ```GBGR```.
- ```nlt```: Non-linear transform. Additional settings are required, depending on the chosen NLT. Valid NLT's are: ```none```, ```quadratic,<sigma>,<alpha>```, ```extended,<theta1>,<theta2>```, and ```extended,<E>,<T1>,<T2>```.
- ```cw```: Precinct column width in multiples of 8. ```Cw=0``` represents full-width.
- ```slh```: Slice height (in lines).
- ```bw```: Nominal bit precision of the wavelet coefficients (restricted to input-bit-depth, ```18```, or ```20```).
- ```fq```: Number of fractional bits of the wavelet coefficients (restricted to ```8```, ```6```, or ```0```).
- ```nlx```: Number of horizontal wavelet decompositions.
- ```nly```: Number of vertical wavelet decompositions (must be smaller than or equal to NLX).
- ```lh```: Long precinct header enforcement flag (```0``` or ```1```).
- ```rl```: Raw-mode selection per packet flag (```0``` or ```1```).
- ```qpih```/```quant```: Quantization type (use ```0``` for dead-zone quantization, and ```1``` for uniform quantization).
- ```fs```: Sign handling strategy (use ```0``` for jointly, and ```1``` for separate packets).
- ```rm```: Run mode (use ```0``` so runs indicate zero prediction residuals, and ```1``` so runs indicate zero coefficients).
- ```sd```: Number of components for which the wavelet decomposition is suppressed.
- ```gains```: Comma separated list of gain values (one value per band, which depends on number of components, subsampling and NLX/NLY). Alternatively, the encoder has some built-in tables for PSNR and Visual optimized compression. Specify either ```psnr``` or ```visual``` to select built-in gains/priorities.
- ```priorities```: Comma separated list of priority values (same amount as gains).

Some examples:

```jxsencoder -vv -DD -c "rate=4;lh=0;rl=1;gains=visual" sintel.ppm sintel.jxs```: Encodes ```sintel.ppm``` to 4 bits per pixel. Profile, level and sublevel are automatically selected. Long precinct header flag is disabled, raw-mode per packet flag is enabled, and matching built-in gains/priorities are applied for visual optimized quality. Verbosity is set to level 2. The double ```D``` option will also print out the full XS configuration string.

```jxsencoder -c "p=Main422.10" -c "rate=2" -w 1920 -h 1080 -d 10 sintel.yuv16_422p sintel.jxs```: Encodes a frame of sintel, given in YUV 422 10-bit format to 2 bits per pixel, using the Main422.10 profile.

Note: One particular useful way to learn and understand the XS configuration syntax and possibilities is to use the decoder to dump (using the ```-D``` option) configurations of the provided reference codestreams of ISO/IEC 21122-4.