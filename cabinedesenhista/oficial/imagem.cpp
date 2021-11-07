#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>

using namespace cv;
int LARGURA = 100;

int main(int argc, char** argv)
{

// Captura a imagem
//~ Mat image = VideoCapture camera(0);
Mat image = imread("eu.jpeg");        //Por conta da camera estragada tive de utilizar uma imagem previamente salva

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
std::cout << "Tamanho da imagem redimencionada: " << resized.rows << "x" << resized.cols <<
std::endl;

Mat generic(resized.rows, resized.cols, CV_8UC1); // matriz generica de um unico canal
Mat redsized = resized.clone();

if (image.empty())
{
std::cout << "Could not open or find the image" << std::endl;
std::cin.get(); //wait for any key press
return -1;
}

Mat image_hsv;
cvtColor(resized, image_hsv, COLOR_BGR2HSV);
Mat canais[3];//matrix generica de tres canais para uso com split//
split(image_hsv, canais);
//~ Mat blurred;
//~ blur(canais[2], blurred, Size(7, 7));
Mat binarizada = generic.clone();
binarizada = canais[2].clone();
normalize(binarizada, binarizada, 0, 255, cv::NORM_MINMAX);
threshold(binarizada, binarizada, 127, 255, THRESH_BINARY_INV);

bool isSuccess = imwrite("pretoebranco.jpeg", binarizada); //salva imagem binarizada relativa ao brilho
if (isSuccess == false)
{
std::cout << "Failed to save the image" << std::endl;
std::cin.get(); //wait for a key press
return -1;
}

std::cout << "PB salvo em arquivo de imagem com sucesso" << std::endl;

binarizada = binarizada/255;
std::__cxx11::string filename = "pretoebranco.csv";

std::ofstream arquivo;
arquivo.open(filename.c_str());
arquivo << format(binarizada, Formatter::FMT_CSV ) << std::endl;
arquivo.close();

std::cout << "Coordenadas da mascara preto/branco salvas em arquivo CSV: " << binarizada.rows << "x" << binarizada.cols <<
std::endl;

//~ std::cout << "binarizada (csv) = " <<  format(binarizada, Formatter::FMT_CSV ) << std::endl; // para conferencia e debug


//________________________________________________________________________________________________________________________

// Matriz de cor vermelha

split(redsized, canais);
//~ blur(canais[2], blurred, Size(7, 7));
Mat vermelho = generic.clone();
Mat antivermelho = generic.clone();
antivermelho = (canais[0]+canais[1])/2;
vermelho = canais[2] - antivermelho;
normalize(vermelho, vermelho, 0, 255, cv::NORM_MINMAX);
threshold(vermelho, vermelho, 245, 255, THRESH_BINARY);
bool isSuccessv = imwrite("vermelho.jpeg", vermelho); //salva imagem binarizada relativa ao vermelho destacado
if (isSuccessv == false)
{
std::cout << "Failed to save the image" << std::endl;
std::cin.get(); //wait for a key press
return -1;
}

std::cout << "Vermelho salvo em arquivo de imagem com sucesso" << std::endl;

vermelho = vermelho*2/255;
filename = "vermelho.csv";

//~ std::ofstream arquivo;
arquivo.open(filename.c_str());
arquivo << format(vermelho, Formatter::FMT_CSV ) << std::endl;
arquivo.close();

std::cout << "Coordenadas da mascara vermelha salvas em arquivo CSV: " << vermelho.rows << "x" << vermelho.cols <<
std::endl;

//~ std::cout << "vermelho (csv) = " <<  format(vermelho, Formatter::FMT_CSV ) << std::endl;

//________________________________________________________________________________________________________________________

// Matriz geral soma das duas matrizes

Mat cartesianas = generic.clone();
cartesianas = binarizada + vermelho; // mascara de preto e vermelho a ser utilizada na impressao

bool osSuccess = imwrite("cartesianas.jpeg", cartesianas); //write the image to a file as JPEG 
if (osSuccess == false)
{
std::cout << "Failed to save the image" << std::endl;
std::cin.get(); //wait for a key press
return -1;
}

std::cout << "Coordenadas da imagem final salvas em arquivo de imagem: " << cartesianas.rows << "x" << cartesianas.cols <<
std::endl;

filename = "cartesianas.csv";

//~ std::ofstream arquivo;
arquivo.open(filename.c_str());
arquivo << format(cartesianas, Formatter::FMT_CSV ) << std::endl;
arquivo.close();

std::cout << "Coordenadas da imagem final salvas em arquivo CSV: " << cartesianas.rows << "x" << cartesianas.cols <<
std::endl;

//~ std::cout << "cartesianas (csv) = " <<  format(cartesianas, Formatter::FMT_CSV ) << std::endl;

return 0;
}
