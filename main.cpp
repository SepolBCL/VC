
/**
 * @brief Função principal do sistema de contagem automática de moedas.
 *
 * Permite ao utilizador escolher um vídeo de teste, processa cada frame para identificar, contar
 * e classificar moedas, exibe estatísticas em tempo real e apresenta um resumo final no terminal.
 *
 * A execução continua em loop, permitindo ao utilizador escolher novamente outro vídeo após a execução anterior.
 *
 * @return 0 ao finalizar o programa.
 */

#define _CRT_SECURE_NO_WARNINGS
#include "vc.hpp"


int main(void)
{
    // Definir locale para permitir acentuação correta no terminal
    setlocale(LC_ALL, "Portuguese");

    // Estrutura para armazenar dados do vídeo
    struct
    {
        int width, height;         ///< Dimensões do vídeo
        int ntotalframes;          ///< Total de frames
        int fps;                   ///< Frames por segundo
        int nframe;                ///< Número do frame atual
    } video;

    // Variáveis de controlo
    int key = 0;
    float soma = 0.0f;                           ///< Soma total do valor das moedas
    int* total = (int*)calloc(9, sizeof(int));   ///< Array para contagem por tipo (0-7) e total geral (8)
    cv::Mat frameMat;                            ///< Frame atual capturado

    // Nome do vídeo a abrir (alocado dinamicamente)
    char* videofile = (char*)malloc(256 * sizeof(char));

    // Loop principal do programa (permite reprocessar vídeos)
    do
    {
        system("cls");

        // Apresenta o menu e permite escolher o vídeo; se 0, termina o programa
        if (escolherVideo(videofile) == 0)
        {
            break;
        }

        // Inicia temporização
        vc_timer();

        // Abre o vídeo selecionado
        cv::VideoCapture capture;
        capture.open(videofile);

        if (!capture.isOpened())
        {
            fprintf(stderr, "Erro ao abrir o ficheiro de vídeo!\n");
            return 1;
        }

        // Obtém as propriedades do vídeo
        video.ntotalframes = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_COUNT));
        video.fps = static_cast<int>(capture.get(cv::CAP_PROP_FPS));
        video.width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
        video.height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));

        system("cls");

        // Processamento frame a frame
        while (true)
        {
            // Lê o próximo frame
            if (!capture.read(frameMat))
                break;

            // Atualiza o número do frame atual
            video.nframe = static_cast<int>(capture.get(cv::CAP_PROP_POS_FRAMES));

            // Processa o frame para identificar e contar moedas
            filtrarMoedas(frameMat, &soma, total);

            // Exibe o resumo atualizado no próprio frame
            resumoFrame(frameMat, total, soma, video.width, video.height, video.ntotalframes, video.fps, video.nframe);

            // Espera tecla, sai do loop se for ESC
            key = cv::waitKey(10);
            if (key == 27) break;
        }

        // Liberta o vídeo e fecha a janela
        capture.release();
        cv::destroyWindow("Trabalho de Visao por Computador");

        // Exibe o resumo final no terminal
        resumoTerminal(total, soma);

         // Finaliza temporização
        vc_timer();

        // Reinicializa as contagens e soma para novo vídeo
        memset(total, 0, 9 * sizeof(int));
        soma = 0.0f;

        // Pausa aguardando interação do utilizador antes de recomeçar o loop
        system("pause");

    } while (true);

    return 0;
}
