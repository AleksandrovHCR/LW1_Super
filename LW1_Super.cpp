#include <iostream>
#include <mpi.h>
#include <vector>
#include <random>

using namespace std;

int main(int argc, char* argv[])
{
    std::vector<std::vector<float>> Matrix_A(3, std::vector<float>(3));//3 на 3
    std::vector<std::vector<float>> Matrix_B(4, std::vector<float>(4));//4 на 4

    //Здесь начинается многопоточность
    vector<float> recieve_matrix_a;
    vector<float> recieve_matrix_b;//Для передачи

    vector<float> row(4);//строка для передачи
    vector<float> result;
    int nTasks, rank;
    MPI_Init(&argc, &argv);//Начало

    MPI_Comm_size(MPI_COMM_WORLD, &nTasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Проверка количества процессов, так как логика жестко завязана на 4 процесса
    if (nTasks != 4) {
        if (rank == 0) {
            cout << "Error: This program must be run with exactly 4 processes!" << endl;
        }
        MPI_Finalize();
        return 0;
    }

    if (rank == 0) {//Материнская задача
        //Инициализация массивов
        std::random_device RD;
        std::mt19937 gen(RD());
        std::uniform_real_distribution<float> dist(0.1, 133.7);

        for (int i = 3; i <= 4; i++) {
            if (i == 3) cout << "\nMatrix 1:\n";
            else  cout << "\nMatrix 2:\n";
            for (int j = 0; j < i; j++) {
                for (int z = 0; z < i; z++) {
                    if (i == 3) {
                        Matrix_A[j][z] = dist(gen);
                        std::cout << Matrix_A[j][z] << " ";
                    }
                    else {
                        Matrix_B[j][z] = dist(gen);
                        std::cout << Matrix_B[j][z] << " ";
                    }
                }
                cout << "\n";
            }
        }
        //Приведение матрицы А к соотв виду
        Matrix_A.resize(4, std::vector<float>(4));
        for (int i = 0; i < 4; i++)
            Matrix_A[i].resize(4);
        for (int i = 0; i < 4; i++) {
            Matrix_A[i][3] = 0.f;
            Matrix_A[3][i] = 0.f;
        }
        Matrix_A[3][3] = 1.f;

        cout << "\nMatrix 1 (Padded):\n";
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                std::cout << Matrix_A[i][j] << " ";
            }
            cout << endl;
        }

        // ИСПРАВЛЕНО: было i < 4 в условии внутреннего цикла, что вызывало бесконечный цикл
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                recieve_matrix_a.push_back(Matrix_A[i][j]); //перенос в одномерный массив
            }
        }
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                recieve_matrix_b.push_back(Matrix_B[i][j]); //перенос в одномерный массив для удобства
            }
        }

        ///Разброс по процессам
        for (int process = 1; process < 4; process++) {
            float* matrix_b = recieve_matrix_b.data();
            float* matrix_a = recieve_matrix_a.data();
            MPI_Send(matrix_b, 16, MPI_FLOAT, process, 99, MPI_COMM_WORLD);
            MPI_Send(&matrix_a[process * 4], 4, MPI_FLOAT, process, 98, MPI_COMM_WORLD);
        }

        for (int i = 0; i < 4; i++) row[i] = recieve_matrix_a[i];

        recieve_matrix_b = recieve_matrix_b;
        recieve_matrix_a.assign(recieve_matrix_a.begin(), recieve_matrix_a.begin() + 4);
    }
    else {
        MPI_Status status;

        // ИСПРАВЛЕНО: Выделяем память в векторах перед приемом данных
        recieve_matrix_b.resize(16);
        MPI_Recv(recieve_matrix_b.data(), 16, MPI_FLOAT, 0, 99, MPI_COMM_WORLD, &status);

        recieve_matrix_a.resize(4);
        MPI_Recv(recieve_matrix_a.data(), 4, MPI_FLOAT, 0, 98, MPI_COMM_WORLD, &status);
    }

    //рассчёты 
    for (int col = 0; col < 4; col++) {
        float sum = 0.0f;
        for (int k = 0; k < 4; k++) {
            sum += recieve_matrix_a[k] * recieve_matrix_b[k * 4 + col];
        }
        result.push_back(sum);
    }

    if (rank == 0) {
        float matrixC[16];

        for (int j = 0; j < 4; j++) matrixC[j] = result[j];

        MPI_Status status;
        for (int source_process = 1; source_process < 4; source_process++) {
            MPI_Recv(&matrixC[source_process * 4], 4, MPI_FLOAT, source_process, 97, MPI_COMM_WORLD, &status);
        }

        cout << "\nResult Matrix C (4x4):\n";
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                cout << matrixC[i * 4 + j] << "\t";
            }
            cout << endl;
        }
    }
    else {
        MPI_Send(result.data(), 4, MPI_FLOAT, 0, 97, MPI_COMM_WORLD);
    }

    MPI_Finalize();//Конец
    return 0;
}
