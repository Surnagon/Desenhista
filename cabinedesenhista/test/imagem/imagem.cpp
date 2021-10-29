//#include <cv.h>
#include "opencv2/opencv.hpp"
using namespace cv;
int LARGURA = 100;

int main(int argc, char** argv)
{
// Read the image file
Mat image = imread("/home/surnagon/Pictures/WhatsApp Image 2020-10-20 at 23.49.07.jpeg");
cout << "Tamanho original da imagem: " << image.rows << "x" image.cols <<
endl;

if (image.empty())
{
cout << "Could not open or find the image" << endl;
cin.get(); //wait for any key press
return -1;
}

Mat image_hsv;
cvtColor(image, image_hsv, CV_BGR2HSV);
Mat canais[3];//declaring a matrix with three channels//
split(image_hsv, canais);
Mat blurred;
blur(canais[2], blurred, Size(7, 7)))
Mat binarizada;
threshold(blurred, binarizada, 50, 255)
Mat resized;
float proportion = image.cols/image.rows;
int row = LARGURA;
int col = proportion*row;
if (col > 1.35*LARGURA) {
	col = 1.35*LARGURA;
}
resize(binarizada, resized, Size(col, row));

bool isSuccess = imwrite("pretoebranco.jpg", resized); //write the image to a file as JPEG 
if (isSuccess == false)
{
cout << "Failed to save the image" << endl;
cin.get(); //wait for a key press
return -1;
}

cout << "Image is successfully saved to a file" << endl;
return 0;
}
