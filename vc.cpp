//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define _CRT_SECURE_NO_WARNINGS

#include <malloc.h>
#include <iostream>
#include <string>
#include <chrono>
#include <opencv2/highgui.hpp>

#include "vc.hpp"

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

#pragma region Funções : Alocar e Libertar uma Imagem
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar memória para uma imagem
IVC* vc_image_new(int width, int height, int channels, int levels)
{
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL) return NULL;
	if ((levels <= 0) || (levels > 256)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar memória de uma imagem
IVC* vc_image_free(IVC* image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}

#pragma endregion

#pragma region Funcões : Leitura e Escrita de imagens
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char* netpbm_get_token(FILE* file, char* tok, int len)
{
	char* t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)));
		if (c != '#') break;
		do c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF) break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}


long int unsigned_char_to_bit(unsigned char* datauchar, unsigned char* databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char* p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char* databit, unsigned char* datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char* p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC* vc_read_image(char* filename)
{
	FILE* file = NULL;
	IVC* image = NULL;
	unsigned char* tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 256;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0) { channels = 1; levels = 2; }	// Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 2) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 256)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels + 1);
			if (image == NULL) return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
	}

	return image;
}


int vc_write_image(char* filename, IVC* image)
{
	FILE* file = NULL;
	unsigned char* tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL) return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 2)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

			fprintf(file, "%s \n%d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s \n%d %d \n%d\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height, image->levels - 1);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}

#pragma endregion

#pragma region Função: vc_gray_negative
/**
 * Função: vc_gray_negative
 * -------------------------
 * Aplica o negativo a uma imagem em tons de cinzento (grayscale),
 * ou seja, inverte a intensidade de cada pixel: pixel = 255 - pixel.
 *
 * Parâmetros:
 *   srddst - ponteiro para a estrutura IVC contendo a imagem a ser processada
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário
 */

int vc_gray_negative(IVC* srddst)
{
	unsigned char* data = (unsigned char*)srddst->data;
	int width = srddst->width;
	int height = srddst->height;
	int channels = srddst->channels;
	int bytesperline = srddst->bytesperline;
	int x, y;
	long int pos;

	// Verifica se os parâmetros são válidos
	if (width <= 0 || height <= 0 || data == NULL)
		return 0;

	// Verifica se a imagem é em tons de cinzento
	if (channels != 1)
		return 0;

	// Percorre todos os pixels da imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			// Calcula a posição do pixel atual
			pos = x * channels + y * bytesperline;

			// Inverte a intensidade do pixel (negativo)
			data[pos] = 255 - data[pos];
		}
	}

	return 1;
}
#pragma endregion

#pragma region Função: vc_rgb_negative

/**
 * Função: vc_rgb_negative
 * -----------------------
 * Aplica o negativo a uma imagem RGB (3 canais), invertendo os valores
 * de cada componente de cor (R, G e B) de cada pixel: componente = 255 - componente.
 *
 * Parâmetros:
 *   srcdst - ponteiro para a estrutura IVC contendo a imagem RGB a ser processada
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário
 */

int vc_rgb_negative(IVC* srcdst)
{
	int x, y;
	long int pos;
	unsigned char* data = (unsigned char*)srcdst->data;

	// Verifica se o ponteiro para a imagem é válido
	if (srcdst == NULL) return 0;

	// Percorre todos os píxeis da imagem
	for (y = 0; y < srcdst->height; y++)
	{
		for (x = 0; x < srcdst->width; x++)
		{
			// Calcula a posição do pixel no array de dados
			pos = srcdst->bytesperline * y + 3 * x;

			// Inverte os valores dos três canais (R, G, B)
			data[pos] = 255 - data[pos];     // Vermelho
			data[pos + 1] = 255 - data[pos + 1]; // Verde
			data[pos + 2] = 255 - data[pos + 2]; // Azul
		}
	}

	return 1;
}

#pragma endregion

#pragma region Função: vc_rgb_get_red_gray
/**
 * Função: vc_rgb_get_red_gray
 * ---------------------------
 * Converte uma imagem RGB para tons de cinzento utilizando apenas o canal vermelho (R).
 * O valor do vermelho de cada pixel é copiado para os canais verde (G) e azul (B),
 * criando uma imagem cinzenta baseada apenas na intensidade do vermelho.
 *
 * Parâmetros:
 *   srcdst - ponteiro para a estrutura IVC contendo a imagem RGB a ser processada
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário
 */

int vc_rgb_get_red_gray(IVC* srcdst)
{
	int x, y;
	long int pos;

	// Acede aos dados da imagem como vetor de bytes
	unsigned char* data = (unsigned char*)srcdst->data;

	// Verifica se o ponteiro para a estrutura é válido
	if (srcdst == NULL) return 0;

	// Percorre todos os pixels da imagem
	for (y = 0; y < srcdst->height; y++)
	{
		for (x = 0; x < srcdst->width; x++)
		{
			// Calcula a posição do pixel no array de dados
			pos = srcdst->bytesperline * y + 3 * x;

			// Copia o valor do canal vermelho para os canais verde e azul
			// (sem alterar o vermelho, mantendo o valor original)
			data[pos] = data[pos];           // Vermelho (mantém)
			data[pos + 1] = data[pos];       // Verde = Vermelho
			data[pos + 2] = data[pos];       // Azul  = Vermelho
		}
	}

	return 1;
}
#pragma endregion

#pragma region Função: vc_rgb_get_green_gray

/**
 * Função: vc_rgb_get_green_gray
 * -----------------------------
 * Converte uma imagem RGB para tons de cinzento utilizando apenas o canal verde (G).
 * O valor do verde de cada pixel é copiado para os canais vermelho (R) e azul (B),
 * criando uma imagem cinzenta baseada apenas na intensidade do verde.
 *
 * Parâmetros:
 *   srcdst - ponteiro para a estrutura IVC contendo a imagem RGB a ser processada
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário
 */

int vc_rgb_get_green_gray(IVC* srcdst)
{
	int x, y;
	long int pos;

	// Acede aos dados da imagem como vetor de bytes
	unsigned char* data = (unsigned char*)srcdst->data;

	// Verifica se o ponteiro para a imagem é válido
	if (srcdst == NULL) return 0;

	// Percorre todos os pixels da imagem
	for (y = 0; y < srcdst->height; y++)
	{
		for (x = 0; x < srcdst->width; x++)
		{
			// Calcula a posição do pixel no array de dados
			pos = srcdst->bytesperline * y + 3 * x;

			// Copia o valor do canal verde para os outros canais
			data[pos] = data[pos + 1]; // Vermelho = Verde
			data[pos + 1] = data[pos + 1]; // Verde (mantém)
			data[pos + 2] = data[pos + 1]; // Azul = Verde
		}
	}

	return 1;
}

#pragma endregion

#pragma region Função: vc_rgb_get_blue_gray

/**
 * Função: vc_rgb_get_blue_gray
 * ----------------------------
 * Converte uma imagem RGB para tons de cinzento utilizando apenas o canal azul (B).
 * O valor do azul de cada pixel é copiado para os canais vermelho (R) e verde (G),
 * criando uma imagem cinzenta baseada apenas na intensidade do azul.
 *
 * Parâmetros:
 *   srcdst - ponteiro para a estrutura IVC contendo a imagem RGB a ser processada
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário
 */

int vc_rgb_get_blue_gray(IVC* srcdst)
{
	int x, y;
	long int pos;

	// Acede aos dados da imagem como vetor de bytes
	unsigned char* data = (unsigned char*)srcdst->data;

	// Verifica se o ponteiro para a imagem é válido
	if (srcdst == NULL) return 0;

	// Percorre todos os pixels da imagem
	for (y = 0; y < srcdst->height; y++)
	{
		for (x = 0; x < srcdst->width; x++)
		{
			// Calcula a posição do pixel no array de dados
			pos = srcdst->bytesperline * y + 3 * x;

			// Copia o valor do canal azul para os outros canais
			data[pos] = data[pos + 2]; // Vermelho = Azul
			data[pos + 1] = data[pos + 2]; // Verde = Azul
			data[pos + 2] = data[pos + 2]; // Azul (mantém)
		}
	}

	return 1;
}

#pragma endregion

#pragma region Função: vc_rgb_to_gray

/**
 * Função: vc_rgb_to_gray
 * ----------------------
 * Converte uma imagem RGB (3 canais) para uma imagem em tons de cinzento (1 canal),
 * utilizando a fórmula de luminância ponderada:
 * Gray = 0.299 * R + 0.587 * G + 0.114 * B
 *
 * Parâmetros:
 *   src - ponteiro para a imagem de entrada (RGB)
 *   dst - ponteiro para a imagem de saída (grayscale)
 *
 * Retorna:
 *   1 se a conversão for bem-sucedida, 0 caso contrário
 */

int vc_rgb_to_gray(IVC* src, IVC* dst)
{
	int x, y;
	long int pos;

	// Ponteiros para os dados das imagens de origem e destino
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	// Verifica se os ponteiros são válidos
	if (src == NULL || dst == NULL) return 0;

	// Verifica se a imagem de origem tem 3 canais e a de destino tem 1
	if (src->channels != 3 || dst->channels != 1) return 0;

	// Percorre todos os píxeis da imagem
	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			// Calcula a posição do pixel no array da imagem RGB
			pos = y * src->bytesperline + x * src->channels;

			// Aplica a fórmula da luminância e guarda o valor na imagem grayscale
			datadst[y * dst->bytesperline + x] = (unsigned char)(0.299 * datasrc[pos] + 0.587 * datasrc[pos + 1] + 0.114 * datasrc[pos + 2]);
		}
	}

	return 1;
}

#pragma endregion

#pragma region Função: vc_rgb_to_hsv
/**
 * Função: vc_rgb_to_hsv
 * ---------------------
 * Converte uma imagem RGB (3 canais) para o espaço de cor HSV (Hue, Saturation, Value).
 * Os valores HSV resultantes são armazenados também como imagem com 3 canais (cada componente em [0,255]).
 *
 * Parâmetros:
 *   src - ponteiro para a imagem RGB de entrada
 *   dst - ponteiro para a imagem HSV de saída
 *
 * Retorna:
 *   1 se a conversão for bem-sucedida, 0 caso contrário
 */

int vc_rgb_to_hsv(IVC* src, IVC* dst)
{
	int x, y;
	long int pos;
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	float r, g, b, hue, saturation, value, min, max, delta;

	// Verificações básicas de ponteiros e canais
	if (src == NULL || dst == NULL) return 0;
	if (src->channels != 3 || dst->channels != 3) return 0;

	// Percorre todos os pixels da imagem
	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			pos = y * src->bytesperline + x * src->channels;

			// Obtém os valores RGB e normaliza para [0,1]
			r = (float)datasrc[pos];
			g = (float)datasrc[pos + 1];
			b = (float)datasrc[pos + 2];
			r /= 255.0f;
			g /= 255.0f;
			b /= 255.0f;

			// Calcula os valores máximo e mínimo entre R, G, B
			max = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
			min = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
			delta = max - min;

			// Valor (V) corresponde ao máximo dos canais RGB
			value = max;

			// Saturação (S)
			if (max > 0.0f)
				saturation = delta / max;
			else
				saturation = 0.0f;

			// Matiz (H)
			if (delta == 0)
				hue = 0.0f; // Sem matiz (cor neutra)
			else
			{
				if (r == max) hue = (g - b) / delta;
				else if (g == max) hue = 2.0f + (b - r) / delta;
				else hue = 4.0f + (r - g) / delta;

				hue *= 60.0f;
				if (hue < 0.0f) hue += 360.0f;
			}

			// Converte H, S e V para a escala de 0 a 255
			datadst[pos] = (unsigned char)(hue / 360.0f * 255.0f);  // H
			datadst[pos + 1] = (unsigned char)(saturation * 255.0f);    // S
			datadst[pos + 2] = (unsigned char)(value * 255.0f);         // V
		}
	}

	return 1;  // Sucesso
}


#pragma endregion

#pragma region Função: vc_hsv_segmentation
/**
 * Função: vc_hsv_segmentation
 * ---------------------------
 * Realiza a segmentação de uma imagem HSV com base em intervalos de H (matiz), S (saturação) e V (valor).
 * Pixels dentro do intervalo definido são marcados como "1" (ou valor máximo em `dst->levels - 1`), os restantes são definidos como 0.
 * A imagem de destino deve ser binária (1 canal).
 *
 * Parâmetros:
 *   src   - imagem HSV de entrada (3 canais)
 *   dst   - imagem binária de saída (1 canal)
 *   hmin  - valor mínimo de matiz (H), em graus (0–360)
 *   hmax  - valor máximo de matiz (H), em graus (0–360)
 *   smin  - saturação mínima (0–100)
 *   smax  - saturação máxima (0–100)
 *   vmin  - valor mínimo de brilho (0–100)
 *   vmax  - valor máximo de brilho (0–100)
 *
 * Retorna:
 *   1 se a segmentação for bem-sucedida, 0 caso contrário
 */

int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	if (!src || !dst || src->channels != 3 || dst->channels != 1)
		return 0;

	int x, y, i = 0;
	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++, i += 3)
		{
			int h = src->data[i];
			int s = src->data[i + 1];
			int v = src->data[i + 2];
			int pos = y * dst->bytesperline + x;

			if (h >= hmin && h <= hmax &&
				s >= smin && s <= smax &&
				v >= vmin && v <= vmax)
			{
				dst->data[pos] = 255;
			}
			else
			{
				dst->data[pos] = 0;
			}
		}
	}
	return 1;
}


#pragma endregion

#pragma region Função: vc_scale_gray_to_color_palette
/**
 * Função: vc_scale_gray_to_color_palette
 * --------------------------------------
 * Converte uma imagem em tons de cinzento (grayscale, 1 canal) para uma imagem RGB (3 canais),
 * aplicando uma paleta de cores tipo "Jet", que vai do azul ao vermelho, passando por ciano, verde e amarelo.
 *
 * Parâmetros:
 *   src - imagem de entrada em grayscale (1 canal)
 *   dst - imagem de saída em RGB (3 canais), com as mesmas dimensões da original
 *
 * Retorna:
 *   1 se a conversão for bem-sucedida, 0 caso contrário
 */

int vc_scale_gray_to_color_palette(IVC* src, IVC* dst) {
	// Verificações básicas
	if (src == NULL || dst == NULL) return 0;
	if (src->channels != 1 || dst->channels != 3) return 0; // Entrada deve ser grayscale e saída RGB
	if (src->width != dst->width || src->height != dst->height) return 0; // As dimensões devem ser iguais

	unsigned char* src_data = src->data;
	unsigned char* dst_data = dst->data;
	int width = src->width;
	int height = src->height;

	// Percorre todos os píxeis da imagem
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int index_gray = y * width + x;   // Índice do píxel na imagem grayscale
			int index_rgb = index_gray * 3;   // Índice correspondente na imagem RGB

			unsigned char gray = src_data[index_gray]; // Valor do píxel em grayscale

			// Mapeamento de cinzento para cor com paleta "Jet"
			unsigned char r, g, b;

			if (gray < 64) {        // Azul → Ciano
				r = 0;
				g = 4 * gray;
				b = 255;
			}
			else if (gray < 128) {  // Ciano → Verde
				r = 0;
				g = 255;
				b = 255 - (4 * (gray - 64));
			}
			else if (gray < 192) {  // Verde → Amarelo
				r = 4 * (gray - 128);
				g = 255;
				b = 0;
			}
			else {                  // Amarelo → Vermelho
				r = 255;
				g = 255 - (4 * (gray - 192));
				b = 0;
			}

			// Escreve os valores RGB no destino
			dst_data[index_rgb] = r; // Red
			dst_data[index_rgb + 1] = g; // Green
			dst_data[index_rgb + 2] = b; // Blue
		}
	}

	return 1;
}

#pragma endregion

#pragma region Função: vc_gray_to_binary

/**
 * Função: vc_gray_to_binary
 * -------------------------
 * Converte uma imagem em tons de cinzento (grayscale, 1 canal) para uma imagem binária (1 canal),
 * com base num valor de limiar (threshold).
 * Todos os píxeis com valor >= threshold são definidos como 1 (branco), os restantes como 0 (preto).
 *
 * Parâmetros:
 *   src       - imagem de entrada (grayscale)
 *   dst       - imagem de saída (binária)
 *   threshold - valor de limiar para binarização (0–255)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário
 */

int vc_gray_to_binary(IVC* src, IVC* dst, int threshold)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos;

	// Verificações de validade da imagem de entrada e saída
	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height < height) || (dst->channels != channels))
		return 0;
	if (channels != 1)
		return 0;

	// Percorre todos os píxeis da imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			// Aplica limiarização: >= threshold → 255, senão → 0
			if (datasrc[pos] >= threshold)
			{
				datadst[pos] = 255;
			}
			else
			{
				datadst[pos] = 0;
			}
		}
	}

	return 1;
}

#pragma endregion

#pragma region Função: vc_gray_to_binary_global_mean
/**
 * Função: vc_gray_to_binary_global_mean
 * -------------------------------------
 * Converte uma imagem em tons de cinzento (grayscale, 1 canal) para uma imagem binária (1 canal),
 * utilizando como limiar o valor da média global dos píxeis da imagem.
 *
 * Parâmetros:
 *   src - imagem de entrada em grayscale
 *   dst - imagem de saída binária (1 canal)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário
 */

int vc_gray_to_binary_global_mean(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos;
	long int sum = 0;
	int threshold;

	// Verificações básicas
	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height < height) || (dst->channels != channels))
		return 0;
	if (channels != 1)
		return 0;

	// Calcula a soma de todos os valores de píxeis
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			sum += datasrc[pos];
		}
	}

	// Calcula a média (limiar global)
	threshold = sum / (width * height);

	// Aplica a binarização com o limiar calculado
	vc_gray_to_binary(src, dst, threshold);

	return 1;
}

#pragma endregion

#pragma region Função: vc_gray_to_binary_midpoint

/**
 * Função: vc_gray_to_binary_midpoint
 * ----------------------------------
 * Aplica binarização adaptativa a uma imagem grayscale usando o método do ponto médio (midpoint).
 * Para cada píxel, calcula-se o valor médio entre o mínimo e o máximo numa vizinhança (janela)
 * e esse valor é usado como limiar local.
 *
 * Parâmetros:
 *   src        - imagem de entrada (grayscale, 1 canal)
 *   dst        - imagem binária de saída (1 canal, mesma dimensão da entrada)
 *   kernelSize - tamanho da janela (deve ser ímpar, ex: 3, 5, 7...)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário
 */

int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernelSize)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, kx, ky;
	int pos, max, min;
	int offset = (kernelSize - 1) / 2;
	int treshold;

	// Verificações básicas
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if (src->channels != 1)
		return 0;

	// Binarização usando ponto médio local
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			max = 0;
			min = src->levels - 1;

			// Percorre a janela local
			for (ky = -offset; ky < offset; ky++)
			{
				for (kx = -offset; kx < offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						int localPos = (y + ky) * bytesperline + (x + kx) * channels;
						unsigned char value = datasrc[localPos];

						if (value > max) max = value;
						if (value < min) min = value;
					}
				}
			}

			// Calcula o limiar como a média entre o máximo e o mínimo
			treshold = (unsigned char)((float)(max + min) / 2);

			// Aplica binarização ao píxel atual
			pos = y * bytesperline + x * channels;
			if (datasrc[pos] > treshold)
				datadst[pos] = 255;
			else
				datadst[pos] = 0;
		}
	}

	return 1;
}

#pragma endregion

#pragma region Função: vc_gray_to_binary_bernsen

/**
 * Função: vc_gray_to_binary_bernsen
 * ---------------------------------
 * Aplica binarização adaptativa a uma imagem em tons de cinzento utilizando o método de Bernsen.
 * Para cada píxel, calcula o mínimo e máximo na sua vizinhança. Se o contraste local (max - min)
 * for menor que um valor limite (`cmin`), o píxel é definido como fundo. Caso contrário, o limiar
 * é definido como a média entre max e min.
 *
 * Parâmetros:
 *   src        - imagem de entrada em grayscale (1 canal)
 *   dst        - imagem de saída binária (1 canal)
 *   kernelSize - tamanho da janela (deve ser ímpar)
 *   cmin       - valor mínimo de contraste local
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário
 */

int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernelSize, int cmin)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, kx, ky;
	int max, min;
	int offset = (kernelSize - 1) / 2;
	int pos;
	unsigned char treshold;

	// Verificações básicas
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if (src->channels != 1)
		return 0;

	// Binarização usando método de Bernsen
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			max = 0;
			min = src->levels - 1;

			// Percorre a janela local
			for (ky = -offset; ky < offset; ky++)
			{
				for (kx = -offset; kx < offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) &&
						(x + kx >= 0) && (x + kx < width))
					{
						pos = (y + ky) * bytesperline + (x + kx) * channels;

						if (datasrc[pos] > max)
							max = datasrc[pos];
						if (datasrc[pos] < min)
							min = datasrc[pos];
					}
				}
			}

			// Define o limiar de acordo com o contraste local
			if ((max - min) < cmin)
				treshold = (unsigned char)((float)(src->levels) / 2.0); // Contraste baixo: assume fundo
			else
				treshold = (unsigned char)((float)(max + min) / 2.0);   // Contraste suficiente: usa média local

			pos = y * bytesperline + x * channels;

			if (datasrc[pos] > treshold)
				datadst[pos] = 255;
			else
				datadst[pos] = 0;

		}
	}

	return 1;
}

#pragma endregion

#pragma region Função: vc_gray_to_binary_niblack

/**
 * Função: vc_gray_to_binary_niblack
 * ---------------------------------
 * Aplica binarização adaptativa a uma imagem em tons de cinzento usando o método de Niblack.
 * O limiar é calculado dinamicamente para cada píxel com base na média (mean) e desvio padrão (stdev)
 * da sua vizinhança (janela). O limiar local é definido como: threshold = mean + k * stdev.
 *
 * Parâmetros:
 *   src        - imagem de entrada em grayscale (1 canal)
 *   dst        - imagem de saída binária (1 canal)
 *   kernelSize - tamanho da janela (deve ser ímpar)
 *   k          - fator multiplicador do desvio padrão (ajusta a sensibilidade)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário
 */

int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernelSize, float k)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = (kernelSize - 1) / 2; // Offset da janela (metade do kernel)
	int pos;
	float sum, sumsq;
	float mean, stdev;
	unsigned char threshold;


	// Verificações básicas
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if (src->channels != 1)
		return 0;

	// Binarização usando método de Niblack
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			sum = 0.0;
			sumsq = 0.0;
			int counter_mean = 0;

			// Calcula a soma dos valores dos píxeis na vizinhança
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						pos = (y + ky) * bytesperline + (x + kx) * channels;
						sum += datasrc[pos];
						counter_mean++; // Número de elementos somados
					}
				}
			}

			mean = sum / counter_mean; // Média da janela
			int counter_stdev = 0;

			// Calcula o desvio padrão dos valores da janela
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						pos = (y + ky) * bytesperline + (x + kx) * channels;
						sumsq += powf(((float)datasrc[pos] - mean), 2); // Soma dos quadrados da diferença
						counter_stdev++;
					}
				}
			}

			stdev = sqrtf(sumsq / counter_stdev); // Desvio padrão da janela

			// Calcula limiar adaptativo
			threshold = (unsigned char)(mean + k * stdev);

			pos = y * bytesperline + x * channels;

			// Aplica binarização
			if (datasrc[pos] > threshold)
				datadst[pos] = 255;
			else
				datadst[pos] = 0;
		}
	}
	return 1;
}

#pragma endregion

#pragma region Função: vc_binary_dilate
/**
 * Função: vc_binary_dilate
 * -------------------------
 * Aplica a operação de **dilatação morfológica** a uma imagem binária.
 * Para cada píxel, verifica se há algum píxel ativo (≠ 0) na vizinhança (definida por um kernel quadrado).
 * Se houver, o píxel atual será definido como 1 (ativo). Caso contrário, permanece 0.
 *
 * Parâmetros:
 *   src     - imagem binária de entrada (1 canal, valores 0 ou 1)
 *   dst     - imagem binária de saída (1 canal, mesma dimensão)
 *   kernel  - tamanho do kernel (deve ser ímpar, ex: 3, 5, 7...)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */

int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = (kernel - 1) / 2; // Metade do tamanho do kernel
	int pos;
	int i, j;
	int pixel;
	int posk;

	// Verificações básicas
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if (src->channels != 1)
		return 0;

	// Dilatação
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			pixel = datasrc[pos]; // Valor atual do píxel

			// Só processa se o pixel atual for 0 (para tentar ativá-lo)
			if (pixel == 0)
			{
				for (ky = -offset; ky <= offset; ky++)
				{
					for (kx = -offset; kx <= offset; kx++)
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						// Verifica se vizinho está dentro dos limites da imagem
						if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
						{
							// Se algum vizinho estiver ativo, ativa este pixel
							pixel |= datasrc[posk];

							// Se encontrou algum vizinho ativo, sai da busca
							if (pixel != 0)
							{
								kx = offset;
								ky = offset;
							}
						}
					}
				}
			}

			// Define o novo valor do píxel na imagem de saída
			if (pixel != 0)
			{
				datadst[pos] = 1;
			}
			else
			{
				datadst[pos] = 0;
			}
		}
	}
	return 1;
}

#pragma endregion

#pragma	region Função: vc_binary_erode
/**
 * Função: vc_binary_erode
 * ------------------------
 * Aplica a operação de **erosão morfológica** a uma imagem binária.
 * Para cada píxel ativo (valor 1), verifica se **todos os vizinhos** na janela definida pelo kernel
 * também estão ativos. Se algum vizinho for 0, o píxel será erodido (definido como 0).
 *
 * Parâmetros:
 *   src     - imagem binária de entrada (1 canal, valores 0 ou 1)
 *   dst     - imagem binária de saída (1 canal, mesma dimensão)
 *   kernel  - tamanho do kernel (deve ser ímpar, ex: 3, 5, 7...)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = (kernel - 1) / 2; // Metade do tamanho do kernel
	int pos;
	int i, j;
	int pixel;
	int posk;

	// Verificações básicas
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if (src->channels != 1)
		return 0;

	// Erosão
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			pixel = datasrc[pos]; // Valor do píxel atual

			// Só processa se o pixel for 1 (ativo)
			if (pixel != 0)
			{
				for (ky = -offset; ky <= offset; ky++)
				{
					for (kx = -offset; kx <= offset; kx++)
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						// Verifica se o vizinho está dentro dos limites da imagem
						if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
						{
							// Erosão: se algum vizinho for 0, o resultado será 0
							pixel &= datasrc[posk];

							// Se encontrou vizinho 0, pode sair da busca
							if (pixel == 0)
							{
								kx = offset;
								ky = offset;
							}
						}
					}
				}
			}

			// Define o valor do píxel na imagem de saída
			if (pixel != 0)
			{
				datadst[pos] = 1;
			}
			else
			{
				datadst[pos] = 0;
			}
		}
	}
	return 1;
}

#pragma endregion

#pragma	region Função: vc_binary_open
/**
 * Função: vc_binary_open
 * -----------------------
 * Realiza a operação morfológica **abertura** numa imagem binária.
 * Abertura = erosão seguida de dilatação. Serve para remover ruído pequeno (pontos brancos isolados).
 *
 * Parâmetros:
 *   src              - imagem binária de entrada (1 canal)
 *   dst              - imagem binária de saída
 *   kernelsizeErode  - tamanho do kernel para a erosão
 *   kernelsizeDilate - tamanho do kernel para a dilatação
 *
 * Retorna:
 *   1 se ambas as operações forem bem-sucedidas, 0 caso contrário.
 */

int vc_binary_open(IVC* src, IVC* dst, int kernelsizeErode, int kernelsizeDilate)
{
	int ret = 1;

	// Cria imagem temporária para armazenar o resultado da erosão
	IVC* temp = vc_image_new(src->width, src->height, src->channels, src->levels);

	// Aplica erosão e depois dilatação
	ret &= vc_binary_erode(src, temp, kernelsizeErode);
	ret &= vc_binary_dilate(temp, dst, kernelsizeDilate);

	// Liberta a memória da imagem temporária
	vc_image_free(temp);

	return ret;
}

#pragma endregion

#pragma	region Função: vc_binary_close
/**
 * Função: vc_binary_close
 * ------------------------
 * Realiza a operação morfológica **fecho (closing)** numa imagem binária.
 * Fecho = dilatação seguida de erosão. Serve para preencher pequenos buracos ou falhas nos objetos.
 *
 * Parâmetros:
 *   src              - imagem binária de entrada (1 canal)
 *   dst              - imagem binária de saída
 *   kernelsizeDilate - tamanho do kernel para a dilatação
 *   kernelsizeErode  - tamanho do kernel para a erosão
 *
 * Retorna:
 *   1 se ambas as operações forem bem-sucedidas, 0 caso contrário.
 */

int vc_binary_close(IVC* src, IVC* dst, int kernelsizeDilate, int kernelsizeErode)
{
	int ret = 1;

	// Cria imagem temporária para armazenar o resultado da dilatação
	IVC* temp = vc_image_new(src->width, src->height, src->channels, src->levels);

	// Aplica dilatação seguida de erosão
	ret &= vc_binary_dilate(src, temp, kernelsizeDilate);
	ret &= vc_binary_erode(temp, dst, kernelsizeErode);

	// Liberta a imagem temporária
	vc_image_free(temp);

	return ret;
}

#pragma endregion

#pragma region Função: vc_binary_blob_labelling
/**
 * Função: vc_binary_blob_labelling
 * --------------------------------
 * Realiza a etiquetagem de blobs (objetos) numa imagem binária.
 * Cada blob é identificado com um número inteiro único (>0).
 * Utiliza algoritmo de duas passagens com resolução de equivalências.
 *
 * Parâmetros:
 *   src - imagem binária de entrada (1 canal, 0 ou 255)
 *   dst - imagem de saída rotulada (1 canal)
 *
 * Retorna:
 *   Número total de blobs encontrados.
 */
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixéis de plano de fundo devem obrigatóriamente ter valor 0
	// Todos os pixéis de primeiro plano devem obrigatóriamente ter valor 255
	// Serão atribuídas etiquetas no intervalo [1,254]
	// Este algoritmo está assim limitado a 255 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A está marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B está marcado, e é menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C está marcado, e é menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D está marcado, e é menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se não há blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;



	return blobs;
}


#pragma endregion

#pragma region Função: vc_binary_blob_info
/**
 * Função: vc_binary_blob_info
 * ---------------------------
 * Calcula a área, perímetro e centro de massa de cada blob numa imagem binária.
 *
 * Parâmetros:
 *   src       - imagem binária de entrada (1 canal)
 *   blobs     - vetor de blobs (saída)
 *   nblobs    - número total de blobs
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta área de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// Área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimetro++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		//blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		//blobs[i].yc = sumy / MAX(blobs[i].area, 1);
		//professor 
		blobs[i].xc = sumx / (blobs[i].area);
		//professor 
		blobs[i].yc = sumy / (blobs[i].area);
	}

	return 1;
}


#pragma	endregion

#pragma region Função: vc_gray_histogram
/**
 * Função: vc_gray_histogram_show
 * ------------------------------
 * Gera uma imagem 256x256 com o histograma normalizado (PDF) da imagem em tons de cinzento (grayscale).
 * Cada coluna da imagem resultante representa a frequência (PDF) de intensidade de cinzento [0-255].
 * A altura da barra é proporcional à frequência relativa da intensidade na imagem original.
 *
 * Parâmetros:
 *   src - imagem de entrada em grayscale (1 canal)
 *   dst - imagem de saída 256x256 (1 canal), onde será desenhado o histograma
 *
 * Retorna:
 *   Apontador para a imagem de saída com o histograma, ou NULL em caso de erro.
 */

IVC* vc_gray_histogram_show(IVC* src, IVC* dst)
{
	// Verifica se a imagem de entrada é válida e se é grayscale
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return NULL;
	if (src->channels != 1)
		return NULL;

	unsigned char* datasrc = (unsigned char*)src->data;
	int nPixels = src->width * src->height;
	int hist[256] = { 0 };        // Histograma absoluto
	float pdf[256] = { 0.0f };    // Distribuição de probabilidade (PDF)
	float histmax = 0.0f;         // Maior valor do PDF (para escalar a altura)
	int i, x, y;

	// === 1. Calcular histograma absoluto ===
	for (int pos = 0; pos < nPixels; pos++)
	{
		hist[datasrc[pos]]++; // Incrementa a contagem para o valor de intensidade
	}

	// === 2. Calcular PDF (frequência relativa) e encontrar o valor máximo do PDF ===
	for (i = 0; i < 256; i++)
	{
		pdf[i] = (float)hist[i] / (float)nPixels;
		if (pdf[i] > histmax)
		{
			histmax = pdf[i]; // Guarda o máximo para normalizar depois
		}
	}

	// === 3. Limpar imagem de saída (assumidamente 256x256 pixels) ===
	for (int pos = 0; pos < (256 * 256); pos++)
	{
		dst->data[pos] = 0; // Tudo preto inicialmente
	}

	// === 4. Desenhar barras verticais do histograma ===
	for (x = 0; x < 256; x++)
	{
		// Altura proporcional à frequência (PDF)
		int barHeight = (int)((pdf[x] / histmax) * 256);

		// Desenhar barra da base (linha 255) até ao topo da barra
		for (y = 255; y >= 256 - barHeight; y--)
		{
			dst->data[y * dst->bytesperline + x] = 1; // Branco
		}
	}

	return dst;
}

#pragma endregion

#pragma region Função: vc_gray_histogram_equalization
/**
 * Função: vc_gray_histogram_equalization
 * --------------------------------------
 * Aplica equalização de histograma a uma imagem em tons de cinzento.
 * A equalização de histograma melhora o contraste da imagem, redistribuindo os níveis de cinza.
 *
 * Parâmetros:
 *   src - imagem de entrada (grayscale, 1 canal)
 *   dst - imagem de saída (grayscale, 1 canal)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */
int vc_gray_histogram_equalization(IVC* src, IVC* dst)
{
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != 1) || (dst->channels != 1)) return 0;

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	long int size = width * height;
	int x, y, i;
	int hist[256] = { 0 };
	float pdf[256] = { 0.0f }, cdf[256] = { 0.0f };
	float cdfmin = 0.0f;

	// Calcular o histograma
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			hist[datasrc[y * bytesperline + x]]++;
		}
	}

	// Calcular pdf e cdf
	for (i = 0; i < 256; i++)
	{
		pdf[i] = (float)hist[i] / size;
		cdf[i] = (i == 0) ? pdf[i] : cdf[i - 1] + pdf[i];
	}

	// Encontrar o primeiro valor não-zero da CDF
	for (i = 0; i < 256; i++)
	{
		if (cdf[i] > 0.0f)
		{
			cdfmin = cdf[i];
			break;
		}
	}

	// Aplicar equalização
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int val = datasrc[y * bytesperline + x];
			datadst[y * bytesperline + x] = (unsigned char)(((cdf[val] - cdfmin) / (1.0f - cdfmin)) * 255.0f + 0.5f);
		}
	}

	return 1;
}


#pragma endregion

#pragma region Função: vc_gray_edge_prewitt
/**
 * Função: vc_gray_edge_prewitt
 * -----------------------------
 * Aplica o operador de Prewitt para detecção de bordas em uma imagem em tons de cinza.
 * O operador de Prewitt calcula a magnitude do gradiente da imagem, destacando as bordas.
 *
 * Parâmetros:
 *   src - imagem de entrada (grayscale, 1 canal)
 *   dst - imagem de saída (grayscale, 1 canal)
 *   th  - limiar para binarização (valores acima deste limiar serão definidos como 255)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */
int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th) {
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int byteperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	long int posA, posB, posC, posD, posE, posF, posG, posH;
	double mag, mx, my;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	for (y = 1; y < height; y++)
	{
		for (x = 1; x < width; x++)
		{
			pos = y * byteperline + x * channels;

			posA = (y - 1) * byteperline + (x - 1) * channels;
			posB = (y - 1) * byteperline + (x)*channels;
			posC = (y - 1) * byteperline + (x + 1) * channels;
			posD = (y)*byteperline + (x - 1) * channels;
			posE = (y)*byteperline + (x + 1) * channels;
			posF = (y + 1) * byteperline + (x - 1) * channels;
			posG = (y + 1) * byteperline + (x)*channels;
			posH = (y + 1) * byteperline + (x + 1) * channels;
			mx = ((-1 * data[posA]) + (1 * data[posC]) + (-1 * data[posD]) + (1 * data[posE]) + (-1 * data[posF]) + (1 * data[posH])) / 3; //?
			my = ((-1 * data[posA]) + (1 * data[posF]) + (-1 * data[posB]) + (1 * data[posG]) + (-1 * data[posC]) + (1 * data[posH])) / 3;

			mag = sqrt((mx * mx) + (my * my));

			if (mag > th)
				dst->data[pos] = 255;
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}
#pragma endregion

#pragma region Função: vc_gray_edge_sobel
/**
 * Função: vc_gray_edge_sobel
 * ---------------------------
 * Aplica o operador de Sobel para detecção de bordas em uma imagem em tons de cinza.
 * O operador de Sobel calcula a magnitude do gradiente da imagem, destacando as bordas.
 *
 * Parâmetros:
 *   src - imagem de entrada (grayscale, 1 canal)
 *   dst - imagem de saída (grayscale, 1 canal)
 *   th  - limiar para binarização (valores acima deste limiar serão definidos como 255)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */
int vc_gray_edge_sobel(IVC* src, IVC* dst, float th) {
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int byteperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	long int posA, posB, posC, posD, posE, posF, posG, posH;
	double mag, mx, my;

	if ((width <= 0) || (height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	for (y = 1; y < height; y++)
	{
		for (x = 1; x < width; x++)
		{
			pos = y * byteperline + x * channels;

			posA = (y - 1) * byteperline + (x - 1) * channels;
			posB = (y - 1) * byteperline + (x)*channels;
			posC = (y - 1) * byteperline + (x + 1) * channels;
			posD = (y)*byteperline + (x - 1) * channels;
			posE = (y)*byteperline + (x + 1) * channels;
			posF = (y + 1) * byteperline + (x - 1) * channels;
			posG = (y + 1) * byteperline + (x)*channels;
			posH = (y + 1) * byteperline + (x + 1) * channels;

			mx = ((-1 * data[posA]) + (1 * data[posC]) + (-2 * data[posD]) + (2 * data[posE]) + (-1 * data[posF]) + (1 * data[posH])) / 3;
			my = ((-1 * data[posA]) + (1 * data[posF]) + (-2 * data[posB]) + (2 * data[posG]) + (-1 * data[posC]) + (1 * data[posH])) / 3;

			mag = sqrt((mx * mx) + (my * my));

			if (mag > th)
				dst->data[pos] = 255;
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}
#pragma endregion

#pragma region Função: vc_gray_lowpass_mean_filter
/**
 * Função: vc_gray_lowpass_mean_filter
 * -----------------------------------
 * Aplica um filtro de média (mean filter) a uma imagem em tons de cinza.
 * O filtro de média suaviza a imagem, reduzindo o ruído.
 *
 * Parâmetros:
 *   src        - imagem de entrada (grayscale, 1 canal)
 *   dst        - imagem de saída (grayscale, 1 canal)
 *   kernelsize - tamanho do kernel (deve ser ímpar)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */
int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = (kernelsize - 1) / 2;
	long int pos, kpos;
	int sum;

	// Error verification
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;

	// Apply mean filter
	for (y = offset; y < height - offset; y++)
	{
		for (x = offset; x < width - offset; x++)
		{
			sum = 0;
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					kpos = (y + ky) * bytesperline + (x + kx) * channels;
					sum += datasrc[kpos];
				}
			}
			pos = y * bytesperline + x * channels;
			datadst[pos] = (unsigned char)(sum / (kernelsize * kernelsize));
		}
	}

	return 1;
}
#pragma endregion

#pragma region Função: vc_gray_lowpass_median_filter
/**
 * Função: vc_gray_lowpass_median_filter
 * -------------------------------------
 * Aplica um filtro de mediana (median filter) a uma imagem em tons de cinza.
 * O filtro de mediana suaviza a imagem, reduzindo o ruído, preservando bordas.
 *
 * Parâmetros:
 *   src        - imagem de entrada (grayscale, 1 canal)
 *   dst        - imagem de saída (grayscale, 1 canal)
 *   kernelsize - tamanho do kernel (deve ser ímpar)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */
int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernelsize)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = (kernelsize - 1) / 2;
	long int pos, kpos;
	unsigned char* ordem;
	int i, j;
	unsigned char temp;

	// Error verification
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;


	// Allocate memory for the sorting window
	ordem = (unsigned char*)malloc(kernelsize * kernelsize * sizeof(unsigned char));
	if (ordem == NULL) return 0;

	// Apply median filter
	for (y = offset; y < height - offset; y++)
	{
		for (x = offset; x < width - offset; x++)
		{
			// Collect pixels in the kernel window
			i = 0;
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					kpos = (y + ky) * bytesperline + (x + kx) * channels;
					ordem[i++] = datasrc[kpos];
				}
			}

			// Sort the window using bubble sort (simple for small kernels)
			for (i = 0; i < kernelsize * kernelsize - 1; i++)
			{
				for (j = i + 1; j < kernelsize * kernelsize; j++)
				{
					if (ordem[i] > ordem[j])
					{
						temp = ordem[i];
						ordem[i] = ordem[j];
						ordem[j] = temp;
					}
				}
			}

			// Get the median value (middle element)
			pos = y * bytesperline + x * channels;
			datadst[pos] = (unsigned char)ordem[(kernelsize * kernelsize) / 2];
		}
	}

	free(ordem);
	return 1;
}
#pragma endregion

#pragma region Função vc_gray_lowpass_gaussian_filter
/**
 * Função: vc_gray_lowpass_gaussian_filter
 * ---------------------------------------
 * Aplica um filtro gaussiano de baixa passagem a uma imagem em tons de cinza.
 * O filtro gaussiano suaviza a imagem, reduzindo o ruído.
 *
 * Parâmetros:
 *   src - imagem de entrada (grayscale, 1 canal)
 *   dst - imagem de saída (grayscale, 1 canal)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */
int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = (5 - 1) / 2;  // Kernel radius (for 5x5 kernel)
	long int pos, kpos;
	float sum;

	// Gaussian kernel values (1D) - corrected from the image
	float gaussarray[5] = { 0.054f, 0.242f, 0.399f, 0.242f, 0.054f };

	// Error checking
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;

	for (y = offset; y < height - offset; y++)
	{
		for (x = offset; x < width - offset; x++)
		{
			sum = 0.0f;


			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					kpos = (y + ky) * bytesperline + (x + kx) * channels;
					sum += ((float)datasrc[kpos]) * gaussarray[kx + offset] * gaussarray[ky + offset];
				}
			}

			// Store result (no need for normalization as kernel is pre-normalized)
			pos = y * bytesperline + x * channels;
			datadst[pos] = (unsigned char)sum;
		}
	}

	return 1;
}
#pragma endregion

#pragma region Função vc_gray_highpass_filter
/**
 * Função: vc_gray_highpass_filter
 * -------------------------------
 * Aplica um filtro passa-alto a uma imagem em tons de cinza.
 * O filtro passa-alto realça os contornos e detalhes da imagem.
 *
 * Parâmetros:
 *   src - imagem de entrada (grayscale, 1 canal)
 *   dst - imagem de saída (grayscale, 1 canal)
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */
int vc_gray_highpass_filter(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int posX, posA, posB, posC, posD, posE, posF, posG, posH;
	int sum;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels;

			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posF = (y + 1) * bytesperline + (x - 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;
			posH = (y + 1) * bytesperline + (x + 1) * channels;

			// Exemplo 3: kernel passa-alto 3x3 (realçado de contornos)
			sum = datasrc[posA] * -1;
			sum += datasrc[posB] * -1;
			sum += datasrc[posC] * -1;
			sum += datasrc[posD] * -1;
			sum += datasrc[posE] * -1;
			sum += datasrc[posF] * -1;
			sum += datasrc[posG] * -1;
			sum += datasrc[posH] * -1;
			sum += datasrc[posX] * 8;

			datadst[posX] = (unsigned char)((float)abs(sum) / (float)9 * 20); // normalização e escala
		}
	}

	return 1;
}
#pragma endregion

#pragma region Função vc_gray_highpass_filter_enhance
/**
 * Função: vc_gray_highpass_filter_enhance
 * ---------------------------------------
 * Aplica um filtro passa-alto realçado a uma imagem em tons de cinza.
 * O filtro realça os contornos e detalhes da imagem, permitindo ajustar o ganho.
 *
 * Parâmetros:
 *   src  - imagem de entrada (grayscale, 1 canal)
 *   dst  - imagem de saída (grayscale, 1 canal)
 *   gain - fator de ganho para realçar os contornos
 *
 * Retorna:
 *   1 se a operação for bem-sucedida, 0 caso contrário.
 */
int vc_gray_highpass_filter_enhance(IVC* src, IVC* dst, int gain)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int posX, posA, posB, posC, posD, posE, posF, posG, posH;
	int sum;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels;

			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posF = (y + 1) * bytesperline + (x - 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;
			posH = (y + 1) * bytesperline + (x + 1) * channels;

			// Exemplo 3: kernel passa-alto 3x3 (realçado de contornos)
			sum = datasrc[posA] * -1;
			sum += datasrc[posB] * -1;
			sum += datasrc[posC] * -1;
			sum += datasrc[posD] * -1;
			sum += datasrc[posE] * -1;
			sum += datasrc[posF] * -1;
			sum += datasrc[posG] * -1;
			sum += datasrc[posH] * -1;
			sum += datasrc[posX] * 8;

			datadst[posX] = (unsigned char)MIN(MAX((float)datasrc[posX] + ((float)sum / 16.0f * (float)gain), 0), 255);

		}
	}

	return 1;
}
#pragma endregion

//**************************************//
// 										//
//  Trabalho Visão por Computador		//
//  2024/2025							//
//										//
//  Fernando Salgueiro		nº 39		//
//  Hugo Lopes				nº 30516	//
//  Claudio Fernandes		nº 30517	//
//  Nuno Cruz				nº 30518	//
//										//
//**************************************//

OVC passou[500];
int cont = 0;

#pragma region Função: escolherVideo
/**
 * @brief Permite ao utilizador escolher um vídeo a partir de uma lista de opções apresentadas no terminal.
 *
 * A função apresenta um menu com 3 opções:
 * - Opção 1: Seleciona "video1.mp4" (12 moedas);
 * - Opção 2: Seleciona "video2.mp4" (29 moedas);
 * - Opção 0: Sai do programa.
 *
 * Se o utilizador escolher uma opção inválida, o menu é repetido até uma opção válida ser selecionada.
 * O nome do ficheiro escolhido é copiado para a variável `videofile`.
 *
 * @param videofile Ponteiro para a string onde será armazenado o nome do vídeo escolhido.
 *
 * @return Retorna 1 se a escolha for válida; retorna 0 se o utilizador optar por sair.
 */
int escolherVideo(char* videofile)
{
	// Exibe cabeçalho institucional e informações sobre o trabalho
	printf("\t_________________________________________________________________________________________________________________________________________\n\n");
	printf("\t\t\t\t\t\t\tIPCA\n\n");
	printf("\t\t\t\t\t\t\tEscola Superior de Tecnologia\n\n");
	printf("\t\t\t\t\t\t\tEngenharia de Sistemas Informáticos PL\n");
	printf("\t\t\t\t\t\t\t2024/2025\n\n");
	printf("\t\t\t\t\t\t\tVisão por Computador\n");
	printf("\t\t\t\t\t\t\tTrabalho Prático:\n");
	printf("\t\t\t\t\t\t\tPermite a quantificação automática de dinheiro em vídeos.\n\n");
	printf("\t__________________________________________________________________________________________________________________________________________\n");

	int escolha = -1;

	// Ciclo até o utilizador escolher uma opção válida (1, 2 ou 0)
	while ((escolha != 0) && (escolha != 1) && (escolha != 2))
	{
		// Exibição do menu de opções
		printf("\n\n\t\t\t\t\t\t\tEscolha um vídeo : \n\n");
		printf("\t\t\t\t\t\t\t\tOpção 1 -> Vídeo 1 (12 Moedas)\n\n");
		printf("\t\t\t\t\t\t\t\tOpção 2 -> Vídeo 2 (29 Moedas)\n\n");
		printf("\t\t\t\t\t\t\t\tOpção 0 -> Sair\n\n");
		printf("\t\t\t\t\t\t\tInsira a sua opção: ");

		// Recebe a opção do utilizador
		scanf("%d", &escolha);

		// Atribuição do nome do vídeo com base na escolha
		if (escolha == 1)
		{
			strcpy(videofile, "video1.mp4");
		}
		else if (escolha == 2)
		{
			strcpy(videofile, "video2.mp4");
		}
		else if (escolha == 0)
		{
			// Se escolher sair, limpa o ecrã e retorna 0
			printf("\n A fechar o programa...\n\n");
			system("cls");
			return 0;
		}
		else
		{
			// Opção inválida: limpar ecrã e mostrar mensagem de erro
			system("cls");
			printf("\nOpção errada!\n\n");
			printf("Tente novamente!\n\n");
		}
	}

	// Retorna 1 indicando uma escolha válida
	return 1;
}

#pragma endregion

#pragma region Função: filtrarMoedas
/**
 * @brief Filtra as moedas na imagem fornecida, utilizando segmentação em HSV e morfologia matemática.
 *
 * Esta função realiza a conversão da imagem BGR para RGB, seguida da conversão para HSV.
 * Depois realiza duas segmentações HSV em diferentes faixas de cor (para capturar diferentes tipos de moedas).
 * As duas segmentações são somadas, aplica-se uma operação morfológica de abertura para limpar ruído.
 * Após isso, realiza a etiquetagem dos blobs encontrados e analisa cada blob, contando e desenhando apenas moedas que passem na linha de reconhecimento.
 * Aplica ainda verificações adicionais de área, perímetro, circularidade e evita duplicação de contagem com base em uma lista de objetos já detetados.
 *
 * @param frame Imagem de entrada (BGR), será também utilizada para desenhar as anotações.
 * @param soma Ponteiro para a variável que armazena a soma do valor das moedas detetadas.
 * @param total Ponteiro para o array que armazena a contagem de moedas por tipo.
 */
void filtrarMoedas(cv::Mat& frame, float* soma, int* total)
{
	int nlabels = 0; // Número de blobs encontrados após etiquetagem

	// Alocação de imagens IVC intermediárias
	IVC* imagem = vc_image_new(frame.cols, frame.rows, frame.channels(), 255);
	IVC* hsv = vc_image_new(frame.cols, frame.rows, 3, 255);
	IVC* binaria = vc_image_new(frame.cols, frame.rows, 1, 255);
	IVC* binaria2 = vc_image_new(frame.cols, frame.rows, 1, 255);
	IVC* binaria3 = vc_image_new(frame.cols, frame.rows, 1, 255);

	// Conversão de BGR (OpenCV) para RGB (IVC)
	bgr_to_rgb(frame, imagem);

	// Conversão de RGB para HSV
	vc_rgb_to_hsv(imagem, hsv);

	// Segmentação para moedas amarelas
	vc_hsv_segmentation(hsv, binaria, 12, 150, 35, 255, 20, 150);

	// Segmentação para moedas castanhas
	vc_hsv_segmentation(hsv, binaria2, 12, 150, 0, 80, 20, 130);

	// Combinação das duas máscaras numa imagem binária final
	somarImagens(binaria, binaria2, binaria3);

	// Conversão da IVC para Mat para aplicar morfologia com OpenCV
	cv::Mat bin_mat3(binaria3->height, binaria3->width, CV_8UC1, binaria3->data);

	// Aplicação da abertura morfológica (remove ruídos e pequenos objetos)
	cv::Mat limpa;
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(9, 9));
	cv::morphologyEx(bin_mat3, limpa, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 3);

	// Copiar o resultado da operação morfológica de volta para a IVC
	memcpy(binaria3->data, limpa.data, binaria3->width * binaria3->height);

	// Etiquetagem dos blobs encontrados na imagem binária
	OVC* blobs = vc_binary_blob_labelling(binaria3, binaria3, &nlabels);

	// Desenhar a linha de reconhecimento (auxiliar visual)
	linhaReconhecimento(frame);

	// Calcula área, perímetro, centroide, etc. de cada blob
	vc_binary_blob_info(binaria3, blobs, nlabels);

	// Ciclo para processar cada blob encontrado
	for (int i = 0; i < nlabels; i++)
	{
		int cx = blobs[i].xc;
		int cy = blobs[i].yc;

		// Verificar se o centro está dentro dos limites da imagem
		if (cx < 0 || cx >= frame.cols || cy < 0 || cy >= frame.rows) continue;

		// Desenhar caixa ao redor do blob
		desenhaBox(frame, blobs[i]);

		// Verificar se a moeda cruza a linha de reconhecimento (com tolerância)
		if (frame.rows / 4 >= blobs[i].yc - 12 && frame.rows / 4 <= blobs[i].yc + 9)
		{
			// Filtrar por área e perímetro mínimos esperados
			if (blobs[i].area < 10000 || blobs[i].perimetro < 300) continue;

			// Escrever aviso de moeda detetada no frame
			escreveMoedaDetetada(frame);

			// Verifica se a moeda já foi contada (evitar duplicação)
			if (verificaRepeticao(passou, blobs[i], cont) == 1)
			{
				// Conta e acumula a moeda
				contarMoeda(frame, blobs[i], soma, total);
				// Regista o blob no histórico para evitar contar novamente
				passou[cont++] = blobs[i];
			}
		}
	}

	// Libertação da memória
	free(blobs);
	vc_image_free(imagem);
	vc_image_free(hsv);
	vc_image_free(binaria);
	vc_image_free(binaria2);
	vc_image_free(binaria3);
}


#pragma endregion

#pragma region Função: bgr_to_rgb
/**
 * @brief Converte uma imagem no formato BGR (padrão do OpenCV) para o formato RGB,
 * armazenando o resultado numa estrutura de imagem IVC.
 *
 * Esta função converte uma imagem OpenCV (`cv::Mat`) que está no espaço de cor BGR
 * para RGB, e depois copia os dados linha a linha para a estrutura `IVC`.
 * Esta conversão é necessária porque a biblioteca OpenCV utiliza BGR por padrão,
 * enquanto o processamento na framework IVC utiliza RGB.
 *
 * @param imagemEntrada Imagem de entrada do OpenCV em BGR (tipo `cv::Mat`).
 * @param imagemSaida Apontador para a estrutura `IVC` previamente alocada onde será armazenada a imagem convertida em RGB.
 *
 * @return Retorna 1 se a conversão foi realizada com sucesso, ou 0 em caso de erro (parâmetros inválidos, dimensões incompatíveis, etc.).
 */
int bgr_to_rgb(const cv::Mat& imagemEntrada, IVC* imagemSaida)
{
	// Verificar se a imagem de entrada está vazia ou se a imagem de saída é nula
	if (imagemEntrada.empty() || imagemSaida == nullptr)
		return 0;

	// Verificar se ambas as imagens possuem 3 canais (BGR e RGB)
	if (imagemEntrada.channels() != 3 || imagemSaida->channels != 3)
		return 0;

	// Criação de uma nova imagem RGB temporária usando OpenCV
	cv::Mat imagemRGB;
	cv::cvtColor(imagemEntrada, imagemRGB, cv::COLOR_BGR2RGB);

	// Validar se as dimensões da imagem convertida coincidem com a estrutura de saída
	if (imagemRGB.cols != imagemSaida->width || imagemRGB.rows != imagemSaida->height)
		return 0;

	// Obter o número de bytes por linha na estrutura IVC
	int bytesPerLine = imagemSaida->bytesperline;

	// Percorrer todas as linhas da imagem
	for (int y = 0; y < imagemRGB.rows; y++)
	{
		// Ponteiro para a linha atual na imagem OpenCV (RGB)
		const cv::Vec3b* linha = imagemRGB.ptr<cv::Vec3b>(y);

		// Ponteiro para a linha correspondente na imagem de saída IVC
		unsigned char* linhaSaida = imagemSaida->data + y * bytesPerLine;

		// Percorrer todos os píxeis da linha
		for (int x = 0; x < imagemRGB.cols; x++)
		{
			// Copiar cada componente RGB para a estrutura IVC
			linhaSaida[x * 3] = linha[x][0]; // R
			linhaSaida[x * 3 + 1] = linha[x][1]; // G
			linhaSaida[x * 3 + 2] = linha[x][2]; // B
		}
	}

	// Retorna 1 indicando sucesso
	return 1;
}

#pragma endregion

#pragma region Função: tipoMoedas

/**
 * @brief Determina o tipo de moeda (valor em centavos) com base em várias características geométricas.
 *
 * Esta função utiliza as características calculadas do blob, nomeadamente o perímetro, área, circularidade e diâmetro.
 * Utiliza limites empíricos definidos para cada moeda de euro, verificando se o blob analisado cumpre os requisitos de cada tipo.
 * Caso cumpra, retorna o valor da moeda em centavos (ex: 200 para 2€, 50 para 0.50€, etc.).
 *
 * Caso o blob não corresponda a nenhum dos tipos definidos, retorna -1.
 *
 * @param perimetro Valor do perímetro do blob.
 * @param area Valor da área do blob.
 * @param circ Valor da circularidade do blob.
 * @param diametro Estimativa do diâmetro da moeda calculado a partir da bounding box.
 *
 * @return Valor da moeda em centavos, ou -1 se não corresponder a nenhuma moeda válida.
 */
int tipoMoedas(int perimetro, int area, float circ, int diametro)
{
	// Filtro rápido inicial: rejeita qualquer blob com área, perímetro ou circularidade fora dos mínimos aceitáveis
	if (area < 10000 || perimetro < 300 || circ <= 0.40)
		return -1;

	// Moeda de 0.50€
	if (diametro >= 177 && diametro <= 181 && area >= 24200 && area <= 25950 && perimetro >= 550 && perimetro <= 560)
	{
		return 50;
	}
	// Moeda de 0.20€
	else if (diametro >= 158 && diametro <= 165 && area >= 19375 && area <= 21615 && perimetro >= 467 && perimetro <= 590)
	{
		return 20;
	}
	// Moeda de 2€
	else if (diametro >= 184 && diametro <= 190 && area >= 27140 && area <= 28140 && perimetro >= 575 && perimetro <= 605)
	{
		return 200;
	}
	// Moeda de 0.10€
	else if (diametro >= 140 && diametro <= 145 && area >= 15460 && area <= 17210 && perimetro >= 430 && perimetro <= 460)
	{
		return 10;
	}
	// Moeda de 0.05€
	else if (diametro >= 150 && diametro <= 157 && area >= 19060 && area <= 19844 && perimetro >= 450 && perimetro <= 493)
	{
		return 5;
	}
	// Moeda de 1€
	else if (diametro >= 169 && diametro <= 172 && area >= 22145 && area <= 23000 && perimetro >= 545 && perimetro <= 745)
	{
		return 100;
	}
	// Moeda de 0.02€
	else if (diametro >= 135 && diametro <= 140 && area >= 14920 && area <= 15470 && perimetro >= 400 && perimetro <= 430)
	{
		return 2;
	}
	// Moeda de 0.01€
	else if (diametro >= 116 && diametro <= 122 && area >= 10740 && area <= 11930 && perimetro >= 340 && perimetro <= 420)
	{
		return 1;
	}

	// Se nenhum critério corresponder, retorna -1 (não reconhecida)
	return -1;
}


#pragma endregion

#pragma region Função: contarMoeda
/**
 * @brief Conta o valor da moeda detetada, atualiza a soma total e incrementa a contagem por tipo.
 *
 * Esta função calcula o diâmetro estimado da moeda, calcula a circularidade e chama a função `tipoMoedas`
 * para obter o valor monetário correspondente.
 * Se a moeda for reconhecida, atualiza o array de contagem de moedas (`total`), acumula o valor na variável `soma`
 * e anota visualmente a moeda no frame (desenha as informações na imagem e imprime no terminal).
 *
 * @param frame Imagem do vídeo onde a moeda foi detetada (a função desenha as anotações nesta imagem).
 * @param blob Estrutura OVC contendo as informações geométricas do blob detetado.
 * @param soma Ponteiro para a variável que acumula a soma total em euros.
 * @param total Ponteiro para o array que armazena a contagem de moedas por tipo (índices 0 a 7 por tipo, índice 8 para total geral).
 */
void contarMoeda(cv::Mat& frame, OVC& blob, float* soma, int* total)
{
	// Estimar o diâmetro médio da moeda com base na bounding box
	int diametro = (blob.width + blob.height) / 2;

	// Calcular a circularidade do blob (4*PI*Area / Perímetro²)
	float circ = calcular_circularidade(&blob);

	// Classificar o tipo de moeda com base nas características calculadas
	int valor = tipoMoedas(blob.perimetro, blob.area, circ, diametro);

	// Atualizar as contagens e soma com base no valor retornado
	switch (valor)
	{
	case 200: total[7]++; *soma += 2.0f; break;      // 2€
	case 100: total[6]++; *soma += 1.0f; break;      // 1€
	case 50:  total[5]++; *soma += 0.5f; break;      // 0.50€
	case 20:  total[4]++; *soma += 0.2f; break;      // 0.20€
	case 10:  total[3]++; *soma += 0.1f; break;      // 0.10€
	case 5:   total[2]++; *soma += 0.05f; break;     // 0.05€
	case 2:   total[1]++; *soma += 0.02f; break;     // 0.02€
	case 1:   total[0]++; *soma += 0.01f; break;     // 0.01€
	default:  valor = -1; break;                     // Não reconhecida
	}

	// Se a moeda foi reconhecida (valor diferente de -1)
	if (valor != -1)
	{
		// Atualiza o total geral de moedas detetadas (índice 8)
		total[8]++;

		// Converte valor para float para exibir em euros
		float valor2 = valor;

		// Imprime no terminal as características da moeda detetada
		printf("\n\t\t\t\t\tMoeda de %.2f Eur -> A: %d, P: %d, C: %.2f, Ø: %d, (x,y): (%d,%d)\n",
			(valor2 / 100), blob.area, blob.perimetro, circ, diametro, blob.xc, blob.yc);

		// Escreve informações da moeda diretamente no frame (overlay gráfico)
		escreverInfoMoeda(frame, blob, valor, circ);
	}
}

#pragma endregion

#pragma region Função: verificaRepeticao
/**
 * @brief Verifica se a moeda atual já foi contada, com base na posição do centro de massa em X.
 *
 * Esta função verifica apenas a última moeda registada no array `passou`, comparando a posição X do centro de massa.
 * Considera-se que a moeda é nova (não repetida) se a posição X estiver fora de uma tolerância de 8 píxeis em relação à última moeda registada.
 * Este método simples previne contagens duplicadas de moedas próximas no eixo X.
 *
 * @note Esta função usa apenas a posição X para evitar contagem dupla, não analisando o eixo Y nem outras características.
 *
 * @param passou Array de moedas já contadas (estrutura OVC).
 * @param atual Estrutura OVC da moeda atual a verificar.
 * @param cont Número atual de moedas registadas em `passou`.
 *
 * @return Retorna 1 se a moeda é considerada nova (não repetida), ou 0 se já foi contada.
 */
int verificaRepeticao(OVC* passou, OVC atual, int cont)
{
	// Se não há moedas registadas, considera como nova
	if (cont == 0)
		return 1;

	// Verifica se a posição X da moeda atual está fora da zona de tolerância (±8 píxeis)
	if (atual.xc < passou[cont - 1].xc - 8 || atual.xc > passou[cont - 1].xc + 8)
	{
		// Fora da zona de tolerância → considera nova moeda
		return 1;
	}
	else
	{
		// Dentro da zona de tolerância → considera repetida
		return 0;
	}
}

#pragma endregion

#pragma region Função: calcular_circularidade
/**
 * @brief Calcula a circularidade de um blob com base na fórmula padrão.
 *
 * A circularidade é uma medida adimensional que indica o quão próximo de um círculo perfeito é um objeto.
 * A fórmula utilizada é: \f$ Circularidade = \frac{4 \times \pi \times Área}{Perímetro^2} \f$
 * - Um círculo perfeito tem circularidade = 1.
 * - Objetos mais alongados ou com contornos irregulares terão circularidade inferior a 1.
 *
 * @note Caso o blob seja nulo ou o perímetro seja 0 (evitando divisão por zero), a função retorna 0.
 *
 * @param blob Ponteiro para a estrutura `OVC` contendo as informações do blob (área e perímetro).
 *
 * @return A circularidade calculada como float. Retorna 0.0f em caso de erro.
 */
float calcular_circularidade(OVC* blob)
{
	// Verificação de segurança: blob nulo ou perímetro zero retorna circularidade 0 (evita divisão por zero)
	if (blob == NULL || blob->perimetro == 0)
		return 0.0f;

	// Definição de PI com precisão de float
	const float PI = 3.14159265358979323846f;

	// Cálculo da circularidade: (4 * PI * área) / (perímetro²)
	return (4.0f * PI * (float)blob->area) / ((float)blob->perimetro * (float)blob->perimetro);
}

#pragma endregion

#pragma region Função: escreverInfoMoeda
/**
 * @brief Escreve informações detalhadas sobre a moeda diretamente na imagem.
 *
 * Esta função desenha sobre a imagem (`cv::Mat`) as informações:
 * - Valor da moeda (em euros ou cêntimos);
 * - Área do blob (número de píxeis);
 * - Circularidade calculada.
 *
 * As informações são escritas à direita da bounding box da moeda, usando a fonte simples do OpenCV.
 *
 * @param image Imagem do frame onde será desenhada a informação.
 * @param blob Estrutura `OVC` com as informações geométricas da moeda.
 * @param valor Valor da moeda detetada (em centavos).
 * @param circ Circularidade calculada da moeda.
 */
void escreverInfoMoeda(cv::Mat& image, OVC blob, int valor, float circ)
{
	char texto[100];                                // Buffer para as mensagens de texto
	cv::Scalar corTexto(255, 0, 0);                 // Cor do texto (Azul em BGR)
	int fontFace = cv::FONT_HERSHEY_SIMPLEX;        // Tipo de fonte OpenCV
	double fontScale = 0.4;                         // Escala da fonte
	int thickness = 1;                              // Espessura da fonte

	// Posição inicial do texto (à direita da bounding box da moeda)
	int xText = blob.x + blob.width + 5;
	int yText = blob.y;

	// Primeira linha: Valor da moeda
	if (valor >= 100)
		sprintf(texto, "%d euro", valor / 100);     // Exibe em euros (ex.: 2 euro)
	else if (valor > 0)
		sprintf(texto, "%d cent", valor);           // Exibe em cêntimos (ex.: 50 cent)
	else
		sprintf(texto, "valor: 0");                 // Valor não reconhecido

	cv::putText(image, texto, cv::Point(xText, yText), fontFace, fontScale, corTexto, thickness);

	// Segunda linha: Área do blob (em píxeis)
	sprintf(texto, "Area: %d", blob.area);
	cv::putText(image, texto, cv::Point(xText, yText + 15), fontFace, fontScale, corTexto, thickness);

	// Terceira linha: Circularidade calculada
	sprintf(texto, "Circ: %.2f", circ);
	cv::putText(image, texto, cv::Point(xText, yText + 30), fontFace, fontScale, corTexto, thickness);
}


#pragma endregion

#pragma region Função: somarImagens
/**
 * @brief Realiza a operação lógica OR entre duas imagens binárias e armazena o resultado numa terceira imagem.
 *
 * Esta função percorre duas imagens binárias (`src1` e `src2`) de mesmo tamanho e combina-as
 * aplicando uma operação de soma lógica (OR pixel a pixel). Se qualquer um dos píxeis for 255 (branco),
 * o resultado será 255; caso contrário, será 0.
 *
 * @note Todas as imagens devem ter 1 canal (binárias) e as mesmas dimensões.
 *
 * @param src1 Ponteiro para a primeira imagem binária de entrada.
 * @param src2 Ponteiro para a segunda imagem binária de entrada.
 * @param dst Ponteiro para a imagem de saída onde será armazenado o resultado da soma.
 *
 * @return Retorna 1 se a operação for bem-sucedida; caso contrário, retorna 0 (erro de parâmetros ou incompatibilidade).
 */
int somarImagens(IVC* src1, IVC* src2, IVC* dst)
{
	// Verificação de ponteiros válidos
	if (!src1 || !src2 || !dst)
		return 0;

	// Verificação se as dimensões das imagens de entrada coincidem
	if (src1->width != src2->width || src1->height != src2->height)
		return 0;

	// Verificação se todas as imagens têm apenas 1 canal (binário)
	if (src1->channels != 1 || src2->channels != 1 || dst->channels != 1)
		return 0;

	// Percorrer todas as linhas da imagem
	for (int y = 0; y < src1->height; y++)
	{
		// Percorrer todos os píxeis da linha
		for (int x = 0; x < src1->width; x++)
		{
			// Calcular a posição linear no buffer da imagem
			int pos = y * src1->bytesperline + x;

			// Soma lógica: se qualquer um dos píxeis for 255, o resultado será 255; caso contrário, 0
			dst->data[pos] = (src1->data[pos] == 255 || src2->data[pos] == 255) ? 255 : 0;
		}
	}

	// Retorna 1 indicando sucesso
	return 1;
}
#pragma endregion

#pragma region Função: desenhaBox
/**
 * @brief Desenha uma caixa delimitadora (bounding box) em torno da moeda e uma cruz no seu centro de massa.
 *
 * Esta função desenha, sobre a imagem colorida (`cv::Mat`), a bounding box (em verde) ao redor do blob (moeda)
 * e uma cruz no centro de massa (em vermelho).
 * Antes de desenhar, verifica se a moeda cumpre os critérios mínimos de área, perímetro, circularidade e diâmetro.
 *
 * @note A função altera diretamente a imagem fornecida como parâmetro.
 *
 * @param image Imagem onde será desenhada a caixa e a cruz.
 * @param blob Estrutura `OVC` contendo as informações geométricas da moeda (bounding box, centro, etc.).
 *
 * @return Retorna 1 se a caixa e cruz foram desenhadas com sucesso; 0 caso contrário (blob não cumpre critérios mínimos).
 */
int desenhaBox(cv::Mat& image, OVC& blob)
{
	// Calcular a circularidade do blob
	float circ = calcular_circularidade(&blob);

	// Estimar o diâmetro médio a partir da bounding box
	int diametro = (blob.width + blob.height) / 2;

	// Verificação rápida para rejeitar blobs que não cumprem os requisitos mínimos de moeda
	if (blob.area <= 10000 || blob.perimetro < 300 || circ <= 0.50 || diametro < 115)
		return 0;

	// Obter propriedades da imagem
	int width = image.cols;
	int height = image.rows;
	int channels = image.channels();
	int step = image.step; // Número de bytes por linha
	unsigned char* data = image.data;

	// === Desenhar bounding box verde ===
	for (int y = blob.y; y <= blob.y + blob.height; y++)
	{
		for (int x = blob.x; x <= blob.x + blob.width; x++)
		{
			// Verificar se dentro dos limites da imagem
			if (x < 0 || x >= width || y < 0 || y >= height)
				continue;

			int pos = y * step + x * channels;

			// Desenhar apenas as bordas da caixa
			if (y == blob.y || y == blob.y + blob.height || x == blob.x || x == blob.x + blob.width)
			{
				data[pos + 0] = 0;     // B (Azul)
				data[pos + 1] = 255;   // G (Verde)
				data[pos + 2] = 0;     // R (Vermelho)
			}
		}
	}

	// === Desenhar cruz vermelha no centro de massa ===
	// Linha vertical da cruz
	for (int y = blob.yc - 10; y <= blob.yc + 10; y++)
	{
		int x = blob.xc;
		if (x < 0 || x >= width || y < 0 || y >= height)
			continue;

		int pos = y * step + x * channels;
		data[pos + 0] = 0;     // Azul
		data[pos + 1] = 0;     // Verde
		data[pos + 2] = 255;   // Vermelho
	}

	// Linha horizontal da cruz
	for (int x = blob.xc - 10; x <= blob.xc + 10; x++)
	{
		int y = blob.yc;
		if (x < 0 || x >= width || y < 0 || y >= height)
			continue;

		int pos = y * step + x * channels;
		data[pos + 0] = 0;     // Azul
		data[pos + 1] = 8;     // Verde claro (quase nulo)
		data[pos + 2] = 255;   // Vermelho
	}

	// Retorna 1 indicando que foi desenhado com sucesso
	return 1;
}

#pragma endregion

#pragma region Função: escreveMoedaDetetada
/**
 * @brief Escreve uma mensagem visual na imagem indicando que uma moeda foi detetada.
 *
 * Esta função desenha no frame três elementos:
 * - Duas linhas de texto informativas (em vermelho) com a mensagem "MOEDA DETETADA !!".
 * - Uma linha horizontal de separação em vermelho (linha de reconhecimento visual).
 *
 * A mensagem é desenhada numa área fixa horizontal, em torno de 1/4 da altura do frame.
 *
 * @param frame Imagem (`cv::Mat`) onde será desenhada a mensagem.
 */
void escreveMoedaDetetada(cv::Mat& frame)
{
	// Primeira mensagem de aviso
	std::string texto = "    !!MOEDA DETETADA !!                                                                                                              !! MOEDA DETETADA !!     ";
	cv::Point posicao(20, frame.rows / 4 - 10);  // Posição da primeira linha de texto (um pouco acima da linha de reconhecimento)
	int fontFace = cv::FONT_HERSHEY_SIMPLEX;     // Fonte padrão do OpenCV
	double fontScale = 0.5;                      // Escala da fonte
	cv::Scalar cor(0, 0, 255);                   // Cor vermelha (BGR)

	// Segunda mensagem de aviso (duplicada, abaixo da linha)
	std::string texto2 = "    !!MOEDA DETETADA !!                                                                                                              !! MOEDA DETETADA !!     ";
	cv::Point posicao2(20, frame.rows / 4 + 15);
	int fontFace2 = cv::FONT_HERSHEY_SIMPLEX;
	double fontScale2 = 0.5;
	cv::Scalar cor2(0, 0, 255);

	// Desenhar ambas as mensagens no frame
	cv::putText(frame, texto, posicao, fontFace, fontScale, cor);
	cv::putText(frame, texto2, posicao2, fontFace2, fontScale2, cor2);

	// Linha horizontal (linha de reconhecimento visual) centrada a 1/4 da altura
	std::string texto3 = "_____________________________________________________________________________________________________________________________________________________________";
	cv::Point posicao3(0, frame.rows / 4);
	int fontFace3 = cv::FONT_HERSHEY_SIMPLEX;
	double fontScale3 = 0.5;
	cv::Scalar cor3(0, 0, 255);

	// Desenhar a linha horizontal no frame
	cv::putText(frame, texto3, posicao3, fontFace3, fontScale3, cor3);
}

#pragma endregion

#pragma region Função: linhaReconhecimento
/**
 * @brief Desenha uma linha horizontal de reconhecimento na imagem.
 *
 * Esta linha serve como referência visual no vídeo, permitindo ao utilizador identificar
 * a zona onde as moedas devem cruzar para serem consideradas válidas (por exemplo, para evitar contagens duplicadas).
 * A linha é desenhada utilizando caracteres "_" ao longo de toda a largura do frame, centrada aproximadamente a 1/4 da altura da imagem.
 *
 * @param frame Imagem (`cv::Mat`) onde a linha será desenhada.
 */
void linhaReconhecimento(cv::Mat& frame)
{
	// Texto composto por sublinhados para simular uma linha horizontal
	std::string texto = "_____________________________________________________________________________________________________________________________________________________________";

	// Posição da linha: altura = 1/4 da altura da imagem, início no canto esquerdo (0, altura/4)
	cv::Point posicao(0, frame.rows / 4);

	// Configuração da fonte
	int fontFace = cv::FONT_HERSHEY_SIMPLEX;
	double fontScale = 0.5;
	cv::Scalar cor(0, 0, 0); // Cor preta (BGR)

	// Desenhar o texto (linha) na imagem
	cv::putText(frame, texto, posicao, fontFace, fontScale, cor);
}

#pragma endregion

#pragma region Função: vc_timer
/**
 * @brief Função de temporização simples que mede e exibe o tempo decorrido entre duas chamadas consecutivas.
 *
 * Esta função utiliza `std::chrono` para calcular o tempo decorrido em segundos desde a primeira chamada.
 * Na primeira execução, apenas ativa o modo de contagem (inicializa o `previousTime`).
 * Na segunda chamada (ou seguintes), calcula o tempo decorrido e exibe no terminal.
 *
 * @note Após mostrar o tempo decorrido, aguarda uma tecla do utilizador (`std::cin.get()`) antes de prosseguir.
 *
 * @warning Esta função utiliza variáveis estáticas locais, o que significa que o temporizador mantém o estado entre chamadas.
 */
void vc_timer(void)
{
	// Variável estática que controla se a função já foi chamada anteriormente
	static bool running = false;

	// Variável estática que armazena o momento da primeira chamada
	static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

	// Primeira chamada: apenas ativa o modo de contagem e não faz mais nada
	if (!running)
	{
		running = true;
	}
	// Segunda chamada (ou seguintes): calcula e exibe o tempo decorrido
	else
	{
		// Captura o tempo atual
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();

		// Calcula a duração entre o tempo atual e o tempo anterior
		std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

		// Converte a duração para segundos em formato double
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
		double nseconds = time_span.count();

		// Exibe o tempo decorrido no terminal
		std::cout << "Tempo decorrido: " << nseconds << " segundos" << std::endl;

		// Aguarda o utilizador pressionar Enter
		std::cin.get();
	}
}

#pragma endregion

#pragma region Função: resumoFrame
/**
 * @brief Desenha no frame um resumo visual do processamento de moedas.
 *
 * Exibe no vídeo as estatísticas do processamento atual:
 * - Resolução do vídeo;
 * - Total de frames e taxa de frames por segundo (FPS);
 * - Número do frame atual;
 * - Total de moedas detetadas (geral e por tipo);
 * - Soma total das moedas;
 * - Identificação da instituição, curso, disciplina, professor e alunos;
 * - Mensagem de instrução ("Prima ESC para sair").
 *
 * @param frame Imagem (`cv::Mat`) onde será desenhado o resumo.
 * @param total Ponteiro para o array com as contagens por tipo de moeda (total[8] contém o total geral).
 * @param soma Soma total calculada das moedas em euros.
 * @param width Largura do frame.
 * @param height Altura do frame.
 * @param totalFrames Número total de frames do vídeo.
 * @param fps Taxa de frames por segundo.
 * @param nframe Número do frame atual.
 */
void resumoFrame(cv::Mat& frame, int* total, float soma, int width, int height, int totalFrames, int fps, int nframe)
{
	char buffer[200];
	int yStart = 230;   // Posição vertical inicial para o resumo das moedas
	int yStep = 20;     // Espaçamento vertical entre linhas

	// Cores para textos
	cv::Scalar corFonte(0, 0, 0);       // Preto
	cv::Scalar corFonte2(100, 0, 0);    // Vermelho escuro

	// Configurações da fonte
	int fontFace = cv::FONT_HERSHEY_SIMPLEX;
	double fontScale = 0.5;
	double fontScale2 = 0.7;

	// Exibir informações do vídeo
	std::string str;
	str = "RESOLUCAO: " + std::to_string(width) + "x" + std::to_string(height);
	cv::putText(frame, str, cv::Point(yStep, 25), fontFace, fontScale2, corFonte);

	str = "TOTAL DE FRAMES: " + std::to_string(totalFrames);
	cv::putText(frame, str, cv::Point(yStep, 50), fontFace, fontScale2, corFonte);

	str = "FRAME RATE: " + std::to_string(fps);
	cv::putText(frame, str, cv::Point(yStep, 75), fontFace, fontScale2, corFonte);

	str = "N. FRAME: " + std::to_string(nframe);
	cv::putText(frame, str, cv::Point(yStep, 100), fontFace, fontScale2, corFonte);

	// Exibir total geral de moedas
	sprintf(buffer, "Total de moedas:");
	cv::putText(frame, buffer, cv::Point(20, yStart), fontFace, fontScale, cv::Scalar(255, 0, 0));
	sprintf(buffer, "%d", total[8]);
	cv::putText(frame, buffer, cv::Point(165, yStart), fontFace, fontScale, cv::Scalar(0, 0, 255));

	// Exibir contagem por tipo de moeda (em ordem de valor decrescente)
	const char* nomes[] = {
		"Moedas 2 Eur", "Moedas 1 Eur", "Moedas 0,50 Eur", "Moedas 0,20 Eur",
		"Moedas 0,10 Eur", "Moedas 0,05 Eur", "Moedas 0,02 Eur", "Moedas 0,01 Eur"
	};

	for (int i = 0; i < 8; i++)
	{
		int idx = 7 - i; // Ordem inversa (do maior valor para o menor)
		sprintf(buffer, "%s: %d", nomes[i], total[idx]);
		cv::putText(frame, buffer, cv::Point(20, yStart + (i + 1) * yStep), fontFace, fontScale, corFonte2);
	}

	// Exibir a soma total em euros
	sprintf(buffer, "Soma:");
	cv::putText(frame, buffer, cv::Point(20, yStart + 9 * yStep), fontFace, fontScale, cv::Scalar(255, 0, 0));
	sprintf(buffer, "%.2f Euros", soma);
	cv::putText(frame, buffer, cv::Point(70, yStart + 9 * yStep), fontFace, fontScale, cv::Scalar(0, 0, 255));

	// Identificação institucional
	sprintf(buffer, "IPCA");
	cv::putText(frame, buffer, cv::Point(yStep, yStart + 13 * yStep), fontFace, 0.6, cv::Scalar(0, 120, 0));
	sprintf(buffer, "Escola Superior de Tecnologia");
	cv::putText(frame, buffer, cv::Point(yStep, yStart + 14 * yStep), fontFace, 0.4, cv::Scalar(150, 0, 0));
	sprintf(buffer, "Engenharia de Sistemas Informaticos PL");
	cv::putText(frame, buffer, cv::Point(yStep, yStart + 15 * yStep), fontFace, 0.4, corFonte2);
	sprintf(buffer, "2024/2025");
	cv::putText(frame, buffer, cv::Point(yStep, yStart + 16 * yStep), fontFace, 0.4, corFonte2);
	sprintf(buffer, "Visao por Computador");
	cv::putText(frame, buffer, cv::Point(yStep, yStart + 17 * yStep), fontFace, 0.4, corFonte2);
	sprintf(buffer, "Professor: Simao Valente");
	cv::putText(frame, buffer, cv::Point(yStep, yStart + 18 * yStep), fontFace, 0.4, corFonte2);

	// Lista dos alunos
	cv::putText(frame, "Alunos:", cv::Point(yStep, yStart + 19 * yStep), fontFace, 0.4, corFonte2);
	cv::putText(frame, "    Fernando Salgueiro n. 39", cv::Point(yStep, yStart + 20 * yStep), fontFace, 0.4, corFonte2);
	cv::putText(frame, "    Hugo Lopes n. 30516", cv::Point(yStep, yStart + 21 * yStep), fontFace, 0.4, corFonte2);
	cv::putText(frame, "    Claudio Fernandes n. 30517", cv::Point(yStep, yStart + 22 * yStep), fontFace, 0.4, corFonte2);
	cv::putText(frame, "    Nuno Cruz n. 30518", cv::Point(yStep, yStart + 23 * yStep), fontFace, 0.4, corFonte2);

	// Mensagem de instrução
	sprintf(buffer, "Prima ESC para sair");
	cv::putText(frame, buffer, cv::Point(1100, yStart + 23 * yStep), fontFace, 0.4, corFonte);

	// Atualizar a janela com o frame anotado
	cv::imshow("Trabalho de Visao por Computador", frame);
}

#pragma endregion

#pragma region Função: resumoTerminal
/**
 * @brief Exibe no terminal um resumo final da contagem de moedas e do valor total.
 *
 * Esta função apresenta no terminal:
 * - Total geral de moedas detetadas;
 * - Contagem por tipo de moeda (de 1 cêntimo até 2 euros);
 * - Valor total em euros;
 * - Informação adicional sobre como consultar características de cada moeda;
 * - Identificação da instituição, disciplina, professor e alunos.
 *
 * @param total Ponteiro para o array com as contagens por tipo de moeda (total[8] contém o total geral).
 * @param soma Valor total acumulado em euros.
 */
void resumoTerminal(int* total, float soma)
{
	// Cabeçalho do resumo
	printf("\n\t\t\t\t\t\t\t*********** Resumo final ***************\n");
	printf("\t\t\t\t\t\t\t---------------------------------------\n\n");

	// Total geral de moedas
	printf("\t\t\t\t\t\t\t\tTotal de moedas: %d \n\n", total[8]);

	// Cabeçalho da tabela
	printf("\t\t\t\t\t\t\t\tvalor\t\t\tQuant.\n");

	// Contagem por tipo de moeda (em ordem crescente de valor)
	printf("\t\t\t\t\t\t\t\tt1\tcêntimo:\t%d \n", total[0]);
	printf("\t\t\t\t\t\t\t\t2\tcêntimos:\t%d \n", total[1]);
	printf("\t\t\t\t\t\t\t\t5\tcêntimos:\t%d \n", total[2]);
	printf("\t\t\t\t\t\t\t\t10\tcêntimos:\t%d \n", total[3]);
	printf("\t\t\t\t\t\t\t\t20\tcêntimos:\t%d \n", total[4]);
	printf("\t\t\t\t\t\t\t\t50\tcêntimos:\t%d \n", total[5]);
	printf("\t\t\t\t\t\t\t\t1\teuro:\t\t%d \n", total[6]);
	printf("\t\t\t\t\t\t\t\t2\teuros:\t\t%d \n", total[7]);

	printf("\t\t\t\t\t\t\t---------------------------------------\n");

	// Valor total calculado em euros
	printf("\t\t\t\t\t\t\t\tValor total: %.2f euros \n", soma);

	printf("\n\t\t\t\t\t\t\t***************************************\n\n");

	// Nota adicional explicativa
	printf("\t\tNota: pode consultar acima as informações sobre cada moeda (Área(A), Perímetro(P), Circularidade(C), Diâmetro(Ø), Centro(x,y))\n");
	printf("\t_________________________________________________________________________________________________________________________________________\n\n");

	// Informações institucionais e de identificação
	printf("\t\t\t\t\t\t\tIPCA\n");
	printf("\t\t\t\t\t\t\tEscola Superior de Tecnologia\n");
	printf("\t\t\t\t\t\t\tEngenharia de Sistemas Informáticos PL\n");
	printf("\t\t\t\t\t\t\t2024/2025\n\n");
	printf("\t\t\t\t\t\t\tVisão por Computador\n");
	printf("\t\t\t\t\t\t\tProfessor: Simão Valente\n\n");
	printf("\t\t\t\t\t\t\tAlunos:\n");
	printf("\t\t\t\t\t\t\t\tFernando Salgueiro n. 39\n");
	printf("\t\t\t\t\t\t\t\tHugo Lopes n. 30516\n");
	printf("\t\t\t\t\t\t\t\tCláudio Fernandes n. 30517\n");
	printf("\t\t\t\t\t\t\t\tNuno Cruz n. 30518\n\n");

	// Mensagem de encerramento
	printf("\t\t\t\t\t\t\t\tObrigado                      ");
	printf("\n\t_________________________________________________________________________________________________________________________________________\n\n");
}

#pragma endregion
