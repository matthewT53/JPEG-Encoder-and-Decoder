# JPEG-Encoder-and-Decoder (In progress)
Jpeg library that is able to encode and decode jpeg images in memory and on disk.

# Language: C
Reason: Simple to use and easy to understand.

# Jpeg Encoder:
The jpeg encoder is able to encode an RGB image to a JPEG image. The JPEG data can then be written to file or stored in memory for mroe processing.

# Features of the encoder:
* Quality setting for the JPEG image where 1 < quality < 100

# Current issues:
* Sometimes random artifacts and lines barely appear in the image.

# Future work:
* Add support for Chroma subsampling for better compression. Subsampling modes to add
  4:2:0, 4:2:2 and 4:1:1
* Optimize code for better encoding performance.
