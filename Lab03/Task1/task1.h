//
// Created by Марк Калюжин on 28.09.2025.
//

#ifndef CS332_TASK1_H
#define CS332_TASK1_H

#include <utility>

#include "../../provider.h"


const int PANEL_HEIGHT = 80;
const int BUTTON_SIZE = 50;
const int BUTTON_MARGIN = 10;
const int COLOR_BUTTON_SIZE = 30;
const int COLOR_PANEL_HEIGHT = 40;

namespace lab3task1 {
    enum Tool {
        PEN,
        CIRCLE,
        FILL,
        FILL_WITH_IMG
    };

    class Button {
    private:
        std::string text;
    public:
        Tool tool;
        cv::Scalar color;
    public:
        Button(int x, std::string text, Tool tool, cv::Scalar color);

        cv::Rect rect;
    };

    class ColorButton {
    public:
        ColorButton(int x, cv::Scalar color);

        cv::Rect rect;
        cv::Scalar color;
    };

    class App {
    private:
        cv::Mat img;
        Tool cur_tool = PEN;
        ll brush_size = 3;
        bool drawing = false;
        cv::Scalar cur_color = cv::Scalar(0, 0, 0);
        cv::Point prev_point;
        cv::Point start_point;

        cv::Mat loaded_img;
        bool tile_mode;
        int offset_x = 0;
        int offset_y = 0;

        vec<Button> tool_buttons;
        vec<ColorButton> color_buttons;

        static void on_mouse(int event, int x, int y, int flags, void *userdata);

        void create_tools_panel();

        void create_color_panel();

        void setup();

        void clear();

        void fill(ll x, ll y);

        void fill_recursive(int x, int y, const cv::Vec3b &target_color, const cv::Vec3b &new_color);

        void load_img(const string &path);

        void fill_recursive_img(int x, int y, const cv::Vec3b &target_color);

        void fill_img(ll x, ll y);


    public:
        App(int h, int w, ll brush_size = 3, cv::Scalar color = cv::Scalar(0, 0, 0)) : img(h, w, CV_8UC3,
                                                                                           cv::Scalar(255, 255, 255)),
                                                                                       brush_size(brush_size),
                                                                                       cur_color(std::move(color)) {};

        void run();
    };
}

#endif //CS332_TASK1_H
