//
// Created by Марк Калюжин on 28.09.2025.
//

#include "task1.h"

#include <utility>

void lab3task1::App::run() {
    cv::namedWindow("Paint");
    cv::setMouseCallback("Paint", on_mouse, this);

    create_tools_panel();
    create_color_panel();
    cv::imshow("Paint", this->img);
    load_img("/Users/kalyuzhin/Developer/CLionProjects/CS332/gats.jpeg");

    while (true) {
        char key = (char) cv::waitKey(1);
        if (key == 27) {
            break;
        } else if (key == 32) {
            clear();
        }
    }
}

void lab3task1::App::on_mouse(int event, int x, int y, int flags, void *userdata) {
    auto *app = static_cast<lab3task1::App *>(userdata);

    if (y > PANEL_HEIGHT + COLOR_PANEL_HEIGHT) {
        if (event == cv::EVENT_LBUTTONDOWN) {
            app->drawing = true;
            if (app->cur_tool == lab3task1::PEN) {
                app->prev_point = cv::Point(x, y);
            } else if (app->cur_tool == lab3task1::CIRCLE) {
                app->start_point = cv::Point(x, y);
            }
        } else if (event == cv::EVENT_MOUSEMOVE) {
            if (app->drawing) {
                if (app->cur_tool == lab3task1::PEN) {
                    cv::line(app->img, app->prev_point, cv::Point(x, y), app->cur_color, app->brush_size);
                    app->prev_point = cv::Point(x, y);
                    cv::imshow("Paint", app->img);
                }
            }
        } else if (event == cv::EVENT_LBUTTONUP) {
            app->drawing = false;
            if (app->cur_tool == lab3task1::CIRCLE) {
                cv::Point end_point(x, y);
                int radius = static_cast<int>(std::sqrt(std::pow(end_point.x - app->start_point.x, 2) +
                                                        std::pow(end_point.y - app->start_point.y, 2)));
                cv::circle(app->img, app->start_point, radius, app->cur_color, app->brush_size);
                cv::imshow("Paint", app->img);
            }
        }
    } else if (y > PANEL_HEIGHT && y < PANEL_HEIGHT + COLOR_PANEL_HEIGHT) {
        if (event == cv::EVENT_LBUTTONDOWN) {
            int local_y = y - PANEL_HEIGHT;
            for (const auto &btn: app->color_buttons) {
                if (btn.rect.contains(cv::Point(x, local_y))) {
                    app->cur_color = btn.color;
                    app->create_color_panel();
                    cv::imshow("Paint", app->img);
                    break;
                }
            }
        }
    } else {
        if (event == cv::EVENT_LBUTTONDOWN) {
            for (const auto &btn: app->tool_buttons) {
                if (btn.rect.contains(cv::Point(x, y))) {
                    app->cur_tool = btn.tool;
                    app->create_tools_panel();
                    cv::imshow("Paint", app->img);
                    break;
                }
            }
        }
    }

    if (event == cv::EVENT_LBUTTONDOWN) {
        if (app->cur_tool == lab3task1::FILL) {
            app->fill(x, y);
            cv::imshow("Paint", app->img);
        }
        if (app->cur_tool == lab3task1::FILL_WITH_IMG) {
            app->fill_img(x, y);
            cv::imshow("Paint", app->img);
        }
    }
}

void lab3task1::App::create_tools_panel() {
    cv::Mat panel = cv::Mat(PANEL_HEIGHT, this->img.cols, CV_8UC3, cv::Scalar(50, 50, 50));
    vec<Tool> tools = {PEN, CIRCLE, FILL, FILL_WITH_IMG};
    vec<std::string> tools_names = {"Pen", "Circle", "Fill", "Fill with img"};
    cv::Scalar text_color = cv::Scalar(255, 255, 255);

    this->tool_buttons.clear();

    for (ll i = 0; i != tools_names.size(); ++i) {
        Button btn = Button(i, tools_names[i], tools[i], text_color);
        this->tool_buttons.push_back(btn);

        cv::Scalar rect_color = (this->cur_tool == tools[i]) ? cv::Scalar(0, 255, 0) : cv::Scalar(100, 100, 100);
        cv::rectangle(panel, btn.rect, rect_color, cv::FILLED);
        cv::rectangle(panel, btn.rect, cv::Scalar(200, 200, 200), 2);

        int base = 0;
        cv::Size text_size = cv::getTextSize(tools_names[i], cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &base);
        cv::Point text_origin(btn.rect.x + (btn.rect.width - text_size.width) / 2,
                              btn.rect.y + (btn.rect.height + text_size.height) / 2);
        cv::putText(panel, tools_names[i], text_origin, cv::FONT_HERSHEY_SIMPLEX, 0.4, text_color, 1);
    }

    panel.copyTo(this->img(cv::Rect(0, 0, this->img.cols, PANEL_HEIGHT)));
}

void lab3task1::App::create_color_panel() {
    cv::Mat panel = cv::Mat(COLOR_PANEL_HEIGHT, this->img.cols, CV_8UC3, cv::Scalar(70, 70, 70));
    vec<cv::Scalar> colors = {cv::Scalar(0, 0, 0), cv::Scalar(0, 0, 255), cv::Scalar(0, 255, 0), cv::Scalar(255, 0, 0)};

    this->color_buttons.clear();

    for (ll i = 0; i != colors.size(); ++i) {
        ColorButton btn = ColorButton(i, colors[i]);
        this->color_buttons.push_back(btn);

        cv::Scalar border_color = (this->cur_color == colors[i]) ? cv::Scalar(255, 255, 255) : cv::Scalar(150, 150,
                                                                                                          150);
        cv::rectangle(panel, btn.rect, btn.color, cv::FILLED);
        cv::rectangle(panel, btn.rect, border_color, 2);
    }

    panel.copyTo(this->img(cv::Rect(0, PANEL_HEIGHT, this->img.cols, COLOR_PANEL_HEIGHT)));
}

lab3task1::Button::Button(int x, std::string text, Tool tool, cv::Scalar color) {
    int cord = BUTTON_MARGIN + x * (BUTTON_SIZE + BUTTON_MARGIN);
    cv::Rect r = cv::Rect(cord, BUTTON_MARGIN, BUTTON_SIZE, BUTTON_SIZE - 20);
    this->rect = r;
    this->color = std::move(color);
    this->text = std::move(text);
    this->tool = tool;
}

lab3task1::ColorButton::ColorButton(int x, cv::Scalar color) {
    int cord_x = BUTTON_MARGIN + x * (COLOR_BUTTON_SIZE + BUTTON_MARGIN);
    int cord_y = BUTTON_MARGIN;
    cv::Rect r = cv::Rect(cord_x, cord_y, COLOR_BUTTON_SIZE, COLOR_BUTTON_SIZE);
    this->rect = r;
    this->color = std::move(color);
}

void lab3task1::App::fill_recursive(int x, int y, const cv::Vec3b &target_color, const cv::Vec3b &new_color) {
    if (x < 0 || x >= img.cols || y < (PANEL_HEIGHT + COLOR_PANEL_HEIGHT) || y >= img.rows) {
        return;
    }

    cv::Vec3b cur_color = img.at<cv::Vec3b>(y, x);

    if (cur_color != target_color || cur_color == new_color) {
        return;
    }

    int left = x;
    while (left >= 0) {
        cv::Vec3b color = img.at<cv::Vec3b>(y, left);
        if (color != target_color || color == new_color) break;
        left--;
    }
    left++;

    int right = x;
    while (right < img.cols) {
        cv::Vec3b color = img.at<cv::Vec3b>(y, right);
        if (color != target_color || color == new_color) break;
        right++;
    }
    right--;

    int mod = abs(left - right) > 1000 ? 20000 : 100;

    for (int i = left; i <= right; i++) {
        img.at<cv::Vec3b>(y, i) = new_color;

        if ((i - left) % mod == 0) {
            cv::imshow("Paint", img);
            cv::waitKey(1);
        }
    }

    for (int i = left; i <= right; i++) {
        fill_recursive(i, y - 1, target_color, new_color);
        fill_recursive(i, y + 1, target_color, new_color);
    }
}

void lab3task1::App::fill(ll x, ll y) {
    if (x >= 0 && x < img.cols && y >= (PANEL_HEIGHT + COLOR_PANEL_HEIGHT) && y < img.rows) {
        cv::Vec3b target_color = img.at<cv::Vec3b>(y, x);
        cv::Vec3b new_color = cv::Vec3b(
                static_cast<uchar>(cur_color[0]),
                static_cast<uchar>(cur_color[1]),
                static_cast<uchar>(cur_color[2])
        );

        fill_recursive(x, y, target_color, new_color);
    }
}

void lab3task1::App::clear() {
    setup();
}

void lab3task1::App::setup() {
    img = cv::Mat(img.size(), img.type(), cv::Scalar(255, 255, 255));
    cv::namedWindow("Paint");
    cv::setMouseCallback("Paint", on_mouse, this);

    create_tools_panel();
    create_color_panel();
    cv::imshow("Paint", this->img);
}

void lab3task1::App::fill_recursive_img(int x, int y, const cv::Vec3b &target_color) {
    if (x < 0 || x >= img.cols || y < (PANEL_HEIGHT + COLOR_PANEL_HEIGHT) || y >= img.rows)
        return;

    cv::Vec3b cur_color = img.at<cv::Vec3b>(y, x);
    if (cur_color != target_color)
        return;

    int left = x;
    while (left >= 0) {
        cv::Vec3b color = img.at<cv::Vec3b>(y, left);
        if (color != target_color) break;
        left--;
    }
    left++;

    int right = x;
    while (right < img.cols) {
        cv::Vec3b color = img.at<cv::Vec3b>(y, right);
        if (color != target_color) break;
        right++;
    }
    right--;

    for (int i = left; i <= right; ++i) {
        int img_x = i - offset_x;
        int img_y = y - offset_y;

        img_x = (img_x % loaded_img.cols + loaded_img.cols) % loaded_img.cols;
        img_y = (img_y % loaded_img.rows + loaded_img.rows) % loaded_img.rows;

        if (img_x >= 0 && img_x < loaded_img.cols &&
            img_y >= 0 && img_y < loaded_img.rows) {
            cv::Vec3b pattern_color = loaded_img.at<cv::Vec3b>(img_y, img_x);
            img.at<cv::Vec3b>(y, i) = pattern_color;
        }
    }

    for (int i = left; i <= right; i++) {
        fill_recursive_img(i, y - 1, target_color);
        fill_recursive_img(i, y + 1, target_color);
    }
}

void lab3task1::App::load_img(const string &path) {
    loaded_img = cv::imread(path, cv::IMREAD_COLOR);
}

void lab3task1::App::fill_img(ll x, ll y) {
    if (x >= 0 && x < img.cols && y >= (PANEL_HEIGHT + COLOR_PANEL_HEIGHT) && y < img.rows) {
        offset_x = x;
        offset_y = y;
        cv::Vec3b target_color = img.at<cv::Vec3b>(y, x);
        fill_recursive_img(x, y, target_color);
    }
}
