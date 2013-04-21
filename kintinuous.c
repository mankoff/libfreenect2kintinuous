
// gcc -lz kintinuous.c -o kintinuous

/*

Date: Nov 2012
From: Thomas Whelan <*****>
To: Ken Mankoff <*****>

int32_t at file beginning for frame count

Then for each frame
         int64_t: timestamp
         int32_t: depthSize
         int32_t: imageSize
         depthSize * unsigned char: depth_compress_buf
         imageSize * unsigned char: encodedImage->data.ptr

That is, firstly a 32-bit signed integer storing the number of frames
in the log, then for each frame a 64-bit signed integer storing the
timestamp (this can be in any units, I recommend microseconds), then a
32-bit signed integer storing the size (in bytes) of the compressed
depth image, a 32-bit signed integer storing the size (in bytes) of
the compressed rgb image.

For depth compression I use the zlib library, in particular the
compress2 function. I normally use the Z_BEST_SPEED setting, however
you can use Z_BEST_COMPRESSION if you want to reduce the log size even
further.

For image compression I use the OpenCV cvEncodeImage function to
encode as a jpg. However, we can just skip this.

Some pseudo code for you would be as follows. All you have to do is
slot in the correct pointer for nextDepthFramePtr and value in
numOfFramesToProcess.

*/

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
#include <dirent.h>

int main(void) {

  // Open log file to write
  FILE * logFile = fopen("kintinuous.klg", "wb+");

  int64_t currentTimeStamp = 0;
  
  // Write the numFrames value to the start of the file, we'll update it later
  int32_t numFrames = 0;
  fwrite(&numFrames, sizeof(int32_t), 1, logFile);
  
  // Buffer for zlib
  int depth_compress_buf_size = 640 * 480 * sizeof(int16_t) * 4;
  uint8_t * depth_compress_buf = (uint8_t*)malloc(depth_compress_buf_size);
  
  DIR *dir;
  struct dirent *ent;
  dir = opendir (".");

  // print all the files and directories within directory
  while ((ent = readdir (dir)) != NULL) {
	if (strcmp(ent->d_name, ".") == 0) 
	  continue;
	if (strcmp(ent->d_name, "..") == 0) 
	  continue;
	if (strcmp(ent->d_name, "kintinuous") == 0) 
	  continue;
	if (strcmp(ent->d_name, "kintinuous.klg") == 0) 
	  continue;
	printf ("%s\n", ent->d_name);

	FILE* fp = fopen(ent->d_name, "r");
	int16_t nextDepthFramePtr[640*480];
	fread(nextDepthFramePtr, sizeof(int16_t), 640*480, fp);
	fclose(fp);

	// This will store the compressed depth image size
	unsigned long compressed_size = depth_compress_buf_size;
	
	// Here nextDepthFramePtr should be of type (unsigned) short, i.e. integer values in mm
	compress2(depth_compress_buf, &compressed_size, (const Bytef*)nextDepthFramePtr, 640 * 480 * sizeof(short), Z_BEST_COMPRESSION);
	
	// We won't store RGB
	int zero = 0;

	// currentTimeStamp = strtoul(&ent->d_name[20], NULL, 10);
	currentTimeStamp = numFrames;
	// printf( "%u\n", currentTimeStamp);
	// currentTimeStamp should be this frame's timestamp, if there's none make one up (numFrames?) 
	fwrite(&currentTimeStamp, sizeof(int64_t), 1, logFile);
	
	// Depth compressed size
	fwrite(&compressed_size, sizeof(int32_t), 1, logFile);
	
	// Zero because there's no RGB image
	fwrite(&zero, sizeof(int32_t), 1, logFile);
	
	// Depth compressed data
	fwrite(depth_compress_buf, compressed_size, 1, logFile);
	
	// Increment frame counter
	numFrames++;
  }
  closedir (dir);
  
  // Free depth buffer
  free(depth_compress_buf);
  
  // Seek back to the start of the log file
  fseek(logFile, 0, SEEK_SET);
  
  // Write the frame count
  fwrite(&numFrames, sizeof(int32_t), 1, logFile);
  
  // Flush and close
  fflush(logFile);
  fclose(logFile);

  return 0;
}
