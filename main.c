// Кирьянова Софья
// Вариант 4
// Разработать программу, находящую в заданной ASCII-строке
// последнюю при перемещении слева направо последовательность N
// символов, каждый элемент которой определяется по условию
// «больше предшествующего» (N вводится как отдельный параметр).

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 5000

void GetSequence(char *str, char *sequence, int input_size, int sequence_size) {
    if ((sequence_size < 2) || (sequence_size > input_size)) {
        printf("Wrong sequence size\n");
        sequence[0] = '\0';
    }
    int end = -1;
    int start = -1;
    int current = 0;
    for (int i = input_size - 1; i > 0; --i) {
        if (end == -1) {
            end = i;
            start = i;
        }
        if (end - start + 1 == sequence_size) {
            break;
        }
        if (str[i] > str[i - 1]) {
            start = i - 1;
        } else {
            end = -1;
            start = -1;
        }
    }
    if (end - start + 1 < sequence_size) {
        printf("There is no sequence with size = %d\n", sequence_size);
        sequence[0] = '\0';
    } else {
        for (int i = start; i <= end; ++i) {
            sequence[current] = str[i];
            ++current;
        }
        sequence[current] = '\0';
    }
}

int main(int argc, char *argv[]) {
    // Проверяем наличие входного  и выходного файлов
    if (argc < 3) {
        printf("Not enough arguments\n");
        exit(1);
    }
    
    // Имена входного и выходного файлов
    char *in_file = argv[1];
    char *out_file = argv[2];
    
    // Открываем входной файл
    int in_fd = open(in_file, O_RDONLY);
    if (in_fd == -1) {
        perror("open");
        exit(1);
    }
    
    // Открываем выходной файл
    int out_fd = open(out_file,O_WRONLY | O_CREAT, 0666);
    if (out_fd == -1) {
        perror("open");
        exit(1);
    }

    // Создание двух каналоdв
    int pipe1[2], pipe2[2];
    
    // Создание трёх процессов
    pid_t pid1, pid2, pid3;

    // Проверка создания каналов
    if ((pipe(pipe1) < 0) || (pipe(pipe2) < 0)) {
        perror("pipe");
        exit(1);
    }
    
    // Проверка создания 1 процесса
    if ((pid1 = fork()) < 0) {
        perror("fork");
        exit(1);
    } else if (pid1 == 0) {
        // Дочерний процесс 1 - чтение данных из входного файла
        printf("Child process 1\n");
        // Закрываем неиспользуемую операцию чтения
        if (close(pipe1[0]) < 0){
            perror("close");
            exit(1);
        }
        
        // Читаем данные
        char buffer[BUFFER_SIZE];
        ssize_t read_bytes = read(in_fd, buffer, BUFFER_SIZE);
        if (read_bytes == -1) {
            perror("read");
            exit(1);
        }
        buffer[read_bytes] = '\0';
        
        // Записываем данные в pipe1
        if (write(pipe1[1], buffer, read_bytes) == -1) {
            perror("write");
            exit(1);
        }
        
        printf("Got buffer, end of Child proccess 1\n");
        printf("%s\n", buffer);
        
        close(in_fd); // Закрытие файлового дескриптора входного файла
        close(pipe1[1]); // Закрытие канала pipe1
        
        exit(0);
    }

    // Проверка создания 2 процесса
    if ((pid2 = fork()) < 0) {
        perror("fork");
        exit(1);
    } else if (pid2 == 0) {
        // Дочерний процесс 2 - решение задачи
        printf("Child process 2\n");
        
        if ((close(pipe1[1]) < 0) || (close(pipe2[0]) < 0)){
            perror("close");
            exit(1);
        }
        
        // Чтение данных из канала pipe1 в buffer
        char buffer[BUFFER_SIZE];
        ssize_t read_bytes = read(pipe1[0], buffer, BUFFER_SIZE);
        if (read_bytes == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        buffer[read_bytes] = '\0';

        // Обработка данных
        int n = (buffer[0] - 48);
        printf("n: %d\n", n);
        char *str = malloc((read_bytes - 1) * sizeof(char));
        char *sequence = malloc((n + 1) * sizeof(char));
        for (int i = 0; i < read_bytes; ++i) {
            str[i] = buffer[i + 3];
        }
        GetSequence(str, sequence, read_bytes, n);

        // Запись данных в pipe2
        if (write(pipe2[1], sequence, read_bytes - 1) == -1) {
            perror("write");
            exit(1);
        }
        printf("New buffer, end of Child proccess 2\n");
        printf("%s\n", sequence);
        
        close(pipe1[0]); // Закрытие конца канала, используемого для чтения
        close(pipe2[1]); // Закрытие конца канала, используемого для записи
        
        exit(0);
    }

    // Проверка создания 3 процесса
    if ((pid3 = fork()) < 0) {
        perror("fork");
        exit(1);
    } else if (pid3 == 0) {
        // Дочерний процесс 3 - запись данных в выходной файл
        printf("Child process 3\n");
        if (close(pipe2[1]) < 0){
            perror("close");
            exit(1);
        }
        
        char buffer[BUFFER_SIZE];
        ssize_t read_bytes = read(pipe2[0], buffer, BUFFER_SIZE);
        // Проверка ошибок при чтении данных из канала
        if (read_bytes < 0) {
            perror("read");
            exit(1);
        }
        buffer[read_bytes] = '\0';
        
        // Записываем данные в выходной файл
        if (write(out_fd, buffer, read_bytes) == -1) {
            perror("write");
            exit(1);
        }
        
        printf("end of Child proccess 3\n");
        
        close(pipe2[0]);
        close(out_fd);
        
        exit(0);
    }
    
    waitpid(pid1, NULL, 0); // Ожидание завершения дочерних процессов
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    close(pipe1[0]); // Родительский процесс закрывает дочерние
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    return 0;
}
