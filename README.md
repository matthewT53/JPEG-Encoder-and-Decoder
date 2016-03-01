# JPEG-Encoder-and-Decoder (In progress)
Jpeg library that is able to encode and decode jpeg images.

# Language: C
Reason: Simple to use and easy to understand.

# Jpeg Encoder:
The jpeg encoder is able to encode an RGB image to a JPEG image. The JPEG data can then be written to file or stored in memory for further processing.

# Features of the encoder:
* Convert BMP to JPEG.
* Quality setting for the JPEG image where 1 < quality < 100
* 3 channels of colour YCbCr.
* 4:4:4 (No Chroma subsampling)

# Current issues:
* Sometimes random artifacts and lines are barely visible.
* Some BMP images have a different ordering for the RGB values which makes encoding the image difficult and sometimes the image has
  colour variations making it different to the original image. 

# Future work:
* Add support for Chroma subsampling for better compression. Subsampling modes to add
  4:2:0, 4:2:2 and 4:1:1
* Optimize code for better encoding performance.
* Increase the portability of the software by using fixed-bit size data types. i.e uint8_t, uint32_t etc
* Find a faster algorithm to replace the DCT-II algorithm since the DCT-II is extremely slow as shown by profiling the program.
