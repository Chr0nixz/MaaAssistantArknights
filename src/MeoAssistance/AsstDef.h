﻿#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ostream>
#include <functional>

namespace json
{
    class value;
}

namespace asst {
    struct Point
    {
        Point() = default;
        Point(const Point&) noexcept = default;
        Point(Point&&) noexcept = default;
        Point(int x, int y) : x(x), y(y) {}
        Point& operator=(const Point&) noexcept = default;
        Point& operator=(Point&&) noexcept = default;
        int x = 0;
        int y = 0;
    };

    struct Rect
    {
        Rect() = default;
        Rect(const Rect&) noexcept = default;
        Rect(Rect&&) noexcept = default;
        Rect(int x, int y, int width, int height)
            : x(x), y(y), width(width), height(height) {}
        Rect operator*(double rhs) const
        {
            return { x, y, static_cast<int>(width * rhs), static_cast<int>(height * rhs) };
        }
        Rect center_zoom(double scale, int max_width = INT_MAX, int max_height = INT_MAX) const
        {
            int half_width_scale = static_cast<int>(width * (1 - scale) / 2);
            int half_hight_scale = static_cast<int>(height * (1 - scale) / 2);
            Rect dst(x + half_width_scale, y + half_hight_scale,
                static_cast<int>(width * scale), static_cast<int>(height * scale));
            if (dst.x < 0) { dst.x = 0; }
            if (dst.y < 0) { dst.y = 0; }
            if (dst.width + dst.x >= max_width) { dst.width = max_width - dst.x; }
            if (dst.height + dst.y >= max_height) { dst.height = max_height - dst.y; }
            return dst;
        }
        Rect& operator=(const Rect&) noexcept = default;
        Rect& operator=(Rect&&) noexcept = default;
        bool empty() const noexcept { return width == 0 || height == 0; }
        bool operator==(const Rect& rhs) const noexcept
        {
            return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
        }
        bool include(const Rect& rhs) const noexcept
        {
            return x <= rhs.x
                && y <= rhs.y
                && (x + width) >= (rhs.x + rhs.width)
                && (y + height) >= (rhs.y + rhs.height);
        }
        std::string to_string() const {
            return "[" + std::to_string(x) + ", "
                + std::to_string(y) + ", "
                + std::to_string(width) + ", "
                + std::to_string(height) + " ]";
        }

        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    struct TextRect {
        TextRect() = default;
        TextRect(const TextRect&) = default;
        TextRect(TextRect&&) noexcept = default;

        explicit operator std::string() const noexcept { return text; }
        explicit operator Rect() const noexcept { return rect; }
        std::string to_string() const {
            return text + " : " + rect.to_string();
        }
        TextRect& operator=(const TextRect&) = default;
        TextRect& operator=(TextRect&&) noexcept = default;
        bool operator==(const TextRect& rhs) const noexcept {
            return text == rhs.text && rect == rhs.rect;
        }

        std::string text;
        Rect rect;
    };
    using TextRectProc = std::function<bool(TextRect&)>;

    enum class AlgorithmType {
        Invaild = -1,
        JustReturn,
        MatchTemplate,
        CompareHist,
        OcrDetect
    };

    struct MatchRect {
        MatchRect() = default;
        MatchRect(const MatchRect&) = default;
        MatchRect(MatchRect&&) noexcept = default;

        explicit operator Rect() const noexcept { return rect; }
        MatchRect& operator=(const MatchRect&) = default;
        MatchRect& operator=(MatchRect&&) noexcept = default;

        AlgorithmType algorithm;
        double score = 0.0;
        Rect rect;
    };

    enum class ProcessTaskAction {
        Invalid = 0,
        BasicClick = 0x100,
        ClickSelf = BasicClick | 1,		// 点击自身位置
        ClickRect = BasicClick | 2,		// 点击指定区域
        ClickRand = BasicClick | 4,		// 点击随机区域
        DoNothing = 0x200,				// 什么都不做
        Stop = 0x400,					// 停止当前Task
        StageDrops = 0x800,			    // 关卡结束，特化动作
        BasicSwipe = 0x1000,
        SwipeToTheLeft = BasicSwipe | 1,	// 往左划一下
        SwipeToTheRight = BasicSwipe | 2,	// 往右划一下
    };

    // 任务信息
    struct TaskInfo {
        virtual ~TaskInfo() = default;
        std::string name;								// 任务名
        AlgorithmType algorithm =						// 图像算法类型
            AlgorithmType::Invaild;
        ProcessTaskAction action =						// 要进行的操作
            ProcessTaskAction::Invalid;
        std::vector<std::string> next;					// 下一个可能的任务（列表）
        int exec_times = 0;								// 任务已执行了多少次
        int max_times = INT_MAX;						// 任务最多执行多少次
        std::vector<std::string> exceeded_next;			// 达到最多次数了之后，下一个可能的任务（列表）
        std::vector<std::string> reduce_other_times;	// 执行了该任务后，需要减少别的任务的执行次数。例如执行了吃理智药，则说明上一次点击蓝色开始行动按钮没生效，所以蓝色开始行动要-1
        asst::Rect specific_rect;						// 指定区域，目前仅针对ClickRect任务有用，会点这个区域
        int pre_delay = 0;								// 执行该任务前的延时
        int rear_delay = 0;								// 执行该任务后的延时
        int retry_times = INT_MAX;						// 未找到图像时的重试次数
        Rect roi;								        // 要识别的区域，若为0则全图识别
        Rect result_move;                               // 识别结果移动：有些结果识别到的，和要点击的不是同一个位置。即识别到了res，点击res + result_move的位置
    };

    // 文字识别任务的信息
    struct OcrTaskInfo : public TaskInfo {
        virtual ~OcrTaskInfo() = default;
        std::vector<std::string> text;					// 文字的容器，匹配到这里面任一个，就算匹配上了
        bool need_full_match = false;					// 是否需要全匹配，否则搜索到子串就算匹配上了
        std::unordered_map<std::string, std::string>
            replace_map;								// 部分文字容易识别错，字符串强制replace之后，再进行匹配
        bool cache = false;								// 是否使用历史区域
        std::vector<Rect> region_of_appeared;			// 曾经出现过的区域：上次处理该任务时，在一些rect里识别到过text，这次优先在这些rect里识别，省点性能
    };

    // 图片匹配任务的信息
    struct MatchTaskInfo : public TaskInfo {
        virtual ~MatchTaskInfo() = default;
        std::string templ_name;					        // 匹配模板图片文件名
        double templ_threshold = 0;						// 模板匹配阈值
        double hist_threshold = 0;						// 直方图比较阈值
        std::pair<int, int> mask_range;                 // 掩码的二值化范围
        bool cache = false;								// 是否使用缓存（直方图），false时就一直用模板匹配。默认为true
    };

    struct HandleInfo {
        std::string class_name;
        std::string window_name;
    };

    struct AdbCmd {
        std::string path;
        std::string connect;
        std::string click;
        std::string swipe;
        std::string display;
        std::string display_regex;
        std::string screencap;
        //std::string pullscreen;
        int display_width = 0;
        int display_height = 0;
    };

    struct EmulatorInfo {
        std::string name;
        HandleInfo handle;
        AdbCmd adb;
        std::string path;
    };

    constexpr double DoubleDiff = 1e-12;
}

namespace std {
    template<>
    class hash<asst::Rect> {
    public:
        size_t operator()(const asst::Rect& rect) const
        {
            return std::hash<int>()(rect.x)
                ^ std::hash<int>()(rect.y)
                ^ std::hash<int>()(rect.width)
                ^ std::hash<int>()(rect.height);
        }
    };
    template<>
    class hash<asst::TextRect> {
    public:
        size_t operator()(const asst::TextRect& textarea) const
        {
            return std::hash<std::string>()(textarea.text)
                ^ std::hash<asst::Rect>()(textarea.rect);
        }
    };
}