#include "include/core/SkPaint.h"
#include "include/core/SkCanvas.h"

extern "C" SkCanvas *canvas;

extern "C"
{

    SkPaint *paint_create_cwrap(void)
    {
        SkPaint *paint = new SkPaint();
        return paint;
    }

    void paint_set_color_cwrap(SkPaint *paint, uint32_t color)
    {
        paint->setColor(color);
    }

    SkColor color_from_rgba_cwrap(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        return SkColorSetARGB(a, r, g, b);
    }

    void canvas_draw_paint_cwrap(SkPaint *paint)
    {
        canvas->drawPaint(*paint);
    }

    void draw_rect_cwrap(float x, float y, float width, float height, SkPaint *paint)
    {
        float x2 = x + width;
        float y2 = y + height;
        canvas->drawRect({x, y, x2, y2}, *paint);
    }

    void draw_round_rect_cwrap(float x, float y, float width, float height, float rx, float ry, SkPaint *paint)
    {
        float x2 = x + width;
        float y2 = y + height;
        canvas->drawRoundRect({x, y, x2, y2}, rx, ry, *paint);
    }
}
