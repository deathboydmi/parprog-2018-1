#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>

#include <omp.h>

#include "./tofunction/tofunction.h"

//#define num_threads 8

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
    };

    auto num_threads = omp_get_max_threads();

    std::vector<Point> points(k);
    std::vector<Characteristic> maxCh(num_threads);

    Point left_point, right_point;

    left_point.x = a;
    right_point.x = b;
    left_point.y = f(left_point.x);
    right_point.y = f(right_point.x);

    points[0] = left_point;
    points[num_threads] = right_point;
    Point minPoint = (left_point.y < right_point.y) ? left_point : right_point;

    auto step = (b - a) / num_threads;
    Point current_point = left_point;
    for (int j = 1; j < num_threads; ++j) {
        current_point.x += step;
        current_point.y = f(current_point.x);
        points[j] = current_point;

        minPoint = (current_point.y < minPoint.y) ? left_point : minPoint;
    }

    int i;
    for (i = num_threads; i < k - 1 - num_threads; i += num_threads) {

        std::sort(points.begin(), points.begin() + i, [](const Point &a, const Point &b) {
            return a.x < b.x;
        });

        for (int j = 1; j <= i; ++j) {
            M = (fabs(points[j].y - points[j - 1].y)) / (points[j].x - points[j - 1].x);
            maxM = (M > maxM) ? M : maxM;
        }
        if (maxM > 0) {
            m = r * maxM;
        }

        for (int j = 0; j < num_threads; ++j) {
            maxCh[j].R = 0;
            maxCh[j].iter = 1;
        }
        for (int j = 1; j <= i; ++j) {
            R = m * (points[j].x - points[j - 1].x) + (pow((points[j].y - points[j - 1].y), 2))
                                                      / (m * (points[j].x - points[j - 1].x)) -
                2 * (points[j].y + points[j - 1].y);

            for (int id = 0; id < num_threads; ++id) {
                if (R > maxCh[id].R) {
                    maxCh[id].R = R;
                    maxCh[id].iter = j;
                    break;
                }
            }
            std::sort(maxCh.begin(), maxCh.end(),
                      [](const Characteristic &a, const Characteristic &b) {
                          return a.R < b.R;
                      }
            );
        }

        if (fabs(points[maxCh[num_threads - 1].iter].x - points[maxCh[num_threads - 1].iter - 1].x) < eps) {
            break;
        }

#pragma omp parallel
        {
            auto current_thread = omp_get_thread_num();
            //for (int current_thread = 0; current_thread < num_threads; ++current_thread) {
            Point newPoint;
            newPoint.x = 0.5 * (points[maxCh[current_thread].iter].x + points[maxCh[current_thread].iter - 1].x)
                         - 0.5 * (points[maxCh[current_thread].iter].y - points[maxCh[current_thread].iter - 1].y) / m;
            newPoint.y = f(newPoint.x);
            points[i + 1 + current_thread] = newPoint;

#pragma omp critical
            {
                minPoint = (newPoint.y < minPoint.y) ? newPoint : minPoint;
            }
        }
        //std::cout << "\t" << minPoint.x << " " << minPoint.y << std::endl;
    }

    std::cout << i << std::endl;
    answer.minX = minPoint.x;
    answer.minY = minPoint.y;
}