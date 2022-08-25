#include "Matrix.h"
#include </home/astrobits/opencilk/cheetah/include/cilk/cilk.h>

int convolvePixel(int x,int y, Matrix& image, Matrix& kernel) {
    int kernelLength = kernel.getRow();
    int V = (kernelLength - 1)/2;

    int counter = 0;
    int convolutionVal = 0;
    // Center the Kernel above the Image pixel location matching the NewImage pixel location
    for (int i = 0; i < kernelLength; i++) {
        
        for (int j = 0; j < kernelLength; j++) {
            // Current Image pixel being applied to ConvolutionVal
            int imagX = x-V+i;
            int imagY = y-V+j;

            // Exclude indices outside Image (edge detection)
            if ( ((imagX >= 0) && (imagX < image.getRow())) &&
                 ((imagY >= 0) && (imagY < image.getCol())) )
            {
                counter += kernel.get(i,j); // Track Kernel values used (flux conservation)
                convolutionVal += kernel.get(i,j) * image.get(imagX, imagY);
            }
        }
    }
    return convolutionVal / counter;
}

void sequentialConvolve(int startX, int endX, int startY, int endY, Matrix& convolvedImage, Matrix& originalImage, Matrix& kernel) {
    for (int i = startX; i < endX; i++) {
        for (int j = startY; j < endY; j++) {
            convolvedImage.set(i,j, convolvePixel(i,j,originalImage,kernel));
        }
    }
}

void parallelConvolve(int startX, int startY, int xSize, int ySize, Matrix& convolvedImage, Matrix& originalImage, Matrix& kernel, int stopX, int stopY) {
    if ((xSize <= stopX) && (ySize <= stopY)) {
        sequentialConvolve(startX, startX+xSize, startY, startY+ySize, convolvedImage, originalImage, kernel);
    }
    else {
        cilk_spawn 
            parallelConvolve(startX, startY, xSize/2, ySize/2, convolvedImage, originalImage, kernel, stopX, stopY);
        cilk_spawn
            parallelConvolve(startX, startY+ySize/2, xSize/2, ySize/2, convolvedImage, originalImage, kernel, stopX, stopY); 
        cilk_spawn
            parallelConvolve(startX+xSize/2, startY, xSize/2, ySize/2, convolvedImage, originalImage, kernel, stopX, stopY); 
        cilk_spawn
            parallelConvolve(startX+xSize/2, startY+ySize/2, xSize/2, ySize/2, convolvedImage, originalImage, kernel, stopX, stopY);
        cilk_sync;
    }
}