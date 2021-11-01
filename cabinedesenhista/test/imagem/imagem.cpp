#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <iostream>
using namespace cv;
int LARGURA = 100;

int main(int argc, char** argv)
{
// Read the image file
//~ Mat image = VideoCapture camera(0);
Mat image = imread("download.jpeg");
std::cout << "Tamanho original da imagem: " << image.rows << "x" << image.cols <<
std::endl;

Mat resized;
float proportion = (float)image.rows/(float)image.cols;
int cols_n = LARGURA;
int rows_n = proportion*cols_n;
if (rows_n > 1.35*LARGURA) {
	rows_n = 1.35*LARGURA;
}
std::cout << "Tamanho original da imagem nova: " << proportion << std::endl;

resize(image, resized, Size(cols_n, rows_n), INTER_LINEAR);

if (image.empty())
{
std::cout << "Could not open or find the image" << std::endl;
std::cin.get(); //wait for any key press
return -1;
}

//~ Mat vermelho;
//~ vermelho = resized[:,2,:]
//~ Mat vermelho_bin;
//~ threshold(vermelho, vermelho_bin, 250, 255, 0);
//~ bool isSuccessv = imwrite("vermelho.jpg", vermelho_bin); //write the image to a file as JPEG 
//~ if (isSuccessv == false)
//~ {
//~ std::cout << "Failed to save the image" << std::endl;
//~ std::cin.get(); //wait for a key press
//~ return -1;
//~ }

Mat image_hsv;
cvtColor(resized, image_hsv, COLOR_BGR2HSV);
Mat canais[3];//declaring a matrix with three channels//
split(image_hsv, canais);
Mat blurred;
blur(canais[2], blurred, Size(7, 7));
Mat binarizada;
threshold(blurred, binarizada, 250, 255, 0);

bool isSuccess = imwrite("pretoebranco.jpg", resized); //write the image to a file as JPEG 
if (isSuccess == false)
{
std::cout << "Failed to save the image" << std::endl;
std::cin.get(); //wait for a key press
return -1;
}

std::cout << "Image is successfully saved to a file" << std::endl;

//~ std::ofstream output("cartesianas.txt"); 
//~ for (k=1; k<resized.cols; k++)
//~ {
    //~ for (l=1; l<resized.rows; l++)
    //~ {
        //~ output << resized[k][l] << " "; // behaves like cout - cout is also a stream
    //~ }
    //~ output << std::endl;
//~ }

return 0;
}
