#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

class LineDrawer {
public:
    static void drawLineBresenham(cv::Mat& image, int x0, int y0, int x1, int y1,
        const cv::Vec3b& color = cv::Vec3b(255, 255, 255)) {

        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);

        int x_step = (x0 < x1) ? 1 : -1;
        int y_step = (y0 < y1) ? 1 : -1;

        int x = x0;
        int y = y0;

        setPixelSafe(image, x, y, color);

        if (dy <= dx) {
            int d = 2 * dy - dx;

            for (int i = 0; i < dx; i++) {
                x += x_step;

                if (d < 0) {
                    d = d + 2 * dy;
                }
                else {
                    y += y_step;
                    d = d + 2 * (dy - dx);
                }

                setPixelSafe(image, x, y, color);
            }
        }
        else {
            int d = 2 * dx - dy;

            for (int i = 0; i < dy; i++) {
                y += y_step;

                if (d < 0) {

                    d = d + 2 * dx;
                }
                else {
                    x += x_step;
                    d = d + 2 * (dx - dy);
                }

                setPixelSafe(image, x, y, color);
            }
        }
    }

    static void drawLineWu(cv::Mat& image, float x0, float y0, float x1, float y1,
        const cv::Vec3b& color = cv::Vec3b(255, 255, 255)) {
        bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);

        if (steep) {
            std::swap(x0, y0);
            std::swap(x1, y1);
        }

        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }

        float dx = x1 - x0;
        float dy = y1 - y0;
        float gradient = (dx == 0) ? 1.0f : dy / dx;

        float y = y0 + gradient;

        for (int x = static_cast<int>(x0) + 1; x <= static_cast<int>(x1) - 1; x++) {
            if (steep) {
                drawPixelWu(image, static_cast<int>(y), x, 1.0f - (y - std::floor(y)), color);
                drawPixelWu(image, static_cast<int>(y) + 1, x, y - std::floor(y), color);
            }
            else {
                drawPixelWu(image, x, static_cast<int>(y), 1.0f - (y - std::floor(y)), color);
                drawPixelWu(image, x, static_cast<int>(y) + 1, y - std::floor(y), color);
            }
            y += gradient;
        }

        if (steep) {
            setPixelSafe(image, static_cast<int>(y0), static_cast<int>(x0), color);
            setPixelSafe(image, static_cast<int>(y1), static_cast<int>(x1), color);
        }
        else {
            setPixelSafe(image, static_cast<int>(x0), static_cast<int>(y0), color);
            setPixelSafe(image, static_cast<int>(x1), static_cast<int>(y1), color);
        }
    }

private:
    static void setPixelSafe(cv::Mat& image, int x, int y, const cv::Vec3b& color) {
        if (x >= 0 && x < image.cols && y >= 0 && y < image.rows) {
            image.at<cv::Vec3b>(y, x) = color;
        }
    }

    static void drawPixelWu(cv::Mat& image, int x, int y, float intensity, const cv::Vec3b& color) {
        if (x >= 0 && x < image.cols && y >= 0 && y < image.rows) {
            cv::Vec3b pixel = image.at<cv::Vec3b>(y, x);
            float factor = std::clamp(intensity, 0.0f, 1.0f);

            pixel[0] = cv::saturate_cast<uchar>(pixel[0] * (1 - factor) + color[0] * factor);
            pixel[1] = cv::saturate_cast<uchar>(pixel[1] * (1 - factor) + color[1] * factor);
            pixel[2] = cv::saturate_cast<uchar>(pixel[2] * (1 - factor) + color[2] * factor);

            image.at<cv::Vec3b>(y, x) = pixel;
        }
    }
};

bool getPointFromUser(const std::string& prompt, int& x, int& y, int max_x, int max_y) {
    std::cout << prompt << std::endl;

    std::cout << "Введите координату X (0-" << max_x << "): ";
    std::cin >> x;

    std::cout << "Введите координату Y (0-" << max_y << "): ";
    std::cin >> y;

    if (x < 0 || x > max_x || y < 0 || y > max_y) {
        std::cout << "Координаты выходят за границы изображения! Максимум: "
            << max_x << "x" << max_y << std::endl;
        return false;
    }

    return true;
}

int main() {
    setlocale(LC_ALL, "rus");

    const int width = 800;
    const int height = 600;

    cv::Mat image(height, width, CV_8UC3, cv::Scalar(0, 0, 0));

    std::cout << "Размер изображения: " << width << "x" << height << std::endl;
    std::cout << std::endl;

    int x0, y0, x1, y1;

    std::cout << "ВВОД ПЕРВОЙ ЛИНИИ:" << std::endl;
    if (!getPointFromUser("Начальная точка:", x0, y0, width - 1, height - 1)) {
        return -1;
    }
    if (!getPointFromUser("Конечная точка:", x1, y1, width - 1, height - 1)) {
        return -1;
    }

    LineDrawer::drawLineBresenham(image, x0, y0, x1, y1, cv::Vec3b(0, 0, 255));

    std::cout << std::endl << "ВВОД ВТОРОЙ ЛИНИИ:" << std::endl;
    if (!getPointFromUser("Начальная точка:", x0, y0, width - 1, height - 1)) {
        return -1;
    }
    if (!getPointFromUser("Конечная точка:", x1, y1, width - 1, height - 1)) {
        return -1;
    }

    LineDrawer::drawLineWu(image, static_cast<float>(x0), static_cast<float>(y0),
        static_cast<float>(x1), static_cast<float>(y1), cv::Vec3b(0, 255, 0));

    cv::imshow("Line Drawing Algorithms - User Input", image);
    cv::waitKey(0);

    cv::imwrite("user_lines_result.png", image);
    std::cout << "Изображение сохранено как 'user_lines_result.png'" << std::endl;

    return 0;
}
