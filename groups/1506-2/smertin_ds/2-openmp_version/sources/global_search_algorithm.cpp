#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <iterator>
#include <algorithm>

#include <omp.h>

#include "./tofunction/tofunction.h"

struct TaskGSA {
    std::string function;
    double left_border;
    double right_border;

    unsigned int num_iter;
    double eps;
} task;

struct AnswerGSA {
    double minX;
    double minY;
} answer;

ToFunction toFunc;

double f(double);

void methodGSA();

int main(int argc, char *argv[]) {
    const char *path_of_test = "../bin/tasks/task_03";

    if (argc > 1) {
        path_of_test = argv[1];
    }

    std::ifstream task_stream(path_of_test, std::ios::in | std::ios::binary);
    if (!task_stream) {
        std::cout << "open " << path_of_test << " error" << std::endl;
        return 1;
    }
    task_stream >> task.function;
    task_stream >> task.left_border;
    task_stream >> task.right_border;
    task_stream >> task.num_iter;
    task_stream >> task.eps;
    task_stream.close();

    toFunc.SetFunction(task.function);
//=============================================================================
    methodGSA();
//=============================================================================

    char path_of_answer[] = "../bin/results/result_00";
    int path_task_length = sizeof(path_of_answer) / sizeof(char);
    int path_answ_length = sizeof(path_of_answer) / sizeof(char);

    int ind_task;
    for (int ind = 0; ind < path_task_length; ++ind) {
        if (path_of_test[ind] == '_') {
            ind_task = ind + 1;
            break;
        }
    }

    int ind_answ;
    for (int ind = 0; ind < path_answ_length; ++ind) {
        if (path_of_answer[ind] == '_') {
            ind_answ = ind + 1;
            break;
        }
    }

    path_of_answer[ind_answ] = path_of_test[ind_task];
    path_of_answer[ind_answ + 1] = path_of_test[ind_task + 1];

    std::ofstream answer_stream(path_of_answer, std::ios::out | std::ios::binary);
    if (!answer_stream) {
        std::cout << "open " << path_of_answer << " error" << std::endl;
        return 1;
    }
    answer_stream << answer.minX << std::endl;
    answer_stream << answer.minY << std::endl;

    answer_stream.close();

    return 0;
}

double f(double x) {
    toFunc.Calculate(x);
}

void methodGSA() {
    const double a = task.left_border;
    const double b = task.right_border;
    double eps = task.eps;
    int k = task.num_iter;

    const double r = 3;
    double m = 1;
    double maxM = 0;
    double last_m;
    double M;
    double R;

    struct Point {
        double x;
        double y;

        Point &operator=(const Point &p) {
            this->x = p.x;
            this->y = p.y;
            return *this;
        }
    };
    struct Characteristic {
        double R;
        int iter;

        bool operator<(const Characteristic &a) const {
            return (R > a.R);
        }
    };

    auto num_threads = omp_get_max_threads();
    omp_set_num_threads(num_threads);

    std::vector<Point> points(k);
    std::vector<Point> newPoints(num_threads);
    std::multiset<Characteristic> maxCh;

    auto start_time = omp_get_wtime();

    points[0].x = a;
    points[num_threads].x = b;
    points[0].y = f(points[0].x);
    points[num_threads].y = f(points[num_threads].x);

    Point newPoint;
    Point minPoint = (points[0].y < points[num_threads].y) ? points[0] : points[num_threads];

    int num_iter = 0;

    auto step = (b - a) / num_threads;

#pragma omp parallel
    {
        Point current_point = points[0];
        Point minPoint_thr = minPoint;
#pragma omp for private(current_point)
        for (int j = 1; j < num_threads; ++j) {
            current_point.x += step;
            current_point.y = f(current_point.x);
            points[j] = current_point;

            minPoint_thr = (current_point.y < minPoint_thr.y) ? current_point : minPoint_thr;
        }
#pragma omp critical
        {
            minPoint = (minPoint_thr.y < minPoint.y) ? minPoint_thr : minPoint;
        }
    }

    for (int i = num_threads; i < k - 1 - num_threads; i += num_threads) {

        std::sort(points.begin(), points.begin() + i, [](const Point &a, const Point &b) {
            return a.x < b.x;
        });

#pragma omp parallel for private(M) reduction(max : maxM)
        for (int j = 1; j <= i; ++j) {
            M = (fabs(points[j].y - points[j - 1].y)) / (points[j].x - points[j - 1].x);
            maxM = (M > maxM) ? M : maxM;
        }

        last_m = m;

        if (maxM > 0) {
            m = r * maxM;
        }

        Characteristic current_ch;
        maxCh.clear();
        for (int j = 1; j <= i; ++j) {
            current_ch.R = m * (points[j].x - points[j - 1].x) +
                           (points[j].y - points[j - 1].y) * (points[j].y - points[j - 1].y)
                           / (m * (points[j].x - points[j - 1].x)) - 2 * (points[j].y + points[j - 1].y);
            current_ch.iter = j;
            maxCh.insert(current_ch);
        }


        if (fabs(points[(*maxCh.cbegin()).iter].x - points[(*maxCh.cbegin()).iter - 1].x) < eps) {
            break;
        }

#pragma omp parallel private(newPoints)
        {
            auto current_thread = omp_get_thread_num();
            auto iter = (*std::next(maxCh.cbegin(), current_thread)).iter;
            newPoints[current_thread].x = 0.5 * (points[iter].x + points[iter - 1].x)
                                          - 0.5 * (points[iter].y - points[iter - 1].y) / m;
            newPoints[current_thread].y = f(newPoint.x);
            points[i + 1 + current_thread] = newPoint;

#pragma omp critical
            {
                minPoint = (newPoints[current_thread].y < minPoint.y) ? newPoints[current_thread] : minPoint;
            }
        }
        ++num_iter;
    }

    auto finish_time = omp_get_wtime();

    std::cout << "\n\tReport:" << std::endl;
    std::cout << "Number of iterations: " << num_iter << std::endl;
    std::cout << "Time of working: " << finish_time - start_time << "\n" << std::endl;

    answer.minX = minPoint.x;
    answer.minY = minPoint.y;
}
