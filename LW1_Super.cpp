#include <iostream>
#include <mpi.h>
#include <vector>
#include <random>
#include <chrono>
using namespace std;


const int REPEATS = 170000;


int test()
{

    std::vector<std::vector<float>> Matrix_A(3, std::vector<float>(3));
    std::vector<std::vector<float>> Matrix_B(4, std::vector<float>(4));

    std::random_device RD;
    std::mt19937 gen(RD());
    std::uniform_real_distribution<float> dist(0.1, 133.7);

    cout << "\nMatrix 1:\n";
    for (int j = 0; j < 3; j++) {
        for (int z = 0; z < 3; z++) {
            Matrix_A[j][z] = dist(gen);
            cout << Matrix_A[j][z] << " ";
        }
        cout << "\n";
    }

    cout << "\nMatrix 2:\n";
    for (int j = 0; j < 4; j++) {
        for (int z = 0; z < 4; z++) {
            Matrix_B[j][z] = dist(gen);
            cout << Matrix_B[j][z] << " ";
        }
        cout << "\n";
    }

    // Матрица А и так 3х3, под неё обрезаем матрицу B (а не дополняем A до 4х4).
    // Берём левый верхний угол 4х4 - первые 3 строки и 3 столбца.
    vector<float> flatA, flatB;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            flatA.push_back(Matrix_A[i][j]);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            flatB.push_back(Matrix_B[i][j]);   // Matrix_B[i][3] и Matrix_B[3][j] не берём

    cout << "\nMatrix 2 (cropped to 3x3):\n";
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            cout << flatB[i * 3 + j] << " ";
        cout << endl;
    }

    float matrixC[9];

    auto start = std::chrono::high_resolution_clock::now();

    for (int rep = 0; rep < REPEATS; rep++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                float sum = 0.0f;
                for (int k = 0; k < 3; k++) {
                    sum += flatA[row * 3 + k] * flatB[k * 3 + col];
                }
                matrixC[row * 3 + col] = sum;
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    cout << "\nResult Matrix C (3x3):\n";
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            cout << matrixC[i * 3 + j] << "\t";
        cout << endl;
    }

    cout << "Execution time: " << duration.count() << " mks ("
        << duration.count() / 1000.0 << " ms)" << endl;

    return 0;
}


int main(int argc, char* argv[])
{
    // test(); //Раскомментировать если нужно замерить последовательно
    std::vector<std::vector<float>> Matrix_A(3, std::vector<float>(3));//3 на 3
    std::vector<std::vector<float>> Matrix_B(4, std::vector<float>(4));//4 на 4

    //Здесь начинается многопоточность
    vector<float> recieve_matrix_a;
    vector<float> recieve_matrix_b;//Для передачи

    vector<float> result;
    float matrixC[9];//Итоговая матрица, 3х3
    long long ops = 0;//Счётчик 
    MPI_Status status;
    int nTasks, rank;
    MPI_Init(&argc, &argv);//Начало

    MPI_Comm_size(MPI_COMM_WORLD, &nTasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nTasks != 3) {   
        if (rank == 0) {
            cout << "Error: We need 3 processes!" << endl;
        }
        MPI_Finalize();
        return 0;
    }
    cout << "Process " << rank << " started" << endl;

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

      
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                recieve_matrix_a.push_back(Matrix_A[i][j]); //перенос в одномерный массив
            }
        }
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                recieve_matrix_b.push_back(Matrix_B[i][j]); //перенос в одномерный массив для удобства, лишние строку/столбец не берём
            }
        }

        cout << "\nMatrix 2 (cropped to 3x3):\n";
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++)
                cout << recieve_matrix_b[i * 3 + j] << " ";
            cout << endl;
        }
    }
    else {
        recieve_matrix_b.resize(9);
        recieve_matrix_a.resize(3);
    }

    MPI_Barrier(MPI_COMM_WORLD);      // Для замеров
    double t0 = MPI_Wtime();

    for (int rep = 0; rep < REPEATS; rep++) {// 1 млн. операций

        if (rank == 0) {
            ///Разброс по процессам
            for (int process = 1; process < 3; process++) {
                MPI_Send(recieve_matrix_b.data(), 9, MPI_FLOAT, process, 99, MPI_COMM_WORLD);
                MPI_Send(&recieve_matrix_a[process * 3], 3, MPI_FLOAT, process, 98, MPI_COMM_WORLD);
                ops += 2;
            }
        }
        else {
            MPI_Recv(recieve_matrix_b.data(), 9, MPI_FLOAT, 0, 99, MPI_COMM_WORLD, &status);
            MPI_Recv(recieve_matrix_a.data(), 3, MPI_FLOAT, 0, 98, MPI_COMM_WORLD, &status);
        }

        //рассчёты
        result.clear();
        for (int col = 0; col < 3; col++) {
            float sum = 0.0f;
            for (int k = 0; k < 3; k++) {
                sum += recieve_matrix_a[k] * recieve_matrix_b[k * 3 + col];
            }
            result.push_back(sum);
        }

        //Возврат результатов в родительскую задачу
        if (rank == 0) {
            for (int j = 0; j < 3; j++) matrixC[j] = result[j];

            for (int source_process = 1; source_process < 3; source_process++) {
                MPI_Recv(&matrixC[source_process * 3], 3, MPI_FLOAT, source_process, 97, MPI_COMM_WORLD, &status);
            }
        }
        else {
            MPI_Send(result.data(), 3, MPI_FLOAT, 0, 97, MPI_COMM_WORLD);
            ops += 1;
        }
    }

    double t1 = MPI_Wtime();//замер времени
    double local_time = t1 - t0, total_time;
    MPI_Reduce(&local_time, &total_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    long long total_ops;
    MPI_Reduce(&ops, &total_ops, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {

        cout << "\nResult Matrix C (3x3):\n";
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                cout << matrixC[i * 3 + j] << "\t";
            }
            cout << endl;
        }
        cout << "Total  execution time: " << total_time << " c" << endl;
        cout << "Total operations: " << total_ops << endl;
        cout << "Rank " << rank << " execution time: " << (t1 - t0) << " c, operations: " << ops << "\n";

    }
    else {
        cout << "Rank " << rank << " execution time: " << (t1 - t0) << " c, operations: " << ops << "\n";

    }


    MPI_Finalize();//Конец
    return 0;
}