
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define VC_DEBUG
#define _CRT_SECURE_NO_WARNINGS
#define VC_DEBUG


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	unsigned char* data;
	int width, height;
	int channels;			// Bin�rio/Cinzentos=1; RGB=3
	int levels;				// Bin�rio=2; Cinzentos [1,256]; RGB [1,256]
	int bytesperline;		// width * channels
} IVC;

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// �rea
	int xc, yc;					// Centro-de-massa
	int perimetro;				// Per�metro
	int label;					// Etiqueta
} OVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROT�TIPOS DE FUN��ES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC* vc_read_image(char* filename);
int vc_write_image(char* filename, IVC* image);

// FUN��ES: ESPA�OS DE CORES
int vc_gray_negative(IVC* srcdst); //calcula o negativo de uma imagem Gray

int vc_rgb_negative(IVC* srcdst); //calcula o negativo de uma imagem RGB

int vc_rgb_get_red_gray(IVC* srcdst); //converte uma imagem RGB numa imagem Gray com o canal vermelho
int vc_rgb_get_green_gray(IVC* srcdst); //converte uma imagem RGB numa imagem Gray com o canal verde
int vc_rgb_get_blue_gray(IVC* srcdst); //converte uma imagem RGB numa imagem Gray com o canal azul
int vc_rgb_to_gray(IVC* src, IVC* dst); //converte uma imagem RGB numa imagem Gray
int vc_rgb_to_hsv(IVC* src, IVC* dst); //converte uma imagem RGB numa imagem HSV
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax); //segmenta uma imagem HSV
int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);//converte uma imagem Gray numa imagem Bin�ria
int vc_gray_to_binary_global_mean(IVC* src, IVC* dst);//converte uma imagem Gray numa imagem Bin�ria com limiar global
int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernelSize);//converte uma imagem Gray numa imagem Bin�ria com limiar de ponto m�dio
int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernelSize, int cmin);//converte uma imagem Gray numa imagem Bin�ria com limiar de Bernsen
int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernelSize, float k);//converte uma imagem Gray numa imagem Bin�ria com limiar de Niblack
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);//dilata��o de uma imagem Bin�ria
int vc_binary_erode(IVC* src, IVC* dst, int kernel);//eros�o de uma imagem Bin�ria
int vc_binary_open(IVC* src, IVC* dst, int kernelsizeErode, int kernelsizeDilate);//abertura de uma imagem Bin�ria
int vc_binary_close(IVC* src, IVC* dst, int kernelsizeDilate, int kernelsizeErode);//fecho de uma imagem Bin�ria
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);//etiquetagem de blobs numa imagem Bin�ria
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);//informa��o de blobs numa imagem Bin�ria
IVC* vc_gray_histogram_show(IVC* src, IVC* dst);//histograma de uma imagem Gray
int vc_gray_histogram_equalization(IVC* src, IVC* dst); //equaliza��o de histograma de uma imagem Gray
int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th); //detec��o de bordas numa imagem Gray com filtro de Prewitt
int vc_gray_edge_sobel(IVC* src, IVC* dst, float th); //detec��o de bordas numa imagem Gray com filtro de Sobel
int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernel); //filtro passa-baixa m�dia de uma imagem Gray
int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernel); //filtro passa-baixa mediana de uma imagem Gray
int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst); //filtro passa-baixa gaussiano de uma imagem Gray
int vc_gray_highpass_filter(IVC* src, IVC* dst); //filtro passa-alta de uma imagem Gray
int vc_gray_highpass_filter_enhance(IVC* src, IVC* dst, int gain); //filtro passa-alta de uma imagem Gray com ganho

//**************************************//
// 										//
//  Trabalho Vis�o por Computador		//
//  2024/2025							//
//										//
//  Fernando Salgueiro		n� 39		//
//  Hugo Lopes				n� 30516	//
//  Claudio Fernandes		n� 30517	//
//  Nuno Cruz				n� 30518	//
//										//
//**************************************//

int escolherVideo(char* videofile);
void filtrarMoedas(cv::Mat& frame, float* soma, int* total);
int bgr_to_rgb(const cv::Mat& imagemEntrada, IVC* imagemSaida);
int tipoMoedas(int perimetro, int area, float circ, int diametro);
void contarMoeda(cv::Mat& limpa, OVC& blob, float* soma, int* total);
int verificaRepeticao(OVC* passou, OVC atual, int cont);
float calcular_circularidade(OVC* blobs);
void escreverInfoMoeda(cv::Mat& image, OVC blob, int valor, float circ);
int somarImagens(IVC* src1, IVC* src2, IVC* dst);
int desenhaBox(cv::Mat& image, OVC& blob);
void escreveMoedaDetetada(cv::Mat& frame);
void linhaReconhecimento(cv::Mat& frame);
void vc_timer(void);
void resumoFrame(cv::Mat& frame, int* total, float soma, int width, int height, int totalFrames, int fps, int nframe);
void resumoTerminal(int* total, float soma);



