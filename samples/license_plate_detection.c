/*
 * Programming introduction with the SOD Embedded Image Processing API.
 * Copyright (C) PixLab | Symisc Systems, https://sod.pixlab.io
 */
/*
 * Compile this file together with the SOD embedded source code to generate
 * the executable. For example:
 *
 *  gcc sod.c license_plate_detection.c -lm -Ofast -march=native -Wall -std=c99 -o sod_img_proc
 *
 * Under Microsoft Visual Studio (>= 2015), just drop `sod.c` and its accompanying
 * header files on your source tree and you're done. If you have any trouble
 * integrating SOD in your project, please submit a support request at:
 * https://sod.pixlab.io/support.html
 */
/*
 * This simple program is a quick introduction on how to embed and start
 * experimenting with SOD without having to do a lot of tedious
 * reading and configuration.
 *
 * Make sure you have the latest release of SOD from:
 *  https://pixlab.io/downloads
 * The SOD Embedded C/C++ documentation is available at:
 *  https://sod.pixlab.io/api.html
 */
#include <stdio.h>
#include <stdlib.h>
#include "sod.h"
/*
 * Frontal License Plate detection without deep-learning. Only image processing code.
 */
static int filter_cb(int width, int height)
{
	/* A filter callback invoked by the blob routine each time
	 * a potential blob region is identified.
	 * We use the `width` and `height` parameters supplied
	 * to discard regions of non interest (i.e. too big or too small).
	 */
	printf("Potential Blob Region\n");
	if ((width > 300 && height > 200) || width < 45 || height < 45)
	{
		/* Ignore small or big boxes (You should take in consideration
		 * U.S plate size here and adjust accordingly).
		 */
		return 0; /* Discarded region */
	}
	return 1; /* Accepted region */
}

// a struct that will hold a sod_box pointer and the number of boxes in it
struct sod_box_list {
	sod_box *boxes;
	int n;
};
typedef struct sod_box_list sod_box_list;

sod_box_list get_bounding_box(char *in_buffer, size_t in_size)
{
	sod_box_list box_list;
	box_list.boxes = 0;
	box_list.n = 0;
	/* Load the input image in the grayscale colorspace */
	sod_img imgIn = sod_img_load_from_mem(in_buffer, in_size, SOD_IMG_GRAYSCALE);
	if (imgIn.data == 0)
	{
		/* Invalid path, unsupported format, memory failure, etc. */
		puts("Cannot load input image..exiting");
		return box_list;
	}
	/* Obtain a binary image first */
	sod_img binImg = sod_threshold_image(imgIn, 0.5);
	sod_free_image(imgIn);
	/*
	 * Perform Canny edge detection next which is a mandatory step
	 */
	sod_img cannyImg = sod_canny_edge_image(binImg, 1 /* Reduce noise */);
	sod_free_image(binImg);
	/*
	 * Dilate the image say 12 times but you should experiment
	 * with different values for best results which depend
	 * on the quality of the input image/frame. */
	sod_img dilImg = sod_dilate_image(cannyImg, 12);
	sod_free_image(cannyImg);
	/* Perform connected component labeling or blob detection
	 * now on the binary, canny edged, Gaussian noise reduced and
	 * finally dilated image using our filter callback that should
	 * discard small or large rectangle areas.
	 */
	printf("Performing blob detection..\n");
	sod_image_find_blobs(dilImg, &box_list.boxes, &box_list.n, filter_cb);
	/* Draw a box on each potential plate coordinates */
	/* Cleanup */
	sod_free_image(dilImg);
	return box_list;
}

int main(int argc, char *argv[])
{
	char *in_buffer;
	FILE *pFile;
	long lSize;
	size_t result;

	/* Input image (pass a path or use the test image shipped with the samples ZIP archive) */
	const char *zInput = argc > 1 ? argv[1] : "samples/plate.jpg";

	pFile = fopen(zInput, "rb");
	if (pFile == NULL)
	{
		fputs("File error", stderr);
		exit(1);
	}

	// obtain file size:
	fseek(pFile, 0, SEEK_END);
	lSize = ftell(pFile);
	rewind(pFile);

	// allocate memory to contain the whole file:
	in_buffer = (char *)malloc(sizeof(char) * lSize);
	if (in_buffer == NULL)
	{
		fputs("Memory error", stderr);
		exit(2);
	}

	// copy the file into the buffer:
	result = fread(in_buffer, 1, lSize, pFile);
	if (result != lSize)
	{
		fputs("Reading error", stderr);
		exit(3);
	}
	/* the whole file is now loaded in the memory buffer. */
	sod_box_list box_list = get_bounding_box(in_buffer, lSize); 
	for (int i = 0; i < box_list.n; i++)
	{
		printf("Plate found at (%d,%d) with width (%dx%d) and confidence %f\n", box_list.boxes[i].x, box_list.boxes[i].y, box_list.boxes[i].w, box_list.boxes[i].h, box_list.boxes[i].score);
		// sod_image_draw_bbox_width(imgCopy, box[i], 5, 255., 0, 225.); // rose box
	}
	// terminate
	fclose(pFile);
	free(in_buffer);
	sod_image_blob_boxes_release(box_list.boxes);

	return 0;
}