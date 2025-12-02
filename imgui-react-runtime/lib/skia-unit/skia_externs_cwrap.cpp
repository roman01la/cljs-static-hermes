#include "include/core/SkPaint.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/ports/SkFontMgr_directory.h"
#include <yoga/Yoga.h>

extern "C" SkCanvas *canvas;
extern "C" sk_sp<SkFontMgr> fontMgr;
extern "C" float sDpiScale;

extern "C"
{

    SkPaint *paint_create_cwrap(void)
    {
        SkPaint *paint = new SkPaint();
        paint->setAntiAlias(true);
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

    void paint_delete_cwrap(SkPaint *paint)
    {
        delete paint;
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

    void create_font_manager_cwrap(const char *path)
    {
        fontMgr = SkFontMgr_New_Custom_Directory(path);
    }

    SkFont *create_font_cwrap(const char *familyName, float size)
    {
        sk_sp<SkFontStyleSet> fontStyleSet(fontMgr->matchFamily(familyName));
        sk_sp<SkTypeface> tf(fontStyleSet->createTypeface(0));
        auto font = new SkFont();
        font->setTypeface(tf);
        font->setSize(size);
        return font;
    }

    void font_delete_cwrap(SkFont *font)
    {
        delete font;
    }

    void draw_simple_text_cwrap(const char *text, float x, float y, SkFont *font, SkPaint *paint)
    {
        SkFontMetrics metrics;
        font->getMetrics(&metrics);
        size_t textLength = strlen(text);
        float adjustedY = y - metrics.fAscent;
        canvas->drawSimpleText(text, textLength, SkTextEncoding::kUTF8, x, adjustedY, *font, *paint);
    }

    // Yoga layout bindings
    YGNodeRef yoga_node_new(void)
    {
        return YGNodeNew();
    }

    void yoga_node_free(YGNodeRef node)
    {
        YGNodeFree(node);
    }

    void yoga_node_set_flex_direction(YGNodeRef node, int direction)
    {
        YGNodeStyleSetFlexDirection(node, (YGFlexDirection)direction);
    }

    void yoga_node_set_width(YGNodeRef node, float width)
    {
        YGNodeStyleSetWidth(node, width);
    }

    void yoga_node_set_height(YGNodeRef node, float height)
    {
        YGNodeStyleSetHeight(node, height);
    }

    void yoga_node_set_flex_grow(YGNodeRef node, float grow)
    {
        YGNodeStyleSetFlexGrow(node, grow);
    }

    void yoga_node_set_flex_basis(YGNodeRef node, float basis)
    {
        YGNodeStyleSetFlexBasis(node, basis);
    }

    void yoga_node_set_padding(YGNodeRef node, int edge, float padding)
    {
        YGNodeStyleSetPadding(node, (YGEdge)edge, padding);
    }

    void yoga_node_set_margin(YGNodeRef node, int edge, float margin)
    {
        YGNodeStyleSetMargin(node, (YGEdge)edge, margin);
    }

    void yoga_node_set_gap(YGNodeRef node, int gutter, float gap)
    {
        YGNodeStyleSetGap(node, (YGGutter)gutter, gap);
    }

    void yoga_node_insert_child(YGNodeRef parent, YGNodeRef child, int index)
    {
        YGNodeInsertChild(parent, child, index);
    }

    void yoga_node_remove_child(YGNodeRef parent, YGNodeRef child)
    {
        YGNodeRemoveChild(parent, child);
    }

    void yoga_node_calculate_layout(YGNodeRef root, float width, float height)
    {
        YGNodeCalculateLayout(root, width, height, YGDirectionLTR);
    }

    float yoga_node_layout_get_left(YGNodeRef node)
    {
        return YGNodeLayoutGetLeft(node);
    }

    float yoga_node_layout_get_top(YGNodeRef node)
    {
        return YGNodeLayoutGetTop(node);
    }

    float yoga_node_layout_get_width(YGNodeRef node)
    {
        return YGNodeLayoutGetWidth(node);
    }

    float yoga_node_layout_get_height(YGNodeRef node)
    {
        return YGNodeLayoutGetHeight(node);
    }
}
