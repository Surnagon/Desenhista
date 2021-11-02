#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>

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

resize(image, resized, Size(cols_n, rows_n), INTER_LINEAR);
std::cout << "Tamanho original da imagem nova: " << resized.rows << "x" << resized.cols <<
std::endl;

Mat redsized = resized.clone();

if (image.empty())
{
std::cout << "Could not open or find the image" << std::endl;
std::cin.get(); //wait for any key press
return -1;
}

Mat image_hsv;
cvtColor(resized, image_hsv, COLOR_BGR2HSV);
Mat canais[3];//declaring a matrix with three channels//
split(image_hsv, canais);
//~ Mat blurred;
//~ blur(canais[2], blurred, Size(7, 7));
Mat binarizada(76,100, CV_8UC1);
binarizada = canais[2].clone();
threshold(binarizada, binarizada, 250, 255, THRESH_BINARY_INV);

bool isSuccess = imwrite("pretoebranco.jpeg", binarizada); //write the image to a file as JPEG 
if (isSuccess == false)
{
std::cout << "Failed to save the image" << std::endl;
std::cin.get(); //wait for a key press
return -1;
}
std::cout << "output (csv) = " <<  format(binarizada, Formatter::FMT_CSV ) << std::endl;
std::cout << "PB is successfully saved to a file " << std::endl;

split(redsized, canais);
//~ blur(canais[2], blurred, Size(7, 7));
Mat vermelho(76,100, CV_8UC1);
vermelho = canais[2].clone();
threshold(canais[2], vermelho, 237, 255, THRESH_BINARY_INV);
bool isSuccessv = imwrite("vermelho.jpeg", vermelho); //write the image to a file as JPEG 
if (isSuccessv == false)
{
std::cout << "Failed to save the image" << std::endl;
std::cin.get(); //wait for a key press
return -1;
}

binarizada = binarizada/255;
vermelho = vermelho*2/255;
Mat cartesianas(76,100, CV_8UC1);
cartesianas = binarizada + vermelho;

std::cout << "Vermelho is successfully saved to a file" << std::endl;
//~ Mat output(7600, 1, CV_8UC1);
//~ std::ofstream output("cartesianas.txt");
//~ for (int k=0; k<binarizada.rows; k++)
//~ {
    //~ for (int l=0; l<binarizada.cols; l++)
    //~ {
        //~ output.at<unsigned char>(k*binarizada.cols+l) << binarizada.at<unsigned char>(k,l); // behaves like cout - cout is also a stream
        //~ output << binarizada.at<unsigned char>(k,l) << " ";
    //~ }
    //~ output << std::endl;
//~ }

bool osSuccess = imwrite("cartesianas.jpeg", cartesianas); //write the image to a file as JPEG 
if (osSuccess == false)
{
std::cout << "Failed to save the image" << std::endl;
std::cin.get(); //wait for a key press
return -1;
}

std::cout << "matrix is successfully saved to a txt file" << cartesianas.rows << "x" << cartesianas.cols <<
std::endl;

std::__cxx11::string filename = "cartesianas.csv";

std::ofstream arquivo;
arquivo.open(filename.c_str());
arquivo << format(cartesianas, Formatter::FMT_CSV ) << std::endl;
arquivo.close();

std::cout << "cartesianas (csv) = " <<  format(cartesianas, Formatter::FMT_CSV ) << std::endl;

return 0;
}
