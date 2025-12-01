const _paint_create = $SHBuiltin.extern_c(
  {},
  function paint_create_cwrap(): c_ptr {
    throw 0;
  }
);

const _paint_set_color = $SHBuiltin.extern_c(
  {},
  function paint_set_color_cwrap(_paint: c_ptr, _color: c_int): void {
    throw 0;
  }
);

const _color_from_rgba = $SHBuiltin.extern_c(
  {},
  function color_from_rgba_cwrap(
    r: c_int,
    g: c_int,
    b: c_int,
    a: c_int
  ): c_ptr {
    throw 0;
  }
);

const _canvas_draw_paint = $SHBuiltin.extern_c(
  {},
  function canvas_draw_paint_cwrap(_paint: c_ptr): void {
    throw 0;
  }
);

const _draw_rect = $SHBuiltin.extern_c(
  {},
  function draw_rect_cwrap(
    x: c_float,
    y: c_float,
    width: c_float,
    height: c_float,
    _paint: c_ptr
  ): void {
    throw 0;
  }
);

const _draw_round_rect = $SHBuiltin.extern_c(
  {},
  function draw_round_rect_cwrap(
    x: c_float,
    y: c_float,
    width: c_float,
    height: c_float,
    rx: c_float,
    ry: c_float,
    _paint: c_ptr
  ): void {
    throw 0;
  }
);

const _sizeof_SkPaint = 80;
