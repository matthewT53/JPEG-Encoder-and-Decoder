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

# Current issues:
* Sometimes random artifacts and lines are barely visible.
* Some BMP images have a different ordering for the RGB values which makes encoding the image difficult and sometimes the image has
  colour variations making it different to the original image. 
* 4:2:0 Subsampling is not currently working

# Future work:
* Start work on the decoder
* Optimize code for better encoding performance.
